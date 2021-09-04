/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (C) 2021 Caleb Connolly <caleb@connolly.tech>
*/

#include <linux/serial_reg.h>

		.macro	addruart, rp, rv, tmp
		ldr	\rp, = CONFIG_DEBUG_UART_PHYS
		ldr	\rv, = CONFIG_DEBUG_UART_VIRT
		.endm

		.macro	waituarttxrdy,rd,rx
1002:		ldr	\rd, [\rx, #UC870X_STAT]	@ STAT
		tst	\rd, #1 << 1		@ TXEMPTY bit
		beq	1002b			@ wait until transmit done
		.endm

		.macro	waituartcts,rd,rx
		.endm

		.macro	senduart,rd,rx
		str	\rd, [\rx, #UC870X_TXDATA]	@ TXDATA
		.endm

		.macro	busyuart,rd,rx
1002:		ldr	\rd, [\rx, #UC870X_STAT]	@ STAT
		tst	\rd, #1 << 1		@ TXEMPTY bit
		beq	1002b			@ wait until transmit done
		.endm
