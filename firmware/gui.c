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
#include "eeprom.h"
#include "art6.h"
#include "icons.h"
#include "sound.h"
#include "strings.h"

// Battery values.
#define BATT_MIN	99	//NiMh: 88
#define BATT_MAX	126	//NiMh: 104

// Message Popup
#define MSG_X	6
#define MSG_Y	8
#define MSG_H	46

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

#define LIST_ROWS	7

#define PAGE_LIMIT	((g_current_layout == GUI_LAYOUT_SYSTEM_MENU)?5:9)

static volatile GUI_LAYOUT g_new_layout = GUI_LAYOUT_NONE;
static GUI_LAYOUT g_current_layout = GUI_LAYOUT_SPLASH;
static volatile GUI_MSG g_new_msg = GUI_MSG_NONE;
static GUI_MSG g_current_msg = GUI_MSG_NONE;

static char g_popup_selected_line = 0;
static char g_popup_lines = 0;
#define GUI_POPUP_RESULT_NONE 0
#define GUI_POPUP_RESULT_CANCEL -1
static char g_popup_result = GUI_POPUP_RESULT_NONE;

static volatile uint32_t g_key_press = KEY_NONE;
static volatile uint32_t g_gui_timeout = 0;
static volatile uint8_t g_update_type = 0;

static GUI_LAYOUT g_main_layout = GUI_LAYOUT_MAIN1;
static MENU_MODE g_menu_mode = MENU_MODE_PAGE;
static int8_t g_menu_mode_dir = 1;

static void gui_show_sticks(void);
static void gui_show_switches(void);
static void gui_show_battery(int x, int y);
static void gui_show_timer(int x, int y);
static void gui_update_trim(void);
static void gui_draw_trim(int x, int y, bool h_v, int value);
static void gui_draw_slider(int x, int y, int w, int h, int range, int value);
static void gui_draw_stick_icon(STICK stick, uint8_t inverse);

static void gui_string_edit(char *string, int8_t delta, uint32_t keys);
static uint32_t gui_bitfield_edit(char *string, uint32_t field, int8_t delta,
		uint32_t keys, uint8_t edit);
static int32_t gui_int_edit(int32_t data, int32_t delta, int32_t min,
		int32_t max);

#define GUI_EDIT_INT_EX( VAR, MIN, MAX, UNITS, EDITACTION ) \
		if (edit) { VAR = gui_int_edit((int)VAR, inc, MIN, MAX); EDITACTION; } \
		lcd_write_int((int)VAR, op_item, FLAGS_NONE); \
		if(UNITS != 0) lcd_write_string(UNITS, op_item, FLAGS_NONE);

#define GUI_EDIT_INT( VAR, MIN, MAX ) GUI_EDIT_INT_EX(VAR, MIN, MAX, 0, {})

#define GUI_EDIT_ENUM( VAR, MIN, MAX, LABELS ) \
		if (edit) VAR = gui_int_edit(VAR, inc, MIN, MAX); \
		lcd_write_string((char*)LABELS[VAR], op_item, FLAGS_NONE);

#define GUI_EDIT_STR( VAR ) \
			prefill_string((char*)VAR, sizeof(VAR));\
			if (!edit) lcd_write_string((char*)VAR, LCD_OP_SET, FLAGS_NONE); \
			else       gui_string_edit((char*)VAR, inc, g_key_press);

#define GUI_CASE( CASE, COL, ACTION ) \
case CASE: \
	{ ACTION } \
	break; \

#define GUI_CASE_COL( CASE, COL, ACTION ) \
case CASE: \
	lcd_set_cursor(COL, line); \
	{ ACTION } \
	break; \

/**
 * @brief  Prefill strig with space up to length
 * @note
 * @param  str to fill
 * @param  len size of memory for string
 * @retval str
 */
static char* prefill_string(char* str, int len) {
	str[--len] = 0;
	while (--len >= 0) {
		if (str[len] == 0) {
			str[len] = ' ';
		}
	}
	return str;
}

/**
 * @brief  Initialise the GUI.
 * @note
 * @param  None
 * @retval None
 */
void gui_init(void) {
	task_register(TASK_PROCESS_GUI, gui_process);
	gui_navigate(GUI_LAYOUT_SPLASH);
}

/**
 * @brief  GUI task.
 * @note
 * @param  data: Update type (UPDATE_TYPE).
 * @retval None
 */
void gui_process(uint32_t data) {
	bool full = FALSE;

	// If we are currently displaying a popup,
	// check the time and schedule a re-check.
	// Also check keys to cancel the popup.
	if (g_current_msg) {
		char closeit = (g_gui_timeout != 0 && system_ticks >= g_gui_timeout);
		char update = g_gui_timeout != 0;
		if( g_update_type & UPDATE_KEYPRESS )
		{
			if( g_popup_selected_line && (g_key_press & (KEY_LEFT|KEY_RIGHT)) )
			{
				if( g_key_press & KEY_LEFT ) g_popup_selected_line--;
				if( g_key_press & KEY_RIGHT ) g_popup_selected_line++;
				if( g_popup_selected_line < 1 ) g_popup_selected_line = 1;
				if( g_popup_selected_line > g_popup_lines ) g_popup_selected_line = g_popup_lines;
				g_new_msg = g_current_msg;
				g_current_msg = 0;
				update = 1;
				g_key_press = KEY_NONE;
			}
			if ( (g_key_press & (KEY_OK | KEY_CANCEL | KEY_SEL)) )
			{
				closeit = 1;
				if( g_key_press & KEY_CANCEL )
					g_popup_result = GUI_POPUP_RESULT_CANCEL;
				else
					g_popup_result = g_popup_selected_line ? g_popup_selected_line : 1;
			}
		}
		if( closeit )
		{
			g_gui_timeout = 0;
			g_current_msg = 0;
			g_new_layout = g_current_layout;
		}
		else {
			task_schedule(TASK_PROCESS_GUI, UPDATE_MSG, 40);
			return;
		}
	}

	// Update common information on new layout.
	if (g_new_layout) {
		g_current_layout = g_new_layout;

		// Clear the screen.
		lcd_draw_rect(0, 0, LCD_WIDTH - 1, LCD_HEIGHT - 1, LCD_OP_CLR,
				RECT_FILL);
		lcd_set_cursor(0, 0);

		if (g_current_layout >= GUI_LAYOUT_MAIN1
				&& g_current_layout <= GUI_LAYOUT_MAIN4) {
			// Trim
			gui_update_trim();

			// Update Battery icon
			gui_show_battery(83, 0);

			// Update the timer
			gui_show_timer(39, 17);

			// Model Name
			lcd_set_cursor(8, 0);
			lcd_write_string((char*) g_model.name, LCD_OP_SET, CHAR_2X);
		}

		full = TRUE;
		g_new_layout = GUI_LAYOUT_NONE;
	}

	// If we have a new popup to display, do it now.
	if (g_new_msg) {
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
	if (!full && g_current_layout >= GUI_LAYOUT_MAIN1
			&& g_current_layout <= GUI_LAYOUT_MAIN4) {
		// Update the trim if needed.
		if ((g_key_press & TRIM_KEYS) != 0) {
			mixer_input_trim(g_key_press);
			gui_update_trim();
		} else if (g_key_press & KEY_MENU) {
			// Long press menu key handling.
			g_main_layout = g_current_layout;
			gui_navigate(GUI_LAYOUT_MENU);
		}

		// Update the battery level
		if ((g_update_type & UPDATE_STICKS) != 0) {
			gui_show_battery(83, 0);
		}

		// Update the timer
		if ((g_update_type & UPDATE_TIMER) != 0) {
			gui_show_timer(39, 17);
		}
	}

	switch (g_current_layout) {
	case GUI_LAYOUT_NONE:
		// Something went badly wrong.
		break; // GUI_LAYOUT_NONE

	case GUI_LAYOUT_SPLASH:
		break; // GUI_LAYOUT_SPLASH

		/**********************************************************************
		 * Main 1
		 *
		 * Displays model name, trim, battery and timer plus a graphical
		 * representation of the stick and pot postitions and key states.
		 *
		 */
	case GUI_LAYOUT_MAIN1:
		if ((g_update_type & UPDATE_STICKS) != 0) {
			gui_show_sticks();
			gui_show_switches();
		}

		if ((g_update_type & UPDATE_KEYPRESS) != 0) {
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
		if ((g_update_type & UPDATE_STICKS) != 0) {
			const int top = 40;
			int scale =
					(g_model.extendedLimits == TRUE) ?
							PPM_LIMIT_EXTENDED : PPM_LIMIT_NORMAL;

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

		if ((g_update_type & UPDATE_KEYPRESS) != 0) {
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
	case GUI_LAYOUT_MAIN3: {
		int top = 40;
		int left = 12;
		const int spacing = 28;
		int scale =
				(g_model.extendedLimits == TRUE) ?
						PPM_LIMIT_EXTENDED : PPM_LIMIT_NORMAL;

		lcd_draw_rect(left, top, left + spacing * 4 - 4, top + 16, LCD_OP_CLR,
				RECT_FILL);

		lcd_set_cursor(left, top);
		left += spacing;
		lcd_write_int(1000 * g_chans[0] / scale, LCD_OP_SET,
				INT_DIV10 | CHAR_CONDENSED);
		lcd_set_cursor(left, top);
		left += spacing;
		lcd_write_int(1000 * g_chans[1] / scale, LCD_OP_SET,
				INT_DIV10 | CHAR_CONDENSED);
		lcd_set_cursor(left, top);
		left += spacing;
		lcd_write_int(1000 * g_chans[2] / scale, LCD_OP_SET,
				INT_DIV10 | CHAR_CONDENSED);
		lcd_set_cursor(left, top);
		left += spacing;
		lcd_write_int(1000 * g_chans[3] / scale, LCD_OP_SET,
				INT_DIV10 | CHAR_CONDENSED);

		top += 8;
		left = 12;
		lcd_set_cursor(left, top);
		left += spacing;
		lcd_write_int(1000 * g_chans[4] / scale, LCD_OP_SET,
				INT_DIV10 | CHAR_CONDENSED);
		lcd_set_cursor(left, top);
		left += spacing;
		lcd_write_int(1000 * g_chans[5] / scale, LCD_OP_SET,
				INT_DIV10 | CHAR_CONDENSED);
		lcd_set_cursor(left, top);
		left += spacing;
		lcd_write_int(1000 * g_chans[6] / scale, LCD_OP_SET,
				INT_DIV10 | CHAR_CONDENSED);
		lcd_set_cursor(left, top);
		left += spacing;
		lcd_write_int(1000 * g_chans[7] / scale, LCD_OP_SET,
				INT_DIV10 | CHAR_CONDENSED);

		if ((g_update_type & UPDATE_KEYPRESS) != 0) {
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
	case GUI_LAYOUT_MAIN4: {
		static uint32_t timer_tick = 0;
		static uint16_t timer = 0;

		if (timer_tick != 0) {
			if (system_ticks >= timer_tick + 1000) {
				timer++;
				timer_tick = system_ticks;
			}
		}

		lcd_set_cursor(37, 40);
		lcd_write_int(timer / 60, LCD_OP_SET, INT_PAD10 | CHAR_4X);
		lcd_write_char(':', LCD_OP_SET, CHAR_4X);
		lcd_write_int(timer % 60, LCD_OP_SET, INT_PAD10 | CHAR_4X);

		if ((g_update_type & UPDATE_KEYPRESS) != 0) {
			if (g_key_press & KEY_RIGHT)
				gui_navigate(GUI_LAYOUT_MAIN1);
			else if (g_key_press & KEY_LEFT)
				gui_navigate(GUI_LAYOUT_MAIN3);
			else if (g_key_press & (KEY_MENU | KEY_CANCEL))
				timer = 0;
			else if (g_key_press & (KEY_OK | KEY_SEL)) {
				if (timer_tick == 0)
					timer_tick = system_ticks;
				else
					timer_tick = 0;
			}
		}
	}
		break; // GUI_LAYOUT_MAIN4

		/**********************************************************************
		 * Menu to select System or Model Menus
		 */
	case GUI_LAYOUT_MENU: {
		static uint8_t item = 0;

		if (full) {
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

		if ((g_update_type & UPDATE_KEYPRESS) != 0) {
			if (g_key_press & KEY_LEFT)
				item = 0;
			else if (g_key_press & KEY_RIGHT)
				item = 1;
			else if (g_key_press & (KEY_SEL | KEY_OK)) {
				if (item)
					gui_navigate(GUI_LAYOUT_MODEL_MENU);
				else
					gui_navigate(GUI_LAYOUT_SYSTEM_MENU);
			} else if (g_key_press & KEY_CANCEL) {
				gui_navigate(g_main_layout);
			}
		}
		if (item == 0) {
			lcd_draw_rect(24, 57, 104, 58, LCD_OP_CLR, RECT_FILL);
			lcd_draw_rect(24, 57, 56, 58, LCD_OP_SET, RECT_FILL);
		} else {
			lcd_draw_rect(24, 57, 104, 58, LCD_OP_CLR, RECT_FILL);
			lcd_draw_rect(72, 57, 104, 58, LCD_OP_SET, RECT_FILL);
		}
	}
		break; // GUI_LAYOUT_MENU

		/**********************************************************************
		 * Common Menu key handling
		 */
	case GUI_LAYOUT_SYSTEM_MENU:
	case GUI_LAYOUT_MODEL_MENU: {
		static uint8_t page = 0;
		static uint8_t list = 0;
		static uint8_t list_top = 0;
		static uint8_t list_limit = 0;
		uint8_t i;
		int8_t inc = 0;

		// Clear the screen.
		lcd_draw_rect(0, 0, LCD_WIDTH - 1, LCD_HEIGHT - 1, LCD_OP_CLR,
				      RECT_FILL);
		lcd_set_cursor(0, 0);

		if (g_key_press & KEY_LEFT) {
			if ((g_menu_mode == MENU_MODE_PAGE) && (page > 0)) {
				page--;
				list = 0;
				list_top = 0;
			} else if (g_menu_mode == MENU_MODE_LIST) {
				if (list > 0) {
					g_menu_mode_dir = 1;
					list--;
				}
				//else
				//g_mode = MENU_MODE_PAGE;
			} else
				inc = -1;
		} else if (g_key_press & KEY_RIGHT) {
			if ((g_menu_mode == MENU_MODE_PAGE) && (page < PAGE_LIMIT)) {
				page++;
				list = 0;
				list_top = 0;
			} else if ((g_menu_mode == MENU_MODE_LIST) && (list < list_limit)) {
				g_menu_mode_dir = 1;
				list++;
			} else
				inc = 1;
		} else if (g_key_press & (KEY_SEL | KEY_OK)) {
			switch (g_menu_mode) {
			case MENU_MODE_PAGE:
				g_menu_mode = MENU_MODE_LIST;
				g_menu_mode_dir = 1;
				break;
			case MENU_MODE_LIST:
				g_menu_mode += g_menu_mode_dir;
				break;
			case MENU_MODE_EDIT:
				g_menu_mode--;
				g_menu_mode_dir = -1;
				break;
			default:
				break;
			}
		} else if (g_key_press & KEY_CANCEL) {
			if (g_menu_mode > 0)
				g_menu_mode--;
			else {
				page = 0;
				list = 0;
				list_top = 0;
				gui_navigate(g_main_layout);
			}
		}

		if (list < list_top)
			list_top = list;
		if (list >= list_top + LIST_ROWS)
			list_top++;

		if (g_current_layout == GUI_LAYOUT_SYSTEM_MENU) {
			/**********************************************************************
			 * System Menu
			 *
			 * This is the main system menu with 6 pages.
			 *
			 */

			lcd_write_string((char*) msg[GUI_HDG_RADIO_SETUP + page],
					LCD_OP_CLR, FLAGS_NONE);
			lcd_set_cursor(110, 0);
			lcd_write_int(page + 1,
					(g_menu_mode == MENU_MODE_PAGE) ? LCD_OP_CLR : LCD_OP_SET,
					FLAGS_NONE);
			lcd_write_string("/6",
					(g_menu_mode == MENU_MODE_PAGE) ? LCD_OP_CLR : LCD_OP_SET,
					FLAGS_NONE);

			/**********************************************************************
			 * System Menu Pages
			 */
			switch (page) {
			case SYS_PAGE_SETUP:
				list_limit = SYS_MENU_LIST1_LEN - 1;
				for (i = list_top;
						(i < list_top + LIST_ROWS) && (i <= list_limit); ++i) {
					const uint8_t line = (i - list_top + 1) * 8;
					LCD_OP op_list = LCD_OP_SET;
					LCD_OP op_item = LCD_OP_SET;
					uint8_t edit = 0;
					if ((g_menu_mode == MENU_MODE_LIST) && (i == list)) {
						op_list = LCD_OP_CLR;
					} else if ((g_menu_mode == MENU_MODE_EDIT
							|| g_menu_mode == MENU_MODE_EDIT_S)
							&& (i == list)) {
						edit = 1;
						op_item = LCD_OP_CLR;
					}

					lcd_set_cursor(0, line);
					lcd_write_string((char*) system_menu_list1[i], op_list,
							FLAGS_NONE);
					lcd_write_string(" ", LCD_OP_SET, FLAGS_NONE);
					switch (i) {
					GUI_CASE_COL(0, 74, GUI_EDIT_STR(g_eeGeneral.ownerName))
					GUI_CASE_COL(1, 92,
							GUI_EDIT_ENUM(g_eeGeneral.beeperVal, BEEPER_SILENT, BEEPER_NORMAL, system_menu_beeper ))
					GUI_CASE_COL(2, 110,
							GUI_EDIT_INT_EX( g_eeGeneral.volume, 0, 15, NULL, sound_set_volume(g_eeGeneral.volume) ))
					GUI_CASE_COL(3, 110,
							GUI_EDIT_INT_EX( g_eeGeneral.contrast, LCD_CONTRAST_MIN, LCD_CONTRAST_MAX, NULL, lcd_set_contrast(g_eeGeneral.contrast) ))
					GUI_CASE_COL(4, 102,
							GUI_EDIT_INT_EX( g_eeGeneral.vBatWarn, BATT_MIN, BATT_MAX, "V", {} ))
					case 5:	// Inactivity Timer
						lcd_set_cursor(110, line);
						if (edit)
							g_eeGeneral.inactivityTimer = gui_int_edit(
									g_eeGeneral.inactivityTimer, inc, 0, 250);
						lcd_write_int(g_eeGeneral.inactivityTimer, op_item,
								FLAGS_NONE);
						lcd_write_char('m', op_item, FLAGS_NONE);
						break;
					case 6:	// Throttle Reverse
						lcd_set_cursor(110, line);
						if (edit)
							g_eeGeneral.throttleReversed = gui_int_edit(
									g_eeGeneral.throttleReversed, inc, 0, 1);
						lcd_write_string(
								(char*) menu_on_off[g_eeGeneral.throttleReversed],
								op_item, FLAGS_NONE);
						break;
					case 7:	// Minute Beep
						lcd_set_cursor(110, line);
						if (edit)
							g_eeGeneral.minuteBeep = gui_int_edit(
									g_eeGeneral.minuteBeep, inc, 0, 1);
						lcd_write_string(
								(char*) menu_on_off[g_eeGeneral.minuteBeep],
								op_item, FLAGS_NONE);
						break;
					case 8:	// Beep Countdown
						lcd_set_cursor(110, line);
						if (edit)
							g_eeGeneral.preBeep = gui_int_edit(
									g_eeGeneral.preBeep, inc, 0, 1);
						lcd_write_string(
								(char*) menu_on_off[g_eeGeneral.preBeep],
								op_item, FLAGS_NONE);
						break;
					case 9:	// Flash on beep
						lcd_set_cursor(110, line);
						if (edit)
							g_eeGeneral.flashBeep = gui_int_edit(
									g_eeGeneral.flashBeep, inc, 0, 1);
						lcd_write_string(
								(char*) menu_on_off[g_eeGeneral.flashBeep],
								op_item, FLAGS_NONE);
						break;
					case 10: // Light Switch
					{
						int8_t sw = g_eeGeneral.lightSw;
						lcd_set_cursor(104, line);
						if (edit) {
							g_eeGeneral.lightSw = gui_int_edit(
									g_eeGeneral.lightSw, inc, -NUM_SWITCHES,
									NUM_SWITCHES);
						}
						if (sw < 0) {
							sw = -sw;
							lcd_write_string("!", op_item, FLAGS_NONE);
						}
						lcd_write_string((char*) switches[sw], op_item,
								FLAGS_NONE);
					}
						break;
					case 11: // Backlight invert
						lcd_set_cursor(110, line);
						if (edit)
							g_eeGeneral.blightinv = gui_int_edit(
									g_eeGeneral.blightinv, inc, 0, 1);
						lcd_write_string(
								(char*) menu_on_off[g_eeGeneral.blightinv],
								op_item, FLAGS_NONE);
						break;
					case 12: // Light timeout
						lcd_set_cursor(104, line);
						if (edit)
							g_eeGeneral.lightAutoOff = gui_int_edit(
									g_eeGeneral.lightAutoOff, inc, 0, 255);
						lcd_write_int(g_eeGeneral.lightAutoOff, op_item,
								FLAGS_NONE);
						lcd_write_char('s', op_item, FLAGS_NONE);
						break;
					case 13: // Light on stick move
						lcd_set_cursor(110, line);
						if (edit)
							g_eeGeneral.lightOnStickMove = gui_int_edit(
									g_eeGeneral.lightOnStickMove, inc, 0, 1);
						lcd_write_string(
								(char*) menu_on_off[g_eeGeneral.lightOnStickMove],
								op_item, FLAGS_NONE);
						break;
					case 14: // Splash screen
						lcd_set_cursor(110, line);
						if (edit)
							g_eeGeneral.disableSplashScreen = gui_int_edit(
									g_eeGeneral.disableSplashScreen, inc, 0, 1);
						lcd_write_string(
								(char*) menu_on_off[1
										- g_eeGeneral.disableSplashScreen],
								op_item, FLAGS_NONE);
						break;
					case 15: // Throttle Warning
						lcd_set_cursor(110, line);
						if (edit)
							g_eeGeneral.disableThrottleWarning = gui_int_edit(
									g_eeGeneral.disableThrottleWarning, inc, 0,
									1);
						lcd_write_string(
								(char*) menu_on_off[1
										- g_eeGeneral.disableThrottleWarning],
								op_item, FLAGS_NONE);
						break;
					case 16: // Switch Warning
						lcd_set_cursor(110, line);
						if (edit)
							g_eeGeneral.disableSwitchWarning = gui_int_edit(
									g_eeGeneral.disableSwitchWarning, inc, 0,
									1);
						lcd_write_string(
								(char*) menu_on_off[1
										- g_eeGeneral.disableSwitchWarning],
								op_item, FLAGS_NONE);
						break;
					case 17: // Default Sw
						lcd_set_cursor(104, line);
						g_eeGeneral.switchWarningStates = gui_bitfield_edit(
								"ABCD", g_eeGeneral.switchWarningStates, inc,
								g_key_press, edit);
						break;
					case 18: // Memory Warning
						lcd_set_cursor(110, line);
						if (edit)
							g_eeGeneral.disableMemoryWarning = gui_int_edit(
									g_eeGeneral.disableMemoryWarning, inc, 0,
									1);
						lcd_write_string(
								(char*) menu_on_off[1
										- g_eeGeneral.disableMemoryWarning],
								op_item, FLAGS_NONE);
						break;
					case 19: // Alarm Warning
						lcd_set_cursor(110, line);
						if (edit)
							g_eeGeneral.disableAlarmWarning = gui_int_edit(
									g_eeGeneral.disableAlarmWarning, inc, 0, 1);
						lcd_write_string(
								(char*) menu_on_off[1
										- g_eeGeneral.disableAlarmWarning],
								op_item, FLAGS_NONE);
						break;
					case 20: // PPSIM
						lcd_set_cursor(110, line);
						if (edit)
							g_eeGeneral.enablePpmsim = gui_int_edit(
									g_eeGeneral.enablePpmsim, inc, 0, 1);
						lcd_write_string(
								(char*) menu_on_off[g_eeGeneral.enablePpmsim],
								op_item, FLAGS_NONE);
						break;
					case 21: // Channel Order & Mode
					{
						int j;
						for (j = STICK_R_H; j <= STICK_L_H; ++j) {
							gui_draw_stick_icon(j, j == g_eeGeneral.stickMode);
							lcd_write_char(' ', LCD_OP_SET, FLAGS_NONE);
						}
					}
						lcd_set_cursor(110, line);
						GUI_EDIT_INT(g_eeGeneral.stickMode, CHAN_ORDER_ATER,
								CHAN_ORDER_RETA)
						;
						break;
						/*
						 case 22: // Channel Order & Mode
						 if (edit)
						 g_eeGeneral.stickMode = gui_int_edit(g_eeGeneral.stickMode, inc, CHAN_ORDER_ATER, CHAN_ORDER_RETA);
						 lcd_write_int(g_eeGeneral.stickMode + 1, op_item, FLAGS_NONE);
						 lcd_write_string("   ", LCD_OP_SET, FLAGS_NONE);
						 break;
						 */
					}
				}
				break; // SYS_PAGE_SETUP

			case SYS_PAGE_TRAINER:
				list_limit = 6;
				for (i = list_top;
						(i < list_top + LIST_ROWS) && (i <= list_limit); ++i) {
					LCD_OP op_list = LCD_OP_SET;
					LCD_OP op_item = LCD_OP_SET;
					uint8_t edit = 0;
					static int8_t col = 0;
					static uint8_t col_edit = 0;

					// First row isn't editable
					if (list == 0)
						list = 1;

					if ((g_menu_mode == MENU_MODE_LIST) && (i == list)) {
						op_list = LCD_OP_CLR;
					} else if (g_menu_mode == MENU_MODE_EDIT
							|| g_menu_mode == MENU_MODE_EDIT_S) {
						if (i == list) {
							edit = 1;
							op_item = LCD_OP_CLR;
						}

						if (g_menu_mode == MENU_MODE_EDIT && (list < 5)) {
							g_menu_mode = MENU_MODE_EDIT_S;
							col = 0;
							col_edit = 1;
						} else if (g_menu_mode == MENU_MODE_EDIT_S) {
							// We do our own navigation for this one.

							if (g_key_press & (KEY_SEL | KEY_OK)) {
								col_edit = 1 - col_edit;
							} else if (g_key_press & (KEY_CANCEL)) {
								col = 0;
								col_edit = 0;
								g_menu_mode = MENU_MODE_LIST;
							}

							if (edit && col_edit == 0) {
								col += inc;
								if (col > 3)
									col = 3;
								if (col < 0)
									col = 0;
							}
						}
					}

					lcd_set_cursor(0, (i - list_top + 1) * 8);

					switch (i) {
					case 0:
						lcd_set_cursor(24, (i - list_top + 1) * 8);
						lcd_write_string((char*) mix_mode_hdr, LCD_OP_SET,
								FLAGS_NONE);
						lcd_set_cursor(66, (i - list_top + 1) * 8);
						lcd_write_string("% src sw", LCD_OP_SET, FLAGS_NONE);
						break;
					case 1:
					case 2:
					case 3:
					case 4:
						// Mode
						if (edit && col_edit && col == 0)
							g_eeGeneral.trainer.mix[i - 1].mode = gui_int_edit(
									g_eeGeneral.trainer.mix[i - 1].mode, inc, 0,
									3);
						lcd_write_string((char*) sticks[i - 1], op_list,
								FLAGS_NONE);
						lcd_write_string(" ", LCD_OP_SET, FLAGS_NONE);
						lcd_write_string(
								(char*) mix_mode[g_eeGeneral.trainer.mix[i - 1].mode],
								(edit && col == 0) ? LCD_OP_CLR : LCD_OP_SET,
								FLAGS_NONE);

						// Amount
						if (edit && col_edit && col == 1)
							g_eeGeneral.trainer.mix[i - 1].studWeight =
									gui_int_edit(
											g_eeGeneral.trainer.mix[i - 1].studWeight,
											inc, -100, 100);
						lcd_set_cursor(66, (i - list_top + 1) * 8);
						lcd_write_int(g_eeGeneral.trainer.mix[i - 1].studWeight,
								(edit && col == 1) ? LCD_OP_CLR : LCD_OP_SET,
								ALIGN_RIGHT);

						// Source
						if (edit && col_edit && col == 2)
							g_eeGeneral.trainer.mix[i - 1].srcChn =
									gui_int_edit(
											g_eeGeneral.trainer.mix[i - 1].srcChn,
											inc, 0, 7);
						lcd_set_cursor(78, (i - list_top + 1) * 8);
						lcd_write_string("ch", LCD_OP_SET, FLAGS_NONE);
						lcd_write_int(g_eeGeneral.trainer.mix[i - 1].srcChn + 1,
								(edit && col == 2) ? LCD_OP_CLR : LCD_OP_SET,
								FLAGS_NONE);

						// Switch
						if (edit && col_edit && col == 3)
							g_eeGeneral.trainer.mix[i - 1].swtch = gui_int_edit(
									g_eeGeneral.trainer.mix[i - 1].swtch, inc,
									-NUM_SWITCHES, NUM_SWITCHES);
						{
							int8_t sw = g_eeGeneral.trainer.mix[i - 1].swtch;
							lcd_set_cursor(102, (i - list_top + 1) * 8);
							if (sw < 0) {
								lcd_write_char('!', LCD_OP_SET, FLAGS_NONE);
								sw = -sw;
							}
							lcd_write_string((char*) switches[sw],
									(edit && col == 3) ?
											LCD_OP_CLR : LCD_OP_SET,
									FLAGS_NONE);
						}
						break;
					case 5:
						lcd_write_string("Multiplier ", op_list, FLAGS_NONE);
						if (edit)
							g_eeGeneral.PPM_Multiplier = gui_int_edit(
									g_eeGeneral.PPM_Multiplier, inc, 10, 50);
						lcd_write_int(g_eeGeneral.PPM_Multiplier, op_item,
								INT_DIV10);
						break;
					case 6: {
						int j;
						lcd_write_string("Cal", op_list, FLAGS_NONE);
						for (j = 0; j < 4; ++j) {
							lcd_set_cursor(43 + j * 25, (i - list_top + 1) * 8);
							lcd_write_int(
									g_ppmIns[j] - g_eeGeneral.trainer.calib[j],
									LCD_OP_SET, ALIGN_RIGHT | CHAR_CONDENSED);
						}

						if (edit && (g_key_press & (KEY_OK | KEY_SEL))) {
							for (j = 0; j < 4; ++j) {
								g_eeGeneral.trainer.calib[j] = g_ppmIns[j];
							}
							g_menu_mode = MENU_MODE_LIST;
						}
					}
						break;
					}
				}
				break; // SYS_PAGE_TRAINER

			case SYS_PAGE_VERSION:
				lcd_set_cursor(30, 2 * 8);
				lcd_write_string("Vers:", LCD_OP_SET, ALIGN_RIGHT);
				lcd_set_cursor(36, 2 * 8);
				lcd_write_int(VERSION_MAJOR, LCD_OP_SET, INT_PAD10);
				lcd_write_char('.', LCD_OP_SET, ALIGN_RIGHT);
				lcd_write_int(VERSION_MINOR, LCD_OP_SET, INT_PAD10);
				lcd_write_char('.', LCD_OP_SET, ALIGN_RIGHT);
				lcd_write_int(VERSION_PATCH, LCD_OP_SET, INT_PAD10);

				lcd_set_cursor(30, 3 * 8);
				lcd_write_string("Date:", LCD_OP_SET, ALIGN_RIGHT);
				lcd_set_cursor(36, 3 * 8);
				lcd_write_string(__DATE__, LCD_OP_SET, FLAGS_NONE);

				lcd_set_cursor(30, 4 * 8);
				lcd_write_string("Time:", LCD_OP_SET, ALIGN_RIGHT);
				lcd_set_cursor(36, 4 * 8);
				lcd_write_string(__TIME__, LCD_OP_SET, FLAGS_NONE);

				lcd_set_cursor(30, 6 * 8);
				lcd_write_string("eMEM:", LCD_OP_SET, ALIGN_RIGHT);
				lcd_set_cursor(36, 6 * 8);
				lcd_write_int(sizeof(g_eeGeneral), LCD_OP_SET, INT_PAD10);
				lcd_set_cursor(50, 6 * 8);
				lcd_write_int(sizeof(g_model), LCD_OP_SET, INT_PAD10);

				break; // SYS_PAGE_VERSION

			case SYS_PAGE_DIAG: {
				uint8_t sw = keypad_get_switches();
				for (i = 0; i < NUM_SWITCHES; ++i) {
					lcd_set_cursor(6 * 6, (2 + i) * 8);
					lcd_write_string((char*) switches[i + 1], LCD_OP_SET,
							FLAGS_NONE);
					lcd_write_char(' ', LCD_OP_SET, FLAGS_NONE);
					lcd_write_int((sw & (1 << i)) ? 1 : 0, LCD_OP_SET,
							FLAGS_NONE);
				}

				i = 0;
				lcd_set_cursor(3 * 6, (2 + i) * 8);
				lcd_write_string("Sel", LCD_OP_SET, ALIGN_RIGHT);
				lcd_set_cursor(4 * 6, (2 + i) * 8);
				lcd_write_int((keypad_scan_keys() == KEY_SEL) ? 1 : 0,
						LCD_OP_SET, FLAGS_NONE);
				i++;
				lcd_set_cursor(3 * 6, (2 + i) * 8);
				lcd_write_string("OK", LCD_OP_SET, ALIGN_RIGHT);
				lcd_set_cursor(4 * 6, (2 + i) * 8);
				lcd_write_int((keypad_scan_keys() == KEY_OK) ? 1 : 0,
						LCD_OP_SET, FLAGS_NONE);
				i++;
				lcd_set_cursor(3 * 6, (2 + i) * 8);
				lcd_write_string("Can", LCD_OP_SET, ALIGN_RIGHT);
				lcd_set_cursor(4 * 6, (2 + i) * 8);
				lcd_write_int((keypad_scan_keys() == KEY_CANCEL) ? 1 : 0,
						LCD_OP_SET, FLAGS_NONE);
				i++;

				i = 0;
				lcd_set_cursor(12 * 6, (2 + i) * 8);
				lcd_write_string("Trim - +", LCD_OP_SET, FLAGS_NONE);
				i++;
				lcd_set_cursor(12 * 6, (2 + i) * 8);
				lcd_write_string("\x0A\x0B\x0C ", LCD_OP_SET, CHAR_NOSPACE);
				lcd_set_cursor(17 * 6, (2 + i) * 8);
				lcd_write_int((keypad_scan_keys() == KEY_CH1_DN) ? 1 : 0,
						LCD_OP_SET, FLAGS_NONE);
				lcd_write_char(' ', LCD_OP_SET, FLAGS_NONE);
				lcd_write_int((keypad_scan_keys() == KEY_CH1_UP) ? 1 : 0,
						LCD_OP_SET, FLAGS_NONE);
				i++;
				lcd_set_cursor(12 * 6, (2 + i) * 8);
				lcd_write_string("\x0D\x0E\x0F ", LCD_OP_SET, CHAR_NOSPACE);
				lcd_set_cursor(17 * 6, (2 + i) * 8);
				lcd_write_int((keypad_scan_keys() == KEY_CH2_DN) ? 1 : 0,
						LCD_OP_SET, FLAGS_NONE);
				lcd_write_char(' ', LCD_OP_SET, FLAGS_NONE);
				lcd_write_int((keypad_scan_keys() == KEY_CH2_UP) ? 1 : 0,
						LCD_OP_SET, FLAGS_NONE);
				i++;
				lcd_set_cursor(12 * 6, (2 + i) * 8);
				lcd_write_string("\x10\x11\x12 ", LCD_OP_SET, CHAR_NOSPACE);
				lcd_set_cursor(17 * 6, (2 + i) * 8);
				lcd_write_int((keypad_scan_keys() == KEY_CH3_DN) ? 1 : 0,
						LCD_OP_SET, FLAGS_NONE);
				lcd_write_char(' ', LCD_OP_SET, FLAGS_NONE);
				lcd_write_int((keypad_scan_keys() == KEY_CH3_UP) ? 1 : 0,
						LCD_OP_SET, FLAGS_NONE);
				i++;
				lcd_set_cursor(12 * 6, (2 + i) * 8);
				lcd_write_string("\x13\x14\x15 ", LCD_OP_SET, CHAR_NOSPACE);
				lcd_set_cursor(17 * 6, (2 + i) * 8);
				lcd_write_int((keypad_scan_keys() == KEY_CH4_DN) ? 1 : 0,
						LCD_OP_SET, FLAGS_NONE);
				lcd_write_char(' ', LCD_OP_SET, FLAGS_NONE);
				lcd_write_int((keypad_scan_keys() == KEY_CH4_UP) ? 1 : 0,
						LCD_OP_SET, FLAGS_NONE);
				i++;
			}
				break; // SYS_PAGE_DIAG

			case SYS_PAGE_ANA: {
				LCD_OP op_list = LCD_OP_SET;
				list_limit = 1;

				if (g_menu_mode == MENU_MODE_EDIT
						|| g_menu_mode == MENU_MODE_LIST) {
					g_eeGeneral.vBatCalib = gui_int_edit(g_eeGeneral.vBatCalib,
							inc, 80, 120);
					op_list = LCD_OP_CLR;
				}

				for (i = 0; i < STICK_ADC_CHANNELS; ++i) {
					lcd_set_cursor(24, (i + 1) * 8);
					lcd_write_char('A', LCD_OP_SET, FLAGS_NONE);
					lcd_write_int(i, LCD_OP_SET, FLAGS_NONE);
					lcd_write_char(' ', LCD_OP_SET, FLAGS_NONE);
					lcd_write_hex(adc_data[i], LCD_OP_SET, FLAGS_NONE);
					lcd_write_string("  ", LCD_OP_SET, FLAGS_NONE);
					// All but battery
					if (i != 6) {
						lcd_write_int(sticks_get_percent(i), LCD_OP_SET,
								FLAGS_NONE);
					} else {
						lcd_write_int(sticks_get_battery(), op_list, INT_DIV10);
						lcd_write_string("v", op_list, FLAGS_NONE);
					}

				}
			}
				break; // SYS_PAGE_ANA

			case SYS_PAGE_CAL:
				lcd_set_cursor(5, 16);
				lcd_draw_message(msg[GUI_MSG_CAL_OK_START], LCD_OP_SET, 0, 0);

				if (g_update_type & UPDATE_STICKS) {
					gui_show_sticks();
				}

				if (g_key_press & KEY_OK) {
					page = 0;
					list = 0;
					list_top = 0;
					gui_navigate(GUI_LAYOUT_STICK_CALIBRATION);
				}
				break; // SYS_PAGE_CAL
			}
		}

		else // GUI_LAYOUT_MODEL_MENU
		{
			/**********************************************************************
			 * Model Menu
			 *
			 * This is the model editing menu with 10 pages.
			 *
			 */

			lcd_write_string((char*) msg[GUI_HDG_MODELSEL + page], LCD_OP_CLR,
					FLAGS_NONE);
			if (page == MOD_PAGE_SETUP)
				lcd_write_int(g_eeGeneral.currModel, LCD_OP_CLR, FLAGS_NONE);
			lcd_set_cursor(104, 0);
			lcd_write_int(page + 1,
					(g_menu_mode == MENU_MODE_PAGE) ? LCD_OP_CLR : LCD_OP_SET,
					ALIGN_RIGHT);
			lcd_set_cursor(110, 0);
			lcd_write_string("/10",
					(g_menu_mode == MENU_MODE_PAGE) ? LCD_OP_CLR : LCD_OP_SET,
					FLAGS_NONE);

			/**********************************************************************
			 * System Menu Pages
			 */
			switch (page) {
			case MOD_PAGE_SELECT: {
				list_limit = MAX_MODELS - 1;
				for (i = list_top;
						(i < list_top + LIST_ROWS) && (i <= list_limit); ++i)
				{
					const uint8_t line = (i - list_top + 1) * 8;
					LCD_OP op_list = LCD_OP_SET;
					if ((g_menu_mode == MENU_MODE_LIST) && (i == list)) {
						op_list = LCD_OP_CLR;
					}
					lcd_set_cursor(0, line);
					lcd_write_string(g_eeGeneral.currModel == i ? "* " : "  ",
							op_list, FLAGS_NONE);
					char name[MODEL_NAME_LEN];
					name[0] = '0' + i / 10;
					name[1] = '0' + i % 10;
					name[2] = ' ';
					name[3] = 0;
					lcd_write_string(name, op_list, FLAGS_NONE);
					eeprom_read_model_name(i, name);
					lcd_write_string(name, op_list, FLAGS_NONE);
					lcd_write_string(" ", LCD_OP_SET, FLAGS_NONE);
				}
				// check if popup was finished, if so the result would show up
				char popupRes = gui_popup_get_result();
				if( popupRes )
				{
					// "preset" popup was answered "OK" so preset the model
					if( popupRes > 0 )
					{
						g_eeGeneral.currModel = list;
						eeprom_init_current_model();
					}
				}
				else
				{
					if( g_key_press & KEY_MENU )
					{
						g_eeGeneral.currModel = list;
						gui_popup(GUI_MSG_OK_TO_RESET_MODEL,0);
					}
					else if( g_menu_mode == MENU_MODE_EDIT )
					{
						// select the model to use
						if( g_key_press & (KEY_SEL|KEY_OK) )
						{
							g_eeGeneral.currModel = list;
							g_menu_mode = MENU_MODE_PAGE;
							gui_navigate(GUI_LAYOUT_MODEL_MENU);
						}
					}
				}
			}
			break;

			case MOD_PAGE_SETUP:
				list_limit = MOD_MENU_LIST1_LEN - 1;
				for (i = list_top;
						(i < list_top + LIST_ROWS) && (i <= list_limit); ++i) {
					const uint8_t line = (i - list_top + 1) * 8;
					LCD_OP op_list = LCD_OP_SET;
					LCD_OP op_item = LCD_OP_SET;
					uint8_t edit = 0;
					if ((g_menu_mode == MENU_MODE_LIST) && (i == list)) {
						op_list = LCD_OP_CLR;
					} else if ((g_menu_mode == MENU_MODE_EDIT
							|| g_menu_mode == MENU_MODE_EDIT_S)
							&& (i == list)) {
						edit = 1;
						op_item = LCD_OP_CLR;
					}

					lcd_set_cursor(0, line);
					lcd_write_string((char*) model_menu_list1[i], op_list,
							FLAGS_NONE);
					lcd_write_string(" ", LCD_OP_SET, FLAGS_NONE);
					switch (i) {
					GUI_CASE_COL(0, 74, GUI_EDIT_STR(g_model.name))
					GUI_CASE_COL(1, 96,
							GUI_EDIT_ENUM( g_model.tmrMode, 0, 5, timer_modes ))
					GUI_CASE_COL(2, 96,
							GUI_EDIT_ENUM( g_model.tmrDir, 0, 1, dir_labels ))
					GUI_CASE_COL(3, 96, GUI_EDIT_INT( g_model.tmrVal, 0, 3600 ))
					GUI_CASE_COL(4, 96,
							GUI_EDIT_ENUM( g_model.traineron, 0, 1, menu_on_off ))
					}
				}
				break;

			case MOD_PAGE_HELI_SETUP:
				// ToDo: Implement!
				break;

			case MOD_PAGE_EXPODR:
				// ToDo: Implement!
				break;

			case MOD_PAGE_MIXER:
			{
				static enum { STATE_SELECT, STATE_POPUP } state = STATE_SELECT;
				list_limit = MAX_MODELS - 1;
				for (i = list_top;
						(i < list_top + LIST_ROWS) && (i <= list_limit); ++i)
				{
					const uint8_t line = (i - list_top + 1) * 8;
					lcd_set_cursor(0, line);
					LCD_OP op_list = LCD_OP_SET;
					LCD_OP op_item = LCD_OP_SET;
					uint8_t edit = 0;
					if ((g_menu_mode == MENU_MODE_LIST) && (i == list)) {
						op_list = LCD_OP_CLR;
					} else if ((g_menu_mode == MENU_MODE_EDIT
							|| g_menu_mode == MENU_MODE_EDIT_S)
							&& (i == list)) {
						edit = 1;
						op_item = LCD_OP_CLR;
					}
					char s[4];
					s[0] = '0' + i / 10;
					s[1] = '0' + i % 10;
					s[2] = ' ';
					s[3] = 0;
					lcd_write_string(s, op_list, FLAGS_NONE);
					lcd_write_int(g_model.mixData[i].destCh,op_list,FLAGS_NONE);
					lcd_write_string(" ", op_list, FLAGS_NONE);
					lcd_write_int(g_model.mixData[i].srcRaw,op_list,FLAGS_NONE);
				}
				// if we were in the popup then the result would show up once
				char popupRes = gui_popup_get_result();
				if( popupRes )
				{
					if( popupRes > 0 )
					{
						// todo - handle selected line: Insert,Delete,Copy,Paste
					}
					//state = STATE_SELECT;
					//g_menu_mode = MENU_MODE_LIST;
					//gui_navigate(GUI_LAYOUT_MODEL_MENU);
				}
				else
				{
					if( g_menu_mode == MENU_MODE_EDIT )
					{
						if( g_key_press & KEY_MENU )
						{
							state = STATE_POPUP;
							gui_popup_select(GUI_MSG_ROW_MENU);
						}
					}
				}
			}
			break;

			case MOD_PAGE_LIMITS:
				// ToDo: Implement!
				break;

			case MOD_PAGE_CURVES:
				// ToDo: Implement!
				break;

			case MOD_PAGE_CUST_SW:
				// ToDo: Implement!
				break;

			case MOD_PAGE_SAFE_SW:
				// ToDo: Implement!
				break;

			case MOD_PAGE_TEMPLATES:
				// ToDo: Implement!
				break;

				// Not navigable through left / right scrolling.

			case MOD_PAGE_MIX_EDIT:
				// ToDo: Implement!
				break;
			case MOD_PAGE_CURVE_EDIT:
				// ToDo: Implement!
				break;

			}
		}
	}
		break;	//GUI_LAYOUT_SYSTEM_MENU, GUI_LAYOUT_MODEL_MENU

		/**********************************************************************
		 * Calibration Screen
		 *
		 * Drives the sticks module into calibration mode and returns to
		 * Main 1 when complete or cancelled.
		 *
		 */
	case GUI_LAYOUT_STICK_CALIBRATION: {
		static CAL_STATE state;
		if (full) {
			// Draw the whole screen.
			state = CAL_LIMITS;
			sticks_calibrate(state);
			lcd_set_cursor(5, 0);
			lcd_draw_message(msg[GUI_MSG_CAL_MOVE_EXTENTS], LCD_OP_SET, 0, 0);
		}

		if ((g_update_type & UPDATE_STICKS) != 0) {
			gui_show_sticks();
		}

		if ((g_update_type & UPDATE_KEYPRESS) != 0) {
			lcd_set_cursor(5, 8);
			if ((g_key_press & KEY_SEL) || (g_key_press & KEY_OK)) {
				lcd_draw_rect(5, 0, 123, BOX_Y - 1, LCD_OP_CLR, RECT_FILL);
				if (state == CAL_LIMITS) {
					state = CAL_CENTER;
					sticks_calibrate(CAL_CENTER);
					lcd_draw_message(msg[GUI_MSG_CAL_CENTRE], LCD_OP_SET,0,0);
				} else if (state == CAL_CENTER) {
					state = CAL_OFF;
					sticks_calibrate(CAL_OFF);
					lcd_draw_message(msg[GUI_MSG_OK_CANCEL], LCD_OP_SET,0,0);
				} else {
					// ToDo: Write the calibration data into EEPROM.
					// eeprom_set_data(EEPROM_ADC_CAL, cal_data);
					gui_navigate(GUI_LAYOUT_MAIN1);
				}
			} else if (g_key_press & KEY_CANCEL) {
				// ToDo: eeprom_get_data(EEPROM_ADC_CAL, cal_data);
				gui_navigate(GUI_LAYOUT_MAIN1);
			}
		}
	}
		break;
	}

	g_key_press = KEY_NONE;

	lcd_update();
}

/**
 * @brief  Update a specific GUI component.
 * @note
 * @param  type: Component type to update.
 * @retval None
 */
void gui_update(UPDATE_TYPE type) {
	if (g_gui_timeout != 0)
		return;

	g_update_type |= type;

	// Don't go crazy on the updates. limit to 25fps.
	task_schedule(TASK_PROCESS_GUI, type, 40);
}

/**
 * @brief  Drive the GUI with keys
 * @note
 * @param  key: The key that was pressed.
 * @retval None
 */
void gui_input_key(KEYPAD_KEY key) {
	g_key_press |= key;
	gui_update(UPDATE_KEYPRESS);
}

/**
 * @brief  Set the GUI to a specific layout.
 * @note
 * @param  layout: The requested layout.
 * @retval None
 */
void gui_navigate(GUI_LAYOUT layout) {
	g_new_layout = layout;
	task_schedule(TASK_PROCESS_GUI, UPDATE_NEW_LAYOUT, 0);
}

/**
 * @brief  Display the requested message.
 * @note	The location and translation will be specific to the current state.
 * @param  state: The requested state.
 * @param  timeout: The number of ms to display the message. 0 for no timeout.
 * @retval None
 */
void gui_popup(GUI_MSG msg, int16_t timeout) {
	g_new_msg = msg;
	g_current_msg = 0;
	g_popup_selected_line = 0;
	g_popup_result = GUI_POPUP_RESULT_NONE;
	if (timeout == 0)
		g_gui_timeout = 0;
	else
		g_gui_timeout = system_ticks + timeout;

	task_schedule(TASK_PROCESS_GUI, UPDATE_MSG, 0);
}


/**
 * @brief  Display the requested message as multiple line, one to be selected..
 * @note   The location and translation will be specific to the current state.
 * @retval None
 */
void gui_popup_select(GUI_MSG msg) {
	g_new_msg = msg;
	g_current_msg = 0;
	g_popup_selected_line = 1;
	g_popup_result = GUI_POPUP_RESULT_NONE;
	g_gui_timeout = 0;
	task_schedule(TASK_PROCESS_GUI, UPDATE_MSG, 0);
}

/**
 * @brief  Returns result of a popup and clears the result (single time use)
 * @note   Call only once as it will set the result the NONE
 * @retval result of GUI popup - NONE(0); CANCEL(-1), 1 or selected line # for popup_select
 */
char gui_popup_get_result() {
	char res = g_popup_result;
	g_popup_result = GUI_POPUP_RESULT_NONE;
	return res;
}

/**
 * @brief  Display the requested message as multiple line, one to be selected..
 * @note   The location and translation will be specific to the current state.
 * @param  selectedLine line - initial selected line
 * @retval None
 */
char gui_popup_select_onkeypress(GUI_MSG msg) {
	if( g_key_press & (KEY_OK|KEY_SEL) )
	{
		return g_popup_selected_line;
	}
	if( g_key_press & KEY_CANCEL )
	{
		return -1;
	}
	if( g_key_press & KEY_LEFT ) g_popup_selected_line--;
	if( g_key_press & KEY_RIGHT ) g_popup_selected_line++;
	if( g_popup_selected_line < 1 ) g_popup_selected_line = 1;
	if( g_popup_selected_line > g_popup_lines ) g_popup_selected_line = g_popup_lines;
	g_new_msg = msg;
	g_current_msg = 0;
	g_gui_timeout = 0;
	task_schedule(TASK_PROCESS_GUI, UPDATE_MSG, 0);
	return 0;
}

/**
 * @brief  Returns the currently displayed layout.
 * @note
 * @param  None
 * @retval The current layout.
 */
GUI_LAYOUT gui_get_layout(void) {
	return g_current_layout;
}

/**
 * @brief  Display The stick position in two squares.
 * @note	Also shows POT bars.
 * @param  None.
 * @retval None.
 */
static void gui_show_sticks(void) {
	int x, y;

	// Stick boxes
	lcd_draw_rect(BOX_L_X, BOX_Y, BOX_L_X + BOX_W, BOX_Y + BOX_H, LCD_OP_CLR,
			RECT_FILL);
	lcd_draw_rect(BOX_R_X, BOX_Y, BOX_R_X + BOX_W, BOX_Y + BOX_H, LCD_OP_CLR,
			RECT_FILL);
	lcd_draw_rect(BOX_L_X, BOX_Y, BOX_L_X + BOX_W, BOX_Y + BOX_H, LCD_OP_SET,
			RECT_ROUNDED);
	lcd_draw_rect(BOX_R_X, BOX_Y, BOX_R_X + BOX_W, BOX_Y + BOX_H, LCD_OP_SET,
			RECT_ROUNDED);

	// Centre point
	lcd_draw_rect(BOX_L_X + BOX_W / 2 - 1, BOX_Y + BOX_H / 2 - 1,
			BOX_L_X + BOX_W / 2 + 1, BOX_Y + BOX_H / 2 + 1, LCD_OP_SET,
			RECT_ROUNDED);
	lcd_draw_rect(BOX_R_X + BOX_W / 2 - 1, BOX_Y + BOX_H / 2 - 1,
			BOX_R_X + BOX_W / 2 + 1, BOX_Y + BOX_H / 2 + 1, LCD_OP_SET,
			RECT_ROUNDED);

	// Stick position (Right)
	x = 2 + (BOX_W - 4) * sticks_get_percent(STICK_R_H) / 100;
	y = BOX_H - 2 - (BOX_H - 4) * sticks_get_percent(STICK_R_V) / 100;
	lcd_draw_rect(BOX_R_X + x - 2, BOX_Y + y - 2, BOX_R_X + x + 2,
			BOX_Y + y + 2, LCD_OP_SET, RECT_ROUNDED);

	// Stick position (Left)
	x = 2 + (BOX_W - 4) * sticks_get_percent(STICK_L_H) / 100;
	y = BOX_H - 2 - (BOX_H - 4) * sticks_get_percent(STICK_L_V) / 100;
	lcd_draw_rect(BOX_L_X + x - 2, BOX_Y + y - 2, BOX_L_X + x + 2,
			BOX_Y + y + 2, LCD_OP_SET, RECT_ROUNDED);

	// VRB
	x = BOX_H * sticks_get_percent(STICK_VRB) / 100;
	lcd_draw_rect(POT_L_X, POT_Y - BOX_H, POT_L_X + POT_W, POT_Y, LCD_OP_CLR,
			RECT_FILL);
	lcd_draw_rect(POT_L_X, POT_Y - x, POT_L_X + POT_W, POT_Y, LCD_OP_SET,
			RECT_FILL);

	// VRA
	x = BOX_H * sticks_get_percent(STICK_VRA) / 100;
	lcd_draw_rect(POT_R_X, POT_Y - BOX_H, POT_R_X + POT_W, POT_Y, LCD_OP_CLR,
			RECT_FILL);
	lcd_draw_rect(POT_R_X, POT_Y - x, POT_R_X + POT_W, POT_Y, LCD_OP_SET,
			RECT_FILL);
}

/**
 * @brief  Display the  switch states.
 * @note
 * @param  None.
 * @retval None.
 */
static void gui_show_switches(void) {
	int x;

	// Switches
	x = keypad_get_switches();
	lcd_set_cursor(SW_L_X, SW_Y);
	lcd_write_string("SWB", (x & SWITCH_SWB) ? LCD_OP_CLR : LCD_OP_SET,
			FLAGS_NONE);
	lcd_set_cursor(SW_L_X, SW_Y + 8);
	lcd_write_string("SWD", (x & SWITCH_SWD) ? LCD_OP_CLR : LCD_OP_SET,
			FLAGS_NONE);
	lcd_set_cursor(SW_R_X, SW_Y);
	lcd_write_string("SWA", (x & SWITCH_SWA) ? LCD_OP_CLR : LCD_OP_SET,
			FLAGS_NONE);
	lcd_set_cursor(SW_R_X, SW_Y + 8);
	lcd_write_string("SWC", (x & SWITCH_SWC) ? LCD_OP_CLR : LCD_OP_SET,
			FLAGS_NONE);
}

/**
 * @brief  Display a battery icon and text with the current level
 * @note
 * @param  x,y: TL location for the information.
 * @retval None.
 */
static void gui_show_battery(int x, int y) {
	int batt;
	int level;

	batt = sticks_get_battery();
	level = 12 * (batt - BATT_MIN) / (BATT_MAX - BATT_MIN);
	level = (level > 12) ? 12 : level;

	// Background
	lcd_draw_rect(x - 1, y, x + 15, y + 7, LCD_OP_CLR, RECT_FILL);

	// Battery Icon
	lcd_draw_rect(x, y + 1, x + 12, y + 6, LCD_OP_SET, RECT_ROUNDED);
	lcd_draw_rect(x + 12, y + 2, x + 14, y + 5, LCD_OP_SET, RECT_ROUNDED);
	lcd_draw_rect(x, y + 2, x + level, y + 5, LCD_OP_SET, RECT_FILL);

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
static void gui_show_timer(int x, int y) {
	// Timer
	lcd_set_cursor(x, y);
	lcd_write_int(g_model.tmrVal / 60, LCD_OP_SET, INT_PAD10 | CHAR_4X);
	lcd_write_string(":", LCD_OP_SET, CHAR_2X);
	lcd_write_int(g_model.tmrVal % 60, LCD_OP_SET, INT_PAD10 | CHAR_4X);
}

/**
 * @brief  Update all 4 trim bars.
 * @note
 * @param  None
 * @retval None
 */
static void gui_update_trim(void) {
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
static void gui_draw_trim(int x, int y, bool h_v, int value) {
	int w, h, v;
	w = (h_v) ? 48 : 6;
	h = (h_v) ? 6 : 48;
	v = 48 * value / (2 * MIXER_TRIM_LIMIT);

	// Draw the value box
	if (h_v) {
		// Clear the background
		lcd_draw_rect(x - 3, y, x + w + 3, y + h, LCD_OP_CLR, RECT_FILL);
		// Draw the centre dot and ends.
		lcd_draw_rect(x + w / 2 - 1, y + h / 2 - 1, x + w / 2 + 1,
				y + h / 2 + 1, LCD_OP_SET, RECT_FILL);
		lcd_draw_line(x, y + h / 2 - 1, x, y + h / 2 + 1, LCD_OP_SET);
		lcd_draw_line(x + w, y + h / 2 - 1, x + w, y + h / 2 + 1, LCD_OP_SET);
		// Main line
		lcd_draw_line(x, y + h / 2, x + w, y + h / 2, LCD_OP_SET);
		// Value Box
		lcd_draw_rect(v + x + w / 2 - 3, y + h / 2 - 3, v + x + w / 2 + 3,
				y + h / 2 + 3, LCD_OP_XOR, RECT_ROUNDED);
	} else {
		// Clear the background
		lcd_draw_rect(x, y - 3, x + w, y + h + 3, LCD_OP_CLR, RECT_FILL);
		// Draw the centre dot and ends.
		lcd_draw_rect(x + w / 2 - 1, y + h / 2 - 1, x + w / 2 + 1,
				y + h / 2 + 1, LCD_OP_SET, RECT_FILL);
		lcd_draw_line(x + w / 2 - 1, y, x + w / 2 + 1, y, LCD_OP_SET);
		lcd_draw_line(x + w / 2 - 1, y + h, x + w / 2 + 1, y + h, LCD_OP_SET);
		// Main line
		lcd_draw_rect(x + w / 2, y, x + w / 2, y + h, LCD_OP_SET, RECT_FILL);
		// Value Box
		lcd_draw_rect(x + w / 2 - 3, -v + y + h / 2 - 3, x + w / 2 + 3,
				-v + y + h / 2 + 3, LCD_OP_XOR, RECT_ROUNDED);
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
static void gui_draw_slider(int x, int y, int w, int h, int range, int value) {
	int i, v, d;

	// Value scaling
	v = (w * value / range) - (w / 2);
	// Division spacing
	d = w / 20;

	// Clear the background
	lcd_draw_rect(x, y, x + w, y + h, LCD_OP_CLR, RECT_FILL);

	// Draw the dashed line
	for (i = 0; i < w; i += d) {
		lcd_set_pixel(x + i, y + h / 2, LCD_OP_SET);
	}

	// Draw the bar
	if (v > 0)
		lcd_draw_rect(x + w / 2, y, x + w / 2 + v, y + h - 1, LCD_OP_XOR,
				RECT_FILL);
	else
		lcd_draw_rect(x + w / 2 + v, y, x + w / 2, y + h - 1, LCD_OP_XOR,
				RECT_FILL);

	// Draw the ends and middle
	lcd_draw_line(x, y, x, y + h - 1, LCD_OP_SET);
	lcd_draw_line(x + w / 2, y, x + w / 2, y + h - 1, LCD_OP_SET);
	lcd_draw_line(x + w, y, x + w, y + h - 1, LCD_OP_SET);
}

/**
 * @brief  Draw an arrow and circle to indicate which stick is described
 * @note
 * @param  stick: The type of stick.
 * @retval None
 */
static void gui_draw_stick_icon(STICK stick, uint8_t inverse) {
	char* p = "\x13\x14\x15";
	switch (stick) {
	case STICK_R_H:
		p = "\x0A\x0B\x0C";
		break;
	case STICK_R_V:
		p = "\x0D\x0E\x0F";
		break;
	case STICK_L_V:
		p = "\x10\x11\x12";
		break;
	default:
		break;
	}
	lcd_write_string(p, inverse ? LCD_OP_CLR : LCD_OP_SET, CHAR_NOSPACE);
}

/**
 * @brief  Draw a box around the string and allow each letter to be modified.
 * @note
 * @param  string: Pointer to string to edit.
 * @param  delta: Amount and direction to change.
 * @param  keys: Any pressed keys.
 * @retval None
 */
static void gui_string_edit(char *string, int8_t delta, uint32_t keys) {
	static int8_t char_index = 0;
	static int8_t edit_mode = 1;
	int i;

	if (keys & KEY_SEL) {
		g_menu_mode = MENU_MODE_EDIT_S;
		edit_mode = 1 - edit_mode;
	} else if (keys & (KEY_CANCEL | KEY_OK)) {
		char_index = 0;
		edit_mode = 1;
		g_menu_mode = MENU_MODE_LIST;
	}

	if (edit_mode) {
		string[char_index] += delta;
		if (string[char_index] < 32)
			string[char_index] = 32;
		if (string[char_index] > 126)
			string[char_index] = 126;
	} else {
		char_index += delta;
	}

	if (char_index < 0)
		char_index = 0;
	if (char_index >= strlen(string))
		char_index = strlen(string) - 1;

	//rolling counter that slows down cursor blinking
	static int8_t roll;
	roll++;
	// draw the whole string
	for (i = 0; i < strlen(string); ++i) {
		LCD_OP op = LCD_OP_CLR;
		int fg = FLAGS_NONE;
		if (i == char_index) {
			if (edit_mode) {
				// this makes a block flashing cursor
				op = roll & 8 ? LCD_OP_SET : LCD_OP_CLR;
			} else {
				// this makes a underline flashing
				fg = roll & 8 ? CHAR_UNDERLINE : FLAGS_NONE;
			}
		}
		lcd_write_char(string[i], op, fg);
	}
}

/**
 * @brief  Draw a box around the string and allow each 'bit' (character) to be enabled / disabled.
 * @note
 * @param  string: Description string.
 * @param  field: Bitfield to edit
 * @param  delta: Amount and direction to change.
 * @param  keys: Any pressed keys.
 * @retval Modified version of bitfield.
 */
static uint32_t gui_bitfield_edit(char *string, uint32_t field, int8_t delta,
		uint32_t keys, uint8_t edit) {
	static int8_t char_index = 0;
	static int8_t edit_mode = 1;
	int i;
	uint32_t ret = field;
	uint16_t lcd_flags = FLAGS_NONE;

	if (edit) {
		if (keys & KEY_SEL) {
			edit_mode = 1 - edit_mode;
			g_menu_mode = MENU_MODE_EDIT_S;
		} else if (keys & (KEY_CANCEL | KEY_OK)) {
			char_index = 0;
			edit_mode = 1;
			g_menu_mode = MENU_MODE_LIST;
		} else if (keys & (KEY_LEFT | KEY_RIGHT)) {
			if (edit_mode) {
				ret ^= 1 << char_index;
			} else {
				char_index += delta;
			}
		}

		if (char_index < 0)
			char_index = 0;
		if (char_index >= strlen(string))
			char_index = strlen(string) - 1;
	}

	for (i = 0; i < strlen(string); ++i) {
		lcd_flags =
				(i == char_index && edit_mode == 0) ?
						CHAR_UNDERLINE : FLAGS_NONE;
		lcd_write_char(string[i], (ret & (1 << i)) ? LCD_OP_CLR : LCD_OP_SET,
				lcd_flags);
	}

	return ret;
}

/**
 * @brief  Highlight and allow the value to be modified.
 * @note
 * @param  data: Pointer to data to edit.
 * @param  delta: Amount and direction to change.
 * @param  min: Minimum allowed value.
 * @param  max: Maximum allowed value.
 * @retval Updated value
 */
static int32_t gui_int_edit(int32_t data, int32_t delta, int32_t min,
		int32_t max) {
	int32_t ret = data;

	ret += delta;

	if (ret > max)
		ret = max;
	if (ret < min)
		ret = min;

	return ret;
}
