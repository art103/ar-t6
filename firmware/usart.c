/*
 * usart.c
 *
 *  Created on: Mar 22, 2015
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
 * routines for sending/receiving through usart1
 * USART1_TX/TIM1_CH2/TIM15_BKIN/PA9
 * USART1_RX/TIM1_CH3/TIM17_BKIN/PA10
 *
 */

#include "stm32f10x.h"
#include "system.h"
#include "usart.h"
#include "stm32f10x_usart.h"

#define USE_QUEUE

///////////////////////////////////////////////////////////////////////////////
// QUEUE - a simple circular buffer
// The _get _put are interrupt-safe as long as only opposites (get vs put)
// are used from task vs interrupt on the same queue.
// This is guaranteed as routines use separate indexes gidx/pidx.
// Note that this implementation wastes one entry in the queue as pixd & gidx
// overlap means empty. One could add a count of #of entries and hence fix it
// but the count itself is yet another byte...so no cookie. Also, with count it
// would be harder to make it IRQ-safe as ++ and -- would be from IRQ and task.

#define QUEUE_SIZE 64

typedef struct _Queue {
	uint8_t pidx;
	uint8_t gidx;
	char buf[QUEUE_SIZE];
} Queue;

/**
 * @brief  Initialize queue structure
 * @param  pCB pointer to QUEUE
 * @retval None
 */
void Queue_init(Queue* pCB) {
	pCB->pidx = 0;
	pCB->gidx = 0;
}

/**
 * @brief  Returns numbers of pending entries in the queue (chars)
 * @param  pCB pointer to QUEUE
 * @retval number of chars in the QUEUE
 */
uint8_t Queue_count(Queue* pCB, char c) {
	int16_t n = (int16_t)pCB->pidx - (int16_t)pCB->gidx;
	if (n < 0)
		n += QUEUE_SIZE;
	return (uint8_t)n;
}

/**
 * @brief  puts a char into a QUEUE
 * @param  pCB pointer to QUEUE
 * @param  c char to put into QUEUE
 * @retval non-zero if added to QUEUE; 0 if there was no space
 */
uint8_t Queue_put(Queue* pCB, char c) {
	uint8_t newIdx = (pCB->pidx + 1) % QUEUE_SIZE;
	if (newIdx != pCB->gidx) {
		pCB->buf[pCB->pidx] = c;
		pCB->pidx = newIdx;
		return 1;
	} else {
		return 0;
	}
}

/**
 * @brief  gets a char from the QUEUE
 * @param  pCB pointer to QUEUE
 * @retval non-zero with 8 lower bits being the char, zero if queue was empty
 */
uint16_t Queue_get(Queue* pCB) {
	uint16_t ret = 0;
	if (pCB->gidx != pCB->pidx) {
		ret = pCB->buf[pCB->gidx];
		ret |= 0x100;
		pCB->gidx = (pCB->gidx + 1) % QUEUE_SIZE;
	}
	return ret;
}

///////////////////////////////////////////////////////////////////////////////

static volatile uint8_t txrunning = 0;
static Queue txbuf;
static Queue rxbuf;
static void (*registered_rx_handler)(uint8_t data) = 0;

///////////////////////////////////////////////////////////////////////////////


/**
 * @brief  Register receive event handler
 * @param  rx_handler function to call when a char is received
 * @note   the "data" char is for information only as it's also added to rx queue
 * @retval None
 */
void usart_register_rx_handler(void (*rx_handler)(uint8_t data)) {
	registered_rx_handler = rx_handler;
}

/**
 * @brief  print char on uart1
 * @param  c char
 * @retval None
 */
void usart_putc(char c) {
#ifdef USE_QUEUE
	while( Queue_put(&txbuf, c) == 0 )
		;
	USART_ITConfig(USART1, USART_IT_TXE, ENABLE);
#else
	USART_SendData(USART1, c);
	while (USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET)
		;
#endif
}

/**
 * @brief  print string on usart1
 * @param  s string
 * @retval None
 */
void usart_puts(char* s) {
	while (*s)
		usart_putc(*s++);
}


/**
 * @brief  Retrieve received char
 * @note   non-blocking call, would return 0 if nothing received
 * @retval non-zero with 8 lower bits being the char, zero if queue was empty
 */
uint16_t usart_getc() {
	return Queue_get(&rxbuf);
}


/**
 * @brief  Initialize the USART1 serial port
 * @param  None
 * @retval None
 */
void usart_init() {
	/* Clock configuration -------------------------------------------------------*/
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1 | RCC_APB2Periph_GPIOA,
			ENABLE);

	/* Configure the GPIO ports( USART1 Transmit and Receive Lines) */
	/* Configure the USART1_Tx as Alternate function Push-Pull */
	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	/* Configure the USART1_Rx as input floating */
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	/* USART1 configuration ------------------------------------------------------*/
	/* USART1 configured as follow:
	 - BaudRate = 115200 baud
	 - Word Length = 8 Bits
	 - One Stop Bit
	 - No parity
	 - Hardware flow control disabled (RTS and CTS signals)
	 - Receive and transmit enabled
	 */
	USART_InitTypeDef USART_InitStructure;
	USART_InitStructure.USART_BaudRate = 115200;
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;
	USART_InitStructure.USART_StopBits = USART_StopBits_1;
	USART_InitStructure.USART_Parity = USART_Parity_No;
	USART_InitStructure.USART_HardwareFlowControl =
			USART_HardwareFlowControl_None;
	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;

	/* Configure the USART1 */
	USART_Init(USART1, &USART_InitStructure);

	/* Enable USART1 interrupt */
	NVIC_InitTypeDef nvic;
	nvic.NVIC_IRQChannel = USART1_IRQn;
	nvic.NVIC_IRQChannelPreemptionPriority = 15;
	nvic.NVIC_IRQChannelSubPriority = 1;
	nvic.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&nvic);

	Queue_init(&txbuf);
	Queue_init(&rxbuf);

	/* Enable the USART1 */
	USART_Cmd(USART1, ENABLE);
	USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);

	// print invitation
	usart_puts("FS-T6\r\n");
}

/**
 * @brief  USART1's interrupt handler
 * @param  None
 * @retval None
 */
void USART1_IRQHandler() {
	if (USART_GetITStatus(USART1, USART_IT_RXNE)) {
		uint8_t data = USART_ReceiveData(USART1);
		USART_ClearITPendingBit(USART1, USART_IT_RXNE); // Clear interrupt flag
		Queue_put(&rxbuf, data);
		if( registered_rx_handler ) registered_rx_handler(data);
	}
	if (USART_GetITStatus(USART1, USART_IT_TXE)) {
		USART_ClearITPendingBit(USART1, USART_IT_TXE); // Clear interrupt flag
		uint16_t data = Queue_get(&txbuf);
		if (data) {
			USART_SendData(USART1, data & 0xFF);
		} else {
			USART_ITConfig(USART1, USART_IT_TXE, DISABLE);
			txrunning = 0;
		}
	}
}
