/*
 * system.c
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
 * Author: Michal Krombholz michkrom@github.com
 */

/* Description:
 *
 * stm32 sys tick interrupt and counter
 *
 */

#include "stm32f10x.h"
#include "stm32f10x_usart.h"


volatile uint32_t system_ticks = 0;


/**
  * @brief  calls Stm32F1xx on-chip bootloader for flashing
  * @param  None
  * @retval None - shall not ever return
  */
void enter_bootloader(void) {
	USART_DeInit(USART1);
	RCC_DeInit();
	SysTick->CTRL = 0;
	SysTick->LOAD = 0;
	SysTick->VAL = 0;

	__ASM volatile (
			"MOVS r4,#1\n"
			"MSR primask, r4\n"
			"LDR r4,=#0x1FFFF000\n"
			"LDR r3,[r4]\n"
			"MSR msp,r3\n"
			"LDR r3,[r4, #4]\n"
			"blx r3\n"
	);
	 // the above ASM does the same but is much shorter
	// and does not need core_cm3.c (for the __set_XXXX())
	#if 0
	__set_PRIMASK(1);
	// changing stack point would invalidate all local var on stack!
	#define SYSMEM ((uint32_t*)0x1FFFF000) /*specific to STM32F1xx*/
	register const uint32_t SP = SYSMEM[0];
	__set_MSP(SYSMEM[0]/*sp addr*/);
	register const uint32_t BL = SYSMEM[1];
	typedef void (*BLFUNC)();
	((BLFUNC)SYSMEM[1]/*bootloader addr*/)();
	#endif
}


/**
  * @brief  Delay timer using the system tick timer
  * @note   for delay <= 2ms may give bigger error
  * @param  delay: delay in ms
  * @retval None
  */
void delay_ms(uint32_t delay)
{
	uint32_t start = system_ticks+1;
	while (system_ticks <  start + delay);
}

/**
  * @brief  Spin loop us delay routine
  * @param  delay: delay in us.
  * @retval None
  */
static uint8_t delay_scale = 1;
void delay_us(uint32_t delay)
{
	volatile uint32_t i;
	for (i = delay * delay_scale; i > 0; --i);
}


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
  * @brief  Initialize all things system/board realted
  * @param  None
  * @retval None
  */
void system_init()
{
	// PLL and stack setup has aready been done.
	SystemCoreClockUpdate();

	 // 4 bits for preemption priority; 0 bit for sub priority (hence ignored)
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_4);

	// 1ms System tick
	SysTick_Config(SystemCoreClock / 1000);
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR, ENABLE);

	// adjust delay_scale
	while(1)
	{
		// get current ticks+1
		uint32_t ts = system_ticks+1;
		// wait for ticks to change so we start close to the tick start
		while(system_ticks-ts==0);
		// delay 1ms with current delay_scale (starting in 1)
		delay_us(1000);
		// if we have a delta of 1 tick then we're done
		if(system_ticks-ts==1) break;
		// otherwise increase scale and start over
		delay_scale++;
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

#define ENABLE_HARD_FAILURE_HELPER 1
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
