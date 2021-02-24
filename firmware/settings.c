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
 * Author: Michal Krombholz (michkrom@github.com)
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
#include "settings.h"

#define DPUTS
#include "debug.h"

volatile EEGeneral g_eeGeneral;
volatile ModelData g_model;
volatile uint8_t g_modelInvalid = 1;
static volatile uint8_t currModel = 0xFF;

#define PAGE_ALIGN 1

/**
 * @brief  compute given model's address in eeprom
 * @note rounds up to the nearest eeprom page
 * @param  modelNumber
 * @retval eeprom address for model
 */
uint16_t settings_model_address(uint8_t modelNumber) {
	if (modelNumber > MAX_MODELS - 1)
		modelNumber = MAX_MODELS - 1;
#if PAGE_ALIGN
	const uint16_t modelAddressBase =
			(sizeof(EEGeneral) + EEPROM_PAGE_SIZE - 1)
			& EEPROM_PAGE_MASK;
	const uint16_t modelSizePageRoundup =
			(sizeof(ModelData) + EEPROM_PAGE_SIZE
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
 * @brief  display EEPROM busy icon
 * @retval None
 */
static void display_busy(uint8_t busy) {
	static uint8_t prev = ' ';
	if (prev != busy) {
		lcd_set_cursor(0, 0);
		lcd_write_char(busy ? 0x05 : ' ', LCD_OP_SET, FLAGS_NONE);
		lcd_update();
		prev = busy;
	}
}

/**
 * @brief  return cheksum for current model
 * @retval checksum
 */
static uint16_t checksum_current_model()
{
	return eeprom_calc_chksum((void*)&g_model,
				sizeof(g_model) - sizeof(g_model.chkSum));
}

/**
 * @brief  saves current model if it's checksum does not match
 * @note
 * @retval None
 */
static void save_current_model_if_modified() {
	// see if current model's settings need to be saved
	uint16_t chksum = checksum_current_model();
	if (chksum != g_model.chkSum) {
		uint16_t modelAddress = settings_model_address(g_eeGeneral.currModel);
		g_model.chkSum = chksum;
		display_busy(1);
		eeprom_write(modelAddress, sizeof(g_model), (void*) &g_model);
		// check after write
		unsigned cs = eeprom_checksum_memory(modelAddress,
				sizeof(g_model) - sizeof(g_model.chkSum));
		if (chksum != cs) {
			unsigned cs2 = eeprom_checksum_memory(modelAddress,
					sizeof(g_model) - sizeof(g_model.chkSum));
			dputs("model save verify failed");
			dputs_hex4(chksum);
			dputs_hex4(cs);
			dputs_hex4(cs2);
			if(chksum != cs2)
				gui_popup(GUI_MSG_EEPROM_INVALID, 0);
		}
	}
}

/**
 * @brief  Initialize global settings
 * @note
 * @retval None
 */
void settings_preset_all() {
	bzero((void*)&g_eeGeneral, sizeof(g_eeGeneral));
	settings_preset_general();
	for(int m = 0; m < MAX_MODELS; m++) {
		g_eeGeneral.currModel = m;
		settings_preset_current_model();
		save_current_model_if_modified();
	}
}

/**
 * @brief  Initialize global settings
 * @note
 * @retval None
 */
void settings_preset_general() {
	bzero((void*)&g_eeGeneral, sizeof(EEGeneral));
	g_eeGeneral.ownerName[sizeof(g_eeGeneral.ownerName) - 1] = 0;
	g_eeGeneral.contrast = (LCD_CONTRAST_MIN + LCD_CONTRAST_MAX) / 2;
	g_eeGeneral.enablePpmsim = 0;
	g_eeGeneral.vBatCalib = 100;
	g_eeGeneral.stickMode = 2;
	g_eeGeneral.volume = 1;
	g_eeGeneral.vBatWarn = 100;
	g_eeGeneral.inactivityTimer = 10;
	if (g_eeGeneral.currModel < MAX_MODELS)
		g_eeGeneral.currModel = MAX_MODELS - 1;
}

/**
 * @brief  Initialize model's mixer data in global g_model
 * @note   current model is g_eeGeneral.currModel
 * @retval None
 */
void settings_preset_current_model_mixers() {
	bzero((void*) &g_model.mixData, sizeof(g_model.mixData));
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
 * @brief  Initialize model's limit data in global g_model
 * @note   current model is g_eeGeneral.currModel
 * @retval None
 */
void settings_preset_current_model_limits() {
	bzero((void*)&g_model.limitData, sizeof(g_model.limitData));
	for (int l = 0;
			l < sizeof(g_model.limitData) / sizeof(g_model.limitData[0]);
			l++) {
		LimitData *p = &g_model.limitData[l];
		// in mixer.c there was +/- 100 on limits which I have removed
		// so now the min/max are true values (no offsets)
		p->min = -100;
		p->max = 100;
	}
}

/**
 * @brief  Initialize model data in global g_model
 * @note   current model is g_eeGeneral.currModel
 * @retval None
 */
void settings_preset_current_model() {
	bzero(&g_model, sizeof(g_model));
	memcpy(&g_model.name, "MODEL    ", 10);
	g_model.name[MODEL_NAME_LEN - 3] = '0' + (g_eeGeneral.currModel / 10);
	g_model.name[MODEL_NAME_LEN - 2] = '0' + (g_eeGeneral.currModel % 10);
	g_model.name[MODEL_NAME_LEN - 1] = 0;
	g_model.protocol = PROTO_PPM;
	g_model.ppmNCH = NUM_CHNOUT;
	g_model.ppmFrameLength = 0;
	g_model.ppmDelay = 2;
	g_model.ppmStart = 0;
	g_model.extendedLimits = FALSE;
	g_model.pulsePol = 0;
	settings_preset_current_model_mixers();
	settings_preset_current_model_limits();
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
	int modelAddr = settings_model_address(g_eeGeneral.currModel);
	eeprom_read(modelAddr, sizeof(g_model),
			(void*) &g_model);
	uint16_t chksum = checksum_current_model();
	if (chksum != g_model.chkSum) {
		dputs("model CS bad on load ");
		dputs_hex4(chksum);
		dputs(" ");
		dputs_hex4(g_model.chkSum);
		dputs(" ");
		uint16_t chksum2 = eeprom_checksum_memory(modelAddr,
				sizeof(g_model) - sizeof(g_model.chkSum));
		dputs_hex4( chksum2 );
		dputs("\r\n");

		settings_preset_current_model();
		// set the checksum so the empty model does not get saved
		g_model.chkSum = eeprom_calc_chksum((void*) &g_model,
				sizeof(g_model) - sizeof(g_model.chkSum));

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
	eeprom_read(settings_model_address(model) + offsetof(ModelData, name),
	MODEL_NAME_LEN, buf);
	buf[MODEL_NAME_LEN - 1] = 0;
}

/**
 * @brief  Read current model into global g_model if g_eeGeneral.currModel changed
 * @note   current models is g_eeGeneral.currModel
 * @retval None
 */
void load_current_model_if_changed() {
	if (g_eeGeneral.currModel != currModel)
		settings_load_current_model();
}


/**
 * @brief  Task to perform non time-critical EEPROM work
 * @note
 * @param  data: task specific data
 * @retval None
 */
static void settings_process(uint32_t data) {
	uint16_t chksum;

	char state = eeprom_state();
	display_busy(state == 'B');

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
			if (chksum != cs) {
				uint16_t chksum2 = eeprom_calc_chksum((void*) &g_eeGeneral,
						sizeof(EEGeneral) - 2);
				dputs("eeprom general CS mismatch after write ");
				dputs_hex4(chksum);
				dputs_hex4(cs);
				dputs_hex4(chksum2);
				dputs("\r\n");
				// err message only when struct was not modified in between
				if( chksum ==chksum2 )
					gui_popup(GUI_MSG_EEPROM_INVALID, 0);
			}
		}
	}

	save_current_model_if_modified();
	load_current_model_if_changed();

#if 0
	dputs_hex4(eeprom_calc_chksum((void*) &g_model,
			   sizeof(g_model) - sizeof(g_model.chkSum)));
#endif

	task_schedule(TASK_PROCESS_EEPROM, 0, 2000);         // sky59   bolo 1000
}

/**
 * @brief  Initialize the settings
 * @note
 * @param  None
 * @retval None
 */
void settings_init(void) {

	// Read the configuration data out of EEPROM. Perform few attempts as it fails occasionally
	int cnt = 3;              // sky59   bolo 2krat
	while( !eeprom_read(0, sizeof(EEGeneral), (void*) &g_eeGeneral) && --cnt ) ;
	uint16_t chksum =
			eeprom_calc_chksum((void*) &g_eeGeneral, sizeof(EEGeneral) - 2);
	if (chksum != g_eeGeneral.chkSum) {
		dputs("eeprom general CS bad ");
		dputs_dec(2);
		dputs_hex4(chksum);
		dputs_hex4(g_eeGeneral.chkSum);
		dputs("\r\n");
		puts_mem(&g_eeGeneral, sizeof(g_eeGeneral));
		gui_popup(GUI_MSG_EEPROM_INVALID, 0);
		// re-Read the configuration data out of EEPROM;
		// perhaps it just fails on first access?
		eeprom_read(0, sizeof(EEGeneral), (void*) &g_eeGeneral);
	}

	// now register eeprom update task
	task_register(TASK_PROCESS_EEPROM, settings_process);
	settings_process(0);
}

