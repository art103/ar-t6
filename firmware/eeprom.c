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
 * This is an IRQ driven EEPROM driver (including I2C).
 *
 * The 8KB is split into 4 blocks of 2KB.
 * The 2KB blocks are cycled between when writing new data.
 * Each block has a checksum. If the checksum fails, the previous block is used.
 *
 */

#include "stm32f10x.h"
#include "myeeprom.h"
#include "tasks.h"

#define I2C_PINS	(1 << 6 | 1 << 7)

#define EEPROM_ADDR 0xA0

typedef struct _block_header
{
	uint32_t	counter; 		// Incremented each time a block is written. Highest n == current block.
	uint32_t	checksum;		// Block checksum (excluding header)
} BLOCK_HDR;

typedef enum _state
{
	STATE_IDLE,
	STATE_START,
	STATE_ADDRESSED1,
	STATE_ADDRESSED2,
	STATE_RESTART,
	STATE_TRANSFERRING,
	STATE_ERROR
} STATE;

static volatile STATE state = STATE_IDLE;
static volatile uint8_t read_write = 1;
static volatile uint16_t addr = 0;

static DMA_InitTypeDef dmaInit;

/**
  * @brief  Initialise the I2C bus and EEPROM.
  * @note
  * @param  None
  * @retval None
  */
void eeprom_init(void)
{
	I2C_InitTypeDef i2cInit;
	GPIO_InitTypeDef gpioInit;
	NVIC_InitTypeDef nvicInit;

	// Enable the I2C block clocks and setup the pins.
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_I2C1, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);

	GPIO_StructInit(&gpioInit);
	gpioInit.GPIO_Speed = GPIO_Speed_2MHz;

	// Configure the Column pins
	gpioInit.GPIO_Pin = I2C_PINS;
	gpioInit.GPIO_Mode = GPIO_Mode_AF_OD;
	GPIO_SetBits(GPIOB, I2C_PINS);
	GPIO_Init(GPIOB, &gpioInit);

	I2C_DeInit(I2C1);

	I2C_StructInit(&i2cInit);
	i2cInit.I2C_ClockSpeed = 200000;
	I2C_Init(I2C1, &i2cInit);
	I2C_Cmd(I2C1, ENABLE);

	I2C_DMACmd(I2C1, DISABLE);

    // Configure the Interrupt to the lowest priority
    nvicInit.NVIC_IRQChannelPreemptionPriority = 0x05;
    nvicInit.NVIC_IRQChannelSubPriority = 0x00;
    nvicInit.NVIC_IRQChannelCmd = ENABLE;
    nvicInit.NVIC_IRQChannel = I2C1_EV_IRQn;
    NVIC_Init(&nvicInit);
    nvicInit.NVIC_IRQChannel = I2C1_ER_IRQn;
    NVIC_Init(&nvicInit);

	// Enable the event interrupt
	I2C_ITConfig(I2C1, I2C_IT_EVT | I2C_IT_ERR | I2C_IT_BUF, ENABLE);

	// Enable the TX and RX DMA channel IRQs
	nvicInit.NVIC_IRQChannel = DMA1_Channel6_IRQn;
    NVIC_Init(&nvicInit);
	nvicInit.NVIC_IRQChannel = DMA1_Channel7_IRQn;
    NVIC_Init(&nvicInit);

    // DMA Configuration
	DMA_DeInit(DMA1_Channel6);	// TX
	DMA_DeInit(DMA1_Channel7);	// RX
	DMA_ITConfig(DMA1_Channel6, DMA_IT_TC, ENABLE);
	DMA_ITConfig(DMA1_Channel7, DMA_IT_TC, ENABLE);

	DMA_StructInit(&dmaInit);
	dmaInit.DMA_PeripheralBaseAddr = (uint32_t)&I2C1->DR;
	dmaInit.DMA_DIR = DMA_DIR_PeripheralSRC;
	dmaInit.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
	dmaInit.DMA_MemoryInc = DMA_MemoryInc_Enable;
	dmaInit.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
	dmaInit.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;
	dmaInit.DMA_Mode = DMA_Mode_Normal;
	dmaInit.DMA_Priority = DMA_Priority_Medium;
	dmaInit.DMA_M2M = DMA_M2M_Disable;
}

/**
  * @brief  Read a block of data from EEPROM.
  * @note
  * @param  offset: EEPROM start byte address
  * @param  length: number of bytes
  * @param  buffer: Destination buffer pointer
  * @retval None
  */
void eeprom_read(uint16_t offset, uint16_t length, uint8_t *buffer)
{
	addr = offset;
	read_write = 1;

	// Configure the DMA controller, but don't enable it yet.
	dmaInit.DMA_MemoryBaseAddr = (uint32_t)buffer;
	dmaInit.DMA_BufferSize = length;

	// Start the I2C transactions.
	I2C_GenerateSTART(I2C1, ENABLE);
}

/**
  * @brief  Write a block of data to EEPROM.
  * @note
  * @param  offset: EEPROM start byte address
  * @param  length: number of bytes
  * @param  buffer: Source buffer pointer
  * @retval None
  */
void eeprom_write(uint16_t offset, uint16_t length, uint8_t *buffer)
{
	addr = offset;
	read_write = 0;

	// Configure the DMA controller, but don't enable it yet.
	dmaInit.DMA_MemoryBaseAddr = (uint32_t)buffer;
	dmaInit.DMA_BufferSize = length;

	// Start the I2C transactions.
	I2C_GenerateSTART(I2C1, ENABLE);
}


/**
  * @brief  I2C Error Handler
  * @note
  * @param  None
  * @retval None
  */
void I2C1_ER_IRQHandler(void)
{
	uint32_t event = I2C_GetLastEvent(I2C1);

	if (event & (I2C_FLAG_AF | I2C_FLAG_TIMEOUT))
	{
		I2C_ClearFlag(I2C1, I2C_FLAG_AF);
		I2C_ClearFlag(I2C1, I2C_FLAG_TIMEOUT);
		I2C_AcknowledgeConfig(I2C1, DISABLE);
		I2C_GenerateSTOP(I2C1, ENABLE);
		state = STATE_ERROR;
	}
}

/**
  * @brief  I2C Event Handler
  * @note	This drives the EEPROM access logic and starts the DMA.
  * @param  None
  * @retval None
  */
void I2C1_EV_IRQHandler(void)
{
	uint32_t event = I2C_GetLastEvent(I2C1);

	switch (state)
	{
		case STATE_IDLE:
			if (event & (I2C_FLAG_MSL | I2C_FLAG_SB))
			{
				state = STATE_START;
				I2C_Send7bitAddress(I2C1, EEPROM_ADDR, I2C_Direction_Transmitter);
			}
		break;

		case STATE_START:
			if (event & I2C_FLAG_ADDR)
			{
				state = STATE_ADDRESSED1;
				I2C_SendData(I2C1, (addr >> 8) & 0xFF);
			}
		break;

		case STATE_ADDRESSED1:
			if (event & (I2C_FLAG_TXE | I2C_FLAG_BTF))
			{
				state = STATE_ADDRESSED2;
				I2C_SendData(I2C1, addr & 0xFF);
			}
		break;

		case STATE_ADDRESSED2:
			if (event & I2C_FLAG_BTF)
			{
				state = STATE_RESTART;
				I2C_GenerateSTART(I2C1, ENABLE);
			}
		break;

		case STATE_RESTART:
			if (event & I2C_FLAG_SB)
			{
				state = STATE_TRANSFERRING;
				if (read_write)
					I2C_Send7bitAddress(I2C1, EEPROM_ADDR, I2C_Direction_Receiver);
				else
					I2C_Send7bitAddress(I2C1, EEPROM_ADDR, I2C_Direction_Transmitter);

				I2C_AcknowledgeConfig(I2C1, ENABLE);
			}
		break;

		case STATE_TRANSFERRING:
		{
			DMA_Channel_TypeDef *channel = (read_write)?DMA1_Channel7:DMA1_Channel6;
			if (event & I2C_FLAG_ADDR)
			{
				// Start receiving / transmitting data into the buffer.
				DMA_Cmd(channel, DISABLE);
				DMA_Init(channel, &dmaInit);
				DMA_Cmd(channel, ENABLE);
			}
			else if (event & (I2C_FLAG_RXNE | I2C_FLAG_BTF))
			{
				if (DMA_GetCurrDataCounter(channel) <= 1)
				{
					I2C_AcknowledgeConfig(I2C1, ENABLE);
					I2C_GenerateSTOP(I2C1, ENABLE);
				}

				// ToDo: Schedule an EEPROM access icon.
			}
		}
		break;

		case STATE_ERROR:
		break;
	}
}

/**
  * @brief  I2C DMA TX Complete Handler
  * @note
  * @param  None
  * @retval None
  */
void DMA1_Channel6_IRQHandler(void)
{

}

/**
  * @brief  I2C DMA RX Complete Handler
  * @note
  * @param  None
  * @retval None
  */
void DMA1_Channel7_IRQHandler(void)
{

}
