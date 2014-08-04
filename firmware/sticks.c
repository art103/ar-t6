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

/* Description:
 *
 * This is an IRQ and DMA driven analogue sampler.
 * The DMA completion routine invokes the mixer directly.
 * Scaling of the data for the GUI is done in a main loop
 * task.
 * Calibration is also handled through an API to this module.
 *
 */

#include "stm32f10x.h"
#include "sticks.h"
#include "keypad.h"
#include "tasks.h"
#include "lcd.h"
#include "gui.h"
#include "mixer.h"
#include "art6.h"

volatile uint32_t adc_data[STICK_ADC_CHANNELS];
volatile ADC_CAL cal_data[STICK_ADC_CHANNELS] = {
		{0, 4096, 2048},
		{0, 4096, 2048},
		{0, 4096, 2048},
		{0, 4096, 2048},
		{0, 4096, 2048},
		{0, 4096, 2048},
		{0, 3100, 1550},
};
volatile int16_t stick_data[STICK_ADC_CHANNELS];

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
	adcInit.ADC_ContinuousConvMode = DISABLE;
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
    nvicInit.NVIC_IRQChannelSubPriority = 1;
    nvicInit.NVIC_IRQChannelPreemptionPriority = 1;
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
}

/**
  * @brief  Process the stick data and drive updates through the GUI.
  * @note   Called from the scheduler.
  * @param  data: Not used.
  * @retval None
  */
void sticks_process(uint32_t data)
{
	int i;

	if (cal_state != CAL_OFF)
	{
		if (cal_state == CAL_LIMITS)
		{
			for (i=0; i<STICK_INPUT_CHANNELS; ++i)
			{
				if (adc_data[i] < cal_data[i].min) cal_data[i].min = adc_data[i];
				if (adc_data[i] > cal_data[i].max) cal_data[i].max = adc_data[i];
			}
		}
		else if (cal_state == CAL_CENTER)
		{
			// Set the stick centres.
			for (i=0; i<STICKS_TO_CALIBRATE; ++i)
			{
				cal_data[i].centre = adc_data[i];
			}

			// Set the remaining centres.
			for (i=STICKS_TO_CALIBRATE-1; i<STICK_INPUT_CHANNELS; ++i)
			{
				cal_data[i].centre = cal_data[i].min + ((cal_data[i].max - cal_data[i].min) / 2);
			}
		}
	}

	gui_update(UPDATE_STICKS);

	// Schedule another update.
	ADC_SoftwareStartConvCmd(ADC1, ENABLE);
}

/**
  * @brief  Calibrate the endpoints and centre of the sticks.
  * @note
  * @param  state: Calibration state to enter.
  * @retval None
  */
void sticks_calibrate(CAL_STATE state)
{
	int i;

	cal_state = state;

	if (state == CAL_LIMITS)
	{
		for (i=0; i<STICKS_TO_CALIBRATE; ++i)
		{
			cal_data[i].min = 0xFFFF;
			cal_data[i].max = 0;
			cal_data[i].centre = 2048;
		}
	}
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
	int32_t val;
	val =  100 * (STICK_LIMIT + stick_data[channel]) / (2*STICK_LIMIT);

	if (val > 100)
		val = 100;
	if (val < 0)
		val = 0;

	return val;
}


/**
  * @brief  This function handles the DMA end of transfer.
  * @param  None
  * @retval None
  */
void DMA1_Channel1_IRQHandler(void)
{
	int32_t tmp;
	int i;

	DMA_ClearITPendingBit(DMA_IT_TC);

	// Scale channels to -STICK_LIMIT to +STICK_LIMIT.
	for (i=0; i<STICK_ADC_CHANNELS; ++i)
	{
		int32_t scale = cal_data[i].max - cal_data[i].min;
		tmp = adc_data[i];
		tmp *= 2*STICK_LIMIT;
		tmp /= scale;
		tmp -= 2*STICK_LIMIT * cal_data[i].centre / scale;

		stick_data[i] = tmp;
	}

	// Don't run the mixer if we're calibrating
	if (cal_state == CAL_OFF)
	{
		// Run the mixer.
		mixer_update();
	}

	task_schedule(TASK_PROCESS_STICKS, 0, 20);
}
