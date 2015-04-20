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

#include "system.h"
#include "usart.h"
#include "tasks.h"
#include "keypad.h"
#include "sticks.h"
#include "lcd.h"
#include "gui.h"
#include "myeeprom.h"
#include "pulses.h"
#include "mixer.h"
#include "sound.h"
#include "eeprom.h"
#include "settings.h"
#include "logo.h"
#include "debug.h"
#include "eeprom.h"


// for now / TBD
// commands are in form:
// [cmd]xx..x\r
// where [cmd] is command
// xx.xx is an optional, command specific character data
// (up to total rcv queue length of 64)
static void remote_process(uint32_t data)
{
	uint16_t cmd = (char)usart_getc();
	if( cmd )
	switch( cmd & 0xFF ) {
	case 'v' :
		usart_puts("ART6-");
		char ver[] = {'0'+VERSION_MAJOR,'.','0'+VERSION_MINOR,'.','0'+VERSION_PATCH};
		usart_puts(ver);
		usart_puts(" " __DATE__ " "__TIME__ " | ");
		puts_dec(sizeof(g_eeGeneral));
		usart_putc(' ');
		puts_dec(sizeof(g_model));
		usart_putc(' ');
		puts_dec(SystemCoreClock / 1000000);
		usart_putc(' ');
		puts_dec(g_latency.g_tmr1Latency_min);
		usart_putc(' ');
		puts_dec(g_latency.g_tmr1Latency_max);
		break;
	case 'd' :
		cmd = usart_getc();
		switch(cmd & 0xFF) {
		// dm - dump model
		case 'm':
			puts_mem(&g_model, sizeof(g_model));
			break;
		// dg - dump general
		case 'g':
			puts_mem(&g_eeGeneral,sizeof(g_eeGeneral));
			break;
		// dump eeprom
		case 'e': {
			for(int i = 0; i < 1<<16; i+= 32) {
				uint8_t buf[32];
				eeprom_read(i,sizeof(buf),&buf);
				puts_mem(&buf,sizeof(buf));
			}
			break;
		}
		}
		break;
	case 'c': {
		puts_hex4( g_eeGeneral.chkSum );
		usart_putc(' ');
		puts_hex4( eeprom_checksum_memory(0, sizeof(g_eeGeneral)-sizeof(g_eeGeneral.chkSum)) );
		usart_putc(' ');
		puts_hex4( eeprom_calc_chksum(&g_eeGeneral, sizeof(g_eeGeneral)-sizeof(g_eeGeneral.chkSum)) );
		usart_putc(' ');
		puts_hex4( g_model.chkSum );
		usart_putc(' ');
		puts_hex4( eeprom_checksum_memory(settings_model_address(g_eeGeneral.currModel),sizeof(g_model)-sizeof(g_model.chkSum)) );
		usart_putc(' ');
		puts_hex4( eeprom_calc_chksum(&g_model,sizeof(g_model)-sizeof(g_eeGeneral.chkSum)) );
		}
		break;
	case 'l' :
		settings_load_current_model();
		break;
	case 'w' :
		usart_puts("todo write");
		break;
	case 'r' :
		usart_puts("todo read");
		break;
	case '?' :
		usart_puts("? r<adr>,<len> w<adr>,<len> ");
		break;
	default:
		usart_putc('?');
		break;
	}
	while(usart_getc()!=0);
	usart_puts("\r\n");
}

/**
  * @brief  Usart receive notification handler
  * @param  data - character received
  * @note   called in interrupt context
  * @retval None
  */
static void rx_handler(char data)
{
	// schedule command action when CR is received (end of command)
	if( data == '\r' )
		task_schedule(TASK_PROCESS_REMOTE, 0, 0);
}


/**
  * @brief  Apply radio settings to other modules
  * @param  None
  * @retval None
  */
void apply_settings()
{
	// set contrast but limit to a reasonable value in case settings were corrupted
	uint16_t contrast = g_eeGeneral.contrast;
	if( contrast < LCD_CONTRAST_MIN ) contrast = LCD_CONTRAST_MIN;
	if( contrast > LCD_CONTRAST_MAX ) contrast = LCD_CONTRAST_MAX;
	lcd_set_contrast(contrast);

	// update volume from global settings
    sound_set_volume(g_eeGeneral.volume);

	if( !g_eeGeneral.disableSplashScreen )
	{
		// Put the logo into out frame buffer
		memcpy(lcd_buffer, logo, LCD_WIDTH * LCD_HEIGHT / 8);
		lcd_update();
		delay_ms(2000);
	}
}

/**
  * @brief  Main Loop for non-IRQ based work
  * @note   Deals with init and non time critical work.
  * @param  None
  * @retval None
  */
int main(void)
{
	// Initialize all things system/board related
	system_init();

	// inistalize uart port
	usart_init();

	// Initialize the task loop.
	task_init();

	// Initialize the keypad scanner (with IRQ wakeup).
	keypad_init();

	// Initialize the LCD
	lcd_init();

	// Initialize the buzzer
	sound_init();

	// Initialize gui task and structure data
	gui_init();

	// Initialize the EEPROM chip access
	eeprom_init();

	// Initalize settings and read data from EEPROM
	settings_init();

	// Apply radio settings
	apply_settings();

	// ToDo: Block here until all switches are set correctly.
	// check_switches();

	// Initialize mixer data
	mixer_init();

	// Initialize the ADC / DMA (sticks)
	sticks_init();

	// Start the radio output
	pulses_init();

	// Navigate gui to the startup page
	gui_navigate(GUI_LAYOUT_MAIN1);

	// for remote commands over usart
	task_register(TASK_PROCESS_REMOTE, remote_process);
	usart_register_rx_handler(rx_handler);

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

// The waiting for interrupt does work,
// However, it makes debugger to lose connection
// even beyond ability to reprogram the CPU with the debugger.
// However, a recovery is possible with STM-UTIL.
#ifdef USE_WFI
		// Wait For an Interrupt
		// __WFI();
		__asm("wfi");
#endif
	}
}
