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
 * This is an IRQ/DMA driven EEPROM driver (including I2C).
 *
 */

#include "stm32f10x.h"
#include "eeprom.h"

// forwards
void eeprom_wait_complete(void);

#define I2C_PINS	(1 << 6 | 1 << 7)

#define EEPROM_ADDR 0xA0

typedef enum _state {
	STATE_IDLE,
	STATE_START,
	STATE_ADDRESSED1,
	STATE_ADDRESSED2,
	STATE_RESTART,
	STATE_TRANSFER_START,
	STATE_TRANSFERRING,
	STATE_COMPLETING,
	STATE_COMPLETE,
	STATE_ERROR
} STATE;

// locals

static volatile STATE state = STATE_ERROR;
static volatile uint8_t read_write = 1;
static volatile uint16_t addr = 0;
static volatile DMA_InitTypeDef g_dmaInit;


/**
 * @brief  Calculate the data's checksum
 * @note
 * @param  buffer: data to sum
 * @param  length: data length
 * @retval checksum
 */
uint16_t eeprom_calc_chksum(void *buffer, uint16_t length) {
	int i;
	uint8_t *ptr = buffer;
	uint16_t sum = 0;
	for (i = 0; i < length; ++i) {
		sum += *ptr++;
	}

	return sum;
}

/**
 * @brief  Compute simple checksum over eeprom memory
 * @note   performs read of the memory
 * @param  offset: EEPROM start byte address
 * @param  length: number of bytes
 * @retval checksum
 */
uint16_t eeprom_checksum_memory(uint16_t offset, uint16_t length) {
	uint16_t sum = 0;
	char buf[32];
	while (length > 0) {
		int thisStep = sizeof(buf) < length ? sizeof(buf) : length;
		eeprom_read(offset, thisStep, buf);
		offset += thisStep;
		length -= thisStep;
		while (thisStep > 0)
			sum += buf[--thisStep];
	}
	return sum;
}

/**
 * @brief  Returns eeprom state character for display
 * @note
 * @retval ' ' - idle; 'E' - error ; 'B' - busy
 */
char eeprom_state() {
	switch(state){
	case STATE_ERROR:
		return 'E';
	case STATE_COMPLETE:
	case STATE_IDLE:
		return ' ';
	default:
		return 'B';
	}
}

/**
 * @brief  Initialise the I2C bus and EEPROM.
 * @note
 * @param  None
 * @retval None
 */
void eeprom_init(void) {
	I2C_InitTypeDef i2cInit;
	GPIO_InitTypeDef gpioInit;
	NVIC_InitTypeDef nvicInit;
	DMA_InitTypeDef dmaInit;

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

	// Enable the TX and RX DMA channel IRQs
	nvicInit.NVIC_IRQChannel = DMA1_Channel6_IRQn;
	NVIC_Init(&nvicInit);
	nvicInit.NVIC_IRQChannel = DMA1_Channel7_IRQn;
	NVIC_Init(&nvicInit);

	// Enable the event interrupt
	I2C_ITConfig(I2C1, I2C_IT_EVT | I2C_IT_ERR | I2C_IT_BUF, ENABLE);

	// DMA Configuration
	DMA_DeInit(DMA1_Channel6);	// TX
	DMA_DeInit(DMA1_Channel7);	// RX
	DMA_ITConfig(DMA1_Channel6, DMA_IT_TC, ENABLE);
	DMA_ITConfig(DMA1_Channel7, DMA_IT_TC, ENABLE);

	DMA_StructInit(&dmaInit);
	dmaInit.DMA_PeripheralBaseAddr = (uint32_t) &I2C1->DR;
	dmaInit.DMA_DIR = DMA_DIR_PeripheralSRC;
	dmaInit.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
	dmaInit.DMA_MemoryInc = DMA_MemoryInc_Enable;
	dmaInit.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
	dmaInit.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;
	dmaInit.DMA_Mode = DMA_Mode_Normal;
	dmaInit.DMA_Priority = DMA_Priority_Medium;
	dmaInit.DMA_M2M = DMA_M2M_Disable;
	g_dmaInit = dmaInit;

	I2C_DMACmd(I2C1, ENABLE);

	I2C_AcknowledgeConfig(I2C1, ENABLE);

	state = STATE_COMPLETE;
}

/**
 * @brief  Read a block of data from EEPROM.
 * @note
 * @param  offset: EEPROM start byte address
 * @param  length: number of bytes
 * @param  buffer: Destination buffer pointer
 * @retval None
 */
void eeprom_read(uint16_t offset, uint16_t length, void *buffer) {
	//do we care here that prev transaction erred?
	//if (state == STATE_ERROR)
	//return;

	eeprom_wait_complete();

	addr = offset;
	read_write = 1;

	// Configure the DMA controller, but don't enable it yet.
	g_dmaInit.DMA_MemoryBaseAddr = (uint32_t) buffer;
	g_dmaInit.DMA_BufferSize = length;
	g_dmaInit.DMA_DIR = DMA_DIR_PeripheralSRC;

	state = STATE_IDLE;

	// Start the I2C transactions.
	I2C_GenerateSTART(I2C1, ENABLE);

	// wait for the read to complete
	eeprom_wait_complete();
}

/**
 * @brief  Write a block of data to EEPROM.
 * @note
 * @param  offset: EEPROM start byte address
 * @param  length: number of bytes
 * @param  buffer: Source buffer pointer
 * @retval None
 */
void eeprom_write(uint16_t offset, uint16_t length, void *buffer) {
	uint16_t written = 0;

	//do we care here that prev transaction erred?
	//if (state == STATE_ERROR)
	//return;

	// Make sure nothing else is pending
	eeprom_wait_complete();

	// We have to split the transfer into page writes.
	while (written < length) {
		addr = offset + written;
		read_write = 0;

		// Compute this write size but check to see if we need to round up to a page.
		// Check only on first write when written==0
		uint16_t towrite = EEPROM_PAGE_SIZE;
		if (written == 0 && offset % EEPROM_PAGE_SIZE != 0) {
			towrite = offset % EEPROM_PAGE_SIZE;
		} else if (length - written < EEPROM_PAGE_SIZE) {
			towrite = length - written;
		}

		// Start the I2C transactions.
		state = STATE_IDLE;
		// Configure the DMA controller, but don't enable it yet.
		g_dmaInit.DMA_MemoryBaseAddr = (uint32_t) buffer + written;
		g_dmaInit.DMA_BufferSize = towrite;
		g_dmaInit.DMA_DIR = DMA_DIR_PeripheralDST;

		I2C_GenerateSTART(I2C1, ENABLE);

		// The rest happens in IRQ&DMA so just wait for it to COMPLETE or ERR
		eeprom_wait_complete();
		if (state == STATE_ERROR)
			break;

		written += towrite;
	}

	// no need to wait for completion here as it was done in the loop above
}

/**
 * @brief  Wait for any pending actions to complete.
 * @note
 * @param  None
 * @retval None
 */
void eeprom_wait_complete(void) {
	while (state != STATE_COMPLETE && state != STATE_ERROR)
		;
}

/**
 * @brief  I2C Error Handler
 * @note
 * @param  None
 * @retval None
 */
void I2C1_ER_IRQHandler(void) {
	uint32_t event = I2C_GetLastEvent(I2C1);

	if (event & (I2C_FLAG_AF | I2C_FLAG_TIMEOUT)) {
		I2C_ClearFlag(I2C1, I2C_FLAG_AF);
		I2C_ClearFlag(I2C1, I2C_FLAG_TIMEOUT);
		// I2C_AcknowledgeConfig(I2C1, DISABLE);
		I2C_GenerateSTOP(I2C1, ENABLE);

		// timeout on addressing - this is ok as we are waiting for EEPROM to complete
		// hence restart the polling
		if (state == STATE_COMPLETING) {
			// wait for STOP
			while (I2C_GetFlagStatus(I2C1, I2C_FLAG_BUSY))
				;
			I2C_GenerateSTART(I2C1, ENABLE);
		} else // really an error
		{
			state = STATE_ERROR;
		}
	}
}

/**
 * @brief  I2C Event Handler
 * @note	This drives the EEPROM access logic and starts the DMA.
 * @param  None
 * @retval None
 */
void I2C1_EV_IRQHandler(void) {
	uint32_t event = I2C_GetLastEvent(I2C1);
#define ISEV(EV) ((event & EV)==EV)

	switch (state) {
	case STATE_IDLE:
		if (ISEV(I2C_EVENT_MASTER_MODE_SELECT)) {
			state = STATE_START;
			I2C_Send7bitAddress(I2C1, EEPROM_ADDR, I2C_Direction_Transmitter);
		}
		break;

	case STATE_START:
		if (ISEV(I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED)) {
			state = STATE_ADDRESSED1;
			I2C_SendData(I2C1, (addr >> 8) & 0xFF);
		}
		break;

	case STATE_ADDRESSED1:
		if (ISEV(I2C_EVENT_MASTER_BYTE_TRANSMITTED)) {
			state = STATE_ADDRESSED2;
			I2C_SendData(I2C1, addr & 0xFF);
		}
		break;

	case STATE_ADDRESSED2:
		if (ISEV(I2C_EVENT_MASTER_BYTE_TRANSMITTED)) {
			if (read_write) {
				state = STATE_RESTART;
				I2C_GenerateSTART(I2C1, ENABLE);
			} else {
				state = STATE_TRANSFER_START;
			}
		}
		break;

	case STATE_RESTART:
		if (ISEV(I2C_EVENT_MASTER_MODE_SELECT)) {
			state = STATE_TRANSFER_START;
			I2C_Send7bitAddress(I2C1, EEPROM_ADDR, I2C_Direction_Receiver);
		}
		break;

	case STATE_TRANSFER_START: {
		if ((event & I2C_FLAG_ADDR) || // already master, only ADDR now hence this does not work: ISEV(I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED) ||
				ISEV(I2C_EVENT_MASTER_BYTE_TRANSMITTED)) {
			state = STATE_TRANSFERRING;
			DMA_Channel_TypeDef *channel = (read_write) ? DMA1_Channel7 :
			DMA1_Channel6;
			DMA_Cmd(channel, DISABLE);
			DMA_ClearFlag(DMA1_FLAG_TC7);
			DMA_ClearFlag(DMA1_FLAG_TC6);
			DMA_Init(channel, (DMA_InitTypeDef*) &g_dmaInit);
			I2C_DMALastTransferCmd(I2C1, ENABLE);
			DMA_Cmd(channel, ENABLE);
		}
	}
		break;
	case STATE_TRANSFERRING: {
		// transfer finished hence complete write
		if (event & I2C_FLAG_TXE) {
			state = STATE_COMPLETING;
			I2C_GenerateSTOP(I2C1, ENABLE);
			// wait for I2C to STOP (could do with IRQ?)
			while (I2C_GetFlagStatus(I2C1, I2C_FLAG_BUSY))
				;
			I2C_GenerateSTART(I2C1, ENABLE);
		}
	}
		break;

	case STATE_COMPLETING: {
		// addressing the eeprom successfully marks the end of write
		if (ISEV(I2C_EVENT_MASTER_MODE_SELECT)) {
			I2C_Send7bitAddress(I2C1, EEPROM_ADDR, I2C_Direction_Transmitter);
		} else if (ISEV(I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED)) {
			I2C_GenerateSTOP(I2C1, ENABLE);
			state = STATE_COMPLETE;
		} else { // if( ISEV(I2C_EVENT_SLAVE_ACK_FAILURE ) )
			I2C_ClearFlag(I2C1, I2C_FLAG_AF);
			I2C_GenerateSTART(I2C1, ENABLE);
		}

	}
		break;

	case STATE_ERROR:
	case STATE_COMPLETE:
		break;
	}
}

/**
 * @brief  I2C DMA TX Complete Handler
 * @note
 * @param  None
 * @retval None
 */
void DMA1_Channel6_IRQHandler(void) {
	DMA_Cmd(DMA1_Channel6, DISABLE);
	DMA_ClearFlag(DMA1_FLAG_TC6);
	DMA_ClearITPendingBit(DMA_IT_TC);
	// dma finished transferring to the I2C but the I2C did not finished yet
}

/**
 * @brief  I2C DMA RX Complete Handler
 * @note
 * @param  None
 * @retval None
 */
void DMA1_Channel7_IRQHandler(void) {
	DMA_Cmd(DMA1_Channel7, DISABLE);
	DMA_ClearFlag(DMA1_FLAG_TC7);
	DMA_ClearITPendingBit(DMA_IT_TC);
	state = STATE_COMPLETE;
	I2C_GenerateSTOP(I2C1, ENABLE);
}
