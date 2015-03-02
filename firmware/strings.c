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

const char * mix_warm[MIX_WARN_MAX] = {
		"off"
		"W1",
		"W2",
		"W3",
};

const char * mix_src[MIX_SRC_MAX] = {
		"off",
		"AIL",
		"ELE",
		"THR",
		"RUD",
		"VRA",
		"VRB",
		"???",
		"???",
		"MAX", // 9
		"FULL", // 10
		"CYC1", // 11
		"CYC2", // 12
		"CYC3",	// 13
		"PPM1",
		"PPM2",
		"PPM3",
		"PPM4",
		"PPM5",
		"PPM6",
		"PPM7",
		"PPM8",
		"CH1", // CHAN_BASE
		"CH2",
		"CH3",
		"CH4",
		"CH5",
		"CH6",
		"CH7",
		"CH8",
		// CHOUT_BASE
};

// must follow _mix_mode and
const char *mix_mode_hdr = "mode";
const char *mix_mode[MIX_MODE_MAX] = {
		"+=",
		"*=",
		":=",
};


const char *mix_curve[MIX_CURVE_MAX] = {
		"x",
		"x>0",
		"x<0",
		"|x|",
		"f>0",
		"f<0",
		"|f|",
		"cv1",
		"cv2",
		"cv3",
		"cv4",
		"cv5",
		"cv6",
		"cv7",
		"cv8",
};

const char* expodr[EXPODR_MAX] = {
		"CH ",   // 0: 0
		"EXPO",  // 1:
		"",      // 2: 1 2
		"WEIGHT",// 3:
		"",      // 4: 3 4
		"Sw1",   // 5: 5
		"Sw2"    // 6: 6
};

const char* drlevel[3] = {
		"Hi",
		"Md",
		"Lo"
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
		"Press [OK] to start Calibration.",
		"Move all controls to their extents then press [OK].",
		"Centre the sticks then press [OK].",
		"OK",
		"Operation Cancelled.",
		"OK:Save Cancel:Abort",
		"Please zero throttle to continue.",
		"Calibration data invalid, please calibrate the sticks.",
		"OK to preset the model?",
		"Preset\nInsert\nDelete\nCopy\nPaste\n",

		// Headings (System)
		"RADIO SETUP",
		"TRAINER",
		"VERSION",
		"DIAGNOSTICS",
		"ANALOG",
		"CALIBRATION",

		// Headings (Model)
		"MODELSEL",
		"SETUP ",
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
};

const char *model_menu_list1[MOD_MENU_LIST1_LEN] = {
		"Model Name",
		"Timer Mode",
		"Timer Dir",
		"Timer Value",
		"Trainer Ok",
		"Thro Trim",
		"Thro Expo",
		"Thrim Incr",
		"PPM Extd Lmts",
		"PPM #Chanels",
		"PPM Delay",
		"PPM Ex Frm Wdth",
};

const char *mixer_edit_list1[MIXER_EDIT_LIST1_LEN] = {
		"Source",
		"Weight",
		"Offset",
		"Trim ON",
		"Curve",
		"Switch",
		"Phase",
		"Warning",
		"Multpx",
		"Delay Up",
		"Delay Dn",
		"Slow Up",
		"Slow Dn"

};


const char* timer_modes[] = {
		"Off",
		"Abs",
		"Stk",
		"Stk%",
		"Sw/!Sw",
		"!m_sw/!m_sw"
};

const char* dir_labels[] = {
		"Down",
		"Up",
};


const char* inverse_labels[] = {
		"---",
		"INV"
};


// TODO: what are they?
const char* safety_switch_mode_labels[] = {
		"M0?",
		"M1?",
		"M2?",
		"M3?"
};

