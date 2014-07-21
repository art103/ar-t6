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

#ifndef PULSES_H
#define PULSES_H

typedef struct t_PXXData {
    uint8_t   header1;
    uint8_t   rxnum;
    uint16_t  flags;
    int16_t   ch1:12;
    int16_t   ch2:12;
    int16_t   ch3:12;
    int16_t   ch4:12;
    int16_t   ch5:12;
    int16_t   ch6:12;
    int16_t   ch7:12;
    int16_t   ch8:12;
    uint8_t   crc;
} __attribute__((packed)) PXXData;

void startPulses(void);
void setupPulses(void);
void setupPulsesPPM(uint8_t proto);

#endif // PULSES_H
