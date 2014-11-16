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

#ifndef _STRINGS_H
#define _STRINGS_H

#define NUM_STICKS		4
#define NUM_POTS		2
#define NUM_SWITCHES	4

#define SYS_MENU_LIST1_LEN	22
#define MOD_MENU_LIST1_LEN	6

typedef enum
{
	GUI_MSG_NONE = 0,
	GUI_MSG_CAL_OK_START,
	GUI_MSG_CAL_MOVE_EXTENTS,
	GUI_MSG_CAL_CENTRE,
	GUI_MSG_OK,
	GUI_MSG_CANCELLED,
	GUI_MSG_OK_CANCEL,
	GUI_MSG_ZERO_THROTTLE,
	GUI_MSG_EEPROM_INVALID,

	// Headings (System Menu)
	GUI_HDG_RADIO_SETUP,
	GUI_HDG_TRAINER,
	GUI_HDG_VERSION,
	GUI_HDG_DIAG,
	GUI_HDG_ANALOG,
	GUI_HDG_CALIBRATION,

	// Headings (Model Menu)
	GUI_HDG_MODELSEL,
	GUI_HDG_SETUP,
	GUI_HDG_HELI_SETUP,
	GUI_HDG_EXPODR,
	GUI_HDG_MIXER,
	GUI_HDG_LIMITS,
	GUI_HDG_CURVES,
	GUI_HDG_CUST_SW,
	GUI_HDG_SAFE_SW,
	GUI_HDG_TEMPLATES,
	GUI_HDG_EDIT_MIX,
	GUI_HDG_CURVE_EDIT,


	GUI_MSG_MAX,
} GUI_MSG;

typedef enum _menu_mode
{
	MENU_MODE_PAGE = 0,
	MENU_MODE_LIST,
	MENU_MODE_EDIT,
	MENU_MODE_EDIT_S
} MENU_MODE;

enum _menu_page
{
	SYS_PAGE_SETUP = 0,
	SYS_PAGE_TRAINER,
	SYS_PAGE_VERSION,
	SYS_PAGE_DIAG,
	SYS_PAGE_ANA,
	SYS_PAGE_CAL,
};

enum _model_page
{
	MOD_PAGE_SELECT = 0,
	MOD_PAGE_SETUP,
	MOD_PAGE_HELI_SETUP,
	MOD_PAGE_EXPODR,
	MOD_PAGE_MIXER,
	MOD_PAGE_LIMITS,
	MOD_PAGE_CURVES,
	MOD_PAGE_CUST_SW,
	MOD_PAGE_SAFE_SW,
	MOD_PAGE_TEMPLATES,
	MOD_PAGE_MIX_EDIT,
	MOD_PAGE_CURVE_EDIT,
};

enum _mix_mode
{
	MIX_MODE_OFF,
	MIX_MODE_ADD,
	MIX_MODE_MULTIPLY,
	MIX_MODE_REPLACE,
	MIX_MODE_MAX
};

enum _sources
{
	SRC_HALF = 0,
	SRC_FULL,
	SRC_CYC,
	SRC_PPM,
	SRC_MIX,
	SRC_TRN,
	SRC_MAX
};

enum _chan_order
{
	CHAN_ORDER_ATER = 0,
	CHAN_ORDER_AETR,
	CHAN_ORDER_RTEA,
	CHAN_ORDER_RETA,
	CHAN_ORDER_MAX,
};

enum _menu_beeper
{
	BEEPER_SILENT = 0,
	BEEPER_NOKEY,
	BEEPER_NORMAL,
	BEEPER_MAX
};

extern const char *switches[NUM_SWITCHES+1];
extern const char *sticks[NUM_STICKS];
extern const char *pots[NUM_POTS];
extern const char *sources[SRC_MAX];
extern const char *mix_mode_hdr;
extern const char *mix_mode[MIX_MODE_MAX];
extern const char *menu_on_off[4];
const char *channel_order[CHAN_ORDER_MAX];
extern const char *system_menu_beeper[BEEPER_MAX];
extern const char *msg[GUI_MSG_MAX];
extern const char *system_menu_list1[SYS_MENU_LIST1_LEN];
extern const char *model_menu_list1[MOD_MENU_LIST1_LEN];
extern const char *timer_modes[];
extern const char *dir_labels[];

#endif // _STRINGS_H
