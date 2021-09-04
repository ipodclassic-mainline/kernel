/*
 * Copyright 2021 (C), Caleb Connolly <caleb@connolly.tech>
 *
 * Flattened Device Tree board initialization
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#include <linux/clk.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/irqchip.h>
#include <linux/mbus.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_net.h>
#include <linux/of_platform.h>
#include <linux/slab.h>
#include <asm/hardware/cache-feroceon-l2.h>
#include <asm/mach/arch.h>
#include <asm/mach/map.h>
#include <asm/setup.h>
#include <linux/console.h>
#include "s5l8702.h"

static struct map_desc s5l8702_io_desc[] __initdata = {
{
	.virtual = (unsigned long) S5L8702_REGS_VIRT_BASE,
	.pfn = __phys_to_pfn(S5L8702_REGS_PHYS_BASE),
	.length = S5L8702_REGS_SIZE,
	.type = MT_DEVICE,
	},
};

extern void early_console_write(struct console *con, const char *s, unsigned n);

static struct console early_console_dev = {
	.name =		"earlycon",
	.write =	early_console_write,
	.flags =	CON_PRINTBUFFER | CON_BOOT,
	.index =	-1,
};

void __init s5l8702_map_io(void)
{
	iotable_init(s5l8702_io_desc, ARRAY_SIZE(s5l8702_io_desc));
	// early_console = &early_console_dev;
	// register_console(&early_console_dev);
}

static void __init s5l8702_dt_init(void)
{
	of_platform_populate(NULL, of_default_bus_match_table,
	NULL, NULL);
}

static const char * const s5l8702_dt_compat[] = {
	"samsung,s5l8702",
	NULL,
};

DT_MACHINE_START(S5L8702, "Samsung S5L8702 iPod Classic (6th Gen)")
	 .init_machine	= s5l8702_dt_init,
	 .map_io = s5l8702_map_io,
	// .init_irq = irqchip_init,
	.dt_compat = s5l8702_dt_compat,
MACHINE_END
