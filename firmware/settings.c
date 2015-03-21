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
 * This file deals with setting: g_Model and g_eeGeneral
 *
 *
 */

#include "eeprom.h"
#include "myeeprom.h"
#include "gui.h"
#include "lcd.h"
#include "tasks.h"

static volatile uint8_t currModel = 0xFF;

#define PAGE_ALIGN 1

/**
 * @brief  compute given model's address in eeprom
 * @note rounds up to the nearest eeprom page
 * @param  modelNumber
 * @retval eeprom address for model
 */
static uint16_t model_address(uint8_t modelNumber) {
	if (modelNumber > MAX_MODELS - 1)
		modelNumber = MAX_MODELS - 1;
#if PAGE_ALIGN
	const uint16_t modelAddressBase = (sizeof(EEGeneral) + EEPROM_PAGE_SIZE - 1)
			& EEPROM_PAGE_MASK;
	const uint16_t modelSizePageRoundup = (sizeof(ModelData) + EEPROM_PAGE_SIZE
			- 1) & EEPROM_PAGE_MASK;
	uint16_t modelAddress = modelAddressBase
			+ modelNumber * modelSizePageRoundup;
#else
	uint16_t modelAddress =
	sizeof(EEGeneral) + modelNumber * sizeof(ModelData);
#endif
	return modelAddress;
}


/**
 * @brief  Initialize model's mider data in global g_model
 * @note   current model is g_eeGeneral.currModel
 * @retval None
 */
void settings_init_current_model_mixers() {
	memset((void*) &g_model.mixData, 0, sizeof(g_model.mixData));
	for (int mx = 0; mx < NUM_CHNOUT; mx++) {
		MixData* md = &g_model.mixData[mx];
		md->destCh = mx + 1;
		md->srcRaw = mx + 1;
		md->mltpx = MLTPX_REP;
		md->weight = 100;
		if (mx > 5) {
			md->srcRaw = MIX_MAX;
			md->swtch = SWITCH_SWA + mx - 6;
		}
	}
}

/**
 * @brief  Initialize model data in global g_model
 * @note   current model is g_eeGeneral.currModel
 * @retval None
 */
void settings_init_current_model() {
	memset((void*) &g_model, 0, sizeof(g_model));
#if FRUGAL
	g_model.name[0] = 'M';
	g_model.name[1] = '0' + (g_eeGeneral.currModel / 10);
	g_model.name[2] = '0' + (g_eeGeneral.currModel % 10);
#else
	memcpy(&g_model.name, "MODEL    ", sizeof(g_model.name));
	g_model.name[MODEL_NAME_LEN - 3] = '0' + (g_eeGeneral.currModel / 10);
	g_model.name[MODEL_NAME_LEN - 2] = '0' + (g_eeGeneral.currModel % 10);
#endif
	g_model.name[MODEL_NAME_LEN - 1] = 0;
	g_model.protocol = PROTO_PPM;
	// g_model.extendedLimits = FALSE;
	g_model.ppmNCH = NUM_CHNOUT;
	g_model.ppmFrameLength = 0;
	g_model.ppmDelay = 2;
	g_model.ppmStart = 0;
	settings_init_current_model_mixers();
	for (int l = 0;
			l < sizeof(g_model.limitData) / sizeof(g_model.limitData[0]); l++) {
		LimitData *p = &g_model.limitData[l];
		// in mixer.c there was +/- 100 on limits which I have removed
		// so now the min/max are true values (no offsets)
		p->min = -100;
		p->max = 100;
		if (l == 2 /* THROTTLE */)
			p->offset = -50 * 10;
		//hence model was bzeroed no need for these
		// p->offset = 0;
		//p->reverse = 0;
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
void settings_load_current_model() {
	if (g_eeGeneral.currModel >= MAX_MODELS)
		g_eeGeneral.currModel = MAX_MODELS - 1;
	// prevent others to use model data as it may be invalid for a moment
	g_modelInvalid = 1;
	eeprom_read(model_address(g_eeGeneral.currModel), sizeof(g_model),
			(void*) &g_model);
	uint16_t chksum = eeprom_calc_chksum((void*) &g_model, sizeof(g_model) - 2);
	if (chksum != g_model.chkSum) {
		settings_init_current_model();
		// set the checksum so the empty model does not get saved
		g_model.chkSum = eeprom_calc_chksum((void*) &g_model,
				sizeof(g_model) - 2);
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
void settings_read_model_name(char model, char buf[MODEL_NAME_LEN]) {
	model = model < MAX_MODELS ? model : MAX_MODELS - 1;
	eeprom_read(model_address(model) + offsetof(ModelData, name),
			MODEL_NAME_LEN, buf);
	buf[MODEL_NAME_LEN - 1] = 0;
}

/**
 * @brief  Read current model into global g_model if g_eeGeneral.currModel changed
 * @note   current models is g_eeGeneral.currModel
 * @retval None
 */
void settings_load_current_model_if_changed() {
	if (g_eeGeneral.currModel != currModel)
		settings_load_current_model();
}

void display_busy(uint8_t busy)
{
	lcd_set_cursor(0, 0);
	lcd_write_char(busy ? 0x05 : ' ', LCD_OP_SET, FLAGS_NONE);
	lcd_update();
}

/**
 * @brief  Task to perform non time-critical EEPROM work
 * @note
 * @param  data: task specific data
 * @retval None
 */
void settings_process(uint32_t data) {
	uint16_t chksum;

	char state = eeprom_state();
	display_busy(state=='B');

	/* do not update eeprom when cal is in progress
	 * it's changing the data on the fly (IRQ) and will cause spurious error messages
	 */
	if (gui_get_layout() != GUI_LAYOUT_STICK_CALIBRATION) {
		// see if general settings need to be saved
		chksum = eeprom_calc_chksum((void*) &g_eeGeneral,
				sizeof(EEGeneral) - 2);
		if (chksum != g_eeGeneral.chkSum) {
			g_eeGeneral.chkSum = chksum;
			display_busy(1);
			eeprom_write(0, sizeof(EEGeneral), (void*) &g_eeGeneral);
			// check after write
			unsigned cs = eeprom_checksum_memory(0, sizeof(EEGeneral) - 2);
			if (chksum != cs)
				gui_popup(GUI_MSG_EEPROM_INVALID, 0);
		}
		// see if current model's settings need to be saved
		chksum = eeprom_calc_chksum((void*) &g_model, sizeof(g_model) - 2);
		if (chksum != g_model.chkSum) {
			uint16_t modelAddress = model_address(g_eeGeneral.currModel);
			g_model.chkSum = chksum;
			display_busy(1);
			eeprom_write(modelAddress, sizeof(g_model), (void*) &g_model);
			// check after write
			unsigned cs = eeprom_checksum_memory(modelAddress,
					sizeof(g_model) - 2);
			if (chksum != cs)
				gui_popup(GUI_MSG_EEPROM_INVALID, 0);
		}
	}

	settings_load_current_model_if_changed();

	task_schedule(TASK_PROCESS_EEPROM, 0, 1000);
}


/**
 * @brief  Initialise the I2C bus and EEPROM.
 * @note
 * @param  None
 * @retval None
 */
void settings_init(void) {
	task_register(TASK_PROCESS_EEPROM, settings_process);

	// Read the configuration data out of EEPROM.
	eeprom_read(0, sizeof(EEGeneral), (void*) &g_eeGeneral);
	uint16_t chksum = eeprom_calc_chksum((void*) &g_eeGeneral,
			sizeof(EEGeneral) - 2);
	if (chksum != g_eeGeneral.chkSum) {
		gui_popup(GUI_MSG_EEPROM_INVALID, 0);
		g_eeGeneral.ownerName[sizeof(g_eeGeneral.ownerName) - 1] = 0;
		g_eeGeneral.contrast = (LCD_CONTRAST_MIN + LCD_CONTRAST_MAX) / 2;
		g_eeGeneral.enablePpmsim = 0;
		g_eeGeneral.vBatCalib = 100;
		// memset(&g_eeGeneral, 0, sizeof(EEGeneral));
		// rechecksum - otherwise it will overwrite
		g_eeGeneral.chkSum = eeprom_calc_chksum((void*) &g_eeGeneral,
				sizeof(EEGeneral) - 2);
		if (g_eeGeneral.currModel < MAX_MODELS)
			g_eeGeneral.currModel = MAX_MODELS - 1;
	}
	task_register(TASK_PROCESS_EEPROM, settings_process);
	settings_process(0);
}

