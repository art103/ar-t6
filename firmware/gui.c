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
#include "lcd.h"
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
		"Please move analog controls to their extents.",
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
	{
		current_layout = new_layout;
		// Clear the screen.
		lcd_draw_rect(0, 0, LCD_WIDTH - 1, LCD_HEIGHT - 1, 0, RECT_FILL);
	}

	if (gui_timeout != 0)
	{
		new_layout = current_layout;
		// Clear the screen.
		lcd_draw_rect(0, 0, LCD_WIDTH - 1, LCD_HEIGHT - 1, 0, RECT_FILL);
	}

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
			#define BOX_W	24
			#define BOX_H	24

			#define BOX_Y	35
			#define BOX_L_X	30
			#define BOX_R_X	72

			#define POT_W	2
			#define POT_Y	(BOX_Y + BOX_H)
			#define POT_L_X 59
			#define POT_R_X 65

			#define SW_Y	(BOX_Y + 6)
			#define SW_L_X	(BOX_L_X - 20)
			#define SW_R_X	(BOX_R_X + BOX_W + 4)

			if (new_layout)
			{
				// Draw the main screen.

				// Stick Boxes
				lcd_draw_rect(BOX_L_X, BOX_Y, BOX_L_X + BOX_W, BOX_Y + BOX_H, 1, RECT_ROUNDED);
				lcd_draw_rect(BOX_R_X, BOX_Y, BOX_R_X + BOX_W, BOX_Y + BOX_H, 1, RECT_ROUNDED);
			}
			switch (data)
			{
				case UPDATE_STICKS:
				{
					int x, y;
					float *pFloat;
					sticks_get(&pFloat);

					// Stick boxes
					lcd_draw_rect(BOX_L_X, BOX_Y, BOX_L_X + BOX_W, BOX_Y + BOX_H, 0, RECT_FILL);
					lcd_draw_rect(BOX_R_X, BOX_Y, BOX_R_X + BOX_W, BOX_Y + BOX_H, 0, RECT_FILL);
					lcd_draw_rect(BOX_L_X, BOX_Y, BOX_L_X + BOX_W, BOX_Y + BOX_H, 1, RECT_ROUNDED);
					lcd_draw_rect(BOX_R_X, BOX_Y, BOX_R_X + BOX_W, BOX_Y + BOX_H, 1, RECT_ROUNDED);

					// Centre point
					lcd_draw_rect(BOX_L_X + BOX_W / 2 - 1, BOX_Y + BOX_H / 2 - 1, BOX_L_X + BOX_W / 2 + 1, BOX_Y + BOX_H / 2 + 1, 1, RECT_ROUNDED);
					lcd_draw_rect(BOX_R_X + BOX_W / 2 - 1, BOX_Y + BOX_H / 2 - 1, BOX_R_X + BOX_W / 2 + 1, BOX_Y + BOX_H / 2 + 1, 1, RECT_ROUNDED);

					// Stick position (Right)
					x = BOX_W / 2 + (BOX_W-2) * pFloat[0] / 200;
					y = BOX_W / 2 + (BOX_W-2) * -pFloat[1] / 200;
					lcd_draw_rect(BOX_R_X + x - 2, BOX_Y + y - 2, BOX_R_X + x + 2, BOX_Y + y + 2, 1, RECT_ROUNDED);

					// Stick position (Left)
					x = BOX_W / 2 + (BOX_W-2) * pFloat[3] / 200;
					y = BOX_W / 2 + (BOX_W-2) * -pFloat[2] / 200;
					lcd_draw_rect(BOX_L_X + x - 2, BOX_Y + y - 2, BOX_L_X + x + 2, BOX_Y + y + 2, 1, RECT_ROUNDED);

					// VRB
					x = BOX_H * (100 + pFloat[5]) / 200;
					lcd_draw_rect(POT_L_X, POT_Y - BOX_H, POT_L_X + POT_W, POT_Y, 0, RECT_FILL);
					lcd_draw_rect(POT_L_X, POT_Y - x, POT_L_X + POT_W, POT_Y, 1, RECT_FILL);

					// VRA
					x = BOX_H * (100 + pFloat[4]) / 200;
					lcd_draw_rect(POT_R_X, POT_Y - BOX_H, POT_R_X + POT_W, POT_Y, 0, RECT_FILL);
					lcd_draw_rect(POT_R_X, POT_Y - x, POT_R_X + POT_W, POT_Y, 1, RECT_FILL);

					x = keypad_get_switches();
					lcd_set_cursor(SW_L_X, SW_Y);
					lcd_write_string("SWB", (x&SWITCH_SWB)?1:0);
					lcd_set_cursor(SW_L_X, SW_Y + 8);
					lcd_write_string("SWD", (x&SWITCH_SWD)?1:0);
					lcd_set_cursor(SW_R_X, SW_Y);
					lcd_write_string("SWA", (x&SWITCH_SWA)?1:0);
					lcd_set_cursor(SW_R_X, SW_Y + 8);
					lcd_write_string("SWC", (x&SWITCH_SWC)?1:0);
				}
				break;

				case UPDATE_MSG:
				break;
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
			if (new_layout)
			{
				// Draw the whole screen.
				lcd_set_cursor(0, 56);
				lcd_write_string("CALIBRATION", 0);
			}
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

		// Draw the background.
		lcd_draw_rect(6, 4, LCD_WIDTH - 6, LCD_HEIGHT - 22, 0, RECT_FILL);
		lcd_draw_rect(6, 4, LCD_WIDTH - 6, LCD_HEIGHT - 22, 1, RECT_NONE);
		lcd_draw_rect(8, 6, LCD_WIDTH - 8, LCD_HEIGHT - 24, 1, RECT_NONE);
		// Draw the message
		lcd_set_cursor(10, 12);
		lcd_draw_message(msg[new_msg], 1);

	}

	new_layout = GUI_LAYOUT_NONE;
	new_msg = GUI_MSG_NONE;

	lcd_update();

	if (gui_timeout != 0)
		task_schedule(TASK_PROCESS_GUI, UPDATE_MSG, gui_timeout);
}

/**
  * @brief  Drive the GUI with stick data.
  * @note
  * @param  data: Scaled stick data.
  * @param  len: number of inputs.
  * @retval None
  */
void gui_update_sticks(void)
{
	switch (current_layout)
	{
		case GUI_LAYOUT_MAIN:
		case GUI_LAYOUT_STICK_CALIBRATION:
			// Don't go crazy on the updates. limit to 25fps.
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
	gui_timeout = timeout;

	task_schedule(TASK_PROCESS_GUI, UPDATE_MSG, 0);
}
