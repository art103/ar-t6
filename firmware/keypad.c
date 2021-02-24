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
 * This is an IRQ driven keypad driver.
 * Pressed keys are stored and scheduled for processing by the main loop.
 * This will then call into the mixer (for trim) and GUI.
 * GUI events are asynchronous, and will be processed on the next main loop cycle.
 *
 */

#include "stm32f10x.h"
#include "system.h"
#include "keypad.h"
#include "tasks.h"
#include "gui.h"
#include "myeeprom.h"
#include "lcd.h"
#include "sound.h"

#define ROW_MASK       (0x07 << 12)
#define COL_MASK       (0x0F << 8)
#define ROW(n)         (1 << (12 + n))
#define COL(n)         (1 << (8 + n))

#define KEY_HOLDOFF			10
#define KEY_REPEAT_DELAY	500
#define KEY_REPEAT_TIME		100

// Keys that have been pressed since the last check.
static uint32_t keys_pressed = 0;
// key repeat/MENU state vars
static uint32_t key_repeat = 0;
static uint32_t key_time = 0;
static uint32_t last_keypress = 0;

static void keypad_process(uint32_t data);

/**
 * @brief  Initialise the keypad scanning pins.
 * @note   Row used as output, Col as input.
 * @param  None
 * @retval None
 */
void keypad_init(void) {
	GPIO_InitTypeDef gpioInit;
	EXTI_InitTypeDef extiInit;
	NVIC_InitTypeDef nvicInit;

	// Enable the GPIO block clocks and setup the pins.
	RCC_APB2PeriphClockCmd(
			RCC_APB2Periph_GPIOB | RCC_APB2Periph_GPIOC | RCC_APB2Periph_AFIO,
			ENABLE);

	gpioInit.GPIO_Speed = GPIO_Speed_2MHz;

	// Configure the Column pins
	gpioInit.GPIO_Pin = COL_MASK;
	gpioInit.GPIO_Mode = GPIO_Mode_Out_OD;
	GPIO_ResetBits(GPIOB, COL_MASK);
	GPIO_Init(GPIOB, &gpioInit);

	// Configure the Row pins and SWA, SWB, SWC.
	gpioInit.GPIO_Pin = ROW_MASK | GPIO_Pin_0 | GPIO_Pin_1 | GPIO_Pin_5;
	gpioInit.GPIO_Mode = GPIO_Mode_IPU;
	GPIO_Init(GPIOB, &gpioInit);

	// Configure SWD.
	gpioInit.GPIO_Pin = GPIO_Pin_13;
	gpioInit.GPIO_Mode = GPIO_Mode_IPU;
	GPIO_Init(GPIOC, &gpioInit);

	// Set the cols as Ext. Interrupt sources.
	GPIO_EXTILineConfig(GPIO_PortSourceGPIOB, 12);
	GPIO_EXTILineConfig(GPIO_PortSourceGPIOB, 13);
	GPIO_EXTILineConfig(GPIO_PortSourceGPIOB, 14);

	gpioInit.GPIO_Pin = 1 << 15;
	gpioInit.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_Init(GPIOC, &gpioInit);

	// Set the rotary encoder as Ext. Interrupt source.
	GPIO_EXTILineConfig(GPIO_PortSourceGPIOC, 15);

	// Configure keypad lines as falling edge IRQs
	extiInit.EXTI_Mode = EXTI_Mode_Interrupt;
	extiInit.EXTI_Trigger = EXTI_Trigger_Falling;
	extiInit.EXTI_LineCmd = ENABLE;
	extiInit.EXTI_Line = KEYPAD_EXTI_LINES;
	EXTI_Init(&extiInit);

	// Configure rotary line as rising + falling edge IRQ
	extiInit.EXTI_Trigger = EXTI_Trigger_Rising_Falling;
	extiInit.EXTI_Line = ROTARY_EXTI_LINES;
	EXTI_Init(&extiInit);

	// Configure the Interrupt to the lowest priority
	nvicInit.NVIC_IRQChannelPreemptionPriority = 15;
	nvicInit.NVIC_IRQChannelSubPriority = 0x0F;
	nvicInit.NVIC_IRQChannelCmd = ENABLE;
	nvicInit.NVIC_IRQChannel = EXTI15_10_IRQn;
	NVIC_Init(&nvicInit);

	task_register(TASK_PROCESS_KEYPAD, keypad_process);
}

/**
 * @brief  Poll to see if a specific key has been pressed
 * @note
 * @param  key: Key to check.
 * @retval bool: TRUE if pressed, FALSE if not.
 */
uint8_t keypad_get_pressed(KEYPAD_KEY key) {
	if ((keys_pressed & key) != 0) {
		keys_pressed &= ~key;
		return TRUE;
	}
	return FALSE;
}

/**
 * @brief  Scan the switches' state
 * @note
 * @param  None
 * @retval uint8_t: Bitmask of the switches
 */
uint8_t keypad_get_switches(void) {
	uint8_t switches = 0;
	if (!GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_0))
		switches |= SWITCH_SWA;

	if (!GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_1))
		switches |= SWITCH_SWB;

	if (!GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_5))
		switches |= SWITCH_SWC;

	if (!GPIO_ReadInputDataBit(GPIOC, GPIO_Pin_13))
		switches |= SWITCH_SWD;

	return switches;
}

/**
 * @brief  Check a specific switch
 * @note  sw==0 always on!
 * @param  sw: The Switch to check. sw==0 always on
 * @retval bool: true if on, false if off.
 */
uint8_t keypad_get_switch(KEYPAD_SWITCH sw) {
	return sw == 0 || (keypad_get_switches() & sw);
}

/**
 * @brief  Check if any switches are up.
 * @note   Blocks transmitter startup if switches aren't safe
 * @param  None
 * @retval None
 */
void check_switches(void) {
	uint16_t previous_ticks = 0;
	uint16_t switchStatus = keypad_get_switches();
	uint8_t mask = 0;
	char dir = 0x00;

	sound_set_volume(10); 

	if (g_eeGeneral.disableSwitchWarning == 1 && (g_eeGeneral.switchWarningStates ^ switchStatus) != 0) {
		lcd_clear();
		lcd_draw_rect(0, 0, LCD_WIDTH - 1, CHAR_HEIGHT*2 + 6, LCD_OP_SET, RECT_FILL);
		
		lcd_set_cursor(CHAR_WIDTH*2, 3);		
		lcd_write_string(msg[GUI_MSG_SWITCH_WARNING], LCD_OP_XOR, NEW_LINE | CHAR_4X);


		lcd_set_cursor(3*CHAR_WIDTH, 7*CHAR_HEIGHT);
		lcd_write_string(msg[GUI_MSG_SET_SWITCH_TO_POS], LCD_OP_SET, FLAGS_NONE);



		sound_play_tune(AU_POT_STICK_MIDDLE);
		previous_ticks = system_ticks;
		
		while( (g_eeGeneral.switchWarningStates ^ switchStatus) != 0 ) {
			// clean row
					lcd_draw_rect(3*CHAR_WIDTH -1, 5*CHAR_HEIGHT-1,
								  19*CHAR_WIDTH+18, 6*CHAR_HEIGHT,
								  LCD_OP_CLR, RECT_FILL);		

			lcd_set_cursor(3*CHAR_WIDTH, 5*CHAR_HEIGHT);
			// list switches in wrong position
			for (int i=0; i<4; i++) {
				// all switches need to be set to appropriate position
				// list only wrong switched
				mask = 0x01 << i;

				if (((g_eeGeneral.switchWarningStates ^ switchStatus) & mask) != 0) {
					lcd_write_string(switches[i+1], LCD_OP_CLR, FLAGS_NONE); // switches[0] is empty switch --> '---'
					
					// write appropriate switch direction
					if( ((0x01 << i) & g_eeGeneral.switchWarningStates) != 0) {
						dir = 0x01;
					} else {
						dir = 0x00; 
					}

					lcd_write_char(dir, LCD_OP_CLR, FLAGS_NONE); 

					lcd_move_cursor(1, 0);
				}
			}

			lcd_update();

			switchStatus = keypad_get_switches();
			if ( system_ticks >= previous_ticks + KEYPAD_SAFETY_TUNE_REPEAT ) {
				sound_play_tune(AU_POT_STICK_MIDDLE);
				previous_ticks = system_ticks;
			}
		}
		// since I fiddled with the volume, change it back to user setting.
		
	}
	sound_set_volume(g_eeGeneral.volume);
}

/**
 * @brief  Abort the key repeat loop
 * @note
 * @param  None
 * @retval None
 */
void keypad_cancel_repeat(void) {
	key_repeat = 0;
	key_time = 0;
	task_deschedule(TASK_PROCESS_KEYPAD);
}

/**
 * @brief  Process keys and drive the GUI.
 * @note   Called from the scheduler.
 * @param  data: EXTI lines that triggered the update.
 * @retval None
 */
static void keypad_process(uint32_t data) {

	// Scan the keys.
	KEYPAD_KEY key = keypad_scan_keys();
	// Scanning the keys causes the IRQ to fire, so de-schedule for now.
	// TODO: suppress IRQ for scan
	task_deschedule(TASK_PROCESS_KEYPAD);

	// nothing pressed now? reset timing but pass along as further states need to know keys are up
	if (key == KEY_NONE) {
		key_repeat = KEY_NONE;
		key_time = 0;
	}
	else {
		if( key_time == 0 ) {
			key_time = system_ticks;
			// first time seen key, schedule Debounce
			task_schedule(TASK_PROCESS_KEYPAD, 0, KEY_HOLDOFF);
			return;
		}
	}

	// key still pressed and Debounced
	// TODO: make sure this is the same key

	// Data is used to send the rotary encoder data.
	// TODO: debounce these somehow
	if( data ) {
		switch (data) {
			case 1:
				key = KEY_RIGHT;
				last_keypress = system_ticks;
				break;
			case 2:
				key = KEY_LEFT;
				last_keypress = system_ticks;
				break;
		}
		key_time = system_ticks;
	}


	// got debounced key? schedule further checking until released
	if( key != KEY_NONE ) {
		task_schedule(TASK_PROCESS_KEYPAD, 0, KEY_REPEAT_TIME);
	}

	// from now on the 'key' may be suppressed and key_time should not be changed!

	// KEY_MENU/KEY_SEL state machine
	// note: KEY_MENU is a virtual key issued by long hold of KEY_SEL
	// the state machine here suppresses issuing KEY_SEL until released quickly, otherwise issues KEY_MENU
	static enum { KEY_SEL_UP, KEY_SEL_DOWN, KEY_SEL_MENU_ISSUED } key_sel_state = KEY_SEL_UP;
	switch( key_sel_state )
	{
	case KEY_SEL_UP:
		if( key & KEY_SEL ) {
			// KEY_SEL pressed, move on but suppress the key for now
			key = KEY_NONE;
			key_sel_state = KEY_SEL_DOWN;
			key_repeat = 0;
		}
		break;
	case KEY_SEL_DOWN:
		if( key & KEY_SEL ) {
			// KEY_SEL still pressed, check timer to issue KEY_MENU, ignore if not expired yet
			if (key_time + KEY_REPEAT_DELAY < system_ticks) {
				key = KEY_MENU;
				key_repeat = 0;
				key_sel_state = KEY_SEL_MENU_ISSUED;
			} else {
				key = KEY_NONE;
			}
		} else {
			// KEY_SEL no longer pressed, issue KEY_SEL as it was not hold down long enough for KEY_MENU long press
			key = KEY_SEL;
			key_sel_state = KEY_SEL_UP;
		}
		break;
	case KEY_SEL_MENU_ISSUED:
		if( !(key & KEY_SEL) ) {
			// KEY_SEL released...reset state
			key_sel_state = KEY_SEL_UP;
		} else {
			key = KEY_NONE;
		}
		break;
	}

	if (key != KEY_NONE ) {
		// handle all but MENU/SEL keys
		if( !(key & (KEY_MENU|KEY_SEL)) ) {
			if( key & TRIM_KEYS ) {
				key_repeat = key;
			} else if( key_time + KEY_REPEAT_TIME < system_ticks ) {
				// For non-trim keys, don't repeat when timer expired
				// we should not be here the first time as repeat time >> holdoff time
				key = KEY_NONE;
			}
		}
	}

	if( key != KEY_NONE ) {
		// Add the key to the pressed list.
		keys_pressed |= key;
		// Send the key to the UI.
		gui_input_key(key);
	}
}

/**
 * @brief  Scan the keypad and return the active key.
 * @note   Will only return the first key found if multiple keys pressed.
 * @param  None
 * @retval KEYPAD_KEY
 *   Returns the active key
 *     @arg KEY_xxx: The key that was pressed
 *     @arg KEY_NONE: No key was pressed
 */
KEYPAD_KEY keypad_scan_keys(void) {
	KEYPAD_KEY key = KEY_NONE;
	bool found = FALSE;
	uint16_t rows;
	uint8_t col;

	for (col = 0; col < 4; ++col) {
		// Walk a '0' down the cols.
		GPIO_SetBits(GPIOB, COL_MASK);
		GPIO_ResetBits(GPIOB, COL(col));

		// Allow some time for the GPIO to settle.
		delay_us(100);

		// The rows are pulled high externally.
		// Any '0' seen here is due to a switch connecting to our active '0' on a column.
		rows = GPIO_ReadInputData(GPIOB);
		if ((rows & ROW_MASK) != ROW_MASK) {
			// Only support one key pressed at a time.
			found = TRUE;
			break;
		}
	}

	// Set the cols to all '0'.
	GPIO_ResetBits(GPIOB, COL_MASK);

	if (found) {
		rows = ~rows & ROW_MASK;
		last_keypress = system_ticks;

		switch (col) {
		case 0:
			if ((rows & ROW(0)) != 0)
				key = KEY_CH1_UP;
			else if ((rows & ROW(1)) != 0)
				key = KEY_CH3_UP;
			break;

		case 1:
			if ((rows & ROW(0)) != 0)
				key = KEY_CH1_DN;
			else if ((rows & ROW(1)) != 0)
				key = KEY_CH3_DN;
			else if ((rows & ROW(2)) != 0)
				key = KEY_SEL;
			break;

		case 2:
			if ((rows & ROW(0)) != 0)
				key = KEY_CH2_UP;
			else if ((rows & ROW(1)) != 0)
				key = KEY_CH4_UP;
			else if ((rows & ROW(2)) != 0)
				key = KEY_OK;
			break;

		case 3:
			if ((rows & ROW(0)) != 0)
				key = KEY_CH2_DN;
			else if ((rows & ROW(1)) != 0)
				key = KEY_CH4_DN;
			else if ((rows & ROW(2)) != 0)
				key = KEY_CANCEL;
			break;

		default:
			break;
		}
	}

	return key;
}

uint32_t key_inactivity() {
	return system_ticks - last_keypress;
}

/**
 * @brief  ExtI IRQ handler
 * @note   Handles IRQ on GPIO lines 10-15.
 * @param  None
 * @retval None
 */
void EXTI15_10_IRQHandler(void) {
	uint32_t flags = EXTI->PR;

	if ((flags & KEYPAD_EXTI_LINES) != 0) {
		// Clear the IRQ
		EXTI->PR = KEYPAD_EXTI_LINES;

		// Schedule the keys to be scanned.
		task_schedule(TASK_PROCESS_KEYPAD, 0, 0);
	}

	if ((flags & ROTARY_EXTI_LINES) != 0) {
		// Clear the IRQ
		EXTI->PR = ROTARY_EXTI_LINES;

		// Read the encoder lines
		uint16_t gpio = GPIO_ReadInputData(GPIOC);

		if ((gpio & (1 << 15)) == 0) {
			// Falling edge
			if ((gpio & (1 << 14)) == 0)
				task_schedule(TASK_PROCESS_KEYPAD, 1, 0);
			else
				task_schedule(TASK_PROCESS_KEYPAD, 2, 0);
		} else {
			// Rising edge
			if ((gpio & (1 << 14)) == 0)
				task_schedule(TASK_PROCESS_KEYPAD, 2, 0);
			else
				task_schedule(TASK_PROCESS_KEYPAD, 1, 0);
		}
	}
}
