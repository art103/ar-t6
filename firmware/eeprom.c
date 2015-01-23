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

// forwards
void eeprom_wait_complete(void);
uint16_t eeprom_calc_chksum(void *buffer, uint16_t length);
void eeprom_process(uint32_t data);
void eeprom_read(uint16_t offset, uint16_t length, void *buffer);
void eeprom_write(uint16_t offset, uint16_t length, void *buffer);

#define EEPROM_PAGE_SIZE 32
#define EEPROM_PAGE_MASK 0xFFE0

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
static volatile uint8_t currModel = 0xFF;

#define PAGE_ALIGN 1

/**
 * @brief  compute given model's address in eeprom
 * @note rounds up to the nearest eeprom page
 * @param  modelNumber
 * @retval eeprom address for model
 */
static uint16_t model_address(uint8_t modelNumber) {
	if( modelNumber > MAX_MODELS - 1 )
		modelNumber = MAX_MODELS - 1;
#if PAGE_ALIGN
	const uint16_t modelAddressBase =
			(sizeof(EEGeneral) + EEPROM_PAGE_SIZE - 1) & EEPROM_PAGE_MASK;
	const uint16_t modelSizePageRoundup =
			(sizeof(ModelData) + EEPROM_PAGE_SIZE - 1) & EEPROM_PAGE_MASK;
	uint16_t modelAddress =
			modelAddressBase + modelNumber * modelSizePageRoundup;
#else
	uint16_t modelAddress =
			sizeof(EEGeneral) + modelNumber * sizeof(ModelData);
#endif
	return modelAddress;
}


/**
 * @brief  Initialize model data in global g_model
 * @note   current model is g_eeGeneral.currModel
 * @retval None
 */
void eeprom_init_current_model() {
	memset((void*)&g_model, 0, sizeof(g_model));
#if FRUGAL
	g_model.name[0] = 'M';
	g_model.name[1] = '0' + (g_eeGeneral.currModel / 10);
	g_model.name[2] = '0' + (g_eeGeneral.currModel % 10);
#else
	memcpy(&g_model.name, "MODEL    ", sizeof(g_model.name));
	g_model.name[MODEL_NAME_LEN-3] = '0' + (g_eeGeneral.currModel / 10);
	g_model.name[MODEL_NAME_LEN-2] = '0' + (g_eeGeneral.currModel % 10);
#endif
	g_model.name[MODEL_NAME_LEN-1] =  0;
	g_model.protocol = PROTO_PPM;
	g_model.extendedLimits = TRUE;
	g_model.ppmFrameLength = 8;
	g_model.ppmDelay = 6;
	g_model.ppmNCH = 8;
	for(int mx=0; mx < 4; mx++)
	{
		MixData* md = &g_model.mixData[mx];
		md->destCh = mx+1;
		md->srcRaw = mx+1;
		md->mltpx = MLTPX_REP;
		md->weight = 100;
	}
	for(int l=0; l < sizeof(g_model.limitData)/sizeof(g_model.limitData[0]); l++)
	{
		LimitData *p = &g_model.limitData[l];
		//p->min = 0;
		p->max = 100;
		//p->offset = 0;
		p->reverse = 0;
	}

	// already initialized to zero
	//g_model.ppmStart = 0;
	//g_model.pulsePol = 0;
}

/**
 * @brief  Read current model into global g_model
 * @note   current model is g_eeGeneral.currModel
 * @retval None
 */
void eeprom_load_current_model() {
	if( g_eeGeneral.currModel >= MAX_MODELS )
		g_eeGeneral.currModel = MAX_MODELS-1;
	// prevent others to use model data as it may be invalid for a moment
	g_modelInvalid = 1;
	eeprom_read( model_address(g_eeGeneral.currModel), sizeof(g_model), (void*)&g_model);
	uint16_t chksum = eeprom_calc_chksum((void*) &g_model, sizeof(g_model) - 2);
	if (chksum != g_model.chkSum) {
		eeprom_init_current_model();
		// set the checksum so the empty model does not get saved
		g_model.chkSum = eeprom_calc_chksum((void*) &g_model, sizeof(g_model) - 2);
		//TODO: give user a warning
	}
	// make sure the string is terminated, by all means!
	g_model.name[sizeof(g_model.name) - 1] = 0;
	currModel = g_eeGeneral.currModel;
	// model is now valid (sane)
	g_modelInvalid = 0;
}

/**
 * @brief  Read given model's name into supplied buffer
 * @param model - model number, 0..MAX_MODELS-1
 * @param buf - buffer to read model into
 * @retval None
 */
void eeprom_read_model_name(char model, char buf[MODEL_NAME_LEN]) {
	 model = model < MAX_MODELS ? model : MAX_MODELS-1;
	 eeprom_read( model_address(model)+offsetof(ModelData, name), MODEL_NAME_LEN, buf );
	 buf[MODEL_NAME_LEN-1]=0;
}

/**
 * @brief  Read current model into global g_model if g_eeGeneral.currModel changed
 * @note   current models is g_eeGeneral.currModel
 * @retval None
 */
void eeprom_load_current_model_if_changed() {
	if( g_eeGeneral.currModel != currModel )
		eeprom_load_current_model();
}

/**
 * @brief  Compute simple checksum over eeprom memory
 * @note   performs read of the memory
 * @param  offset: EEPROM start byte address
 * @param  length: number of bytes
 * @retval checksum
 */
unsigned eeprom_checksum_memory(uint16_t offset, uint16_t length) {
	unsigned sum = 0;
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

	task_register(TASK_PROCESS_EEPROM, eeprom_process);

	// Read the configuration data out of EEPROM.
	eeprom_read(0, sizeof(EEGeneral), (void*)&g_eeGeneral);
	uint16_t chksum = eeprom_calc_chksum((void*)&g_eeGeneral, sizeof(EEGeneral) - 2);
	if (chksum != g_eeGeneral.chkSum)
	{
		gui_popup(GUI_MSG_EEPROM_INVALID, 0);
		g_eeGeneral.ownerName[sizeof(g_eeGeneral.ownerName) - 1] = 0;
		g_eeGeneral.contrast = (LCD_CONTRAST_MIN+LCD_CONTRAST_MAX)/2;
		g_eeGeneral.enablePpmsim = FALSE;
		g_eeGeneral.vBatCalib = 100;
		// memset(&g_eeGeneral, 0, sizeof(EEGeneral));
		// rechecksum - otherwise it will overwrite
		g_eeGeneral.chkSum = eeprom_calc_chksum((void*)&g_eeGeneral, sizeof(EEGeneral) - 2);
		if( g_eeGeneral.currModel < MAX_MODELS )
			g_eeGeneral.currModel = MAX_MODELS -1;
	}
	eeprom_process(0);
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

	lcd_set_cursor(0, 0);
	lcd_write_char(0x05, LCD_OP_SET, FLAGS_NONE);
	lcd_update();

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

	lcd_set_cursor(0, 0);
	lcd_write_char(state == STATE_ERROR ? 'E' : ' ', LCD_OP_SET, FLAGS_NONE);
	lcd_update();
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
 * @brief  Task to perform non time-critical EEPROM work
 * @note
 * @param  data: task specific data
 * @retval None
 */
void eeprom_process(uint32_t data) {
	uint16_t chksum;

	/* do not update eeprom when cal is in progress
	 * it's changing the data on the fly (IRQ) and will cause spurious error messages
	 */
	if(gui_get_layout() != GUI_LAYOUT_STICK_CALIBRATION)
	{
		// see if general settings need to be saved
		chksum = eeprom_calc_chksum((void*)&g_eeGeneral, sizeof(EEGeneral) - 2);
		if (chksum != g_eeGeneral.chkSum) {
			g_eeGeneral.chkSum = chksum;
			eeprom_write(0, sizeof(EEGeneral), (void*)&g_eeGeneral);
			// check after write
			unsigned cs = eeprom_checksum_memory(0, sizeof(EEGeneral) - 2);
			if (chksum != cs)
				gui_popup(GUI_MSG_EEPROM_INVALID, 0);
		}
		// see if current model's settings need to be saved
		chksum = eeprom_calc_chksum((void*)&g_model, sizeof(g_model) - 2);
		if (chksum != g_model.chkSum) {
			uint16_t modelAddress = model_address(g_eeGeneral.currModel);
			g_model.chkSum = chksum;
			eeprom_write( modelAddress, sizeof(g_model), (void*)&g_model);
			// check after write
			unsigned cs = eeprom_checksum_memory(modelAddress, sizeof(g_model) - 2);
			if (chksum != cs)
				gui_popup(GUI_MSG_EEPROM_INVALID, 0);
		}
	}

	eeprom_load_current_model_if_changed();

	task_schedule(TASK_PROCESS_EEPROM, 0, 1000);
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
			DMA_Init(channel, (DMA_InitTypeDef*)&g_dmaInit);
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
		} else {// if( ISEV(I2C_EVENT_SLAVE_ACK_FAILURE ) )
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
