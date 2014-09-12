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

#ifndef SOUND_H
#define SOUND_H

typedef enum _tune
{
	STARTUP,
	AU_MIX_WARNING_1,
	AU_MIX_WARNING_2,
	AU_MIX_WARNING_3,
	AU_POT_STICK_MIDDLE,
	AU_INACTIVITY,
} TUNE;
void sound_init(void);
void sound_play_tune(TUNE index);
void sound_play_tone(uint16_t freq, uint16_t duration);
void sound_set_volume(uint8_t volume);

#endif // SOUND_H
