/*
 * system.c
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * Author: Michal Krombholz michkrom@github.com
 */

/* Description:
 *
 * stm32 sys tick interrupt and counter, delay routines
 *
 */

#ifndef SYSTEM_H_
#define SYSTEM_H_

#include <stdint.h>

// 1ms tick counter
extern volatile uint32_t system_ticks;

// CPU core clock [Hz]
extern uint32_t SystemCoreClock;

// Jumps to stm32 build-in bootloader
void enter_bootloader(void);

// Long delay measured with system_ticks
void delay_ms(uint32_t delay);

// Small delay may be inacurate for long term (use only for <1ms)
void delay_us(uint32_t delay);

// Initialize all things system
extern void system_init();

#endif /* SYSTEM_H_ */
