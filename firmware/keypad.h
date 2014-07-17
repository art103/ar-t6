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
 
#include <stdint.h>

#define KEYPAD_EXTI_LINES	(EXTI_Line8 | EXTI_Line9 | EXTI_Line10 | EXTI_Line11)
#define ROTARY_EXTI_LINES	(EXTI_Line15)

typedef enum
{
	KEY_NONE = 0,
    KEY_CH1_UP,	// Trim(s)
    KEY_CH1_DN,
    KEY_CH2_UP,
    KEY_CH2_DN,
    KEY_CH3_UP,
    KEY_CH3_DN,
    KEY_CH4_UP,
    KEY_CH4_DN,
    KEY_SEL,	// Rotary encoder click
    KEY_OK,
    KEY_CANCEL,
    KEY_LEFT,	// Rotary encoder
    KEY_RIGHT	// Rotary encoder
} KEYPAD_KEY;

/**
  * @brief  Initialise the keypad scanning pins.
  * @note   Row used as output, Col as input.
  * @param  None
  * @retval None
  */
void keypad_init(void);

/**
  * @brief  Scan the keypad and return the active key.
  * @note   Will only return the first key found if multiple keys pressed.
  * @param  None
  * @retval KEYPAD_KEY
  *   Returns the active key
  *     @arg KEY_xxx: The key that was pressed
  *     @arg KEY_NONE: No key was pressed
  */
KEYPAD_KEY keypad_scan(void);

/**
  * @brief  Process keys and drive updates through the system.
  * @note   Called from the scheduler.
  * @param  data: EXTI lines that triggered the update.
  * @retval None
  */
void keypad_process(uint32_t data);


