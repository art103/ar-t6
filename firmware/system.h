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
 * stm32 sys tick interrupt and counter
 *
 */

#ifndef SYSTEM_H_
#define SYSTEM_H_

#include <stdint.h>

// main.c but used only in gui.c
void enter_bootloader(void);

extern volatile uint32_t system_ticks;
extern system_init();

extern uint32_t SystemCoreClock;          /*!< System Clock Frequency (Core Clock) */

#endif /* SYSTEM_H_ */
