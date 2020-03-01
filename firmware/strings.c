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

#include "strings.h"
#include "system.h"


// declaring arrays to hold *const (constant pointers)
// makes gcc put these arrays in rodata hence in flash
// otherwise they would have to me marked with __attribute__((section(".rodata")))

const char * const switches[NUM_SWITCHES+1]  = {
		"---",
		"SWA",
		"SWB",
		"SWC",
		"SWD",
};

const char * const switches_mask[1<<NUM_SWITCHES]  = {
		"----",
		"A---",
		"-B--",
		"AB--",
		"--C-",
		"A-C-",
		"-BC-",
		"ABC-",
		"---D",
		"A--D",
		"-B-D",
		"AB-D",
		"--CD",
		"A-CD",
		"-BCD",
		"ABCD"
};


const char * const sticks[NUM_STICKS] = { //logical stick names (MODE 2 physical order)
		"AIL",
		"ELE", 
		"THR", 
		"RUD"
};

const char * const pots[NUM_POTS] = {
		"VRA",
		"VRB",
};


const char * const sources[SRC_MAX] = {
		"HALF",
		"FULL",
		"CYC",	// CYC1-CYC3
		"PPM",	// PPM1-PPM8
		"CH",	// CH1-CH16
		"TR"	// Trainer SRC
};


const char * const mix_warn[MIX_WARN_MAX] = {
		"off",
		"W1",
		"W2",
		"W3",
};


const char * const mix_src[MIX_SRCS_MAX] = {
		"---",
		"AIL",
		"ELE",
		"THR",
		"RUD",
		"VRA",
		"VRB",
		"VRC",
		"MAX",  // 8 MIX_MAX
		"FULL", // 9 MIX_FULL
		"CYC1", // 10
		"CYC2", // 11
		"CYC3",	// 12
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
const char * const mix_mode_hdr = "mode";


const char * const mix_mode[MIX_MODE_MAX] = {
		"+=",
		"*=",
		":=",
};



const char * const mix_curve[MIX_CURVE_MAX] = {
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


const char * const expodr[EXPODR_MAX] = {
		"CH ",   // 0: 0
		"EXPO",  // 1:
		"",      // 2: 1 2
		"WEIGHT",// 3:
		"",      // 4: 3 4
		"Sw1",   // 5: 5
		"Sw2"    // 6: 6
};


const char * const drlevel[3] = {
		"Hi",
		"Md",
		"Lo"
};


const char * const menu_on_off[4] = {
		"OFF",
		"ON",
		"off",
		"on"
};


const char * const channel_order[CHAN_ORDER_MAX] = {
		"ATER",
		"AETR",
		"RTEA",
		"RETA",
};


const char * const system_menu_beeper[BEEPER_MAX] = {
		"Silent",
		"NoKey",
		"Normal",
};


const char * const msg[GUI_MSG_MAX] = {
		"",
		"Press [OK] to start Calibration.",
		"Move all controls to their extents then press [OK].",
		"Centre the sticks then press [OK].",
		"OK",
		"Operation Cancelled.",
		"OK:Save Cancel:Abort",
		"Please zero throttle to continue.",
		"SW warning!",
		"\x09",
		"Calibration data invalid, please calibrate the sticks.",
		"Preset model?\n\nNo\nYes",
		"Reset to factory default?\n\nNo\nYes",
		"Preset\nInsert\nDelete\nCopy\nPaste",/*GUI_MSG_ROW_MENU*/
		"Firmware Upgrade?\n\nNo\nYes",/*GUI_MSG_FW_UPGRADE*/

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
		"CURVE",
};


const char * const system_menu_list1[SYS_MENU_LIST1_LEN] = {
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


const char * const model_menu_list1[MOD_MENU_LIST1_LEN] = {
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
		"Center Beep",
};


const char * const mixer_edit_list1[MIXER_EDIT_LIST1_LEN] = {
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

const char * const heli_menu_list[HELI_MENU_LIST_LEN] = {
		"Swash type",
		"Swash ring %",
		"Coll. source",
		"ELE invert",
		"AIL invert",
		"COL invert"
};

const char * const timer_modes[] = {
		"Off",
		"Abs",
		"Stk",
		"Stk%",
		"Sw/!Sw",
		"!m_sw/!m_sw"
};


const char * const dir_labels[] = {
		"Down",
		"Up",
};



const char * const inverse_labels[] = {
		"---",
		"INV"
};

const char * const swash_type_labels[SWASH_TYPE_MAX] = {
	"---",
	"120",
	"120X",
	"140",
	"90"
};

// TODO: what are they?

const char * const safety_switch_mode_labels[] = {
		"M0?",
		"M1?",
		"M2?",
		"M3?"
};

// future??? - popup header
// #define HEADER_SEPARATOR '\x05' // make header text from text before HEADER_SEPARATOR char
// int char_pos(char *s, char c) { //
// 	int retval = -1;
// 	int auxI = 0;
// 	char *auxS;
// 	auxS = s;
// 	while (*auxS != 0) {
// 		if (*auxS == HEADER_SEPARATOR) {
// 			retval = auxI;
// 			break;
// 		}
// 		auxS++;
// 		auxI++;
// 	}

// 	return retval;
// }

// char * get_popup_content(char * str) {
// 	char *auxS;
// 	auxS = str;
// 	str = str + char_pos(str, HEADER_SEPARATOR) +1;
// 	return str;
// }

// char * get_popup_header(char * str) {
// 	char *auxS;
// 	auxS = str;
// 	char *retval;
// 	retval = 0;
// 	int pos = char_pos(str, HEADER_SEPARATOR);
// 	if (pos >= 0) {
// 		*retval = (char*) malloc(pos+1);
// 		for(int i=0; i<pos; i++) {
// 			retval[i] = str[i];
// 		}
// 		retval[pos] = 0x00;
//  	}
// 	return retval;
// }
