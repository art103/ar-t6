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

#define NUM_ADC_CHANNELS	7
#define NUM_INPUT_CHANNELS	6
#define NUM_TO_CALIBRATE	4

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

extern volatile uint32_t adc_data[NUM_ADC_CHANNELS];
extern ADC_CAL cal_data[NUM_ADC_CHANNELS];
extern float analog[NUM_ADC_CHANNELS];

void sticks_init(void);
void sticks_process(uint32_t data);
void sticks_calibrate(void);
int sticks_get(float **data);

#endif // _STICKS_H
