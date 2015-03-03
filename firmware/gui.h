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

#ifndef _GUI_H
#define _GUI_H

#include "keypad.h"
#include "strings.h"

typedef enum
{
	GUI_LAYOUT_NONE = 0,
	GUI_LAYOUT_SPLASH,
	GUI_LAYOUT_MAIN1,
	GUI_LAYOUT_MAIN2,
	GUI_LAYOUT_MAIN3,
	GUI_LAYOUT_MAIN4,
	GUI_LAYOUT_MENU,
	GUI_LAYOUT_SYSTEM_MENU,
	GUI_LAYOUT_MODEL_MENU,
	GUI_LAYOUT_STICK_CALIBRATION,

} GUI_LAYOUT;

typedef enum
{
	UPDATE_NEW_LAYOUT = 0x00,
	UPDATE_MSG = 0x01,
	UPDATE_STICKS = 0x02,
	UPDATE_TRIM = 0x04,
	UPDATE_KEYPRESS = 0x08,
	UPDATE_TIMER = 0x10
} UPDATE_TYPE;

typedef enum _menu_mode {
	MENU_MODE_PAGE = 0,
	MENU_MODE_LIST,
	MENU_MODE_COL,
	MENU_MODE_EDIT,
	MENU_MODE_EDIT_S, /* used for string editor */
	MENU_MODE_LIST_SUB /* list menu with submenu (mixer and curves)*/

} MENU_MODE;


void gui_init(void);
void gui_process(uint32_t data);

void gui_update(UPDATE_TYPE type);
void gui_input_key(KEYPAD_KEY key);

void gui_navigate(GUI_LAYOUT layout);
void gui_popup(GUI_MSG msg, int16_t timeout);
void gui_popup_select(GUI_MSG msg);
char gui_popup_get_result();
uint8_t gui_offset_curves(int8_t max, int8_t min, int8_t a, int8_t b, int8_t x);

GUI_LAYOUT gui_get_layout(void);

#endif // _GUI_H
