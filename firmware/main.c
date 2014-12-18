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
 * This module contains the entrypoint, setup and main loop.
 *
 */

#include "stm32f10x.h"
#include "tasks.h"
#include "keypad.h"
#include "sticks.h"
#include "lcd.h"
#include "gui.h"
#include "myeeprom.h"
#include "pulses.h"
#include "mixer.h"
#include "sound.h"
#include "eeprom.h"
#include "logo.h"

volatile EEGeneral  g_eeGeneral;
volatile ModelData  g_model;
volatile uint8_t g_modelInvalid = 1;
uint8_t SlaveMode;		// Trainer Slave

/**
  * @brief  This function handles the SysTick.
  * @param  None
  * @retval None
  */
void SysTick_Handler(void)
{
	system_ticks++;
}

/**
  * @brief  Main Loop for non-IRQ based work
  * @note   Deals with init and non time critical work.
  * @param  None
  * @retval None
  */
int main(void)
{
	// PLL and stack setup has aready been done.
	SystemCoreClockUpdate();

	// 1ms System tick
	SysTick_Config(SystemCoreClock / 1000);
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR, ENABLE);

	// Initialize the task loop.
	task_init();

	// Initialize the keypad scanner (with IRQ wakeup).
	keypad_init();

	// Initialize the LCD and display logo.
	lcd_init();
	gui_init();

	// Initialize the EEPROM
	eeprom_init();

	// set contrast but to a reasonable value
	uint16_t contrast = g_eeGeneral.contrast;
	if( contrast < LCD_CONTRAST_MIN ) contrast = LCD_CONTRAST_MIN;
	if( contrast > LCD_CONTRAST_MAX ) contrast = LCD_CONTRAST_MAX;
	lcd_set_contrast(contrast);

	if( !g_eeGeneral.disableSplashScreen )
	{
		// Put the logo into out frame buffer
		memcpy(lcd_buffer, logo, LCD_WIDTH * LCD_HEIGHT / 8);
		lcd_update();
		delay_ms(2000);
	}

	// ToDo: Block here until all switches are set correctly.
	//check_switches();

	// Initialize the buzzer
	sound_init();

	mixer_init();

	// Initialize the ADC / DMA
	sticks_init();

	// Start the radio output.
	pulses_init();

	gui_navigate(GUI_LAYOUT_MAIN1);

	/*
	 * The main loop will sit in low power mode waiting for an interrupt.
	 *
	 * The ADC is running in continuous scanning mode with DMA transfer of the results to memory.
	 * An interrupt will fire when the full conversion scan has completed.
	 * This will schedule the "PROCESS_STICKS" task.
	 * The switches (SWA-SWD) will be polled at this point.
	 *
	 * Keys (trim, buttons and scroll wheel) are interrupt driven. "PROCESS_KEYS" will be scheduled
	 * when any of them are pressed.
	 *
	 * PPM is driven by Timer0 in interrupt mode autonomously from pwm_data.
	 *
	 */

	while (1)
	{
		/*
		static uint8_t tick;
		lcd_set_cursor(0, 48);
		lcd_write_int(tick++, 1, 0);
		lcd_update();
		*/

		// Process any tasks.
		task_process_all();


		// Wait for an interrupt
		//PWR_EnterSTANDBYMode();
	}
}

/**
  * @brief  This function handles NMI exception.
  * @param  None
  * @retval None
  */
void NMI_Handler(void)
{
}

#define ENABLE_HARD_FAILURE_HELPER 0
#if ENABLE_HARD_FAILURE_HELPER

static
void prvGetRegistersFromStack( uint32_t *pulFaultStackAddress )
{
	/* These are volatile to try and prevent the compiler/linker optimising them
	away as the variables never actually get used.  If the debugger won't show the
	values of the variables, make them global my moving their declaration outside
	of this function. */
	volatile uint32_t r0;
	volatile uint32_t r1;
	volatile uint32_t r2;
	volatile uint32_t r3;
	volatile uint32_t r12;
	volatile uint32_t lr; /* Link register. */
	volatile uint32_t pc; /* Program counter. */
	volatile uint32_t psr;/* Program status register. */

    r0 = pulFaultStackAddress[ 0 ];
    r1 = pulFaultStackAddress[ 1 ];
    r2 = pulFaultStackAddress[ 2 ];
    r3 = pulFaultStackAddress[ 3 ];

    r12 = pulFaultStackAddress[ 4 ];
    lr = pulFaultStackAddress[ 5 ];
    pc = pulFaultStackAddress[ 6 ];
    psr = pulFaultStackAddress[ 7 ];

    /* When the following line is hit, the variables contain the register values. */
    for( ;; );
}


/* The prototype shows it is a naked function - in effect this is just an
assembly function. */
void HardFault_Handler( void ) __attribute__( ( naked ) );


/**
  * @brief  This function handles Hard Fault exception.
  * @param  None
  * @retval None
  */
void HardFault_Handler(void)
{
    __asm volatile
    (
        " tst lr, #4                                                \n"
        " ite eq                                                    \n"
        " mrseq r0, msp                                             \n"
        " mrsne r0, psp                                             \n"
        " ldr r1, [r0, #24]                                         \n"
        " ldr r2, handler2_address_const                            \n"
        " bx r2                                                     \n"
        " handler2_address_const: .word prvGetRegistersFromStack    \n"
    );
}

#else

/**
  * @brief  This function handles Hard Fault exception.
  * @param  None
  * @retval None
  */
void HardFault_Handler(void)
{
	while(1) {}
}
#endif
