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
 * This is the main GUI module. It is used to respond to key presses,
 * display and edit model and system variables.
 */

#include "stm32f10x.h"
#include "tasks.h"
#include "lcd.h"
#include "sticks.h"
#include "mixer.h"
#include "pulses.h"
#include "gui.h"
#include "myeeprom.h"
#include "art6.h"
#include "icons.h"

// Message Popup
#define MSG_X	6
#define MSG_Y	16
#define MSG_H	32

// Stick boxes
#define BOX_W	22
#define BOX_H	22
#define BOX_Y	34
#define BOX_L_X	32
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

static volatile GUI_LAYOUT new_layout = GUI_LAYOUT_NONE;
static GUI_LAYOUT current_layout = GUI_LAYOUT_SPLASH;
static volatile GUI_MSG new_msg = GUI_MSG_NONE;
static GUI_MSG current_msg = GUI_MSG_NONE;
static volatile KEYPAD_KEY key_press = KEY_NONE;
static volatile uint32_t gui_timeout = 0;

static volatile uint8_t update_type = 0;

static const char *msg[GUI_MSG_MAX] = {
		"",
		"Please move all analog controls to their extents then press OK.",
		"Please centre the sticks then press OK.",
		"OK",
		"Operation Cancelled.",
		"OK:Save Cancel:Abort"
};


static void gui_show_sticks(void);
static void gui_show_switches(void);
static void gui_show_battery(int x, int y);
static void gui_show_timer(int x, int y);
static void gui_update_trim(void);
static void gui_draw_trim(int x, int y, bool h_v, int value);
static void gui_draw_slider(int x, int y, int w, int h, int range, int value);

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

	if (current_msg)
	{
		if (gui_timeout != 0 && system_ticks >= gui_timeout)
		{
			gui_timeout = 0;
			current_msg = 0;
			new_layout = current_layout;
		}
		else
		{
			task_schedule(TASK_PROCESS_GUI, UPDATE_MSG, 40);
			return;
		}
	}

	// Draw infrequently updated information.
	if (new_layout)
	{
		current_layout = new_layout;

		// Clear the screen.
		lcd_draw_rect(0, 0, LCD_WIDTH - 1, LCD_HEIGHT - 1, LCD_OP_CLR, RECT_FILL);
		lcd_set_cursor(0, 0);

		if (current_layout >= GUI_LAYOUT_MAIN1 && current_layout <= GUI_LAYOUT_MAIN4)
		{
			// Trim
			gui_update_trim();

			// Update Battery icon
			gui_show_battery(83, 0);

			// Update the timer
			gui_show_timer(39, 17);

			// Model Name
			lcd_set_cursor(8, 0);
			lcd_write_string(g_model.name, LCD_OP_SET, CHAR_2X);
		}

		full = TRUE;
		new_layout = GUI_LAYOUT_NONE;
	}

	if (new_msg)
	{
		current_msg = new_msg;

		// Draw the background.
		lcd_draw_rect(MSG_X, MSG_Y, LCD_WIDTH - MSG_X, MSG_Y + MSG_H, LCD_OP_CLR, RECT_FILL);
		lcd_draw_rect(MSG_X, MSG_Y, LCD_WIDTH - MSG_X, MSG_Y + MSG_H, LCD_OP_SET, FLAGS_NONE);
		lcd_draw_rect(MSG_X + 2, MSG_Y + 2, LCD_WIDTH - MSG_X - 2, MSG_Y + MSG_H - 2, LCD_OP_SET, FLAGS_NONE);
		// Draw the message
		lcd_set_cursor(MSG_X + 4, MSG_Y + 4);
		lcd_draw_message(msg[new_msg], LCD_OP_SET);

		new_msg = GUI_MSG_NONE;

		lcd_update();

		return;
	}

	// Global information
	if (!full && current_layout >= GUI_LAYOUT_MAIN1 && current_layout <= GUI_LAYOUT_MAIN4)
	{
		// Trim
		if ((key_press & TRIM_KEYS) != 0)
		{
			gui_update_trim();
		}
		else if (key_press & KEY_MENU)
		{
			gui_navigate(GUI_LAYOUT_MENU);
		}

		// Update the battery level
		if ((update_type & UPDATE_STICKS) != 0)
		{
			gui_show_battery(83, 0);
		}

		// Update the timer
		if ((update_type & UPDATE_TIMER) != 0)
		{
			gui_show_timer(39, 17);
		}
	}

	switch (current_layout)
	{
		case GUI_LAYOUT_NONE:
			// Something went badly wrong.
		break;

		case GUI_LAYOUT_SPLASH:
		break;

		/**********************************************************************
		 * Main 1
		 *
		 * Displays model name, trim, battery and timer plus a graphical
		 * representation of the stick and pot postitions and key states.
		 *
		 */
		case GUI_LAYOUT_MAIN1:
			if ((update_type & UPDATE_STICKS) != 0)
			{
				gui_show_sticks();
				gui_show_switches();
			}

			if ((update_type & UPDATE_KEYPRESS) != 0)
			{
				if (key_press & KEY_RIGHT)
					gui_navigate(GUI_LAYOUT_MAIN2);
				else if (key_press & KEY_LEFT)
					gui_navigate(GUI_LAYOUT_MAIN4);
			}
		break;

		/**********************************************************************
		 * Main 2
		 *
		 * Displays model name, trim, battery and timer plus slider bars of
		 * the first 8 PPM output channels.
		 *
		 */
		case GUI_LAYOUT_MAIN2:
			if ((update_type & UPDATE_STICKS) != 0)
			{
				const int top = 40;
				int scale = (g_model.extendedLimits == TRUE)?PPM_LIMIT_EXTENDED:PPM_LIMIT_NORMAL;


				// Left 4 sliders
				gui_draw_slider(11, top, 	  48, 3, 2*scale, scale + g_chans[0]);
				gui_draw_slider(11, top + 4,  48, 3, 2*scale, scale + g_chans[1]);
				gui_draw_slider(11, top + 8,  48, 3, 2*scale, scale + g_chans[2]);
				gui_draw_slider(11, top + 12, 48, 3, 2*scale, scale + g_chans[3]);

				// Right 4 sliders
				gui_draw_slider(67, top, 	  48, 3, 2*scale, scale + g_chans[4]);
				gui_draw_slider(67, top + 4,  48, 3, 2*scale, scale + g_chans[5]);
				gui_draw_slider(67, top + 8,  48, 3, 2*scale, scale + g_chans[6]);
				gui_draw_slider(67, top + 12, 48, 3, 2*scale, scale + g_chans[7]);
			}

			if ((update_type & UPDATE_KEYPRESS) != 0)
			{
				if (key_press & KEY_RIGHT)
					gui_navigate(GUI_LAYOUT_MAIN3);
				else if (key_press & KEY_LEFT)
					gui_navigate(GUI_LAYOUT_MAIN1);
			}
		break;

		/**********************************************************************
		 * Main 3
		 *
		 * Displays model name, trim, battery and timer plus condensed
		 * decimal representation of the PPM channel outputs.
		 *
		 */
		case GUI_LAYOUT_MAIN3:
		{
			int top = 40;
			int left = 12;
			const int spacing = 28;

			lcd_draw_rect(left, top, left + spacing * 4 - 4, top + 16, LCD_OP_CLR, RECT_FILL);

			lcd_set_cursor(left, top);
			left += spacing;
			lcd_write_int(1000 * g_chans[0] / STICK_LIMIT, LCD_OP_SET, INT_DIV10 | CHAR_CONDENSED);
			lcd_set_cursor(left, top);
			left += spacing;
			lcd_write_int(1000 * g_chans[1] / STICK_LIMIT, LCD_OP_SET, INT_DIV10 | CHAR_CONDENSED);
			lcd_set_cursor(left, top);
			left += spacing;
			lcd_write_int(1000 * g_chans[2] / STICK_LIMIT, LCD_OP_SET, INT_DIV10 | CHAR_CONDENSED);
			lcd_set_cursor(left, top);
			left += spacing;
			lcd_write_int(1000 * g_chans[3] / STICK_LIMIT, LCD_OP_SET, INT_DIV10 | CHAR_CONDENSED);

			top += 8;
			left = 12;
			lcd_set_cursor(left, top);
			left += spacing;
			lcd_write_int(1000 * g_chans[4] / STICK_LIMIT, LCD_OP_SET, INT_DIV10 | CHAR_CONDENSED);
			lcd_set_cursor(left, top);
			left += spacing;
			lcd_write_int(1000 * g_chans[5] / STICK_LIMIT, LCD_OP_SET, INT_DIV10 | CHAR_CONDENSED);
			lcd_set_cursor(left, top);
			left += spacing;
			lcd_write_int(1000 * g_chans[6] / STICK_LIMIT, LCD_OP_SET, INT_DIV10 | CHAR_CONDENSED);
			lcd_set_cursor(left, top);
			left += spacing;
			lcd_write_int(1000 * g_chans[7] / STICK_LIMIT, LCD_OP_SET, INT_DIV10 | CHAR_CONDENSED);

			if ((update_type & UPDATE_KEYPRESS) != 0)
			{
				if (key_press & KEY_RIGHT)
					gui_navigate(GUI_LAYOUT_MAIN4);
				else if (key_press & KEY_LEFT)
					gui_navigate(GUI_LAYOUT_MAIN2);
			}
		}
		break;

		/**********************************************************************
		 * Main 4
		 *
		 * Displays model name, trim, battery and timer plus runtime timer
		 */
		case GUI_LAYOUT_MAIN4:
			lcd_set_cursor(37, 40);
			lcd_write_string("00:00", LCD_OP_SET, CHAR_4X);

			if ((update_type & UPDATE_KEYPRESS) != 0)
			{
				if (key_press & KEY_RIGHT)
					gui_navigate(GUI_LAYOUT_MAIN1);
				else if (key_press & KEY_LEFT)
					gui_navigate(GUI_LAYOUT_MAIN3);
			}
		break;

		/**********************************************************************
		 * Menu to select System or Model Menus
		 */
		case GUI_LAYOUT_MENU:
		{
			static uint8_t item = 0;

			if (full)
			{
				lcd_set_cursor(25, 0);
				lcd_write_string("Settings Menu", LCD_OP_CLR, FLAGS_NONE);

				icon_draw(0, 24, 16);
				icon_draw(1, 72, 16);

				lcd_set_cursor(24, 48);
				lcd_write_string("Radio", LCD_OP_SET, FLAGS_NONE);
				lcd_set_cursor(72, 48);
				lcd_write_string("Model", LCD_OP_SET, FLAGS_NONE);

				lcd_draw_rect(24, 57, 56, 58, LCD_OP_SET, RECT_FILL);
			}

			if ((update_type & UPDATE_KEYPRESS) != 0)
			{
				if (key_press & KEY_LEFT)
				{
					item = 0;
					lcd_draw_rect(24, 57, 104, 58, LCD_OP_CLR, RECT_FILL);
					lcd_draw_rect(24, 57, 56, 58, LCD_OP_SET, RECT_FILL);
				}
				else if (key_press & KEY_RIGHT)
				{
					item = 1;
					lcd_draw_rect(24, 57, 104, 58, LCD_OP_CLR, RECT_FILL);
					lcd_draw_rect(72, 57, 104, 58, LCD_OP_SET, RECT_FILL);
				}
				else if (key_press & (KEY_SEL | KEY_OK))
				{
					if (item)
						gui_navigate(GUI_LAYOUT_MODEL_MENU);
					else
						gui_navigate(GUI_LAYOUT_SYSTEM_MENU);
				}
				else if (key_press & KEY_CANCEL)
				{
					gui_navigate(GUI_LAYOUT_MAIN1);
				}
			}
		}
		break;

		case GUI_LAYOUT_SYSTEM_MENU:
		//break;

		case GUI_LAYOUT_MODEL_MENU:
		//break;

		/**********************************************************************
		 * Calibration Screen
		 *
		 * Drives the sticks module into calibration mode and returns to
		 * Main 1 when complete or cancelled.
		 *
		 */
		case GUI_LAYOUT_STICK_CALIBRATION:
		{
			static CAL_STATE state;
			if (full)
			{
				// Draw the whole screen.
				state = CAL_LIMITS;
				sticks_calibrate(state);
				lcd_set_cursor(5, 0);
				lcd_draw_message(msg[GUI_MSG_CAL_MOVE_EXTENTS], LCD_OP_SET);
			}

			if ((update_type & UPDATE_STICKS) != 0)
			{
				gui_show_sticks();
			}

			if ((update_type & UPDATE_KEYPRESS) != 0)
			{
				lcd_set_cursor(5, 8);
				if ((key_press & KEY_SEL) || (key_press & KEY_OK))
				{
					lcd_draw_rect(5, 0, 123, BOX_Y-1, LCD_OP_CLR, RECT_FILL);
					if (state == CAL_LIMITS)
					{
						state = CAL_CENTER;
						sticks_calibrate(CAL_CENTER);
						lcd_draw_message(msg[GUI_MSG_CAL_CENTRE], LCD_OP_SET);
					}
					else if (state == CAL_CENTER)
					{
						state = CAL_OFF;
						sticks_calibrate(CAL_OFF);
						lcd_draw_message(msg[GUI_MSG_OK_CANCEL], LCD_OP_SET);
					}
					else
					{
						// ToDo: Write the calibration data into EEPROM.
						// eeprom_set_data(EEPROM_ADC_CAL, cal_data);
						gui_navigate(GUI_LAYOUT_MAIN1);
					}
				}
				else if (key_press & KEY_CANCEL)
				{
					// ToDo: eeprom_get_data(EEPROM_ADC_CAL, cal_data);
					gui_navigate(GUI_LAYOUT_MAIN1);
				}
			}
		}
		break;
	}

	key_press = KEY_NONE;

	lcd_update();
}

/**
  * @brief  Update a specific GUI component.
  * @note
  * @param  type: Component type to update.
  * @retval None
  */
void gui_update(UPDATE_TYPE type)
{
	if (gui_timeout != 0)
		return;

	update_type |= type;

	// Don't go crazy on the updates. limit to 25fps.
	task_schedule(TASK_PROCESS_GUI, type, 40);
}

/**
  * @brief  Drive the GUI with keys
  * @note
  * @param  key: The key that was pressed.
  * @retval None
  */
void gui_input_key(KEYPAD_KEY key)
{
	key_press |= key;
	gui_update(UPDATE_KEYPRESS);
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
void gui_popup(GUI_MSG msg, int16_t timeout)
{
	new_msg = msg;
	current_msg = 0;
	if (timeout == 0)
		gui_timeout = 0;
	else
		gui_timeout = system_ticks + timeout;

	task_schedule(TASK_PROCESS_GUI, UPDATE_MSG, 0);
}


/**
  * @brief  Display The stick position in two squares.
  * @note	Also shows POT bars.
  * @param  None.
  * @retval None.
  */
static void gui_show_sticks(void)
{
	int x, y;

	// Stick boxes
	lcd_draw_rect(BOX_L_X, BOX_Y, BOX_L_X + BOX_W, BOX_Y + BOX_H, LCD_OP_CLR, RECT_FILL);
	lcd_draw_rect(BOX_R_X, BOX_Y, BOX_R_X + BOX_W, BOX_Y + BOX_H, LCD_OP_CLR, RECT_FILL);
	lcd_draw_rect(BOX_L_X, BOX_Y, BOX_L_X + BOX_W, BOX_Y + BOX_H, LCD_OP_SET, RECT_ROUNDED);
	lcd_draw_rect(BOX_R_X, BOX_Y, BOX_R_X + BOX_W, BOX_Y + BOX_H, LCD_OP_SET, RECT_ROUNDED);

	// Centre point
	lcd_draw_rect(BOX_L_X + BOX_W / 2 - 1, BOX_Y + BOX_H / 2 - 1, BOX_L_X + BOX_W / 2 + 1, BOX_Y + BOX_H / 2 + 1, LCD_OP_SET, RECT_ROUNDED);
	lcd_draw_rect(BOX_R_X + BOX_W / 2 - 1, BOX_Y + BOX_H / 2 - 1, BOX_R_X + BOX_W / 2 + 1, BOX_Y + BOX_H / 2 + 1, LCD_OP_SET, RECT_ROUNDED);

	// Stick position (Right)
	x = 2 + (BOX_W-4) * sticks_get_percent(STICK_R_H) / 100;
	y = BOX_H - 2 - (BOX_H-4) * sticks_get_percent(STICK_R_V) / 100;
	lcd_draw_rect(BOX_R_X + x - 2, BOX_Y + y - 2, BOX_R_X + x + 2, BOX_Y + y + 2, LCD_OP_SET, RECT_ROUNDED);

	// Stick position (Left)
	x = 2 + (BOX_W-4) * sticks_get_percent(STICK_L_H) / 100;
	y = BOX_H - 2 - (BOX_H-4) * sticks_get_percent(STICK_L_V) / 100;
	lcd_draw_rect(BOX_L_X + x - 2, BOX_Y + y - 2, BOX_L_X + x + 2, BOX_Y + y + 2, LCD_OP_SET, RECT_ROUNDED);

	// VRB
	x = BOX_H * sticks_get_percent(STICK_VRB) / 100;
	lcd_draw_rect(POT_L_X, POT_Y - BOX_H, POT_L_X + POT_W, POT_Y, LCD_OP_CLR, RECT_FILL);
	lcd_draw_rect(POT_L_X, POT_Y - x, POT_L_X + POT_W, POT_Y, LCD_OP_SET, RECT_FILL);

	// VRA
	x = BOX_H * sticks_get_percent(STICK_VRA) / 100;
	lcd_draw_rect(POT_R_X, POT_Y - BOX_H, POT_R_X + POT_W, POT_Y, LCD_OP_CLR, RECT_FILL);
	lcd_draw_rect(POT_R_X, POT_Y - x, POT_R_X + POT_W, POT_Y, LCD_OP_SET, RECT_FILL);
}

/**
  * @brief  Display the  switch states.
  * @note
  * @param  None.
  * @retval None.
  */
static void gui_show_switches(void)
{
	int x;

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

	// Background
	lcd_draw_rect(x-1, y, x+15, y+7, LCD_OP_CLR, RECT_FILL);

	// Battery Icon
	lcd_draw_rect(x, y+1, x+12, y+6, LCD_OP_SET, RECT_ROUNDED);
	lcd_draw_rect(x+12, y+2, x+14, y+5, LCD_OP_SET, RECT_ROUNDED);
	lcd_draw_rect(x, y+2, x+level, y+5, LCD_OP_SET, RECT_FILL);

	// Voltage
	lcd_set_cursor(x + 15, y);
	lcd_write_int(batt, LCD_OP_SET, INT_DIV10);
	lcd_write_string("v", LCD_OP_SET, FLAGS_NONE);
}

/**
  * @brief  Display the current timer value.
  * @note
  * @param  x, y: Position on screen (starting cursor)
  * @retval None
  */
static void gui_show_timer(int x, int y)
{
	// Timer
	lcd_set_cursor(x, y);
	lcd_write_int(g_model.tmrVal / 60, LCD_OP_SET, INT_PAD10 | CHAR_4X);
	lcd_write_string(":", LCD_OP_SET, INT_PAD10 | CHAR_2X);
	lcd_write_int(g_model.tmrVal % 60, LCD_OP_SET, INT_PAD10 | CHAR_4X);
}

/**
  * @brief  Update all 4 trim bars.
  * @note
  * @param  None
  * @retval None
  */
static void gui_update_trim(void)
{
	// Update the trim bars.
	gui_draw_trim(0, 8, FALSE, mixer_get_trim(STICK_L_V));
	gui_draw_trim(11, 57, TRUE, mixer_get_trim(STICK_L_H));
	gui_draw_trim(121, 8, FALSE, mixer_get_trim(STICK_R_V));
	gui_draw_trim(67, 57, TRUE, mixer_get_trim(STICK_R_H));
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

	// Draw the value box
	if (h_v)
	{
		// Clear the background
		lcd_draw_rect(x - 3, y, x+w + 3, y+h, LCD_OP_CLR, RECT_FILL);
		// Draw the centre dot and ends.
		lcd_draw_rect(x + w/2 - 1, y + h/2 - 1, x + w/2 + 1, y + h/2 + 1, LCD_OP_SET, RECT_FILL);
		lcd_draw_line(x, y + h/2 - 1, x, y + h/2 + 1, LCD_OP_SET);
		lcd_draw_line(x + w, y + h/2 - 1, x + w, y + h/2 + 1, LCD_OP_SET);
		// Main line
		lcd_draw_line(x, y + h/2, x + w, y + h/2, LCD_OP_SET);
		// Value Box
		lcd_draw_rect(v + x + w/2 - 3,
					  y + h/2 - 3,
					  v + x + w/2 + 3,
					  y + h/2 + 3, LCD_OP_XOR, RECT_ROUNDED);
	}
	else
	{
		// Clear the background
		lcd_draw_rect(x, y - 3, x+w, y+h + 3, LCD_OP_CLR, RECT_FILL);
		// Draw the centre dot and ends.
		lcd_draw_rect(x + w/2 - 1, y + h/2 - 1, x + w/2 + 1, y + h/2 + 1, LCD_OP_SET, RECT_FILL);
		lcd_draw_line(x + w/2 - 1, y, x + w/2 + 1, y, LCD_OP_SET);
		lcd_draw_line(x + w/2 - 1, y + h, x + w/2 + 1, y + h, LCD_OP_SET);
		// Main line
		lcd_draw_rect(x + w/2, y, x + w/2, y + h, LCD_OP_SET, RECT_FILL);
		// Value Box
		lcd_draw_rect(x + w/2 - 3,
					  -v + y + h/2 - 3,
					  x + w/2 + 3,
					  -v + y + h/2 + 3, LCD_OP_XOR, RECT_ROUNDED);
	}
}

/**
  * @brief  Display a slider bar with the supplied parameters
  * @note
  * @param  x, y: The TL position of the bar on screen.
  * @param  w, h: The size of the bar.
  * @param  range: Full scale slider range (0 - range).
  * @param  value: The value of the slider.
  * @retval None
  */
static void gui_draw_slider(int x, int y, int w, int h, int range, int value)
{
	int i, v, d;

	// Value scaling
	v = (w * value / range) - (w/2);
	// Division spacing
	d = w / 20;

	// Clear the background
	lcd_draw_rect(x, y, x+w, y+h, LCD_OP_CLR, RECT_FILL);

	// Draw the dashed line
	for (i=0; i<w; i += d)
	{
		lcd_set_pixel(x + i, y + h/2, LCD_OP_SET);
	}

	// Draw the bar
	if (v > 0)
		lcd_draw_rect(x + w/2, y, x + w/2 + v, y + h - 1, LCD_OP_XOR, RECT_FILL);
	else
		lcd_draw_rect(x + w/2 + v, y, x + w/2, y + h - 1, LCD_OP_XOR, RECT_FILL);

	// Draw the ends and middle
	lcd_draw_line(x, y, x, y+h-1, LCD_OP_SET);
	lcd_draw_line(x+w/2, y, x+w/2, y+h-1, LCD_OP_SET);
	lcd_draw_line(x+w, y, x+w, y+h-1, LCD_OP_SET);
}

