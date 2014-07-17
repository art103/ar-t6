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

#include "stm32f10x_gpio.h"
#include "stm32f10x_exti.h"
#include "misc.h"
#include "keypad.h"

/**
  * @brief  ExtI IRQ handler
  * @note   Handles IRQ on GPIO lines 10-15.
  * @param  None
  * @retval None
  */
void EXTI15_10_IRQHandler(void)
{
	uint32_t flags = EXTI->PR;
	
	if ((flags & KEYPAD_EXTI_LINES) != 0)
	{
		// Schedule the keys to be scanned.
		task_schedule(TASK_PROCESS_KEYPAD, 0);
		
		// Clear the IRQ
		EXTI->PR = KEYPAD_EXTI_LINES;
	}
	
	if ((flags & ROTARY_EXTI_LINES) != 0)
	{
		// Read the encoder lines
		uint16_t gpio = GPIO_ReadInputData(GPIOB);
		
		if ((gpio & (1 << 15)) == 0)
		{
			// Falling edge
			if ((gpio & (1 << 14)) == 0)
				task_schedule(TASK_PROCESS_KEYPAD, 1);
			else
				task_schedule(TASK_PROCESS_KEYPAD, 2);
		}
		else
		{
			// Rising edge
			if ((gpio & (1 << 14)) == 0)
				task_schedule(TASK_PROCESS_KEYPAD, 2);
			else
				task_schedule(TASK_PROCESS_KEYPAD, 1);
		}
		
		// Clear the IRQ
		EXTI->PR = ROTARY_EXTI_LINES;
	}
}

/**
  * @brief  ExtI IRQ handler
  * @note   Handles IRQ on GPIO lines 5-9.
  * @param  None
  * @retval None
  */
void EXTI9_5_IRQHandler(void)
{
	uint32_t flags = EXTI->PR;
	if ((flags & KEYPAD_EXTI_LINES) != 0)
	{
		// Schedule the keys to be scanned.
		task_schedule(TASK_PROCESS_KEYPAD, 0);
		
		// Clear the IRQ
		EXTI->PR = KEYPAD_EXTI_LINES;
	}
}
