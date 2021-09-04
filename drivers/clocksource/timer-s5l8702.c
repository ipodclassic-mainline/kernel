// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (C) Caleb Connolly 2021
 * Author:  Caleb Connolly <caleb@connolly.tech>
 *
 * Inspired by timer-stm32.c from Maxime Coquelin
 */

#include <linux/kernel.h>
#include <linux/clocksource.h>
#include <linux/clockchips.h>
#include <linux/delay.h>
#include <linux/irq.h>
#include <linux/interrupt.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <linux/clk.h>
#include <linux/reset.h>
#include <linux/sched_clock.h>
#include <linux/slab.h>

#include "timer-of.h"

// Controller registers
#define TSTAT   0x118

// Register offsets (from a given timer base) (e.g. TIMER_F is 0xc0 base)
#define TCON   0x00 // Control / config register
  #define TCON_INT0 BIT(16)
  #define TCON_INT1 BIT(17)
  #define TCON_INT0_EN (1 << 12)
  #define TCON_INT1_EN (1 << 13)
enum {
	TCON_CLK_SEL_CLK_2 = (0 << 8),
	TCON_CLK_SEL_CLK_4 = (1 << 8),
	TCON_CLK_SEL_CLK_16 = (2 << 8),
	TCON_CLK_SEL_CLK_64 = (3 << 8),
	TCON_CLK_SEL_CLK = (4 << 8),
};
  #define TCON_CLK_SEL_ECLK (1 << 6)
  #define TCON_CLK_SEL_PCLK (0 << 6)
enum {
	TCON_MODE_SEL_INTERVAL = (0 << 4),
	TCON_MODE_SEL_PWM = (1 << 4),
	TCON_MODE_SEL_ONESHOT = (2 << 4),
	TCON_MODE_SEL_CAPTURE = (3 << 4),
};

#define TCMD    0x04
  #define TCMD_CLR BIT(1)
  #define TCMD_EN BIT(0)
// Both 32-bit registers
#define TDATA0  0x08
#define TDATA1  0x0C
#define TPSC    0x10 // Prescaler / TxPRE in RB
#define TCNT  0x14

/**
 * For now, just use TIMER E as the sched clock.
 * The bootloader (??) configured it as 12MHz
 * so we can just use it here...
 * See usec_timer_init() in RB
 */
#define ECLK 12000000
#define TIMER_E_CNT 0xB4

#define s5l_timer_from_of(to, timer_name) ({ \
	struct s5l_timer_private *pd = (struct s5l_timer_private*)to->private_data; \
	pd->timer_name; \
})

/**
 * s5l_timer - represent a timer unit.
 * @name: The name of the timer
 * @reg_offset: Register offset of this timer
 * @prescaler: The prescaler value to use for this timer
 * @en_int0: Should we enable the first interrupt for this timer?
 * @en_int1: Should we enable the second interrupt for this timer?
 * @clk_sel: 
 * @counter_bits: Number of bits in this counter
 */
struct s5l_timer {
	char* name;
	int __iomem reg_offset;
	int prescaler;
	bool en_int0;
	bool en_int1;
	bool use_eclk;
	int clk_sel;
	unsigned int mode_sel;
	int counter_bits;
};

struct s5l_timer_private {
	int bits;
	struct s5l_timer *s5l_timer_clkevt;
};

static unsigned int __iomem *s5l_timer_cnt __read_mostly;

static struct delay_timer s5l_timer_delay;

static inline void s5l_timer_write(unsigned int val, struct timer_of *to,
	struct s5l_timer *timer, unsigned int addr)
{
	pr_debug("%s: val: 0x%x -> addr: 0x%px", __func__, val, timer_of_base(to) + timer->reg_offset + addr);
	writel_relaxed(val, timer_of_base(to) + timer->reg_offset + addr);
}

static inline unsigned int s5l_timer_read(struct timer_of *to, struct s5l_timer *timer, unsigned int addr)
{
	return readl_relaxed(timer_of_base(to) + timer->reg_offset + addr);
}

static unsigned long notrace s5l_read_sched_clock(void)
{
	return readl_relaxed(s5l_timer_cnt);
}

static unsigned long s5l_read_delay(void)
{
	return readl_relaxed(s5l_timer_cnt);
}

static void s5l_timer_start(struct timer_of *to, struct s5l_timer *timer)
{
	pr_info("%s: Starting timer: '%s'\n", __func__, timer->name);
	s5l_timer_write(TCMD_EN, to, timer, TCMD);
}

static void s5l_timer_clear(struct timer_of *to, struct s5l_timer *timer)
{
	pr_info("%s: Clearing timer: '%s'\n", __func__, timer->name);
	s5l_timer_write(TCMD_CLR, to, timer, TCMD);
}

static void s5l_timer_stop(struct timer_of *to, struct s5l_timer *timer)
{
	pr_info("%s: Stopping timer: '%s'\n", __func__, timer->name);
	s5l_timer_write(0, to, timer, TCMD);
}

/**
 * Clear and configure a timer.
 * DOES NOT START TIMER
 */
static void s5l_timer_reset(struct timer_of *to, struct s5l_timer *timer)
{
	unsigned int tcon = 0;
	
	tcon |= (timer->en_int0 ? TCON_INT0_EN : 0);
	tcon |= timer->en_int1 ? TCON_INT1_EN : 0;
	tcon |= timer->clk_sel;
	tcon |= timer->use_eclk ? TCON_CLK_SEL_ECLK : TCON_CLK_SEL_PCLK;
	tcon |= (0 << 11);
	tcon |= timer->mode_sel;

	pr_debug("%s: Resetting timer: '%s', tcon: 0x%x\n", __func__, timer->name, tcon);

	s5l_timer_write(tcon, to, timer, TCON);

	s5l_timer_write(timer->prescaler, to, timer, TPSC);
}

static int s5l_clock_event_set_next_event(unsigned long evt,
					    struct clock_event_device *clkevt)
{
	struct timer_of *to = to_timer_of(clkevt);
	struct s5l_timer *s5l_timer_clkevt = s5l_timer_from_of(to, s5l_timer_clkevt);
	unsigned long now;

	// Don't bother supporting the second interval register yet.
	s5l_timer_write(evt, to, s5l_timer_clkevt, TDATA0);
	now = s5l_timer_read(to, s5l_timer_clkevt, TCNT);

	pr_debug("%s: timer '%s', evt: %d, now: %lu\n", __func__,
		s5l_timer_clkevt->name, evt, now);

	return 0;
}

static int s5l_clock_event_set_periodic(struct clock_event_device *clkevt)
{
	int ret;
	struct timer_of *to = to_timer_of(clkevt);
	struct s5l_timer *timer_clkevt = s5l_timer_from_of(to, s5l_timer_clkevt);
	unsigned int tf_en = s5l_timer_read(to, timer_clkevt, TCMD);

	pr_debug("%s: timer: '%s", __func__, timer_clkevt->name);

	s5l_timer_stop(to, timer_clkevt);
	s5l_timer_reset(to, timer_clkevt);

	ret = s5l_clock_event_set_next_event(timer_of_period(to), clkevt);

	s5l_timer_write((1 << 1) | tf_en, to, timer_clkevt, TCMD);

	s5l_timer_start(to, timer_clkevt);

	return ret;
}

static int s5l_clock_event_set_oneshot(struct clock_event_device *clkevt)
{
	struct timer_of *to = to_timer_of(clkevt);
	struct s5l_timer *timer_clkevt = s5l_timer_from_of(to, s5l_timer_clkevt);

	pr_debug("%s: timer: '%s", __func__, timer_clkevt->name);

	s5l_timer_start(to, timer_clkevt);

	return 0;
}

// Clear pending clock events
static int s5l_clock_event_shutdown(struct clock_event_device *clkevt)
{
	struct timer_of *to = to_timer_of(clkevt);
	struct s5l_timer *timer_clkevt = s5l_timer_from_of(to, s5l_timer_clkevt);

	pr_info("%s: timer: '%s", __func__, timer_clkevt->name);

	s5l_timer_write(0, to, timer_clkevt, TDATA0);
	s5l_timer_write(0, to, timer_clkevt, TDATA1);

	return 0;
}

static irqreturn_t s5l_clock_event_handler(int irq, void *dev_id)
{
	struct clock_event_device *clkevt = (struct clock_event_device *)dev_id;
	struct timer_of *to = to_timer_of(clkevt);
	struct s5l_timer_private *pd = to->private_data;

	pr_debug("%s: TSTAT = 0x%x\n", __func__, readl_relaxed(timer_of_base(to) + TSTAT));

	writel_relaxed((0x07 << 16), timer_of_base(to) + TSTAT);

	if (clockevent_state_periodic(clkevt))
		s5l_clock_event_set_next_event(timer_of_period(to), clkevt);
	else
		s5l_clock_event_shutdown(clkevt);

	clkevt->event_handler(clkevt);

	return IRQ_HANDLED;
}

static int __init s5l_clocksource_init(struct timer_of *to)
{
	const char *name = to->np->full_name;
	int ret;

	sched_clock_register((void*)s5l_read_sched_clock, 32, ECLK);
	ret = clocksource_mmio_init(s5l_timer_cnt, name,
				     ECLK, 300,
				     32, clocksource_mmio_readl_up);

	s5l_timer_delay.read_current_timer = s5l_read_delay;
	s5l_timer_delay.freq = ECLK;
	register_current_timer_delay(&s5l_timer_delay);

	return ret;
}

static void __init s5l_clockevent_init(struct timer_of *to)
{
	struct s5l_timer_private *pd = to->private_data;
	struct s5l_timer *s5l_timer_clkevt;

	pd->s5l_timer_clkevt = kzalloc(sizeof(*pd->s5l_timer_clkevt), GFP_KERNEL);
	s5l_timer_clkevt = pd->s5l_timer_clkevt;

	s5l_timer_clkevt->name = "counter_f"; // Use Timer F as a counter
	s5l_timer_clkevt->reg_offset = 0xC0;
	s5l_timer_clkevt->prescaler = 0;
	s5l_timer_clkevt->en_int0 = true;
	s5l_timer_clkevt->en_int1 = false;
	s5l_timer_clkevt->clk_sel = TCON_CLK_SEL_CLK;
	s5l_timer_clkevt->mode_sel = TCON_MODE_SEL_INTERVAL;
	s5l_timer_clkevt->use_eclk = true;
	s5l_timer_clkevt->counter_bits = 32;
		

	to->clkevt.name = s5l_timer_clkevt->name;
	to->clkevt.features = CLOCK_EVT_FEAT_PERIODIC;// | CLOCK_EVT_FEAT_ONESHOT;
	to->clkevt.set_state_shutdown = s5l_clock_event_shutdown;
	to->clkevt.set_state_periodic = s5l_clock_event_set_periodic;
	//to->clkevt.set_state_oneshot = s5l_clock_event_set_oneshot;
	to->clkevt.tick_resume = s5l_clock_event_shutdown;
	to->clkevt.set_next_event = s5l_clock_event_set_next_event;
	to->clkevt.rating = 300;

	s5l_timer_reset(to, s5l_timer_clkevt);

	clockevents_config_and_register(&to->clkevt, timer_of_rate(to), 0, 0);

	pr_info("%s: S5L clockevent driver initialized\n",
		to->np->full_name);
}

int __init s5l_timer_init(struct device_node *node)
{
	struct reset_control *rstc;
	struct timer_of *to;
	struct s5l_timer_private *pd;
	int ret;

	to = kzalloc(sizeof(*to), GFP_KERNEL);
	if (!to)
		return -ENOMEM;

	to->flags = TIMER_OF_IRQ | TIMER_OF_CLOCK | TIMER_OF_BASE;
	to->of_irq.handler = s5l_clock_event_handler;

	ret = timer_of_init(node, to);
	if (ret)
		goto err;

	to->private_data =
		kzalloc(sizeof(struct s5l_timer_private), GFP_KERNEL);
	if (!to->private_data)
	{
		ret = -ENOMEM;
		goto deinit;
	}
	pd = to->private_data;

	// Address of timer E which the bootloader configured for us
	s5l_timer_cnt = timer_of_base(to) + TIMER_E_CNT;

	// pr_info("%s: counter %px has val %d", __func__, s5l_timer_cnt, *s5l_timer_cnt);

	ret = s5l_clocksource_init(to);
	if (ret)
		goto deinit;

	s5l_clockevent_init(to);
	return 0;

deinit:
	timer_of_cleanup(to);
err:
	kfree(to);
	return ret;
}

TIMER_OF_DECLARE(s5l, "samsung,s5l-timers", s5l_timer_init);
