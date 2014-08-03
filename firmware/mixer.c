/*
 * Author: Richard Taylor <richard@artaylor.co.uk>
 *
 * Based on er9x: Erez Raviv <erezraviv@gmail.com>
 * Based on th9x -> http://code.google.com/p/th9x/
 * Original Author not known.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include "stm32f10x.h"
#include "tasks.h"

#include "art6.h"
#include "myeeprom.h"
#include "pulses.h"
#include "sticks.h"
#include "mixer.h"
#include "sound.h"

static int16_t trim_increment;
static int16_t trim_data[STICKS_TO_TRIM];

/**
  * @brief  Initialise the mixer.
  * @note
  * @param  None
  * @retval None
  */
void mixer_init(void)
{
	// Coarse trim
	trim_increment = 10;
}

/**
  * @brief  The main mixer function
  * @note	This is called from the DMA completion routine,
  *         so keep stack usage and duration to a minimum!
  * @param  None.
  * @retval None.
  */
void mixer_update(void)
{
#if 1
	int i;

	// Input data is in stick_data[].
	// Values are +/- STICK_LIMIT


	// =================================
	// Apply Safety Switches.
	// =================================

	// =================================
	// Output Channel Data
	// =================================

	// Scale channels to -1024 to +1024.
	for (i=0; i<STICK_INPUT_CHANNELS; ++i)
	{
		g_chans[i] = PPM_LIMIT_EXTENDED * stick_data[i] / STICK_LIMIT;
	}
#else
	g_chans[0] = 0;
	g_chans[1] = 0;
	g_chans[2] = 0;
	g_chans[3] = 0;
	g_chans[4] = 500;
	g_chans[5] = -500;
	g_chans[6] = 0;
	g_chans[7] = 0;
#endif

}

/**
  * @brief  Receive key presses and update the trim data
  * @note
  * @param  key: Which trim key was pressed.
  * @retval None
  */
void mixer_input_trim(KEYPAD_KEY key)
{
	uint8_t channel = 0;
	int8_t increment = 0;
	uint8_t endstop = 0;

	switch (key)
	{
	case KEY_CH1_UP:
		channel = 0; increment = 1;	break;
	case KEY_CH2_UP:
		channel = 1; increment = 1;	break;
	case KEY_CH3_UP:
		channel = 2; increment = 1;	break;
	case KEY_CH4_UP:
		channel = 3; increment = 1;	break;

	case KEY_CH1_DN:
		channel = 0; increment = -1; break;
	case KEY_CH2_DN:
		channel = 1; increment = -1; break;
	case KEY_CH3_DN:
		channel = 2; increment = -1; break;
	case KEY_CH4_DN:
		channel = 3; increment = -1; break;

	default:
		break;
	}

	if (increment > 0)
	{
		if (trim_data[channel] < MIXER_TRIM_LIMIT)
			trim_data[channel] += trim_increment;
		else
			endstop = 1;
	}
	else
	{
		if (trim_data[channel] > -MIXER_TRIM_LIMIT)
			trim_data[channel] -= trim_increment;
		else
			endstop = 1;
	}

	if (trim_data[channel] == 0)
		endstop = 1;

	sound_play_tone(500 + 250*trim_data[channel]/MIXER_TRIM_LIMIT, (endstop != 0)?200:50 );
}

/**
  * @brief  Return the current value for the specified input.
  * @note
  * @param  stick: The input to return trim for
  * @retval int16_t trim value between +/- MIXER_TRIM_LIMIT.
  */
int16_t mixer_get_trim(STICK stick)
{
	if (stick >= STICK_R_H && stick <= STICK_L_H)
		return trim_data[stick];
	return 0;
}
