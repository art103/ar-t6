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

#ifndef _STICKS_H
#define _STICKS_H

#define STICK_ADC_CHANNELS		7
#define STICK_INPUT_CHANNELS	6
#define STICKS_TO_CALIBRATE		4
#define STICKS_TO_TRIM			4

#define STICK_LIMIT				1024

typedef enum
{
	STICK_R_H = 0,
	STICK_R_V,
	STICK_L_V,
	STICK_L_H,
	STICK_VRA,
	STICK_VRB,
	STICK_BAT
} STICK;

typedef struct _adc_cal
{
	uint32_t min;
	uint32_t max;
	uint32_t centre;
} ADC_CAL;

typedef enum
{
	CAL_OFF,
	CAL_LIMITS,
	CAL_CENTER
} CAL_STATE;

extern volatile uint32_t adc_data[STICK_ADC_CHANNELS];
extern ADC_CAL cal_data[STICK_ADC_CHANNELS];
extern int16_t stick_data[STICK_ADC_CHANNELS];

void sticks_init(void);
void sticks_process(uint32_t data);
void sticks_calibrate(void);
int16_t sticks_get(STICK chan);
int16_t sticks_get_percent(STICK chan);

#endif // _STICKS_H
