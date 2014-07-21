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

#include "stm32f10x.h"
#include "tasks.h"

#include "art6.h"
#include "myeeprom.h"
#include "pulses.h"


#define PULSES_WORD_SIZE	72
#define PULSES_BYTE_SIZE	(PULSES_WORD_SIZE * 2)

#define PPM_IN	(1 << 7)
#define PPM_OUT	(1 << 11)

#define PPM_CENTER 		1500
#define PPM_STOP_LEN	(g_model.ppmDelay * 50 + 300)

// Exported globals
struct t_latency g_latency ;
volatile int16_t g_chans512[NUM_CHNOUT];

// Private globals
static volatile union p2mhz_t
{
    uint16_t pword[PULSES_WORD_SIZE] ;   //72
    uint8_t pbyte[PULSES_BYTE_SIZE] ;   //144
} pulses2MHz ;

static uint8_t heartbeat;
static uint8_t Current_protocol;

static int16_t g_ppmIns[8];
static uint8_t ppmInState = 0; //0=unsync 1..8= wait for value i-1

static bool trainer_out = FALSE;

void setupPulsesPPM(uint8_t proto);
void set_trainer_port_ppm(void);
void set_trainer_port_capture(void);

void startPulses()
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

	gpioInit.GPIO_Mode = GPIO_Mode_IPU;
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
	//TIM_ICStructInit(&timIcInit);

	// 2MHz time base
	timInit.TIM_Prescaler = 11;
	TIM_TimeBaseInit(TIM2, &timInit);
	//TIM_TimeBaseInit(TIM3, &timInit);

	// TIM2 (PPM Output)
	//TIM_OC1Init(TIM2, &timOcInit);

	// TIM3 (PPM Input)
	timIcInit.TIM_Channel = TIM_Channel_2;
	//TIM_ICInit(TIM3, &timIcInit);

	// Enable Timer interrupts
	TIM_ITConfig(TIM2, TIM_FLAG_CC1, ENABLE);
	//TIM_ITConfig(TIM3, TIM_FLAG_CC1, ENABLE);

    // Configure the Interrupt to the highest priority
    nvicInit.NVIC_IRQChannelPreemptionPriority = 1;
    nvicInit.NVIC_IRQChannelSubPriority = 1;
    nvicInit.NVIC_IRQChannelCmd = ENABLE;
    nvicInit.NVIC_IRQChannel = TIM2_IRQn;
    NVIC_Init(&nvicInit);

    nvicInit.NVIC_IRQChannel = TIM3_IRQn;
    NVIC_Init(&nvicInit);

	Current_protocol = g_model.protocol + 10;		// Not the same!
	g_model.protocol = PROTO_PPM;
	g_eeGeneral.enablePpmsim = FALSE;
	SlaveMode = FALSE;
	g_model.ppmStart = 0;
	g_model.extendedLimits = FALSE;
	g_model.ppmFrameLength = 0;
	g_model.ppmDelay = 0;
	g_model.ppmNCH = 0;
	g_model.pulsePol = 0;

	setupPulses();
}

void setupPulses()
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
			set_trainer_port_capture();

			// 8-Ch PPM: Next frame starts in 20 mS
			TIM_SetAutoreload(TIM2, 2*(25000 + g_model.ppmFrameLength * 1000));
	        TIM_SetCounter(TIM2, 0);
	        TIM_SetCompare1(TIM2, PPM_STOP_LEN);
			TIM_Cmd(TIM2, ENABLE);
            break;

        case PROTO_PPM16 :
		case PROTO_PPMSIM :
			if ( required_protocol == PROTO_PPMSIM )
			{
				setupPulsesPPM(PROTO_PPMSIM);
		        // Hold PPM output low
				GPIO_ResetBits(GPIOA, PPM_OUT);
			}
			else
			{
				TIM_SetAutoreload(TIM2, 50000 + g_model.ppmDelay * 50 * 2 + g_model.ppmFrameLength * 1000);
		        TIM_SetCounter(TIM2, 0);
		        TIM_SetCompare1(TIM2, PPM_STOP_LEN);
				TIM_Cmd(TIM2, ENABLE);
				setupPulsesPPM(PROTO_PPM16);
			}

			// Use PPM-RX as an output
			set_trainer_port_ppm();
			break;
		}
    } // current != required

    switch(required_protocol)
    {
    case PROTO_PPM:
        setupPulsesPPM( PROTO_PPM );		// Don't enable interrupts through here
        break;
    case PROTO_PPM16 :
        setupPulsesPPM( PROTO_PPM );		// Don't enable interrupts through here
        // PPM16 pulses are set up automatically within the interrupts
        break ;
    }
}

void setupPulsesPPM( uint8_t proto )
{
	int16_t PPM_range;
	uint8_t startChan = g_model.ppmStart;
	uint8_t i;
	uint16_t total = 0; // Running total so we can avoid resetting the timer count and avoid jitter.
	  
	// Total frame length = 22500usec
	// each pulse is 1.0..2.0ms long including a 300us stop tail
	uint16_t *ptr = (proto == PROTO_PPM) ? pulses2MHz.pword : &pulses2MHz.pword[PULSES_WORD_SIZE/2] ;
	uint8_t p = ( ( proto == PROTO_PPM16) ? 16 : 8 ) + g_model.ppmNCH ; // Channels
	int32_t rest = 22500 + g_model.ppmFrameLength * 1000; // Minimum Framelen = 22.5 ms

	p += startChan;

	if (proto != PROTO_PPM)
	{
		total += PPM_STOP_LEN;
		*ptr++ = total;
	}

	PPM_range = g_model.extendedLimits ? 1000 : 500;   //range of 0.7..1.7msec
	for (i = (proto == PROTO_PPM16) ? p-8 : startChan; i < p; i++)
	{
		// Get the channel and limit the range.
		int16_t v = g_chans512[i] * 3;	// -1024 - 1024
		if (v > PPM_range) v = PPM_range;
		if (v < -PPM_range) v = -PPM_range;
		v += PPM_CENTER;
		
		// Channel
		rest -= v + PPM_STOP_LEN;
		total += v - PPM_STOP_LEN;
		*ptr++ = total;
		// Stop
		total += PPM_STOP_LEN;
		*ptr++ = total;
	}

	// Make sure there is at least 9ms end-of-frame
	if (rest < 9000) rest = 9000;

	// end-of-frame
	total += rest;
	*ptr++ = total;

	if (proto == PROTO_PPM)
	{
		// Stop
		//total += q;
		//*ptr++ = total;
	}

	*ptr = 0;
}

void set_trainer_port_ppm()
{
	GPIO_InitTypeDef gpioInit;

	GPIO_ResetBits(GPIOA, PPM_IN);
	gpioInit.GPIO_Mode = GPIO_Mode_Out_PP;
	gpioInit.GPIO_Pin = PPM_IN;
	gpioInit.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &gpioInit);

	TIM_Cmd(TIM2, DISABLE);
}

void set_trainer_port_capture()
{
	GPIO_InitTypeDef gpioInit;

	GPIO_ResetBits(GPIOA, PPM_IN);
	gpioInit.GPIO_Mode = GPIO_Mode_IPU;
	gpioInit.GPIO_Pin = PPM_IN;
	gpioInit.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &gpioInit);


	TIM_SetAutoreload(TIM3, 50000);
    TIM_SetCounter(TIM3, 0);
	TIM_Cmd(TIM3, ENABLE);
}

/**
  * @brief  Timer 2 Interrupt Handler
  * @note	Used as a 2MHz time base for pulse generation.
  * 		The overflow count is set by this handler based on the duration of each pulse.
  * @param  None
  * @retval None
  */
void TIM2_IRQHandler(void)
{
    static uint8_t   pulsePol;
    static uint16_t *pulsePtr = pulses2MHz.pword;

    // For measuring the latency.
    uint16_t dt = TIM_GetCounter(TIM2);

    // Toggle the output bit.
    if(pulsePol)
    {
    	GPIO_SetBits(GPIOA, PPM_OUT);
    	if (trainer_out)
        	GPIO_SetBits(GPIOA, PPM_IN);
        pulsePol = 0;
    }
    else
    {
    	GPIO_ResetBits(GPIOA, PPM_OUT);
    	if (trainer_out)
        	GPIO_ResetBits(GPIOA, PPM_IN);
        pulsePol = 1;
    }

    // Set the compare value for the next pulse.
    TIM_SetCompare1(TIM2, *pulsePtr * 2);

    // Store the min / max latency.
	if ( (uint8_t)dt > g_latency.g_tmr1Latency_max) g_latency.g_tmr1Latency_max = dt ;    // max has leap, therefore vary in length
	if ( (uint8_t)dt < g_latency.g_tmr1Latency_min) g_latency.g_tmr1Latency_min = dt ;    // max has leap, therefore vary in length

	// If we're at the end of the sequence
    if (*pulsePtr == 0)
    {
    	// Go back to the beginning.
        pulsePtr = pulses2MHz.pword;
        // Toggle the output level
        pulsePol = !g_model.pulsePol;

        // Pause the timer whilst we set the next sequence.
        TIM_Cmd(TIM2, DISABLE);

        setupPulses();

        if ( (g_model.protocol == PROTO_PPM) || (g_model.protocol == PROTO_PPM16) )
        {
            // Reset and start the timer.
	        TIM_SetCounter(TIM2, 0);
	        TIM_SetCompare1(TIM2, 0);
            TIM_Cmd(TIM2, ENABLE);
        }
    }
    else
    {
    	pulsePtr++;
    }

    heartbeat |= HEART_TIMER2Mhz;
    TIM_ClearITPendingBit(TIM2, TIM_FLAG_CC1);
}

// Timer3 used for PPM_IN pulse width capture. Counter running at 16MHz / 8 = 2MHz
// equating to one count every half millisecond. (2 counts = 1ms). Control channel
// count delta values thus can range from about 1600 to 4400 counts (800us to 2200us),
// corresponding to a PPM signal in the range 0.8ms to 2.2ms (1.5ms at center).
// (The timer is free-running and is thus not reset to zero at each capture interval.)
void TIM3_IRQHandler(void)
{
    uint16_t capture = TIM_GetCapture1(TIM3);

    static uint16_t lastCapt = 0;
    uint16_t val = (capture - lastCapt) / 2;
    lastCapt = capture;

    // We prcoess g_ppmInsright here to make servo movement as smooth as possible
    // while under trainee control
    if(ppmInState && ppmInState<=8)
    {
        if(val > 800 && val < 2200)
        {
            g_ppmIns[ppmInState++ - 1] =
                    (int16_t)(val - 1500)*(g_eeGeneral.PPM_Multiplier+10)/10; //+-500 != 512, but close enough.
        }
        else
        {
            ppmInState = 0; // not triggered
        }
    }
    else
    {
        if(val>4000 && val < 16000)
        {
            ppmInState=1; // triggered
        }
    }
    TIM_ClearITPendingBit(TIM3, TIM_FLAG_CC1);
}
