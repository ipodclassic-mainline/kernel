// SPDX-License-Identifier: GPL-2.0
/*
 * S5L8702 specific register definitions
 *
 * Copyright (C) 2020 Caleb Connolly <caleb@Connolly.tech>
 */
#include <asm/memory.h>

// IRAM0
#define IRAM0_BASE	UL(0x22000000)
#define IRAM0_SIZE	0x20000
// IRAM1
#define IRAM1_BASE	UL(0x22020000)
#define IRAM1_SIZE	0x20000
// DRAM
#define DRAM_BASE	UL(0x08000000)
