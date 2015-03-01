/*
 * Author: Richard Taylor <richard@artaylor.co.uk>
 *
 * Based on er9x: Erez Raviv <erezraviv@gmail.com>
 * Based on th9x -> http://code.google.com/p/th9x/
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

#ifndef _MIXER_H
#define _MIXER_H

#include "sticks.h"
#include "keypad.h"

#define MIXER_TRIM_LIMIT	100

void mixer_init(void);
void mixer_update(void);

void mixer_input_trim(KEYPAD_KEY key);
int16_t mixer_get_trim(STICK stick);

int16_t expo(int16_t x, int16_t k);

#endif // _MIXER_H
