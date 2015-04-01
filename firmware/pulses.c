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
 * This is a standalone IRQ driven module that will take the values in
 * g_chans[] and send them out of the PPM-OUT pin.
 *
 * g_ppmIns[] receives up to 8 Channels on the PPM-IN pin.
 *
 * ToDo: Implement a second set of 8 PPM outputs on the PPM-IN pin.
 * Currently this will just mirror the PPM-OUT pin when set to output mode.
 */

#include "stm32f10x.h"
#include "tasks.h"

#include "art6.h"
#include "myeeprom.h"
#include "pulses.h"


#define PULSES_WORD_SIZE	72
#define PULSES_BYTE_SIZE	(PULSES_WORD_SIZE * 2)

#define PPM_IN	(1 << 7)
#define PPM_OUT	(1 << 11)

// All timings in us
#define PPM_CENTER 			1500
#define PPM_STOP_LEN		(300 + g_model.ppmDelay * 50)
#define PPM_MIN_FRAME_LEN  	22500
#define PPM_MAX_FRAME_LEN	60000
#define PPM_MIN_GAP_LEN		9000

// Exported globals
volatile struct t_latency g_latency = { 0xFF, 0 } ;
// TODO: what units are g_chans? (a relative full scale +-1024 or in us?)
// for now they are relative and conv to us is in pulses.c
volatile int16_t g_chans[NUM_CHNOUT]; 	// -1024 - 1024

static uint8_t SlaveMode;		// Trainer Slave

// Private globals
static union p1mhz_t
{
    uint16_t pword[PULSES_WORD_SIZE] ;   //72
    uint8_t pbyte[PULSES_BYTE_SIZE] ;   //144
} pulses_1us;

static volatile uint8_t heartbeat;
static volatile uint8_t Current_protocol;

volatile int16_t g_ppmIns[8];
volatile uint8_t ppmInValid;
static volatile uint8_t ppmInState = 0; //0=unsync 1..8= wait for value i-1

static bool trainer_out = FALSE;

static void pulses_setup(void);
static void pulses_setup_ppm(uint8_t proto);
static void pulses_set_trainer_port_ppm(void);
static void pulses_set_trainer_port_capture(void);

/**
  * @brief  Initialise the PPM module
  * @note	Once running, this is an autonomous IRQ driven setup.
  * @param  None.
  * @retval None.
  */
void pulses_init(void)
{
	GPIO_InitTypeDef gpioInit;
	NVIC_InitTypeDef nvicInit;
	TIM_TimeBaseInitTypeDef timInit;
	TIM_OCInitTypeDef timOcInit;
	TIM_ICInitTypeDef timIcInit;

	// Enable the GPIO block clocks and setup the pins.
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2 | RCC_APB1Periph_TIM3, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);

	// Setup the PPM-IN and PPM-OUT pins.
	GPIO_ResetBits(GPIOA, PPM_IN | PPM_OUT);
	gpioInit.GPIO_Speed = GPIO_Speed_50MHz;

	gpioInit.GPIO_Mode = GPIO_Mode_AF_OD;
	gpioInit.GPIO_Pin = PPM_IN;
	GPIO_Init(GPIOA, &gpioInit);

	gpioInit.GPIO_Mode = GPIO_Mode_Out_PP;
	gpioInit.GPIO_Pin = PPM_OUT;
	GPIO_Init(GPIOA, &gpioInit);

	// (re)initialize the timers.
	TIM_DeInit(TIM2);
	TIM_DeInit(TIM3);
	TIM_TimeBaseStructInit(&timInit);
	TIM_OCStructInit(&timOcInit);
	TIM_ICStructInit(&timIcInit);

	// 1MHz time base
	timInit.TIM_Prescaler = SystemCoreClock / 1000000 - 1; // 1us
	TIM_TimeBaseInit(TIM2, &timInit);
	TIM_TimeBaseInit(TIM3, &timInit);

	// TIM2 (PPM Output)
	TIM_OC1Init(TIM2, &timOcInit);

	// TIM3 (PPM Input)
	timIcInit.TIM_Channel = TIM_Channel_2;
	timIcInit.TIM_ICPolarity = TIM_ICPolarity_Falling;
	timIcInit.TIM_ICSelection = TIM_ICSelection_DirectTI;
	timIcInit.TIM_ICPrescaler = TIM_ICPSC_DIV1;
	timIcInit.TIM_ICFilter = 0x0F;

	TIM_ICInit(TIM3, &timIcInit);

	TIM_SelectInputTrigger(TIM3, TIM_TS_TI1F_ED);
	TIM_CCxCmd(TIM3, TIM_Channel_2, TIM_CCx_Enable);

	// Enable Timer interrupts
	TIM_ITConfig(TIM2, TIM_FLAG_CC1, ENABLE);
	TIM_ITConfig(TIM3, TIM_FLAG_CC2, ENABLE);

	// configure to the highest priority 0:0 (above stick's DMA)
    nvicInit.NVIC_IRQChannelPreemptionPriority = 0;
    nvicInit.NVIC_IRQChannelSubPriority = 0;
    nvicInit.NVIC_IRQChannelCmd = ENABLE;
    nvicInit.NVIC_IRQChannel = TIM2_IRQn;
    NVIC_Init(&nvicInit);

    nvicInit.NVIC_IRQChannel = TIM3_IRQn;
    NVIC_Init(&nvicInit);

	//ToDo: Set these properly before calling init.
	Current_protocol = g_model.protocol + 10;		// Not the same!
	SlaveMode = FALSE;

	pulses_setup();
}

/**
  * @brief  Set the protocol and initialize the pulse data.
  * @note	Can be called from ISR or main loop.
  * @param  None.
  * @retval None.
  */
static void pulses_setup(void)
{
	uint8_t required_protocol ;
	required_protocol = g_model.protocol ;
	// Sort required_protocol depending on student mode and PPMSIM allowed

	if ( g_eeGeneral.enablePpmsim )
	{
		if ( SlaveMode )
		{
			required_protocol = PROTO_PPMSIM ;
		}
	}

    if (Current_protocol != required_protocol)
    {
        Current_protocol = required_protocol ;
        // switch mode here

        // Pause the timer and reset the count.
        TIM_Cmd(TIM2, DISABLE);

        switch(required_protocol)
        {
		default:
		case PROTO_PPM:
			// Use PPM-RX as an input
			pulses_set_trainer_port_capture();

			// 8-Ch PPM: Next frame starts in 20 mS
			TIM_SetAutoreload(TIM2, PPM_MAX_FRAME_LEN);
	        TIM_SetCounter(TIM2, 0);
	        TIM_SetCompare1(TIM2, PPM_STOP_LEN);
			TIM_Cmd(TIM2, ENABLE);
            break;

        case PROTO_PPM16 :
		case PROTO_PPMSIM :
			if ( required_protocol == PROTO_PPMSIM )
			{
				pulses_setup_ppm(PROTO_PPMSIM);
		        // Hold PPM output low
				GPIO_ResetBits(GPIOA, PPM_OUT);
			}
			else
			{
				TIM_SetAutoreload(TIM2, PPM_MAX_FRAME_LEN);
		        TIM_SetCounter(TIM2, 0);
		        TIM_SetCompare1(TIM2, PPM_STOP_LEN);
				TIM_Cmd(TIM2, ENABLE);
				pulses_setup_ppm(PROTO_PPM16);
			}

			// Use PPM-RX as an output
			pulses_set_trainer_port_ppm();
			break;
		}
    } // current != required

    switch(required_protocol)
    {
    case PROTO_PPM:
    	pulses_setup_ppm( PROTO_PPM );		// Don't enable interrupts through here
        break;
    case PROTO_PPM16 :
    	pulses_setup_ppm( PROTO_PPM );		// Don't enable interrupts through here
        // PPM16 pulses are set up automatically within the interrupts
        break ;
    }
}

/**
  * @brief  Configure the PPM pulse data from g_chans.
  * @note: TODO: very high potential for data corruption due to pointer arithmetic gone wild!
  * @param  proto: The radio protocol.
  * @retval None.
  */
void pulses_setup_ppm( uint8_t proto )
{
	// bail out when model is in flux (read from eeprom) to avoid miscomputation of chanel#/start and hence pointer gone wild
	if( g_modelInvalid )
		return;

	uint8_t startChan = g_model.ppmStart;
	uint16_t position = 0; // Running total so we can avoid resetting the timer count and avoid jitter.

	// Total frame length = 22500usec
	// each pulse is 0.5..2.5ms long including a 300us stop tail
	uint16_t *ptr = (proto == PROTO_PPM) ? pulses_1us.pword : &pulses_1us.pword[PULSES_WORD_SIZE/2] ;
	uint8_t p = g_model.ppmNCH; // Channels

	p += startChan;

	// add start pulse to sequence
	if (proto != PROTO_PPM)
	{
		position += PPM_STOP_LEN;
		*ptr++ = position;
	}

	int16_t PPM_range = g_model.extendedLimits ? PPM_LIMIT_EXTENDED : PPM_LIMIT_NORMAL;   // range of 0.7 - 2.3ms or 1.0 - 2.0ms
	uint8_t start = (proto == PROTO_PPM16) ? p-8 : startChan;
	// restore sanity when model got corrupted and avoid wild pointer 'ptr'
	if( start >= NUM_CHNOUT ) start = NUM_CHNOUT-1;
	if( p >= NUM_CHNOUT ) p = NUM_CHNOUT-1;
	for (uint8_t i = start; i < p; i++)
	{
		// Get the channel relative value
		int32_t v = g_chans[i];	// -1024 - 1024 (+/RESX)
		// scale to max PPM_range (essentially convert to us)
		v = (v * PPM_range + 1024/2) / 1024 /*RESX*/;
		if (v > PPM_range) v = PPM_range;
		if (v < -PPM_range) v = -PPM_range;
		v += PPM_CENTER;

		// part of width is used for the "STOP" pulse width so subtract it
		v -= PPM_STOP_LEN;
		if( v < PPM_STOP_LEN ) v = PPM_STOP_LEN;

		// Channel
		position += v;
		*ptr++ = position; // DANGER! if channels # are wrong *prt would run wild

		// end-of-channel
		position += PPM_STOP_LEN;
		*ptr++ = position; // DANGER! if channels # are wrong *prt would run wild
	}

	// compute the finnal gap between PPM sequences (frames)
	int32_t frameLength = g_model.ppmFrameLength * 1000; // Minimum Framelen = 22.5 ms
	if( frameLength < PPM_MIN_FRAME_LEN ) frameLength = PPM_MIN_FRAME_LEN;
	int32_t gap = frameLength - position;
	if (gap < PPM_MIN_GAP_LEN) gap = PPM_MIN_GAP_LEN;

	// end-of-frame
	position += gap;
	*ptr++ = position;

	if (proto == PROTO_PPM)
	{
		// Stop
		position += PPM_STOP_LEN;
		*ptr++ = position;
	}
    // mark the end of the sequence
	*ptr = 0;

	// for debugging
	if( ptr >= &pulses_1us.pword[sizeof(pulses_1us.pword)/sizeof(pulses_1us.pword[0])])
		// error, and now it's too late! this here is just so you can put a breakpoint
		return;
}

/**
  * @brief  Configure the trainer port and ISR in PPM output mode.
  * @note
  * @param  None.
  * @retval None.
  */
void pulses_set_trainer_port_ppm()
{
	GPIO_InitTypeDef gpioInit;

	GPIO_ResetBits(GPIOA, PPM_IN);
	gpioInit.GPIO_Mode = GPIO_Mode_Out_PP;
	gpioInit.GPIO_Pin = PPM_IN;
	gpioInit.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &gpioInit);

	TIM_Cmd(TIM2, DISABLE);
}

/**
  * @brief  Configure the trainer port and ISR in PPM input mode.
  * @note
  * @param  None.
  * @retval None.
  */
void pulses_set_trainer_port_capture()
{
	GPIO_InitTypeDef gpioInit;

	GPIO_ResetBits(GPIOA, PPM_IN);
	gpioInit.GPIO_Mode = GPIO_Mode_IPU;
	gpioInit.GPIO_Pin = PPM_IN;
	gpioInit.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &gpioInit);


	TIM_SetAutoreload(TIM3, 30000);
    TIM_SetCounter(TIM3, 0);
	TIM_Cmd(TIM3, ENABLE);
}

/**
  * @brief  Timer 2 Interrupt Handler
  * @note	Used as a time base for pulse generation.
  * 		The overflow count is set by this handler based on the duration of each pulse.
  * 		1us time base
  * @param  None
  * @retval None
  */
void TIM2_IRQHandler(void)
{
    static uint8_t   pulsePol;
    static uint16_t *pulsePtr = pulses_1us.pword;

    // For measuring the latency - difference between current count and desired
    int16_t dt = TIM2->CNT /*TIM_GetCounter(TIM2)*/ - *(pulsePtr-1);

    TIM2->SR = (uint16_t)~TIM_FLAG_CC1; /* TIM_ClearITPendingBit(TIM2, TIM_FLAG_CC1); */

    // Toggle the output bit.
    if(pulsePol)
    {
    	GPIOA->BSRR = PPM_OUT; /*GPIO_SetBits(GPIOA, PPM_OUT);*/
    	if (trainer_out)
    		GPIOA->BSRR = PPM_IN; /* GPIO_SetBits(GPIOA, PPM_IN);*/
        pulsePol = 0;
    }
    else
    {
    	GPIOA->BRR = PPM_OUT; /* GPIO_ResetBits(GPIOA, PPM_OUT); */
    	if (trainer_out)
    		GPIOA->BRR = PPM_IN; /*GPIO_ResetBits(GPIOA, PPM_IN);*/
        pulsePol = 1;
    }

    // Store the min / max latency.
	if ( (uint8_t)dt > g_latency.g_tmr1Latency_max) g_latency.g_tmr1Latency_max = dt ;    // max has leap, therefore vary in length
	if ( (uint8_t)dt < g_latency.g_tmr1Latency_min) g_latency.g_tmr1Latency_min = dt ;    // max has leap, therefore vary in length

	// If we're at the end of the sequence
    if (*pulsePtr == 0)
    {
    	// Go back to the beginning.
        pulsePtr = pulses_1us.pword;
        // Set initial polarity of the output level to the inverse
        pulsePol = !g_model.pulsePol;

        // Pause the timer whilst we set the next sequence.
        TIM2->CR1 &= (uint16_t)(~((uint16_t)TIM_CR1_CEN)); /*TIM_Cmd(TIM2, DISABLE);*/

        pulses_setup();

        if ( (g_model.protocol == PROTO_PPM) || (g_model.protocol == PROTO_PPM16) )
        {
            // Reset and start the timer.
        	TIM2->SR = (uint16_t)~TIM_FLAG_CC1; /*TIM_ClearITPendingBit(TIM2, TIM_FLAG_CC1);*/
        	TIM2->CNT = 0; /*TIM_SetCounter(TIM2, 0);*/
        	TIM2->CCR1 = PPM_STOP_LEN; /*TIM_SetCompare1(TIM2, PPM_STOP_LEN);*/
        	TIM2->CR1 |= TIM_CR1_CEN; /*TIM_Cmd(TIM2, ENABLE);*/
        }
    }
    else
    {
        // Set the compare value for the next pulse.
    	TIM2->CCR1 = *pulsePtr; /*TIM_SetCompare1(TIM2, *pulsePtr);*/
    	pulsePtr++;
    }

    heartbeat |= HEART_TIMER_PULSES;
}

/**
  * @brief  Timer 3 Interrupt Handler
  * @note	Used as a time base for PPM pulse input (trainer port).
  *         Timer3 used for PPM_IN pulse width capture. Counter running at 16MHz / 8 = 2MHz
  *         equating to one count every half millisecond. (2 counts = 1ms). Control channel
  *         count delta values thus can range from about 1600 to 4400 counts (800us to 2200us),
  *         corresponding to a PPM signal in the range 0.8ms to 2.2ms (1.5ms at center).
  *         (The timer is free-running and is thus not reset to zero at each capture interval.)
  * @param  None
  * @retval None
  */
void TIM3_IRQHandler(void)
{
    static uint16_t lastCapt = 0;
    uint16_t capture = TIM_GetCapture1(TIM3);
    int16_t val = (capture - lastCapt) / 2;
	int16_t PPM_range;

    lastCapt = capture;

    // We process g_ppmIns right here to make servo movement as smooth as possible
    // while under trainee control
    if(ppmInState && ppmInState <= 8)
    {
    	PPM_range = g_model.extendedLimits ? PPM_LIMIT_EXTENDED : PPM_LIMIT_NORMAL;   // range of 0.5 - 2.5ms or  0.8 - 2.2ms
    	val -= PPM_CENTER;
        if (val > -PPM_range && val < PPM_range)
        {
        	// -700 - 700 Max
            g_ppmIns[ppmInState++ - 1] = val * (g_eeGeneral.PPM_Multiplier + 10) / 10; // +/- 700 != 512, but close enough.
        }
        else
        {
            ppmInState = 0; // not triggered
        }
    }
    else
    {
        if(val > 4000 && val < 16000)
        {
            ppmInState = 1; // triggered
            TIM_SetCounter(TIM3, 0);
            //last_cap = 0;
        }
    }

    if (TIM_GetITStatus(TIM3, TIM_IT_CC2) != RESET)
    {
        TIM_ClearITPendingBit(TIM3, TIM_FLAG_CC2);
    }
}
