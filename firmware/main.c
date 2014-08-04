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

/* Description:
 *
 * This module contains the entrypoint, setup and main loop.
 *
 */

#include "stm32f10x.h"
#include "tasks.h"
#include "keypad.h"
#include "sticks.h"
#include "lcd.h"
#include "gui.h"
#include "myeeprom.h"
#include "pulses.h"
#include "mixer.h"
#include "sound.h"

EEGeneral  g_eeGeneral;
ModelData  g_model;
uint8_t SlaveMode;		// Trainer Slave

/**
  * @brief  This function handles the SysTick.
  * @param  None
  * @retval None
  */
void SysTick_Handler(void)
{
	system_ticks++;
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

	// Initialize the buzzer
	sound_init();

	// Block here until all switches are set correctly.
	//check_switches();
	strncpy(g_model.name, "SK450 Quad", 10);
	g_model.tmrVal = 360;

	mixer_init();

	// Start the radio output.
	pulses_init();

	gui_navigate(GUI_LAYOUT_MAIN1);

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
		/*
		static uint8_t tick;
		lcd_set_cursor(0, 48);
		lcd_write_int(tick++, 1, 0);
		lcd_update();
		*/

		// Process any tasks.
		task_process_all();


		// Wait for an interrupt
		//PWR_EnterSTANDBYMode();
	}
}
