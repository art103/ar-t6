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

#include "stm32f10x.h"
#include "tasks.h"
#include "gui.h"

typedef enum
{
	UPDATE_NEW_LAYOUT,
	UPDATE_MSG,
	UPDATE_STICKS,
	UPDATE_KEYPRESS
} UPDATE_TYPE;

static GUI_LAYOUT new_layout = GUI_LAYOUT_NONE;
static GUI_LAYOUT current_layout = GUI_LAYOUT_SPLASH;
static GUI_MSG new_msg = GUI_MSG_NONE;
static GUI_MSG current_msg = GUI_MSG_NONE;
static KEYPAD_KEY key_press = KEY_NONE;
static uint32_t gui_timeout = 0;

static const char *msg[GUI_MSG_MAX] = {
		"",
		"Please move sticks and POTs to all extents.",
		"Please centre the sticks.",
		"OK",
		"Operation Cancelled."
};


/**
  * @brief  Initialise the GUI.
  * @note
  * @param  None
  * @retval None
  */
void gui_init(void)
{
	task_register(TASK_PROCESS_GUI, gui_process);
}

/**
  * @brief  GUI task.
  * @note
  * @param  data: Update type (UPDATE_TYPE).
  * @retval None
  */
void gui_process(uint32_t data)
{
	if (new_layout)
		current_layout = new_layout;

	switch (current_layout)
	{
		case GUI_LAYOUT_NONE:
			// Something went badly wrong.
		break;

		case GUI_LAYOUT_SPLASH:
			if (new_layout)
			{
				// Draw the whole splash screen.
			}
		break;

		case GUI_LAYOUT_MAIN:
			if (new_layout)
			{
				// Draw the main screen.
			}
		break;

		case GUI_LAYOUT_MAIN2:
		break;

		case GUI_LAYOUT_MAIN3:
		break;

		case GUI_LAYOUT_SYSTEM_MENU:
		break;

		case GUI_LAYOUT_MODEL_MENU:
		break;

		case GUI_LAYOUT_STICK_CALIBRATION:
			switch (data)
			{
				case UPDATE_STICKS:
				break;

				case UPDATE_MSG:

				break;
			}
		break;
	}

	if (new_msg)
	{
		current_msg = new_msg;

		// Draw the message
		lcd_draw_message(msg[new_msg], 0, 1);
	}

	new_layout = GUI_LAYOUT_NONE;
	new_msg = GUI_MSG_NONE;
}

/**
  * @brief  Drive the GUI with stick data.
  * @note
  * @param  data: Scaled stick data.
  * @param  len: number of inputs.
  * @retval None
  */
void gui_input_sticks(float *data, uint8_t len)
{
	switch (current_layout)
	{
		case GUI_LAYOUT_STICK_CALIBRATION:
			// Don't do crazy on the updates. limit to 25fps.
			task_schedule(TASK_PROCESS_GUI, UPDATE_STICKS, 40);
		break;

		default:
			// No need to update the GUI for other layouts.
		break;
	}
}

/**
  * @brief  Drive the GUI with keys
  * @note
  * @param  key: The key that was pressed.
  * @retval None
  */
void gui_input_key(KEYPAD_KEY key)
{
	key_press = key;

	if ( (key >= KEY_SEL && key <= KEY_RIGHT) ||
		 (current_layout == GUI_LAYOUT_MAIN) )
	{
		task_schedule(TASK_PROCESS_GUI, UPDATE_KEYPRESS, 0);
	}
}

/**
  * @brief  Set the GUI to a specific layout.
  * @note
  * @param  layout: The requested layout.
  * @retval None
  */
void gui_set_layout(GUI_LAYOUT layout)
{
	new_layout = layout;
	task_schedule(TASK_PROCESS_GUI, UPDATE_NEW_LAYOUT, 0);
}

/**
  * @brief  Display the requested message.
  * @note	The location and translation will be specific to the current state.
  * @param  state: The requested state.
  * @param  timeout: The number of ms to display the message. 0 for no timeout.
  * @retval None
  */
void gui_set_message(GUI_MSG msg, uint16_t timeout)
{
	new_msg = msg;
	if (timeout != 0)
		gui_timeout = system_ticks + timeout;
	else
		gui_timeout = 0;

	task_schedule(TASK_PROCESS_GUI, UPDATE_MSG, 0);
}
