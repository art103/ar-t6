/*
 *                  Copyright 2014 ARTaylor.co.uk
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
 * Author: Richard Taylor (richard@artaylor.co.uk)
 */

#include "stm32f10x.h"
#include "tasks.h"
#include "keypad.h"
#include "lcd.h"

volatile uint32_t system_ticks = 0;

/**
  * @brief  Delay timer using the system tick timer
  * @param  delay: delay in ms.
  * @retval None
  */
void delay_ms(uint32_t delay)
{
	uint32_t start = system_ticks;
	while (system_ticks <  start + delay);
}

/**
  * @brief  Spin loop us delay routine
  * @param  delay: delay in us.
  * @retval None
  */
void delay_us(uint32_t delay)
{
	volatile uint32_t i;
	for (i=0; i < delay * 3; ++i);
}

/**
  * @brief  Main Loop for non-IRQ based work
  * @note   Deals with init and non time critical work.
  * @param  None
  * @retval None
  */
int main(void)
{
	// PLL and stack setup has aready been done.

	// 1ms System tick
	SysTick_Config(SystemCoreClock / 1000);

	// Initialize the task loop.
	task_init();

	// Initialize the LCD and display logo.
	lcd_init();

	// Initialize the keypad scanner (with IRQ wakeup).
	keypad_init();

	while (1)
	{
		task_process_all();
	}
}
