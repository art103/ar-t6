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
#include "eeprom.h"
#include "myeeprom.h"
#include "gui.h"
#include "lcd.h"
#include "tasks.h"

#define I2C_PINS	(1 << 6 | 1 << 7)

#define EEPROM_ADDR 0xA0

typedef enum _state
{
	STATE_IDLE,
	STATE_START,
	STATE_ADDRESSED1,
	STATE_ADDRESSED2,
	STATE_RESTART,
	STATE_TRANSFERRING,
	STATE_COMPLETE,
	STATE_ERROR
} STATE;

static volatile STATE state = STATE_ERROR;
static volatile uint8_t read_write = 1;
static volatile uint16_t addr = 0;
static volatile DMA_InitTypeDef dmaInit;

// Cache of the checksum of the EEPROM contents.
// Used to check periodically to see if we need to save.
static uint16_t g_eeGeneral_chksum = 0;
static uint16_t g_model_chksum = 0;

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
	dmaInit.DMA_PeripheralBaseAddr = (uint32_t)&I2C1->DR;
	dmaInit.DMA_DIR = DMA_DIR_PeripheralSRC;
	dmaInit.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
	dmaInit.DMA_MemoryInc = DMA_MemoryInc_Enable;
	dmaInit.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
	dmaInit.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;
	dmaInit.DMA_Mode = DMA_Mode_Normal;
	dmaInit.DMA_Priority = DMA_Priority_Medium;
	dmaInit.DMA_M2M = DMA_M2M_Disable;

	I2C_DMACmd(I2C1, ENABLE);

	I2C_AcknowledgeConfig(I2C1, ENABLE);

	state = STATE_COMPLETE;

	task_register(TASK_PROCESS_EEPROM, eeprom_process);

	// Read the configuration data out of EEPROM.
	eeprom_read(0, sizeof(EEGeneral), &g_eeGeneral);
	eeprom_wait_complete();
	g_eeGeneral_chksum = eeprom_calc_chksum(&g_eeGeneral, sizeof(EEGeneral) - 2);
	if (g_eeGeneral_chksum != g_eeGeneral.chkSum)
	{
		gui_popup(GUI_MSG_EEPROM_INVALID, 0);
		memset(&g_eeGeneral, 0, sizeof(EEGeneral));
	}
	else
	{
		eeprom_read(sizeof(g_eeGeneral) + g_eeGeneral.currModel * sizeof(g_model), sizeof(g_model), (void*)&g_model);
		eeprom_wait_complete();
		g_model_chksum = eeprom_calc_chksum((void*)&g_model, sizeof(g_model) - 2);
		if (g_model_chksum != g_model.chkSum)
		{
			memset(&g_model, 0, sizeof(g_model));
		}
	}
	task_schedule(TASK_PROCESS_EEPROM, 0, 1000);
}

/**
  * @brief  Read a block of data from EEPROM.
  * @note
  * @param  offset: EEPROM start byte address
  * @param  length: number of bytes
  * @param  buffer: Destination buffer pointer
  * @retval None
  */
void eeprom_read(uint16_t offset, uint16_t length, void *buffer)
{
	if (state == STATE_ERROR)
		return;

	eeprom_wait_complete();

	addr = offset;
	read_write = 1;

	// Configure the DMA controller, but don't enable it yet.
	dmaInit.DMA_MemoryBaseAddr = (uint32_t)buffer;
	dmaInit.DMA_BufferSize = length;
	dmaInit.DMA_DIR = DMA_DIR_PeripheralSRC;

	state = STATE_IDLE;

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
void eeprom_write(uint16_t offset, uint16_t length, void *buffer)
{
	int i;
	uint16_t written = 0;

	if (state == STATE_ERROR)
		return;

	lcd_set_cursor(0, 0);
	lcd_write_char(0x05, LCD_OP_SET, FLAGS_NONE);
	lcd_update();

	// We have to split the transfer into page writes.
	for (i=0; i<(length / EEPROM_PAGE_SIZE) + 1; ++i)
	{
		uint16_t towrite = EEPROM_PAGE_SIZE;

		eeprom_wait_complete();
		delay_us(5500);

		addr = offset + written;
		read_write = 0;

		if (length - written < EEPROM_PAGE_SIZE)
			towrite = length - written;

		if (towrite > 0)
		{
			// Configure the DMA controller, but don't enable it yet.
			dmaInit.DMA_MemoryBaseAddr = (uint32_t)buffer + written;
			dmaInit.DMA_BufferSize = towrite;
			dmaInit.DMA_DIR = DMA_DIR_PeripheralDST;

			state = STATE_IDLE;

			// Start the I2C transactions.
			I2C_GenerateSTART(I2C1, ENABLE);
		}
		written += towrite;
	}

	lcd_set_cursor(0, 0);
	lcd_write_char(' ', LCD_OP_SET, FLAGS_NONE);
	lcd_update();
}

/**
  * @brief  Wait for any pending actions to complete.
  * @note
  * @param  None
  * @retval None
  */
void eeprom_wait_complete(void)
{
	while (state != STATE_COMPLETE);
}

/**
  * @brief  Calculate the data's checksum
  * @note
  * @param  buffer: data to sum
  * @param  length: data length
  * @retval checksum
  */
uint16_t eeprom_calc_chksum(void *buffer, uint16_t length)
{
	int i;
	uint8_t *ptr = buffer;
	uint16_t sum = 0;
	for (i=0; i<length; ++i)
	{
		sum += *ptr++;
	}

	return sum;
}

/**
  * @brief  Task to perform non time-critical EEPROM work
  * @note
  * @param  data: task specific data
  * @retval None
  */
void eeprom_process(uint32_t data)
{
	uint16_t chksum;

	if (gui_get_layout() >= GUI_LAYOUT_MAIN1 && gui_get_layout() <= GUI_LAYOUT_MAIN4)
	{
		chksum = eeprom_calc_chksum(&g_eeGeneral, sizeof(EEGeneral) - 2);
		if (chksum != g_eeGeneral_chksum)
		{
			g_eeGeneral.chkSum = chksum;
			eeprom_write(0, sizeof(EEGeneral), &g_eeGeneral);
			g_eeGeneral_chksum = chksum;
		}

		chksum = eeprom_calc_chksum(&g_model, sizeof(ModelData) - 2);
		if (chksum != g_model_chksum)
		{
			g_model.chkSum = chksum;
			eeprom_write(sizeof(EEGeneral) + g_eeGeneral.currModel * sizeof(ModelData), sizeof(ModelData), &g_model);
			g_model_chksum = chksum;
		}
	}

	task_schedule(TASK_PROCESS_EEPROM, 0, 1000);
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
	DMA_Channel_TypeDef *channel = (read_write)?DMA1_Channel7:DMA1_Channel6;
	static uint8_t dmaRunning = 0;

	switch (state)
	{
		case STATE_IDLE:
			dmaRunning = 0;
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
				if (read_write)
				{
					state = STATE_RESTART;
					I2C_GenerateSTART(I2C1, ENABLE);
				}
				else
				{
					state = STATE_TRANSFERRING;
				}
			}
		break;

		case STATE_RESTART:
			if (event & I2C_FLAG_SB)
			{
				I2C_Send7bitAddress(I2C1, EEPROM_ADDR, I2C_Direction_Receiver);
				state = STATE_TRANSFERRING;
			}
		break;

		case STATE_TRANSFERRING:
		{
			//if (event & I2C_FLAG_ADDR)
			if (dmaRunning == 0)
			{
				dmaRunning = 1;
				//DMA_Cmd(channel, DISABLE);
				DMA_Init(channel, &dmaInit);
				I2C_DMALastTransferCmd(I2C1, ENABLE);
				DMA_Cmd(channel, ENABLE);
			}
			else if (event & I2C_FLAG_TXE)
			{
				if (DMA_GetCurrDataCounter(channel) <= 1)
				{
					state = STATE_COMPLETE;
					I2C_GenerateSTOP(I2C1, ENABLE);
				}
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
void DMA1_Channel6_IRQHandler(void)
{
	DMA_Cmd(DMA1_Channel6, DISABLE);
	DMA_ClearFlag(DMA1_FLAG_TC6);
	DMA_ClearITPendingBit(DMA_IT_TC);
	//state = STATE_COMPLETE;
}

/**
  * @brief  I2C DMA RX Complete Handler
  * @note
  * @param  None
  * @retval None
  */
void DMA1_Channel7_IRQHandler(void)
{
	DMA_Cmd(DMA1_Channel7, DISABLE);
	DMA_ClearFlag(DMA1_FLAG_TC7);
	DMA_ClearITPendingBit(DMA_IT_TC);
	state = STATE_COMPLETE;
	I2C_GenerateSTOP(I2C1, ENABLE);
}
