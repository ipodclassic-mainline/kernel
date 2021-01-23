// SPDX-License-Identifier: GPL-2.0


#include <asm/mach/arch.h>
#include <asm/mach/map.h>
#include <asm/io.h>
#include <linux/of_irq.h>
#include <linux/of_address.h>

#include "s5l8xxx.h"

/*
 * Following will create 16MB static virtual/physical mappings
 * PHYSICAL		VIRTUAL
 * 0x08000000		0x08000000 <- DRAM
 * 0x30000000		0x30000000 <- IO space
 */
struct map_desc s5l8702_io_desc[] __initdata = {
	{
		.virtual	= IOMEM(0x08000000),
		.pfn		= __phys_to_pfn(0x08000000),
		.length		= SZ_64M,
		.type		= MT_DEVICE_CACHED
	},
	{
		.virtual	= IOMEM(0x30000000),
		.pfn		= __phys_to_pfn(0x30000000),
		.length		= SZ_256M,
		.type		= MT_DEVICE
	},
};

/* This will create static memory mapping for selected devices */
void __init s5l8702_map_io(void)
{
	iotable_init(s5l8702_io_desc, ARRAY_SIZE(s5l8702_io_desc));
}

static const struct of_device_id timer_of_match[] __initdata = {
	{ .compatible = "samsung,s5l8702-system-timer", },
	{ },
};

// void __init s5l8702_init_time(void)
// {
// 	struct device_node *np;
// 	int irq, ret;
// 	void __iomem *gpt_base;

// 	np = of_find_matching_node(NULL, timer_of_match);
// 	if (!np) {
// 		pr_err("%s: No timer passed via DT\n", __func__);
// 		return;
// 	}

// 	irq = irq_of_parse_and_map(np, 0);
// 	if (!irq) {
// 		pr_err("%s: No irq passed for timer via DT\n", __func__);
// 		return;
// 	}

// 	gpt_base = of_iomap(np, 0);
// 	if (!gpt_base) {
// 		pr_err("%s: of iomap failed\n", __func__);
// 		return;
// 	}

	

// 	return;

// // err_prepare_enable_clk:
// // 	clk_put(gpt_clk);
// // err_iomap:
// // 	iounmap(gpt_base);
// }

#define UTXH0                   (*(volatile unsigned long *)(0x3CC00020))     /* Transmit Buffer Register */


void __init s5l8702_beep(void) {
	// PCON0 = (PCON0 & 0x00ffffff) | 0x53000000;

	// uint32_t level = 0;

	while (true)
    {
        UTXH0 = 'A'
        //udelay(500);
    }
}

static const char *s5l8702_dt_match[] __initdata = {
       "samsung,s5l8702",
        NULL
 };

DT_MACHINE_START(S5L8702, "S5L8702")
    .map_io		=	s5l8702_map_io,
	.init_machine = s5l8702_beep,
	.dt_compat = s5l8702_dt_match,
MACHINE_END