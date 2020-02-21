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
#include "lcd.h"
#include "sticks.h"

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
} MENU_MODE;

// MenuContext used for Page/Subpage edits (System&Model)
typedef struct {
	uint8_t page; // current menu page
	uint8_t submenu_page; // submenu page if any for this page
	uint8_t popup; // popup id if any for this page

	uint8_t item; // current selected item (row in list or item in the form)
	uint8_t item_limit; // #-1 of items (rows or fields) in a page
	int8_t col;  // current column (0 for single column row)
	int8_t col_limit; // #-1 of columns in this row
	uint8_t top_row;  // current top row (scroll offset)
	uint8_t cur_row_y; // precomputed LCD y offset of current line
	int8_t copy_row; // row # of last "copy" (to be used in next "paste")

	MENU_MODE menu_mode :3; // current state of navigation
	uint8_t edit :1; // 1 if this row/item is in active "edit" mode; 0 otherwise
	int8_t inc :4; // -1,0,1 - used to pass to the value editors for increment/decrement, bigger values for accelerated inc on int edit 
	LCD_OP op_list :2; // opacity of text currently being printed in a row (used mainly for row heading)
	LCD_OP op_item :2; // opacity of text of item currently being printed
	uint8_t form :1; // option - a form like behavior - stops row scrolling
} MenuContext;

// Battery values.
#define BATT_MIN	99	//NiMh: 88
#define BATT_MAX	126	//NiMh: 104



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
static int8_t g_menu_mode_dir = 1;





void gui_init(void);
void gui_process(uint32_t data);

void gui_update(UPDATE_TYPE type);
void gui_input_key(KEYPAD_KEY key);

void gui_navigate(GUI_LAYOUT layout);
void gui_popup(GUI_MSG msg, int16_t timeout);
void gui_popup_select(GUI_MSG msg);
char gui_popup_get_result();
static void light_management();

GUI_LAYOUT gui_get_layout(void);

//static MenuContext g_menuContext = {0};

static void gui_show_sticks(void);
static void gui_show_switches(void);
static void gui_show_battery(int x, int y);
static void gui_show_timer(int x, int y, int seconds);
static void gui_update_trim(void);
static void gui_draw_trim(int x, int y, uint8_t h_v, int value);
static void gui_draw_slider(int x, int y, int w, int h, int range, int value);
static void gui_draw_stick_icon(STICK stick, uint8_t inverse);

static void gui_string_edit(MenuContext* pCtx, char *string, uint32_t keys);
static uint32_t gui_bitfield_edit(MenuContext* pCtx, char *string,
		uint32_t field, int8_t delta, uint32_t keys);
static int32_t gui_int_edit(int32_t data, int32_t delta, int32_t min,
		int32_t max);



#endif // _GUI_H
