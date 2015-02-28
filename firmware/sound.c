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

/* Description:
 *
 * This is an IRQ driven speaker driver.
 * Very simple sequences of single tone and duration can be sequenced
 * with minimal CPU intervention.
 * It uses PWM and IRQs to drive the speaker and run the sequence.
 *
 */

#include "stm32f10x.h"
#include "tasks.h"
#include "myeeprom.h"
#include "sound.h"

#define BUZZER_PIN	(1 << 8)

static volatile uint32_t stop_time = 0;

static const uint16_t tune_1[] = {
		400, 100,
		500, 100,
		600, 100,
		800, 100,
		0, 0
};

static const uint16_t *tunes[] = {
	tune_1
};

static volatile const uint16_t *tune = 0;


/**
  * @brief  Initialise the sound timer
  * @note	Once running, this is an autonomous IRQ driven setup.
  * @param  None.
  * @retval None.
  */
void sound_init(void)
{
	GPIO_InitTypeDef gpioInit;
	NVIC_InitTypeDef nvicInit;
	TIM_TimeBaseInitTypeDef timInit;
	TIM_OCInitTypeDef timOcInit;

	// Enable the GPIO block clocks and setup the pins.
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_TIM1, ENABLE);

	// Setup the Buzzer pins.
	GPIO_SetBits(GPIOA, BUZZER_PIN);
	gpioInit.GPIO_Speed = GPIO_Speed_50MHz;

	gpioInit.GPIO_Mode = GPIO_Mode_AF_PP;
	gpioInit.GPIO_Pin = BUZZER_PIN;
	GPIO_Init(GPIOA, &gpioInit);

	// (re)initialize the timers.
	TIM_DeInit(TIM1);
	TIM_TimeBaseStructInit(&timInit);
	TIM_OCStructInit(&timOcInit);

	// 1MHz time base
	timInit.TIM_Prescaler =  SystemCoreClock/1000000 - 1;
	TIM_TimeBaseInit(TIM1, &timInit);

	// TIM1 (Tone Output)
	timOcInit.TIM_OCMode = TIM_OCMode_PWM1;
	timOcInit.TIM_OutputState = TIM_OutputState_Enable;
	timOcInit.TIM_Pulse = 10;
	timOcInit.TIM_OCPolarity = TIM_OCPolarity_High;
	timOcInit.TIM_OCIdleState = TIM_OCIdleState_Reset;

	TIM_OC1Init(TIM1, &timOcInit);
	TIM_OC1PreloadConfig(TIM1, TIM_OCPreload_Enable);
	TIM_ARRPreloadConfig(TIM1, ENABLE);
	TIM_CtrlPWMOutputs(TIM1, ENABLE);

	// Enable Timer interrupts
	TIM_ITConfig(TIM1, TIM_FLAG_CC1, ENABLE);

    // Configure the Interrupt priority to low.
    nvicInit.NVIC_IRQChannelPreemptionPriority = 15;
    nvicInit.NVIC_IRQChannelSubPriority = 15;
    nvicInit.NVIC_IRQChannelCmd = ENABLE;
    nvicInit.NVIC_IRQChannel = TIM1_CC_IRQn;
    NVIC_Init(&nvicInit);

    sound_set_volume(g_eeGeneral.volume);
    sound_play_tune(0);
}

void sound_set_volume(uint8_t volume)
{
	TIM_OCInitTypeDef timOcInit;

	TIM_OCStructInit(&timOcInit);

	// TIM1 (Tone Output)
	timOcInit.TIM_OCMode = TIM_OCMode_PWM1;
	timOcInit.TIM_OutputState = TIM_OutputState_Enable;
	timOcInit.TIM_Pulse = 10 * volume;
	timOcInit.TIM_OCPolarity = TIM_OCPolarity_High;
	timOcInit.TIM_OCIdleState = TIM_OCIdleState_Reset;

	TIM_OC1Init(TIM1, &timOcInit);
}

/**
  * @brief  Play a tune at index n.
  * @note
  * @param  tune: Index of the tune.
  * @retval None.
  */
void sound_play_tune(TUNE index)
{
	// ToDo: Add some more tunes :)
	index = 0;

	tune = tunes[index];

    if (tune != 0)
    {
		sound_play_tone(tune[0], tune[1]);
		tune += 2;

		if (*tune == 0)
			tune = 0;
    }
}

/**
  * @brief  Play a tone at freq Hz for duration ms.
  * @note
  * @param  freq: Frequency of the tone in Hz.
  * @param  duration: Duration of the tone in ms.
  * @retval None.
  */
void sound_play_tone(uint16_t freq, uint16_t duration)
{
	stop_time = system_ticks + duration;

	TIM_SetAutoreload(TIM1, 1000000 / freq);
	TIM_Cmd(TIM1, ENABLE);
}

/**
  * @brief  Timer 1 CC Interrupt Handler
  * @note	Used as a tone generator.
  * @param  None
  * @retval None
  */
void TIM1_CC_IRQHandler(void)
{
    TIM_ClearITPendingBit(TIM1, TIM_FLAG_CC1);

    if (system_ticks > stop_time)
    {
        if (tune != 0)
        {
			sound_play_tone(tune[0], tune[1]);
			tune += 2;

			if (*tune == 0)
				tune = 0;
        }
        else
        	TIM_Cmd(TIM1, DISABLE);
    }

}
