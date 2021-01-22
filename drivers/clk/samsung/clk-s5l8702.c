/*
 * SPEAr6xx machines clock framework source file
 *
 * Copyright (C) 2012 ST Microelectronics
 * Viresh Kumar <vireshk@kernel.org>
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2. This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#include <linux/clk-provider.h>
#include <linux/io.h>
#include <linux/spinlock_types.h>
#include <linux/of_device.h>
#include <linux/platform_device.h>
#include <linux/regmap.h>

#include <dt-bindings/clock/samsung,s5l8702.h>

#define to_s5l_clk_pll(_hw) container_of(_hw, struct s5l_clk_pll, hw)

#define S5L_PLL_CLK(_num) \
	{ .num = _num, .name = "pll" #_num, },

struct s5l_clk_pll {
	u32 num;
	char* name;
};

struct s5l_clk_common {
	struct clk_hw hw;
	struct regmap *base;
};

struct s5l_clk_pll {
	
	u32 num; // Which pll
};

static __initconst struct s5l_clk_pll s5l_pll_clks[] = {
	S5L_PLL_CLK(0)
	S5L_PLL_CLK(1)
	S5L_PLL_CLK(2)
};

static void __init s5l8702_pll_init(struct device_node *np, struct s5l_clk_pll *pll_clks[]) {
	const char *parent_name;
	struct clk *clk;
	
	parent_name = of_clk_get_parent_name(np, 0);
	if (!parent_name) {
		return;
	}
}

static void __init __s5l8702_pll_init(struct device_node *np)
{
	clkgen_c32_pll_setup(np, &s5l_pll_clks);
}
CLK_OF_DECLARE(c32_pll0, "samsung,s5l8702-pll", __s5l8702_pll_init);


// static int s5l8702_clk_probe(struct platform_device *pdev) {
// 	struct s5l_clk_common *s5l_clk_dev;

// 	s5l_clk_dev = devm_kzalloc(&pdev->dev, sizeof(*s5l_clk_dev), GFP_KERNEL);
// 	if (!s5l_clk_dev) {
// 		return -ENOMEM;
// 	}

// 	platform_set_drvdata(pdev, s5l_clk_dev);

// 	s5l_clk_dev->base = devm_platform_ioremap_resource(pdev, 0);

// 	return 0;
// }

// static const struct clk_ops s5l8702_clk_ops_pll[] = {
	
// };

// static const struct of_device_id s5l8702_clk_ids[] = {
// 	// The pll clocks include everything after the fixed oscillators, and before the clock gates.
// 	// That means the extra OSCSEL is handled here too
// 	{ .compatible = "samsung,s5l8702-pll",
// 	}, { /* Sentinel */ },
// };

// static struct platform_driver s5l8702_clk_controller = {
// 	.probe	= s5l8702_clk_probe,
// 	.driver	= {
// 		.name	= "s5l8702-clk",
// 		.of_match_table	= s5l8702_clk_ids,
// 	},
// };

// module_platform_driver(s5l8702_clk_controller);