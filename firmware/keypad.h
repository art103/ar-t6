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

#ifndef _KEYPAD_H
#define _KEYPAD_H

#include <stdint.h>

#define KEYPAD_EXTI_LINES	(EXTI_Line12 | EXTI_Line13 | EXTI_Line14)
#define ROTARY_EXTI_LINES	(EXTI_Line15)

#define TRIM_KEYS	(KEY_CH1_UP | KEY_CH1_DN | KEY_CH2_UP | KEY_CH2_DN | KEY_CH3_UP | KEY_CH3_DN | KEY_CH4_UP | KEY_CH4_DN)
typedef enum
{
	KEY_NONE = 0,
    KEY_CH1_UP = 0x0001,	// Trim(s)
    KEY_CH1_DN = 0x0002,
    KEY_CH2_UP = 0x0004,
    KEY_CH2_DN = 0x0008,
    KEY_CH3_UP = 0x0010,
    KEY_CH3_DN = 0x0020,
    KEY_CH4_UP = 0x0040,
    KEY_CH4_DN = 0x0080,
    KEY_SEL = 0x0100,	// Rotary encoder click
    KEY_OK = 0x0200,
    KEY_CANCEL = 0x0400,
    KEY_LEFT = 0x0800,	// Rotary encoder
    KEY_RIGHT = 0x1000,	// Rotary encoder

    /* Long press Keys */
    KEY_MENU = 0x2000,
} KEYPAD_KEY;

typedef enum
{
	SWITCH_SWA = 0x01,
	SWITCH_SWB = 0x02,
	SWITCH_SWC = 0x04,
	SWITCH_SWD = 0x08,
} KEYPAD_SWITCH;

void keypad_init(void);
KEYPAD_KEY keypad_scan_keys(void);
bool keypad_get_pressed(KEYPAD_KEY key);
uint8_t keypad_get_switches(void);
bool keypad_get_switch(KEYPAD_SWITCH sw);
void keypad_cancel_repeat(void);

#endif // _KEYPAD_H

