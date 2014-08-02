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
#include "sticks.h"
#include "mixer.h"
#include "gui.h"

// Message Popup
#define MSG_X	6
#define MSG_Y	16
#define MSG_H	32

// Stick boxes
#define BOX_W	24
#define BOX_H	24
#define BOX_Y	35
#define BOX_L_X	30
#define BOX_R_X	72
// POT bars
#define POT_W	2
#define POT_Y	(BOX_Y + BOX_H)
#define POT_L_X 59
#define POT_R_X 65
// Switch Labels
#define SW_Y	(BOX_Y + 6)
#define SW_L_X	(BOX_L_X - 20)
#define SW_R_X	(BOX_R_X + BOX_W + 4)

typedef enum
{
	UPDATE_NEW_LAYOUT,
	UPDATE_MSG,
	UPDATE_STICKS,
	UPDATE_KEYPRESS
} UPDATE_TYPE;

static volatile GUI_LAYOUT new_layout = GUI_LAYOUT_NONE;
static GUI_LAYOUT current_layout = GUI_LAYOUT_SPLASH;
static volatile GUI_MSG new_msg = GUI_MSG_NONE;
static GUI_MSG current_msg = GUI_MSG_NONE;
static volatile KEYPAD_KEY key_press = KEY_NONE;
static uint32_t gui_timeout = 0;

static const char *msg[GUI_MSG_MAX] = {
		"",
		"Please move analog controls to their extents.",
		"Please centre the sticks.",
		"OK",
		"Operation Cancelled."
};

static void gui_show_sticks(void);
static void gui_show_battery(int x, int y);
static void gui_draw_trim(int x, int y, bool h_v, int value);

/**
  * @brief  Initialise the GUI.
  * @note
  * @param  None
  * @retval None
  */
void gui_init(void)
{
	task_register(TASK_PROCESS_GUI, gui_process);
	gui_navigate(GUI_LAYOUT_SPLASH);
}

/**
  * @brief  GUI task.
  * @note
  * @param  data: Update type (UPDATE_TYPE).
  * @retval None
  */
void gui_process(uint32_t data)
{
	bool full = FALSE;

	if (gui_timeout != 0)
	{
		new_layout = current_layout;
		gui_timeout = 0;
	}

	if (new_layout)
	{
		current_layout = new_layout;
		// Clear the screen.
		lcd_draw_rect(0, 0, LCD_WIDTH - 1, LCD_HEIGHT - 1, 0, RECT_FILL);
		lcd_set_cursor(0, 0);

		full = TRUE;
		new_layout = GUI_LAYOUT_NONE;
	}

	if (new_msg)
	{
		current_msg = new_msg;

		// Draw the background.
		lcd_draw_rect(MSG_X, MSG_Y, LCD_WIDTH - MSG_X, MSG_Y + MSG_H, 0, RECT_FILL);
		lcd_draw_rect(MSG_X, MSG_Y, LCD_WIDTH - MSG_X, MSG_Y + MSG_H, 1, FLAGS_NONE);
		lcd_draw_rect(MSG_X + 2, MSG_Y + 2, LCD_WIDTH - MSG_X - 2, MSG_Y + MSG_H - 2, 1, FLAGS_NONE);
		// Draw the message
		lcd_set_cursor(MSG_X + 4, MSG_Y + 4);
		lcd_draw_message(msg[new_msg], 1);

		new_msg = GUI_MSG_NONE;

		lcd_update();

		// Make sure we re-draw the GUI.
		new_layout = current_layout;

		return;
	}

	if (key_press != KEY_NONE)
		data = UPDATE_KEYPRESS;

	switch (current_layout)
	{
		case GUI_LAYOUT_NONE:
			// Something went badly wrong.
		break;

		case GUI_LAYOUT_SPLASH:
			if (full)
			{
				// Draw the main screen.
				lcd_write_string("SPLASH", LCD_OP_CLR, FLAGS_NONE);
			}
		break;

		case GUI_LAYOUT_MAIN:
			if (full)
			{
				// Draw the main screen.
				lcd_write_string("MAIN", LCD_OP_CLR, FLAGS_NONE);

				// Update the trim bars.
				gui_draw_trim(0, 8, FALSE, mixer_get_trim(STICK_L_V));
				gui_draw_trim(11, 57, TRUE, mixer_get_trim(STICK_L_H));
				gui_draw_trim(121, 8, FALSE, mixer_get_trim(STICK_R_V));
				gui_draw_trim(67, 57, TRUE, mixer_get_trim(STICK_R_H));
			}

			switch (data)
			{
				case UPDATE_STICKS:
				{
					gui_show_sticks();
					gui_show_battery(83, 0);
				}
				break;

				case UPDATE_KEYPRESS:
					if (key_press == KEY_RIGHT)
						gui_navigate(GUI_LAYOUT_MAIN2);
					else if (key_press == KEY_LEFT)
						gui_navigate(GUI_LAYOUT_MAIN3);
				break;
			}
		break;

		case GUI_LAYOUT_MAIN2:
			if (full)
			{
				// Draw the main screen.
				lcd_write_string("MAIN2", LCD_OP_CLR, FLAGS_NONE);
			}
			switch (data)
			{
				case UPDATE_STICKS:
				{
					//gui_show_sticks();
					//gui_show_battery(83, 0);
				}
				break;

				case UPDATE_KEYPRESS:
					if (key_press == KEY_RIGHT)
						gui_navigate(GUI_LAYOUT_MAIN3);
					else if (key_press == KEY_LEFT)
						gui_navigate(GUI_LAYOUT_MAIN);
					else if (key_press >= KEY_CH1_UP && key_press <= KEY_CH4_DN)
					{
						// Update the trim bars.
						gui_draw_trim(0, 5, FALSE, mixer_get_trim(STICK_L_V));
						gui_draw_trim(15, 57, TRUE, mixer_get_trim(STICK_L_H));
						gui_draw_trim(121, 5, FALSE, mixer_get_trim(STICK_R_V));
						gui_draw_trim(58, 57, TRUE, mixer_get_trim(STICK_R_H));
					}
				break;
			}
		break;

		case GUI_LAYOUT_MAIN3:
			if (full)
			{
				// Draw the main screen.
				lcd_write_string("MAIN3", LCD_OP_CLR, FLAGS_NONE);
			}

			switch (data)
			{
				case UPDATE_STICKS:
				{
					//gui_show_sticks();
					//gui_show_battery(83, 0);
				}
				break;

				case UPDATE_KEYPRESS:
					if (key_press == KEY_RIGHT)
						gui_navigate(GUI_LAYOUT_MAIN);
					else if (key_press == KEY_LEFT)
						gui_navigate(GUI_LAYOUT_MAIN2);
				break;
			}
		break;

		case GUI_LAYOUT_SYSTEM_MENU:
		break;

		case GUI_LAYOUT_MODEL_MENU:
		break;

		case GUI_LAYOUT_STICK_CALIBRATION:
			if (full)
			{
				// Draw the whole screen.
				lcd_write_string("CALIBRATION", LCD_OP_CLR, FLAGS_NONE);
			}
			switch (data)
			{
				case UPDATE_STICKS:
				break;
			}
		break;
	}

	key_press = KEY_NONE;

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
	if (gui_timeout != 0)
		return;

	switch (current_layout)
	{
		case GUI_LAYOUT_MAIN:
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
  * @brief  Pop one level of the UI stack
  * @note
  * @param  None.
  * @retval None
  */
void gui_back(void)
{
}

/**
  * @brief  Set the GUI to a specific layout.
  * @note
  * @param  layout: The requested layout.
  * @retval None
  */
void gui_navigate(GUI_LAYOUT layout)
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
void gui_popup(GUI_MSG msg, uint16_t timeout)
{
	new_msg = msg;
	gui_timeout = timeout;

	task_schedule(TASK_PROCESS_GUI, UPDATE_MSG, 0);
}


/**
  * @brief  Display The stick position in two squares.
  * @note	Also shows POT bars and switch states.
  * @param  None.
  * @retval None.
  */
static void gui_show_sticks(void)
{
	int x, y;

	// Stick boxes
	lcd_draw_rect(BOX_L_X, BOX_Y, BOX_L_X + BOX_W, BOX_Y + BOX_H, 0, RECT_FILL);
	lcd_draw_rect(BOX_R_X, BOX_Y, BOX_R_X + BOX_W, BOX_Y + BOX_H, 0, RECT_FILL);
	lcd_draw_rect(BOX_L_X, BOX_Y, BOX_L_X + BOX_W, BOX_Y + BOX_H, 1, RECT_ROUNDED);
	lcd_draw_rect(BOX_R_X, BOX_Y, BOX_R_X + BOX_W, BOX_Y + BOX_H, 1, RECT_ROUNDED);

	// Centre point
	lcd_draw_rect(BOX_L_X + BOX_W / 2 - 1, BOX_Y + BOX_H / 2 - 1, BOX_L_X + BOX_W / 2 + 1, BOX_Y + BOX_H / 2 + 1, 1, RECT_ROUNDED);
	lcd_draw_rect(BOX_R_X + BOX_W / 2 - 1, BOX_Y + BOX_H / 2 - 1, BOX_R_X + BOX_W / 2 + 1, BOX_Y + BOX_H / 2 + 1, 1, RECT_ROUNDED);

	// Stick position (Right)
	x = (BOX_W-3) * sticks_get_percent(STICK_R_H) / 100;
	y = BOX_W - (BOX_W-3) * sticks_get_percent(STICK_R_V) / 100;
	lcd_draw_rect(BOX_R_X + x - 2, BOX_Y + y - 2, BOX_R_X + x + 2, BOX_Y + y + 2, 1, RECT_ROUNDED);

	// Stick position (Left)
	x = (BOX_W-3) * sticks_get_percent(STICK_L_H) / 100;
	y = BOX_W - (BOX_W-3) * sticks_get_percent(STICK_L_V) / 100;
	lcd_draw_rect(BOX_L_X + x - 2, BOX_Y + y - 2, BOX_L_X + x + 2, BOX_Y + y + 2, 1, RECT_ROUNDED);

	// VRB
	x = BOX_H * sticks_get_percent(STICK_VRB) / 100;
	lcd_draw_rect(POT_L_X, POT_Y - BOX_H, POT_L_X + POT_W, POT_Y, 0, RECT_FILL);
	lcd_draw_rect(POT_L_X, POT_Y - x, POT_L_X + POT_W, POT_Y, 1, RECT_FILL);

	// VRA
	x = BOX_H * sticks_get_percent(STICK_VRA) / 100;
	lcd_draw_rect(POT_R_X, POT_Y - BOX_H, POT_R_X + POT_W, POT_Y, 0, RECT_FILL);
	lcd_draw_rect(POT_R_X, POT_Y - x, POT_R_X + POT_W, POT_Y, 1, RECT_FILL);

	// Switches
	x = keypad_get_switches();
	lcd_set_cursor(SW_L_X, SW_Y);
	lcd_write_string("SWB", (x&SWITCH_SWB)?LCD_OP_SET:LCD_OP_CLR, FLAGS_NONE);
	lcd_set_cursor(SW_L_X, SW_Y + 8);
	lcd_write_string("SWD", (x&SWITCH_SWD)?LCD_OP_SET:LCD_OP_CLR, FLAGS_NONE);
	lcd_set_cursor(SW_R_X, SW_Y);
	lcd_write_string("SWA", (x&SWITCH_SWA)?LCD_OP_SET:LCD_OP_CLR, FLAGS_NONE);
	lcd_set_cursor(SW_R_X, SW_Y + 8);
	lcd_write_string("SWC", (x&SWITCH_SWC)?LCD_OP_SET:LCD_OP_CLR, FLAGS_NONE);
}

#define BATT_MIN	99	//NiMh: 88
#define BATT_MAX	126	//NiMh: 104

/**
  * @brief  Display a battery icon and text with the current level
  * @note
  * @param  x,y: TL location for the information.
  * @retval None.
  */
static void gui_show_battery(int x, int y)
{
	int batt;
	int level;

	batt = sticks_get_percent(STICK_BAT) * 129 / 100;
	level = 12 * (batt - BATT_MIN) / (BATT_MAX - BATT_MIN);
	level = (level > 12)?12:level;

	// Battery Icon
	lcd_draw_rect(x, y+2, x+12, y+5, 0, RECT_FILL);
	lcd_draw_rect(x, y+1, x+12, y+6, 1, RECT_ROUNDED);
	lcd_draw_rect(x+12, y+2, x+14, y+5, 1, RECT_ROUNDED);
	lcd_draw_rect(x, y+2, x+level, y+5, 1, RECT_FILL);

	// Voltage
	lcd_set_cursor(x + 15, y);
	lcd_write_int(batt, 1, INT_DIV10);
	lcd_write_string("v", LCD_OP_SET, FLAGS_NONE);
}

/**
  * @brief  Display a trim bar with the supplied value
  * @note
  * @param  x, y: The TL position of the bar on screen.
  * @param  h_v: Whether the trim is horizontal (TRUE) or vertical (FALSE)
  * @param  value: The value of the trim (location of the rectangle) from -100 to +100.
  * @retval None
  */
static void gui_draw_trim(int x, int y, bool h_v, int value)
{
	int w, h, v;
	w = (h_v)?48:6;
	h = (h_v)?6:48;
	v = 48 * value / (2 * MIXER_TRIM_LIMIT);

	// Draw the line and centre dot.
	lcd_draw_rect(x, y, x+w, y+h, LCD_OP_CLR, RECT_FILL);
	lcd_draw_rect(x + w/2 - 1, y + h/2 - 1, x + w/2 + 1, y + h/2 + 1, LCD_OP_SET, RECT_FILL);

	// Draw the value box
	if (h_v)
	{
		lcd_draw_line(x, y + h/2, x + w, y + h/2, LCD_OP_SET);
		lcd_draw_rect(v + x + w/2 - 3,
					  y + h/2 - 3,
					  v + x + w/2 + 3,
					  y + h/2 + 3, LCD_OP_XOR, RECT_ROUNDED);
	}
	else
	{
		lcd_draw_rect(x + w/2, y, x + w/2, y + h, LCD_OP_SET, RECT_FILL);
		lcd_draw_rect(x + w/2 - 3,
					  v + y + h/2 - 3,
					  x + w/2 + 3,
					  v + y + h/2 + 3, LCD_OP_XOR, RECT_ROUNDED);
	}




}
