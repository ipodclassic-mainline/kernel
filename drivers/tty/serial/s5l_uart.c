// SPDX-License-Identifier: GPL-2.0
// Copyright (c) 2021 Caleb Connolly <caleb@connolly.tech>

#include <linux/console.h>
#include <linux/module.h>
#include <linux/of_irq.h>
#include <linux/platform_device.h>
#include <linux/serial_core.h>
#include <linux/tty.h>
#include <linux/tty_flip.h>

#include <asm/setup.h>

#define UC870X_STAT				0x10
#define 	UC870X_STAT_RDY		(1 << 1)
#define UC870X_TXDATA			0x20

#define S5L_UART_NAME			"s5l-uart"
#define S5L_UART_DEV_NAME		"ttySL"

static struct uart_port s5l_uart_port;

// static const struct uart_ops apple_uart_ops = {
//     .tx_empty       = apple_uart_tx_empty,
//     .set_mctrl      = apple_uart_set_mctrl,
//     .get_mctrl      = apple_uart_get_mctrl,
//     .stop_tx        = apple_uart_stop_tx,
//     .start_tx       = apple_uart_start_tx,
//     .stop_rx        = apple_uart_stop_rx,
//     .enable_ms      = apple_uart_enable_ms,
//     .break_ctl      = apple_uart_break_ctl,
//     .startup        = apple_uart_startup,
//     .shutdown       = apple_uart_shutdown,
//     .set_termios    = apple_uart_set_termios,
//     .type           = apple_uart_type,
//     .config_port    = apple_uart_config_port,
// };

static void s5l_uart_console_putchar(struct uart_port *port, int c)
{
    while(readl(port->membase + UC870X_STAT) & UC870X_STAT_RDY)
        cpu_relax();

    writel(c, port->membase + UC870X_TXDATA);
}

static void s5l_uart_console_write(struct console *co, const char *s, unsigned int count)
{
	struct uart_port *port = &s5l_uart_port;
    uart_console_write(port, s, count, s5l_uart_console_putchar);
}

static int __init s5l_uart_console_setup(struct console *co, char *options)
{
	struct uart_port *port = &s5l_uart_port;
    int baud = 115200;
    int parity = 'n';
    int flow = 'n';
    int bits = 8;

    if (!port->membase)
        return -ENODEV;

    if (options)
        uart_parse_options(options, &baud, &parity, &bits, &flow);

    return uart_set_options(port, co, baud, parity, bits, flow);
}

static struct uart_driver s5l_uart_driver;
static struct console s5l_uart_console = {
    .name   = S5L_UART_DEV_NAME,
    .write  = s5l_uart_console_write,
    .device = uart_console_device,
    .setup  = s5l_uart_console_setup,
    .flags  = CON_PRINTBUFFER,
    .index  = -1,
    .data   = &s5l_uart_driver,
};

// static void s5l_uart_console_write(struct console *co, const char *s, unsigned int count)
// {
//     struct uart_port *port = &s5l_uart_port;

//     uart_console_write(port, s, count, s5l_uart_console_putchar);
// }

static int __init s5l_uart_console_init(void)
{
    register_console(&s5l_uart_console);
    return 0;
}
console_initcall(s5l_uart_console_init);


static void s5l_uart_early_console_write(struct console *co, const char *s, u_int count)
{
    struct earlycon_device *dev = co->data;

    uart_console_write(&dev->port, s, count, s5l_uart_console_putchar);
}

static int __init s5l_uart_early_console_setup(struct earlycon_device *device, const char *opt)
{
	early_print("Setup s5l_uart!");
    if (!device->port.membase)
        return -ENODEV;
    device->con->write = s5l_uart_early_console_write;

    return 0;
}

OF_EARLYCON_DECLARE(s5l_uart, "samsung,s5l-uart", s5l_uart_early_console_setup);

static struct  uart_driver s5l_uart_driver = {
    .owner       = THIS_MODULE,
    .driver_name = S5L_UART_NAME,
    .dev_name    = S5L_UART_DEV_NAME,
    .cons        = &s5l_uart_console,
    .nr          = 1,
};

// static int s5l_uart_probe(struct platform_device *pdev)
// {
//     struct uart_port *port = &s5l_uart_port;
//     struct resource *res;
//     int ret;

//     res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
//     if (res == NULL) {
//         dev_err(&pdev->dev, "Missing regs\n");
//         ret = -ENODEV;
//         return ret;
//     }
//     port->membase = devm_ioremap(&pdev->dev, res->start,
//                             resource_size(res));

//     ret = platform_get_irq(pdev, 0);
//     s5l_uart_irq_num = ret;

//     port->irq = s5l_uart_irq_num;
//     port->uartclk = 12000000;
//     port->fifosize = 16;
//     port->iotype = UPIO_MEM32;
//     port->flags = UPF_BOOT_AUTOCONF | UPF_FIXED_PORT;
//     port->line = 0;
//     port->ops = &s5l_uart_ops;
//     port->dev = &pdev->dev;

//     ret = uart_add_one_port(&s5l_uart_driver, port);
//     if (ret) {
//         dev_err(&pdev->dev, "Adding port %d failed: %d\n", index, ret);
//         return ret;
//     }

//     return 0;
// }

// static int s5l_uart_remove(struct platform_device *pdev)
// {
//     struct uart_port *port = &s5l_uart_port;

//     uart_remove_one_port(&s5l_uart_driver, port);

//     return 0;
// }

static const struct of_device_id s5l_uart_dt_ids[] = {
    { .compatible = "samsung,s5l-uart" },
    { /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, s5l_uart_dt_ids);

// static struct platform_driver s5l_uart_driver = {
//     .probe          = s5l_uart_probe,
//     .remove         = s5l_uart_remove,
//     .driver         = {
//         .name   = S5L_UART_NAME,
//         .of_match_table = s5l_uart_dt_ids,
//     },
// };

// static int __init s5l_uart_init(void)
// {
//     int ret = uart_register_driver(&s5l_uart_driver);

//     if (ret) {
//         pr_err("%s: uart registration failed: %d\n", __func__, ret);
//         return ret;
//     }
//     ret = platform_driver_register(&s5l_uart_driver);
//     if (ret) {
//         uart_unregister_driver(&s5l_uart_driver);
//         pr_err("%s: drv registration failed: %d\n", __func__, ret);
//         return ret;
//     }

//     return 0;
// }

// static void __exit s5l_uart_exit(void)
// {
//     platform_driver_unregister(&s5l_uart_driver);
//     uart_unregister_driver(&s5l_uart_driver);
// }

// module_init(s5l_uart_init);
// module_exit(s5l_uart_exit);

MODULE_AUTHOR("Caleb Connolly <caleb@connolly.tech>");
MODULE_DESCRIPTION("S5L8702 (Apple iPod Classic) UART Driver");
MODULE_LICENSE("GPL");