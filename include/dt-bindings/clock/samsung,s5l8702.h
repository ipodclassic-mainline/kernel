/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2020 Caleb Connolly <caleb@connolly.tech>
 *
 * Device Tree binding constants clock controllers of Samsung S5L8702.
 */

#ifndef _DT_BINDINGS_CLOCK_SAMSUNG_S5L8702_CLOCK_H
#define _DT_BINDINGS_CLOCK_SAMSUNG_S5L8702_CLOCK_H

/*
 * Let each exported clock get a unique index, which is used on DT-enabled
 * platforms to lookup the clock from a clock specifier. These indices are
 * therefore considered an ABI and so must not be changed. This implies
 * that new clocks should be added either in free spaces between clock groups
 * or at the end.
 */

// Fixed oscillators
#define CLKOSC0		0 // 12 MHz
#define CLKOSC1		0 // 32.768 KHz
#define CLKOSC2		2 // aka UNKOSC, 24 MHz
// One of CLKOSC0 or CLKOSC1
#define OSCCLK		3

// Core clocks
#define PLL0		0
#define PLL1		1
#define PLL2		2

// Clock gates happen here, then some clock dividers
// and then some other clocks

#define FCLK		20
#define U2L_CLK		21
#define SVID_CLK	22
#define AUDX_CLK	23
#define ECLK		24
#define MIU_CLK		ECLK // MIU and ECLK come from the same divider
						 // and are therefore always the same
#define U5L_CLK		25

// FCLK derived clocks
// CDIV
#define ARM_CLK		30 // CPU core clock
// HDIV
#define SDR_CLK		32
#define CLK_SHA1	33
#define CLK_LCD		34
#define CLK_USBOTG	35
#define CLK_SMX		36
#define CLK_SM1		37
#define CLK_ATA		38
#define CLK_UNK		39
#define CLK_UNK		40
#define CLK_NAND	41
#define CLK_SDCI	42
#define CLK_AES		43
#define CLK_UNK		44
#define CLK_ECC		45
#define CLK_UNK		46
#define CLK_EV0		47
#define CLK_EV1		48
#define CLK_EV2		49
#define CLK_UNK		50
#define CLK_UNK		51
#define CLK_UNK		52
#define CLK_UNK		53
#define CLK_UNK		54
#define CLK_UNK		55
#define CLK_UNK		56
#define CLK_UNK		57
#define CLK_DMA0	58
#define CLK_DMA1	59
#define CLK_UNK		60
#define CLK_UNK		61
#define CLK_UNK		62
#define CLK_ROM		63
#define CLK_UNK		64

// PCLK derived clocks
// PDIV
#define CLK_RTC		70
#define CLK_CWHEEL	71
#define CLK_SPI0	72
#define CLK_USBPHY	73
#define CLK_I2C0	74
#define CLK_TIMER	75
#define CLK_I2C1	76
#define CLK_I2S0	77
#define CLK_UNK		78
#define CLK_UART	79
#define CLK_I2S1	80
#define CLK_SPI1	81
#define CLK_GPIO	82
#define CLK_SBOOT	83
#define CLK_CHIPID	84
#define CLK_SPI2	85
#define CLK_I2S2	86
#define CLK_UNK		87

#endif /* _DT_BINDINGS_CLOCK_SAMSUNG_S5L8702_CLOCK_H */
