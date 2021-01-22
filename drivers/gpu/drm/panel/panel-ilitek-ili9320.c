// SPDX-License-Identifier: GPL-2.0-only
/*
 * Ilitek ILI9320 TFT LCD driver.
 * 
 * This driver is for the panel found in the iPod classic (6th Gen).
 *
 * Copyright (C) 2020 Caleb Connolly <caleb@connolly.tech>
 * Derived from drivers/drm/gpu/panel/panel-samsung-ld9040.c
 */
#include <linux/bitops.h>
#include <linux/gpio/consumer.h>
#include <linux/module.h>
#include <linux/of_device.h>
#include <linux/regmap.h>
#include <linux/regulator/consumer.h>
#include <linux/spi/spi.h>

#include <video/mipi_display.h>
#include <video/of_videomode.h>
#include <video/videomode.h>

#include <drm/drm_modes.h>
#include <drm/drm_panel.h>

struct ili9320 {
	struct device *dev;
	struct drm_panel panel;
	struct regmap *regmap;
	enum ili9320_input input;
	struct videomode vm;
};

static int ili9320_probe(struct spi_device *spi)
{
	struct device *dev = &spi->dev;
	struct ili9320 *ili;
	const struct regmap_config *regmap_config;
	u8 gamma;
	u32 val;
	int ret;
	int i;

	ili = devm_kzalloc(dev, sizeof(struct ili9320), GFP_KERNEL);
	if (!ili)
		return -ENOMEM;

	spi_set_drvdata(spi, ili);

	ili->dev = dev;

	/*
	 * Every new incarnation of this display must have a unique
	 * data entry for the system in this driver.
	 */
	ili->conf = of_device_get_match_data(dev);
	if (!ili->conf) {
		dev_err(dev, "missing device configuration\n");
		return -ENODEV;
	}

	val = ili->conf->vreg1out_mv;
	if (!val) {
		/* Default HW value, do not touch (should be 4.5V) */
		ili->vreg1out = U8_MAX;
	} else {
		if (val < 3600) {
			dev_err(dev, "too low VREG1OUT\n");
			return -EINVAL;
		}
		if (val > 6000) {
			dev_err(dev, "too high VREG1OUT\n");
			return -EINVAL;
		}
		if ((val % 100) != 0) {
			dev_err(dev, "VREG1OUT is no even 100 microvolt\n");
			return -EINVAL;
		}
		val -= 3600;
		val /= 100;
		dev_dbg(dev, "VREG1OUT = 0x%02x\n", val);
		ili->vreg1out = val;
	}

	val = ili->conf->vcom_high_percent;
	if (!val) {
		/* Default HW value, do not touch (should be 91%) */
		ili->vcom_high = U8_MAX;
	} else {
		if (val < 37) {
			dev_err(dev, "too low VCOM high\n");
			return -EINVAL;
		}
		if (val > 100) {
			dev_err(dev, "too high VCOM high\n");
			return -EINVAL;
		}
		val -= 37;
		dev_dbg(dev, "VCOM high = 0x%02x\n", val);
		ili->vcom_high = val;
	}

	val = ili->conf->vcom_amplitude_percent;
	if (!val) {
		/* Default HW value, do not touch (should be 114%) */
		ili->vcom_high = U8_MAX;
	} else {
		if (val < 70) {
			dev_err(dev, "too low VCOM amplitude\n");
			return -EINVAL;
		}
		if (val > 132) {
			dev_err(dev, "too high VCOM amplitude\n");
			return -EINVAL;
		}
		val -= 70;
		val >>= 1; /* Increments of 2% */
		dev_dbg(dev, "VCOM amplitude = 0x%02x\n", val);
		ili->vcom_amplitude = val;
	}

	for (i = 0; i < ARRAY_SIZE(ili->gamma); i++) {
		val = ili->conf->gamma_corr_neg[i];
		if (val > 15) {
			dev_err(dev, "negative gamma %u > 15, capping\n", val);
			val = 15;
		}
		gamma = val << 4;
		val = ili->conf->gamma_corr_pos[i];
		if (val > 15) {
			dev_err(dev, "positive gamma %u > 15, capping\n", val);
			val = 15;
		}
		gamma |= val;
		ili->gamma[i] = gamma;
		dev_dbg(dev, "gamma V%d: 0x%02x\n", i + 1, gamma);
	}

	ili->supplies[0].supply = "vcc"; /* 2.7-3.6 V */
	ili->supplies[1].supply = "iovcc"; /* 1.65-3.6V */
	ili->supplies[2].supply = "vci"; /* 2.7-3.6V */
	ret = devm_regulator_bulk_get(dev, ARRAY_SIZE(ili->supplies),
				      ili->supplies);
	if (ret < 0)
		return ret;
	ret = regulator_set_voltage(ili->supplies[0].consumer,
				    2700000, 3600000);
	if (ret)
		return ret;
	ret = regulator_set_voltage(ili->supplies[1].consumer,
				    1650000, 3600000);
	if (ret)
		return ret;
	ret = regulator_set_voltage(ili->supplies[2].consumer,
				    2700000, 3600000);
	if (ret)
		return ret;

	ili->reset_gpio = devm_gpiod_get_optional(dev, "reset", GPIOD_OUT_HIGH);
	if (IS_ERR(ili->reset_gpio)) {
		dev_err(dev, "failed to get RESET GPIO\n");
		return PTR_ERR(ili->reset_gpio);
	}

	spi->bits_per_word = 8;
	ret = spi_setup(spi);
	if (ret < 0) {
		dev_err(dev, "spi setup failed.\n");
		return ret;
	}
	regmap_config = &ili9320_regmap_config;
	ili->regmap = devm_regmap_init(dev, &ili9320_regmap_bus, dev,
				       regmap_config);
	if (IS_ERR(ili->regmap)) {
		dev_err(dev, "failed to allocate register map\n");
		return PTR_ERR(ili->regmap);
	}

	ret = regmap_read(ili->regmap, ili9320_CHIP_ID, &val);
	if (ret) {
		dev_err(dev, "can't get chip ID (%d)\n", ret);
		return ret;
	}
	if (val != ili9320_CHIP_ID_MAGIC) {
		dev_err(dev, "chip ID 0x%0x2, expected 0x%02x\n", val,
			ili9320_CHIP_ID_MAGIC);
		return -ENODEV;
	}

	/* Probe the system to find the display setting */
	if (ili->conf->input == ili9320_INPUT_UNKNOWN) {
		ret = regmap_read(ili->regmap, ili9320_ENTRY, &val);
		if (ret) {
			dev_err(dev, "can't get entry setting (%d)\n", ret);
			return ret;
		}
		/* Input enum corresponds to HW setting */
		ili->input = (val >> 4) & 0x0f;
		if (ili->input >= ili9320_INPUT_UNKNOWN)
			ili->input = ili9320_INPUT_UNKNOWN;
	} else {
		ili->input = ili->conf->input;
	}

	drm_panel_init(&ili->panel, dev, &ili9320_drm_funcs,
		       DRM_MODE_CONNECTOR_DPI);

	drm_panel_add(&ili->panel);

	return 0;
}

static const struct of_device_id ili9320_of_match[] = {
	{
		.compatible = "ilitek,ili9320",
	},
	{ }
};
MODULE_DEVICE_TABLE(of, ili9320_of_match);

static struct spi_driver ili9320_driver = {
	.probe = ili9320_probe,
	.remove = ili9320_remove,
	.driver = {
		.name = "panel-ili9320",
		.of_match_table = ili9320_of_match,
	},
};
module_spi_driver(ili9320_driver);

MODULE_AUTHOR("Caleb Connolly <caleb@connolly.tech>");
MODULE_DESCRIPTION("ILI9320 LCD panel driver");
MODULE_LICENSE("GPL v2");
