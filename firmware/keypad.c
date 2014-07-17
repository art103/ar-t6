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

#include "keypad.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x_exti.h"
#include "misc.h"

#define ROW_MASK       (0x07 << 12)
#define COL_MASK       (0x0F << 8)
#define ROW(n)         (1 << (12 + n))
#define COL(n)         (1 << (8 + n))
// Row 0
#define KEY_CH1_UP     (~COL(0) & COL_MASK)
#define KEY_CH1_DN     (~COL(1) & COL_MASK)
#define KEY_CH2_UP     (~COL(2) & COL_MASK)
#define KEY_CH2_DN     (~COL(3) & COL_MASK)
// Row 1
#define KEY_CH3_UP     (~COL(0) & COL_MASK)
#define KEY_CH3_DN     (~COL(1) & COL_MASK)
#define KEY_CH4_UP     (~COL(2) & COL_MASK)
#define KEY_CH4_DN     (~COL(3) & COL_MASK)
// Row 2
//#define KEY_           (~COL(0) & COL_MASK)
#define KEY_SEL        (~COL(1) & COL_MASK)
#define KEY_OK         (~COL(2) & COL_MASK)
#define KEY_CANCEL     (~COL(3) & COL_MASK)

/**
  * @brief  Initialise the keypad scanning pins.
  * @note   Row used as output, Col as input.
  * @param  None
  * @retval None
  */
void keypad_init(void)
{
	GPIO_InitTypeDef cols = { 
								.GPIO_Pin = COL_MASK,
								.GPIO_Speed = GPIO_Speed_2MHz,
								.GPIO_Mode = GPIO_Mode_IN_FLOATING
						    };

	GPIO_InitTypeDef rows = { 
								.GPIO_Pin = ROW_MASK,
								.GPIO_Speed = GPIO_Speed_2MHz,
								.GPIO_Mode = GPIO_Mode_Out_OD
						    };
	
	GPIO_InitTypeDef gpioInit;
	EXTI_InitTypeDef extiInit;
	NVIC_InitTypeDef nvicInit;
	
	// Enable the GPIO block clocks and setup the pins.
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB | RCC_APB2Periph_AFIO, ENABLE);

	gpioInit.GPIO_Speed = GPIO_Speed_2MHz;

	// Configure the Column pins
	gpioInit.GPIO_Pin = COL_MASK;
	gpioInit.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_Init(GPIOB, &gpioInit);
	
	// Configure the Row pins.
	gpioInit.GPIO_Pin = ROW_MASK;
	gpioInit.GPIO_Mode = GPIO_Mode_Out_OD;
	GPIO_Init(GPIOB, &gpioInit);

	// Set the cols as Ext. Interrupt sources.
    GPIO_EXTILineConfig(GPIOB, 8);
    GPIO_EXTILineConfig(GPIOB, 9);
    GPIO_EXTILineConfig(GPIOB, 10);
    GPIO_EXTILineConfig(GPIOB, 11);
    
	// Set the rotary encoder as Ext. Interrupt source.
    GPIO_EXTILineConfig(GPIOB, 15);

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
    nvicInit.NVIC_IRQChannelPreemptionPriority = 0x0F;
    nvicInit.NVIC_IRQChannelSubPriority = 0x0F;
    nvicInit.NVIC_IRQChannelCmd = ENABLE;

	// Enable the NVIC lines
    nvicInit.NVIC_IRQChannel = EXTI9_5_IRQn;
    NVIC_Init(&nvicInit); 
    nvicInit.NVIC_IRQChannel = EXTI15_10_IRQn;
    NVIC_Init(&nvicInit); 

	task_register(TASK_PROCESS_KEYPAD, keypad_process);
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
KEYPAD_KEY keypad_scan_keys(void)
{
	KEYPAD_KEY key = KEY_NONE;
	bool found = false;
	uint8_t row;
	uint16_t cols;
	
	for (row = 0; row < 3; ++row)
	{
		// Walk a '0' down the rows.
		GPIO_SetBits(GPIOB, ROW_MASK);
		GPIO_ResetBits(GPIOB, ROW(row));
		
		// Allow some time for the GPIO to settle.
		DelayUS(500);
		
		// The columns are pulled high externally. 
		// Any '0' seen here is due to a switch connecting to our active '0' on a row.
		cols = GPIO_ReadInputData(GPIOB);
		if ((cols & COL_MASK) != COL_MASK)
		{
			// Only support one key pressed at a time.
			found = true;
			break;
		}
	}

	// Set the rows to all off.
	GPIO_ResetBits(GPIOB, ROW_MASK);
	
	if (found)
	{
		cols = ~cols & COL_MASK;
		
		switch (row)
		{
			case 0:
				if ((cols & COL(0)) != 0)
					key = KEY_CH1_UP;
				else if ((cols & COL(1)) != 0)
					key = KEY_CH1_DN;
				else if ((cols & COL(2)) != 0)
					key = KEY_CH2_UP;
				else if ((cols & COL(3)) != 0)
					key = KEY_CH2_DN;
			break;
			
			case 1:
				if ((cols & COL(0)) != 0)
					key = KEY_CH3_UP;
				else if ((cols & COL(1)) != 0)
					key = KEY_CH3_DN;
				else if ((cols & COL(2)) != 0)
					key = KEY_CH4_UP;
				else if ((cols & COL(3)) != 0)
					key = KEY_CH4_DN;
			break;
			
			case 2:
				if ((cols & COL(1)) != 0)
					key = KEY_SEL;
				else if ((cols & COL(2)) != 0)
					key = KEY_OK;
				else if ((cols & COL(3)) != 0)
					key = KEY_CANCEL;
			break;
			
			default:
			break;
		}
	}
	
	return key;
}

/**
  * @brief  Process keys and drive updates through the system.
  * @note   Called from the scheduler.
  * @param  data: EXTI lines that triggered the update.
  * @retval None
  */
void keypad_process(uint32_t data)
{
	KEYPAD_KEY key = keypad_scan_keys();
	
	// Data is used to send the rotary encoder data.
	if (data != 0)
	{
		switch (data)
		{
			case 1:
				key = KEY_LEFT;
			break;
			case 2:
				key = KEY_RIGHT;
			break;
			default:
			break;
		}
	}
	
	switch (key)
	{
		case KEY_CH1_UP:
			// mixer_adjust_trim(1, 1);
		break;
		case KEY_CH1_DN:
			// mixer_adjust_trim(1, -1);
		break;
		case KEY_CH2_UP:
			// mixer_adjust_trim(2, 1);
		break;
		case KEY_CH2_DN:
			// mixer_adjust_trim(2, -1);
		break;
		case KEY_CH3_UP:
			// mixer_adjust_trim(3, 1);
		break;
		case KEY_CH3_DN:
			// mixer_adjust_trim(3, -1);
		break;
		case KEY_CH4_UP:
			// mixer_adjust_trim(4, 1);
		break;
		case KEY_CH4_DN:
			// mixer_adjust_trim(4, -1);
		break;

		case KEY_OK:
		case KEY_SEL:
		case KEY_CANCEL:
		case KEY_LEFT:
		case KEY_RIGHT:
			// gui_key_input(key);
		break
		
		default: 
		break;
	}
}
