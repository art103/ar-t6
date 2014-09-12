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

#ifndef _EEPROM_H
#define _EEPROM_H

#include <stdint.h>

#define EEPROM_PAGE_SIZE 32

void eeprom_init(void);
void eeprom_read(uint16_t offset, uint16_t length, void *buffer);
void eeprom_write(uint16_t offset, uint16_t length, void *buffer);
void eeprom_wait_complete(void);
uint16_t eeprom_calc_chksum(void *buffer, uint16_t length);
void eeprom_process(uint32_t data);

#endif // _KEYPAD_H

