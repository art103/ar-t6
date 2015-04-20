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
 * Author: Michal Krombholz (michkrom@github.com)
 *
 */

#ifndef _SETTINGS_H
#define _SETTING_H

#include <stdint.h>

void settings_init();
void settings_preset_all();
void settings_preset_general();
void settings_preset_current_model();
void settings_preset_current_model_mixers();
void settings_preset_current_model_limits();
void settings_read_model_name(char model, char buf[]);

uint16_t model_address(uint8_t modelNumber);
void settings_load_current_model();

#endif // _EEPROM_H
