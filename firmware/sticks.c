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
#include "myeeprom.h"
#include "art6.h"

volatile uint16_t adc_data[STICK_ADC_CHANNELS];
volatile int16_t stick_data[STICK_ADC_CHANNELS];

static CAL_STATE cal_state = CAL_OFF;

/**
 * @brief  Initialise the stick scanning.
 * @note   Starts the ADC continuous sampling.
 * @param  None
 * @retval None
 */
void sticks_init(void) {
	ADC_InitTypeDef adcInit;
	DMA_InitTypeDef dmaInit;
	GPIO_InitTypeDef gpioInit;
	NVIC_InitTypeDef nvicInit;
	TIM_TimeBaseInitTypeDef timInit;
	TIM_OCInitTypeDef timOC;

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
	adcInit.ADC_ExternalTrigConv = ADC_ExternalTrigConv_T4_CC4;
	adcInit.ADC_NbrOfChannel = STICK_ADC_CHANNELS;
	ADC_Init(ADC1, &adcInit);

	// Setup the regular channel cycle
	for (i = 0; i < STICK_ADC_CHANNELS; ++i) {
		ADC_RegularChannelConfig(ADC1, ADC_Channel_0 + i, i + 1,
				ADC_SampleTime_239Cycles5);
	}

	// Enable ADC1 + DMA
	ADC_Cmd(ADC1, ENABLE);
	ADC_DMACmd(ADC1, ENABLE);

	// Calibrate ADC1
	ADC_ResetCalibration(ADC1);
	while (ADC_GetResetCalibrationStatus(ADC1))
		;
	ADC_StartCalibration(ADC1);
	while (ADC_GetCalibrationStatus(ADC1))
		;

	nvicInit.NVIC_IRQChannel = DMA1_Channel1_IRQn;
	nvicInit.NVIC_IRQChannelSubPriority = 1;
	nvicInit.NVIC_IRQChannelPreemptionPriority = 1;
	nvicInit.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&nvicInit);

	// DMA Configuration
	DMA_DeInit(DMA1_Channel1);
	DMA_StructInit(&dmaInit);
	dmaInit.DMA_PeripheralBaseAddr = (uint32_t) &ADC1->DR;
	dmaInit.DMA_MemoryBaseAddr = (uint32_t) &adc_data[0];
	dmaInit.DMA_DIR = DMA_DIR_PeripheralSRC;
	dmaInit.DMA_BufferSize = STICK_ADC_CHANNELS;
	dmaInit.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
	dmaInit.DMA_MemoryInc = DMA_MemoryInc_Enable;
	dmaInit.DMA_PeripheralDataSize = DMA_PeripheralDataSize_HalfWord;
	dmaInit.DMA_MemoryDataSize = DMA_MemoryDataSize_HalfWord;
	dmaInit.DMA_Mode = DMA_Mode_Circular;
	dmaInit.DMA_Priority = DMA_Priority_VeryHigh;
	dmaInit.DMA_M2M = DMA_M2M_Disable;

	// Configure and enable the DMA
	DMA_Init(DMA1_Channel1, &dmaInit);
	DMA_Cmd(DMA1_Channel1, ENABLE);
	DMA_ITConfig(DMA1_Channel1, DMA_IT_TC, ENABLE);

	// TIM4 OC4 is ADC conversion trigger
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM4, ENABLE);
	TIM_DeInit(TIM4);

	/* TIM4 init */
	TIM_TimeBaseStructInit(&timInit);
	timInit.TIM_Period = 20-1; /* 20 ms */
	timInit.TIM_Prescaler = 24*1000; /* 24*1000 CLKs in 1 ms @ 24MHz */
	timInit.TIM_ClockDivision = 0x0;
	timInit.TIM_CounterMode = TIM_CounterMode_Up;
	TIM_TimeBaseInit(TIM4, &timInit);

	/* TIM4 OC4 configuration in PWM mode to generate ADC_ExternalTrigConv_T4_CC4 */
	TIM_OCStructInit(&timOC);
	timOC.TIM_OCMode = TIM_OCMode_PWM1;
	timOC.TIM_OutputState = TIM_OutputState_Enable;
	timOC.TIM_Pulse = 1;
	timOC.TIM_OCPolarity = TIM_OCPolarity_Low;
	TIM_OC4Init(TIM4, &timOC);

	/* TIM2 enable counter */
	TIM_Cmd(TIM4, ENABLE);

	/* enable ADC triggering */
	ADC_ExternalTrigConvCmd(ADC1, ENABLE);

	task_register(TASK_PROCESS_STICKS, sticks_process);
	task_schedule(TASK_PROCESS_STICKS, 0, 20);
}

/**
 * @brief  Update sticks' GUI. All processing in IRQ.
 * @note   Called from the scheduler.
 * @param  data: Not used.
 * @retval None
 */
void sticks_process(uint32_t data) {
	int i;
	// Scale channels to -RESX to +RESX
	// For GUI purpose only.
	for (i = 0; i < STICK_ADC_CHANNELS; ++i) {
		int tmp;
		tmp = adc_data[i];
		if (adc_data[i] >= g_eeGeneral.calData[i].centre) {
			tmp -= g_eeGeneral.calData[i].centre;
			tmp *= RESX;
			stick_data[i] = tmp
					/ (g_eeGeneral.calData[i].max
							- g_eeGeneral.calData[i].centre);
		} else {
			tmp = g_eeGeneral.calData[i].centre - tmp;
			tmp *= -RESX;
			stick_data[i] = tmp
					/ (g_eeGeneral.calData[i].centre
							- g_eeGeneral.calData[i].min);
		}
	}

	gui_update(UPDATE_STICKS);
	task_schedule(TASK_PROCESS_STICKS, 0, 20);
}

/**
 * @brief  Calibrate the endpoints and centre of the sticks.
 * @note
 * @param  state: Calibration state to enter.
 * @retval None
 */
void sticks_calibrate(CAL_STATE state) {
	int i;

	cal_state = state;

	if (state == CAL_LIMITS) {
		for (i = 0; i < STICKS_TO_CALIBRATE; ++i) {
			g_eeGeneral.calData[i].min = 0x7FFF;
			g_eeGeneral.calData[i].max = 0;
			g_eeGeneral.calData[i].centre = 2048;
		}

		// Battery
		g_eeGeneral.calData[i].min = 0;
		g_eeGeneral.calData[i].max = 3100;
		g_eeGeneral.calData[i].centre = 1550;
	}
}

/**
 * @brief  Get the stick data.
 * @note
 * @param  channel: The stick channel to return
 * @retval int16_t Channel Value
 */
int16_t sticks_get(STICK channel) {
	return stick_data[channel];
}

/**
 * @brief  Get the stick data scaled to 0-100.
 * @note
 * @param  channel: The stick channel to return
 * @retval Channel Value (%)
 */
int16_t sticks_get_percent(STICK channel) {
	int32_t val;
	val = 100 * (RESX + stick_data[channel]) / (2 * RESX);

	if (val > 100)
		val = 100;
	if (val < 0)
		val = 0;

	return val;
}

/**
 * @brief  Get the battery Voltage
 * @note
 * @param
 * @retval battery voltage in 10mV
 */
uint16_t sticks_get_battery(void) {
	uint16_t val = sticks_get_percent(STICK_BAT) * 129 / 100;
	val = val * g_eeGeneral.vBatCalib / 100;

	return val;
}

/**
 * @brief  This function handles the DMA end of transfer
 *          and processing of ADC stick data then calls the mixer.
 * @param  None
 * @retval None
 */
void DMA1_Channel1_IRQHandler(void) {

	DMA_ClearFlag(DMA1_FLAG_TC1);
	DMA_ClearITPendingBit(DMA_IT_TC);

	// Don't run the mixer if we're calibrating
	if (cal_state == CAL_OFF) {
		if (!g_modelInvalid) {
			// Run the mixer.
			mixer_update();
		}
	} else // if (cal_state != CAL_OFF)
	{
		int i;
		if (cal_state == CAL_LIMITS) {
			for (i = 0; i < STICK_INPUT_CHANNELS; ++i) {
				if (adc_data[i] < g_eeGeneral.calData[i].min)
					g_eeGeneral.calData[i].min = adc_data[i];
				if (adc_data[i] > g_eeGeneral.calData[i].max)
					g_eeGeneral.calData[i].max = adc_data[i];
			}
		} else if (cal_state == CAL_CENTER) {
			// Set the stick centres.
			for (i = 0; i < STICKS_TO_CALIBRATE; ++i) {
				g_eeGeneral.calData[i].centre = adc_data[i];
			}

			// Set the remaining centres.
			for (i = STICKS_TO_CALIBRATE; i < STICK_INPUT_CHANNELS; ++i) {
				g_eeGeneral.calData[i].centre = g_eeGeneral.calData[i].min
						+ ((g_eeGeneral.calData[i].max
								- g_eeGeneral.calData[i].min) / 2);
			}
		}
	}
}
