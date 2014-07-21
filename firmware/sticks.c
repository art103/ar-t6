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

#include "stm32f10x.h"
#include "sticks.h"
#include "keypad.h"
#include "tasks.h"
#include "lcd.h"
#include "gui.h"
#include "art6.h"

volatile uint32_t adc_data[STICK_ADC_CHANNELS];
ADC_CAL cal_data[STICK_ADC_CHANNELS] = {
		{0, 4096, 2048},
		{0, 4096, 2048},
		{0, 4096, 2048},
		{0, 4096, 2048},
		{0, 4096, 2048},
		{0, 4096, 2048},
		{0, 3100, 1550},
};
int16_t stick_data[STICK_ADC_CHANNELS];

static CAL_STATE cal_state = CAL_OFF;

/**
  * @brief  Initialise the stick scanning.
  * @note   Starts the ADC continuous sampling.
  * @param  None
  * @retval None
  */
void sticks_init(void)
{
	ADC_InitTypeDef adcInit;
	DMA_InitTypeDef dmaInit;
	GPIO_InitTypeDef gpioInit;
	NVIC_InitTypeDef nvicInit;
	int i;

	// Enable the ADC and DMA clocks
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1 | RCC_APB2Periph_GPIOA, ENABLE);

	gpioInit.GPIO_Speed = GPIO_Speed_50MHz;
	gpioInit.GPIO_Pin = 0x7F;
	gpioInit.GPIO_Mode = GPIO_Mode_AIN;
	GPIO_Init(GPIOA, &gpioInit);

	ADC_DeInit(ADC1);

	// Setup the ADC init structure
	ADC_StructInit(&adcInit);
	adcInit.ADC_ContinuousConvMode = ENABLE;
	adcInit.ADC_ScanConvMode = ENABLE;
	adcInit.ADC_ExternalTrigConv = ADC_ExternalTrigConv_None;
	adcInit.ADC_NbrOfChannel = STICK_ADC_CHANNELS;
	ADC_Init(ADC1, &adcInit);

	// Setup the regular channel cycle
	for (i=0; i<STICK_ADC_CHANNELS; ++i)
	{
		ADC_RegularChannelConfig(ADC1, ADC_Channel_0 + i, i + 1, ADC_SampleTime_239Cycles5);
	}

	// Enable ADC1 + DMA
	ADC_Cmd(ADC1, ENABLE);
	ADC_DMACmd(ADC1, ENABLE);

	// Calibrate ADC1
	ADC_ResetCalibration(ADC1);
    while(ADC_GetResetCalibrationStatus(ADC1));
    ADC_StartCalibration(ADC1);
    while(ADC_GetCalibrationStatus(ADC1));

	ADC_SoftwareStartConvCmd(ADC1, ENABLE);

	nvicInit.NVIC_IRQChannel = DMA1_Channel1_IRQn;
    nvicInit.NVIC_IRQChannelSubPriority = 0;
    nvicInit.NVIC_IRQChannelPreemptionPriority = 0;
    nvicInit.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&nvicInit);

    // DMA Configuration
	DMA_DeInit(DMA1_Channel1);
	dmaInit.DMA_PeripheralBaseAddr = (uint32_t)&ADC1->DR;
	dmaInit.DMA_MemoryBaseAddr = (uint32_t)&adc_data[0];
	dmaInit.DMA_DIR = DMA_DIR_PeripheralSRC;
	dmaInit.DMA_BufferSize = STICK_ADC_CHANNELS;
	dmaInit.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
	dmaInit.DMA_MemoryInc = DMA_MemoryInc_Enable;
	dmaInit.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Word;
	dmaInit.DMA_MemoryDataSize = DMA_MemoryDataSize_Word;
	dmaInit.DMA_Mode = DMA_Mode_Circular;
	dmaInit.DMA_Priority = DMA_Priority_VeryHigh;
	dmaInit.DMA_M2M = DMA_M2M_Disable;

	// Configure and enable the DMA
	DMA_Init(DMA1_Channel1, &dmaInit);
	DMA_Cmd(DMA1_Channel1, ENABLE);
	DMA_ITConfig(DMA1_Channel1, DMA_IT_TC, ENABLE);

	task_register(TASK_PROCESS_STICKS, sticks_process);

	// ToDo: Read the calibration data out of EEPROM.
	// if (eeprom_get_data(EEPROM_ADC_CAL, cal_data) != 0)
	//sticks_calibrate();
	gui_navigate(GUI_LAYOUT_MAIN);
}

/**
  * @brief  Process the stick data and drive updates through the system.
  * @note   Called from the scheduler.
  * @param  data: Not used.
  * @retval None
  */
void sticks_process(uint32_t data)
{
	int i;

	if (cal_state != CAL_OFF)
	{
		if (keypad_get_pressed(KEY_OK) || keypad_get_pressed(KEY_SEL))
		{
			if (cal_state == CAL_LIMITS)
			{
				cal_state = CAL_CENTER;
				gui_popup(GUI_MSG_CAL_CENTRE, 0);
			}
			else
			{
				// Set the stick centres.
				for (i=0; i<STICKS_TO_CALIBRATE; ++i)
				{
					cal_data[i].centre = adc_data[i];
				}

				/*
				// Set the POT centres.
				cal_data[4].centre = cal_data[4].min + ((cal_data[4].max - cal_data[4].min) / 2);
				cal_data[5].centre = cal_data[5].min + ((cal_data[5].max - cal_data[5].min) / 2);

				// VBatt has no centre.
				cal_data[6].centre = 0;
				*/

				// ToDo: Write the calibration data into EEPROM.
				// eeprom_set_data(EEPROM_ADC_CAL, cal_data);

				cal_state = CAL_OFF;
				gui_back();
				gui_popup(GUI_MSG_OK, 500);
			}
		}
		else if (keypad_get_pressed(KEY_CANCEL))
		{
			// Abort the calibration and restore data from EEPROM.
			cal_state = CAL_OFF;
			// ToDo: eeprom_get_data(EEPROM_ADC_CAL, cal_data);

			gui_back();
			gui_popup(GUI_MSG_CANCELLED, 500);
		}
		else if (cal_state == CAL_LIMITS)
		{
			for (i=0; i<STICKS_TO_CALIBRATE; ++i)
			{
				if (adc_data[i] < cal_data[i].min) cal_data[i].min = adc_data[i];
				if (adc_data[i] > cal_data[i].max) cal_data[i].max = adc_data[i];
			}
		}
	}

	gui_update_sticks();

	if (cal_state == CAL_OFF)
	{
		// ToDo: mixer_input_sticks(analog);
	}
}

/**
  * @brief  Calibrate the endpoints and centre of the sticks.
  * @note
  * @param  None
  * @retval None
  */
void sticks_calibrate(void)
{
	int i;

	cal_state = CAL_LIMITS;

	for (i=0; i<STICKS_TO_CALIBRATE; ++i)
	{
		cal_data[i].min = -1;
		cal_data[i].max = 0;
	}

	// Clear the key state.
	keypad_get_pressed(KEY_OK);
	keypad_get_pressed(KEY_SEL);
	keypad_get_pressed(KEY_CANCEL);

	gui_navigate(GUI_LAYOUT_STICK_CALIBRATION);
	gui_popup(GUI_MSG_CAL_MOVE_EXTENTS, 0);
}

/**
  * @brief  Get the stick data.
  * @note
  * @param  channel: The stick channel to return
  * @retval int16_t Channel Value
  */
int16_t sticks_get(STICK channel)
{
	return stick_data[channel];
}

/**
  * @brief  Get the stick data scaled to 0-100.
  * @note
  * @param  channel: The stick channel to return
  * @retval Channel Value (%)
  */
int16_t sticks_get_percent(STICK channel)
{
	return 100 * (STICK_LIMIT + stick_data[channel]) / (2*STICK_LIMIT);
}
