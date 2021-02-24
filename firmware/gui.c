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

#include <stddef.h>
#include "system.h"
#include "tasks.h"
#include "lcd.h"
#include "sticks.h"
#include "mixer.h"
#include "pulses.h"
#include "gui.h"
#include "myeeprom.h"
#include "settings.h"
#include "art6.h"
#include "icons.h"
#include "sound.h"
#include "strings.h"
#include "keypad.h"

/* As I need to split this huge file and standard splitting by header files needs a huge rewrite, I decide make some ugly "plug" includes 
 * I hope this will be temporary solution
 */

// most of all #define
#include "gui_define.i"

// helper funcs
#include "gui_func.i"

static MenuContext context = {0};

#include "gui_row_edit_func.i"

#include "gui_timer_and_backlight.i"

#include "gui_prepare.i"

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

	uint8_t full = FALSE;

	// TODO: separate task
	timer_update();

	backlight_management();

	// clear popup result until OK/SEL/CANCEL pressed,
	// then only allow one chance to process it (for safety of it was not handled)
	g_popup_result = GUI_POPUP_RESULT_NONE;
	// If we are currently displaying a popup,
	// check the time and schedule a re-check.
	// Also check keys to cancel the popup.
	if (g_current_msg)
	{
		char closeit = (g_gui_timeout != 0 && system_ticks >= g_gui_timeout);
		if (g_update_type & UPDATE_KEYPRESS)
		{
			if (g_popup_selected_line && (g_key_press & (KEY_LEFT | KEY_RIGHT)))
			{
				if (g_key_press & KEY_LEFT)
					g_popup_selected_line--;
				if (g_key_press & KEY_RIGHT)
					g_popup_selected_line++;
				if (g_popup_selected_line < context.popup_menu_first_line)
					g_popup_selected_line = context.popup_menu_first_line; // = 1
				if (g_popup_selected_line > g_popup_lines)
					g_popup_selected_line = g_popup_lines;
				g_new_msg = g_current_msg;
				g_current_msg = 0;
				g_key_press = KEY_NONE;
			}
			if ((g_key_press & (KEY_OK | KEY_CANCEL | KEY_SEL)))
			{
				closeit = 1;
				if (g_key_press & KEY_CANCEL)
					g_popup_result = GUI_POPUP_RESULT_CANCEL;
				else
					g_popup_result =
						g_popup_selected_line ? g_popup_selected_line : 1;
			}
		}
		if (closeit)
		{
			g_gui_timeout = 0;
			g_current_msg = 0;
			g_new_layout = g_current_layout;
			// clear all pending keys as we just closed the popup
			// and follow through to let the popup result be processed
			g_key_press = 0;
		}
		else
		{
			task_schedule(TASK_PROCESS_GUI, UPDATE_MSG, 40);
			return;
		}
	}

	// Update common information on new layout.
	if (g_new_layout)
	{
		g_current_layout = g_new_layout;

		// Clear the screen.
		lcd_draw_rect(0, 0, LCD_WIDTH - 1, LCD_HEIGHT - 1, LCD_OP_CLR,
					  RECT_FILL);
		lcd_set_cursor(0, 0);

		if (g_current_layout >= GUI_LAYOUT_MAIN1 && g_current_layout <= GUI_LAYOUT_MAIN4)
		{
			// Trim
			gui_update_trim();
			// Update Battery icon
			gui_show_battery(83, 0);
			// Update the timer
			gui_show_timer(TIMER_X, 17, timer);
			// Model Name
			lcd_set_cursor(8, 0);

			lcd_write_string((char *)g_model.name, LCD_OP_SET, CHAR_2X);
		}

		full = TRUE;
		g_new_layout = GUI_LAYOUT_NONE;
	}

	// If we have a new popup to display, do it now.
	if (g_new_msg)
	{
		g_current_msg = g_new_msg;

		// Draw the background.
		lcd_draw_rect(MSG_X, MSG_Y, LCD_WIDTH - MSG_X, MSG_Y + MSG_H,
					  LCD_OP_CLR, RECT_FILL);
		lcd_draw_rect(MSG_X, MSG_Y, LCD_WIDTH - MSG_X, MSG_Y + MSG_H,
					  LCD_OP_SET, FLAGS_NONE);
		lcd_draw_rect(MSG_X + 2, MSG_Y + 2, LCD_WIDTH - MSG_X - 2,
					  MSG_Y + MSG_H - 2, LCD_OP_SET, FLAGS_NONE);
		// Draw the message
		lcd_set_cursor(MSG_X + 4, MSG_Y + 4);
		g_popup_lines = lcd_draw_message(msg[g_new_msg], LCD_OP_SET, LCD_OP_CLR,
										 g_popup_selected_line);

		g_new_msg = GUI_MSG_NONE;

		lcd_update();

		task_schedule(TASK_PROCESS_GUI, UPDATE_MSG, 40);

		return;
	}

	// Global information (on Main layouts)
	if (!full && g_current_layout >= GUI_LAYOUT_MAIN1 && g_current_layout <= GUI_LAYOUT_MAIN4)
	{
		// Update the trim if needed.
		if ((g_key_press & TRIM_KEYS) != 0)
		{
			mixer_input_trim(g_key_press);
			gui_update_trim();
		}
		else if (g_key_press & KEY_MENU)
		{
			// Long press menu key handling.
			g_main_layout = g_current_layout;
			gui_navigate(GUI_LAYOUT_MENU);
		}

		// Update the battery level
		if ((g_update_type & UPDATE_STICKS) != 0)
		{
			gui_show_battery(83, 0);
		}

		// Update the timer
		//if ((g_update_type & UPDATE_TIMER) != 0) {
		gui_show_timer(TIMER_X, 17, timer);
		//}
	}

	switch (g_current_layout)
	{
	case GUI_LAYOUT_NONE:
		// Something went badly wrong.
		break; // GUI_LAYOUT_NONE

	case GUI_LAYOUT_SPLASH:
		break; // GUI_LAYOUT_SPLASH

		/**********************************************************************
		 * Main 1
		 *
		 * Displays model name, trim, battery and timer plus a graphical
		 * representation of the stick and pot positions and key states.
		 *
		 */
	case GUI_LAYOUT_MAIN1:
		if ((g_update_type & UPDATE_STICKS) != 0)
		{
			gui_show_sticks();
			gui_show_switches();
		}

		if ((g_update_type & UPDATE_KEYPRESS) != 0)
		{
			if (g_key_press & KEY_RIGHT)
				gui_navigate(GUI_LAYOUT_MAIN2);
			else if (g_key_press & KEY_LEFT)
				gui_navigate(GUI_LAYOUT_MAIN4);
		}
		break; // GUI_LAYOUT_MAIN1

		/**********************************************************************
		 * Main 2
		 *
		 * Displays model name, trim, battery and timer plus slider bars of
		 * the first 8 PPM output channels.
		 *
		 */
	case GUI_LAYOUT_MAIN2:
		if ((g_update_type & UPDATE_STICKS) != 0)
		{
			const int top = 40;
			const int scale = 1024; // RESX

			// Left 4 sliders
			gui_draw_slider(11, top, 48, 3, 2 * scale, scale + g_chans[0]);
			gui_draw_slider(11, top + 4, 48, 3, 2 * scale, scale + g_chans[1]);
			gui_draw_slider(11, top + 8, 48, 3, 2 * scale, scale + g_chans[2]);
			gui_draw_slider(11, top + 12, 48, 3, 2 * scale, scale + g_chans[3]);

			// Right 4 sliders
			gui_draw_slider(67, top, 48, 3, 2 * scale, scale + g_chans[4]);
			gui_draw_slider(67, top + 4, 48, 3, 2 * scale, scale + g_chans[5]);
			gui_draw_slider(67, top + 8, 48, 3, 2 * scale, scale + g_chans[6]);
			gui_draw_slider(67, top + 12, 48, 3, 2 * scale, scale + g_chans[7]);
		}

		if ((g_update_type & UPDATE_KEYPRESS) != 0)
		{
			if (g_key_press & KEY_RIGHT)
				gui_navigate(GUI_LAYOUT_MAIN3);
			else if (g_key_press & KEY_LEFT)
				gui_navigate(GUI_LAYOUT_MAIN1);
		}
		break; // GUI_LAYOUT_MAIN2

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
		const int scale = 1024;

		lcd_draw_rect(left, top, left + spacing * 4 - 4, top + 16, LCD_OP_CLR, RECT_FILL);

		lcd_set_cursor(left, top);
		left += spacing;
		lcd_write_int(1000 * g_chans[0] / scale, LCD_OP_SET, INT_DIV10 | CHAR_CONDENSED);
		lcd_set_cursor(left, top);
		left += spacing;
		lcd_write_int(1000 * g_chans[1] / scale, LCD_OP_SET, INT_DIV10 | CHAR_CONDENSED);
		lcd_set_cursor(left, top);
		left += spacing;
		lcd_write_int(1000 * g_chans[2] / scale, LCD_OP_SET, INT_DIV10 | CHAR_CONDENSED);
		lcd_set_cursor(left, top);
		left += spacing;
		lcd_write_int(1000 * g_chans[3] / scale, LCD_OP_SET, INT_DIV10 | CHAR_CONDENSED);

		top += 8;
		left = 12;
		lcd_set_cursor(left, top);
		left += spacing;
		lcd_write_int(1000 * g_chans[4] / scale, LCD_OP_SET, INT_DIV10 | CHAR_CONDENSED);
		lcd_set_cursor(left, top);
		left += spacing;
		lcd_write_int(1000 * g_chans[5] / scale, LCD_OP_SET, INT_DIV10 | CHAR_CONDENSED);
		lcd_set_cursor(left, top);
		left += spacing;
		lcd_write_int(1000 * g_chans[6] / scale, LCD_OP_SET, INT_DIV10 | CHAR_CONDENSED);
		lcd_set_cursor(left, top);
		left += spacing;
		lcd_write_int(1000 * g_chans[7] / scale, LCD_OP_SET, INT_DIV10 | CHAR_CONDENSED);

		if ((g_update_type & UPDATE_KEYPRESS) != 0)
		{
			if (g_key_press & KEY_RIGHT)
				gui_navigate(GUI_LAYOUT_MAIN4);
			else if (g_key_press & KEY_LEFT)
				gui_navigate(GUI_LAYOUT_MAIN2);
		}
	}
	break; // GUI_LAYOUT_MAIN3

		/**********************************************************************
		 * Main 4
		 *
		 * Displays model name, trim, battery and timer plus runtime timer
		 */
	case GUI_LAYOUT_MAIN4:

		gui_show_timer(TIMER_X, 40, g_model.tmrVal);

		if ((g_update_type & UPDATE_KEYPRESS) != 0)
		{
			if (g_key_press & KEY_RIGHT)
				gui_navigate(GUI_LAYOUT_MAIN1);
			else if (g_key_press & KEY_LEFT)
				gui_navigate(GUI_LAYOUT_MAIN3);
			// else if (g_key_press & (KEY_MENU | KEY_CANCEL))
			// 	timer_start_stop();
			// else if (g_key_press & (KEY_OK | KEY_SEL)) {
			// 	timer_restart();
			// }
		}
		break; // GUI_LAYOUT_MAIN4

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

		if ((g_update_type & UPDATE_KEYPRESS) != 0)
		{
			if (g_key_press & KEY_LEFT)
				item = 0;
			else if (g_key_press & KEY_RIGHT)
				item = 1;
			else if (g_key_press & (KEY_SEL | KEY_OK))
			{
				if (item)
					gui_navigate(GUI_LAYOUT_MODEL_MENU);
				else
					gui_navigate(GUI_LAYOUT_SYSTEM_MENU);
			}
			else if (g_key_press & KEY_CANCEL)
			{
				gui_navigate(g_main_layout);
			}
		}
		if (item == 0)
		{
			lcd_draw_rect(24, 57, 104, 58, LCD_OP_CLR, RECT_FILL);
			lcd_draw_rect(24, 57, 56, 58, LCD_OP_SET, RECT_FILL);
		}
		else
		{
			lcd_draw_rect(24, 57, 104, 58, LCD_OP_CLR, RECT_FILL);
			lcd_draw_rect(72, 57, 104, 58, LCD_OP_SET, RECT_FILL);
		}
	}
	break; // GUI_LAYOUT_MENU

		/**********************************************************************
		 * Common Menu key handling
		 */
	case GUI_LAYOUT_SYSTEM_MENU:
	case GUI_LAYOUT_MODEL_MENU:
#include "gui_menu.i"
		break;
		//case GUI_LAYOUT_SYSTEM_MENU GUI_LAYOUT_MODEL_MENU

	/**********************************************************************
		 * Calibration Screen
		 *
		 * Drives the sticks module into calibration mode and returns to
		 * Main 1 when complete or cancelled.
		 *
		 */
	case GUI_LAYOUT_STICK_CALIBRATION:
	{

		if (full && cal_state == CAL_OFF)
		{
			// Draw the whole screen.
			lcd_draw_message(msg[GUI_MSG_CAL_MOVE_EXTENTS], LCD_OP_SET, 0, 0);
			sticks_calibrate(CAL_LIMITS);
		}

		if ((g_update_type & UPDATE_STICKS) != 0)
		{
			gui_show_sticks();
		}

		if ((g_update_type & UPDATE_KEYPRESS) != 0)
		{
			lcd_set_cursor(5, 8);
			if ((g_key_press & KEY_SEL) || (g_key_press & KEY_OK))
			{
				lcd_draw_rect(5, 0, 123, BOX_Y - 1, LCD_OP_CLR, RECT_FILL);
				switch (cal_state)
				{
				case CAL_LIMITS:
					sticks_calibrate(CAL_CENTER);
					lcd_draw_message(msg[GUI_MSG_CAL_CENTRE], LCD_OP_SET, 0, 0);
					break;
				case CAL_CENTER:
					sticks_calibrate(CAL_OFF);
					lcd_draw_message(msg[GUI_MSG_OK_CANCEL], LCD_OP_SET, 0, 0);
					break;
				default:
					// Cal done, quit and let eeprom task write the data
					gui_navigate(GUI_LAYOUT_MAIN1);
				}
			}
			else if (g_key_press & KEY_CANCEL)
			{
				// ToDo: eeprom_get_data(EEPROM_ADC_CAL, cal_data);
				gui_navigate(GUI_LAYOUT_MAIN1);
			}
		}
		break;
	}
	} // switch (g_current_layout)

	g_key_press = KEY_NONE;

	lcd_update();
}
