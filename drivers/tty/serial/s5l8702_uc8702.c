// SPDX-License-Identifier: GPL-2.0
/*
 * S5L8702 Uart driver, ported from Rockbox sources
 * Copyright (C) 2014 by Cástor Muñoz
 * Author: Caleb Connolly <caleb@connolly.tech>
 *
 * UC870x: UART controller for s5l870x
 *
 * This UART is similar to the UART described in s5l8700 datasheet,
 * (see also s3c2416 and s3c6400 datasheets). On s5l8701/2 the UC870x
 * includes autobauding, and fine tunning for Tx/Rx on s5l8702.
 */

#include <linux/io.h>
#include <linux/platform_device.h>
#include <linux/init.h>
#include <linux/sysrq.h>
#include <linux/console.h>
#include <linux/tty.h>
#include <linux/tty_flip.h>
#include <linux/serial_core.h>
#include <linux/serial.h>
#include <linux/delay.h>
#include <linux/clk.h>
#include <linux/of.h>

static struct s3c24xx_serial_drv_data s3c2410_serial_drv_data = {
	.info = &(struct s3c24xx_uart_info) {
		.name		= "Samsung S3C2410 UART",
		.type		= PORT_S3C2410,
		.fifosize	= 16,
		.rx_fifomask	= S3C2410_UFSTAT_RXMASK,
		.rx_fifoshift	= S3C2410_UFSTAT_RXSHIFT,
		.rx_fifofull	= S3C2410_UFSTAT_RXFULL,
		.tx_fifofull	= S3C2410_UFSTAT_TXFULL,
		.tx_fifomask	= S3C2410_UFSTAT_TXMASK,
		.tx_fifoshift	= S3C2410_UFSTAT_TXSHIFT,
		.def_clk_sel	= S3C2410_UCON_CLKSEL0,
		.num_clks	= 2,
		.clksel_mask	= S3C2410_UCON_CLKMASK,
		.clksel_shift	= S3C2410_UCON_CLKSHIFT,
	},
	.def_cfg = &(struct s3c2410_uartcfg) {
		.ucon		= S3C2410_UCON_DEFAULT,
		.ufcon		= S3C2410_UFCON_DEFAULT,
	},
};

static const struct of_device_id s5l8xxx_uart_dt_match[] = {
	{ .compatible = "samsung,s3c2410-uart",
		.data = (void *)S3C2410_SERIAL_DRV_DATA },
	{},
};
MODULE_DEVICE_TABLE(of, s5l8xxx_uart_dt_match);

static struct platform_driver s5l8xxx_serial_driver = {
	.probe		= s5l8xxx_serial_probe,
	.remove		= s5l8xxx_serial_remove,
	.id_table	= s5l8xxx_serial_driver_ids,
	.driver		= {
		.name	= "s5l-uart",
		.of_match_table	= of_match_ptr(s5l8xxx_uart_dt_match),
	},
};

module_platform_driver(s5l8xxx_serial_driver);