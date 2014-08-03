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

typedef enum
{
	GUI_LAYOUT_NONE = 0,
	GUI_LAYOUT_SPLASH,
	GUI_LAYOUT_MAIN1,
	GUI_LAYOUT_MAIN2,
	GUI_LAYOUT_MAIN3,
	GUI_LAYOUT_MAIN4,
	GUI_LAYOUT_SYSTEM_MENU,
	GUI_LAYOUT_MODEL_MENU,
	GUI_LAYOUT_STICK_CALIBRATION,

} GUI_LAYOUT;

typedef enum
{
	GUI_MSG_NONE = 0,
	GUI_MSG_CAL_MOVE_EXTENTS,
	GUI_MSG_CAL_CENTRE,
	GUI_MSG_OK,
	GUI_MSG_CANCELLED,
	GUI_MSG_OK_CANCEL,
	GUI_MSG_MAX,
} GUI_MSG;

typedef enum
{
	UPDATE_NEW_LAYOUT = 0x00,
	UPDATE_MSG = 0x01,
	UPDATE_STICKS = 0x02,
	UPDATE_TRIM = 0x04,
	UPDATE_KEYPRESS = 0x08,
	UPDATE_TIMER = 0x10
} UPDATE_TYPE;


void gui_init(void);
void gui_process(uint32_t data);

void gui_update(UPDATE_TYPE type);
void gui_input_key(KEYPAD_KEY key);

void gui_back(void);
void gui_navigate(GUI_LAYOUT layout);
void gui_popup(GUI_MSG msg, int16_t timeout);

#endif // _GUI_H
