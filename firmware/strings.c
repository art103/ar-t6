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
#include "strings.h"

const char *switches[NUM_SWITCHES+1] = {
		"---",
		"SWA",
		"SWB",
		"SWC",
		"SWD",
};

const char *sticks[NUM_STICKS] = {
		"AIL",
		"ELE",
		"THR",
		"RUD",
};

const char *pots[NUM_POTS] = {
		"VRA",
		"VRB",
};

const char *sources[SRC_MAX] = {
		"HALF",
		"FULL",
		"CYC",	// CYC1-CYC3
		"PPM",	// PPM1-PPM8
		"CH",	// CH1-CH16
		"ch"	// Trainer SRC
};

const char *mix_mode_hdr = "mode";
const char *mix_mode[MIX_MODE_MAX] = {
		"off",
		"+=",
		"*=",
		":=",
};

const char *menu_on_off[4] = {
		"OFF",
		"ON",
		"off",
		"on"
};

const char *channel_order[CHAN_ORDER_MAX] = {
		"ATER",
		"AETR",
		"RTEA",
		"RETA",
};

const char *system_menu_beeper[BEEPER_MAX] = {
		"Silent",
		"NoKey",
		"Normal",
};

const char *msg[GUI_MSG_MAX] = {
		"",
		"Press OK to start Calibration.",
		"Please move all analog controls to their extents then press OK.",
		"Please centre the sticks then press OK.",
		"OK",
		"Operation Cancelled.",
		"OK:Save Cancel:Abort",
		"Please zero throttle to continue.",
		"Calibration data invalid, please calibrate the sticks.",

		// Headings (System)
		"RADIO SETUP",
		"TRAINER",
		"VERSION",
		"DIAG",
		"ANA",
		"CALIBRATION",

		// Headings (Model)
		"MODELSEL",
		"SETUP nn",
		"HELI SETUP",
		"EXPO/DR",
		"MIXER",
		"LIMITS",
		"CURVES",
		"CUSTOM SWITCHES",
		"SAFETY SWITCHES",
		"TEMPLATES",
		"EDIT MIX",
		"CURVE nn",
};

const char *system_menu_list1[SYS_MENU_LIST1_LEN] = {
		"Owner Name",
		"Beeper",
		"Volume",
		"Contrast",
		"Battery Warning",
		"Inactivity Alarm",
		"Throttle Reverse",
		"Minute beep",
		"Beep countdown",
		"Flash on beep",
		"Light switch",
		"Backlight invert",
		"Light off after",
		"Light on Stk Mv",
		"Splash Screen",
		"Throttle Warning",
		"Switch Warning",
		"Default Sw",
		"Memory Warning",
		"Alarm Warning",
		"Enable PPMSIM",
		"Mode",
		"  ",
};
