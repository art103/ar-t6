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
#include "sticks.h"
#include "lcd.h"
#include "gui.h"

/**
  * @brief  Main Loop for non-IRQ based work
  * @note   Deals with init and non time critical work.
  * @param  None
  * @retval None
  */
int main(void)
{
	uint8_t tick;
	// PLL and stack setup has aready been done.

	// 1ms System tick
	SysTick_Config(SystemCoreClock / 1000);
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR, ENABLE);

	// Initialize the task loop.
	task_init();

	// Initialize the keypad scanner (with IRQ wakeup).
	keypad_init();

	// Initialize the LCD and display logo.
	lcd_init();
	gui_init();

	// Initialize the ADC / DMA
	sticks_init();

	/*
	 * The main loop will sit in low power mode waiting for an interrupt.
	 *
	 * The ADC is running in continuous scanning mode with DMA transfer of the results to memory.
	 * An interrupt will fire when the full conversion scan has completed.
	 * This will schedule the "PROCESS_STICKS" task.
	 * The switches (SWA-SWD) will be polled at this point.
	 *
	 * Keys (trim, buttons and scroll wheel) are interrupt driven. "PROCESS_KEYS" will be scheduled
	 * when any of them are pressed.
	 *
	 * PPM is driven by Timer0 in interrupt mode autonomously from pwm_data.
	 *
	 */

	while (1)
	{
		// Process any tasks.
		task_process_all();

		/*
		lcd_set_cursor(0, 48);
		lcd_write_int(tick++, 1, 0);
		lcd_update();
		*/

		// Wait for an interrupt
		//PWR_EnterSTANDBYMode();
	}
}
