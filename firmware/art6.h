/*
 * Author - Richard Taylor <richard@artaylor.co.uk>
 *
 * Based on er9x -> http://code.google.com/p/er9x/
 * Based on th9x -> http://code.google.com/p/th9x/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */
#ifndef _ART6_H
#define _ART6_H


#include <string.h>
#include <stddef.h>

extern uint8_t SlaveMode;

//#include "file.h"

//#ifdef FIX_MODE
//extern const prog_uint8_t APM stickScramble[] ;
////extern const prog_uint8_t APM modeFix[] ;
//uint8_t modeFixValue( uint8_t value ) ;
//#else
//extern const prog_uint8_t APM modn12x3[] ;
//#endif
//
//extern const prog_char APM Str_OFF[] ;
//extern const prog_char APM Str_ON[] ;
//extern const prog_char APM Str_Switch_warn[] ;
//
////extern const prog_uint8_t APM chout_ar[] ;
//extern const prog_uint8_t APM bchout_ar[] ;
//
////convert from mode 1 to mode g_eeGeneral.stickMode
////NOTICE!  =>  1..4 -> 1..4
//extern uint8_t convert_mode_helper(uint8_t x) ;
//
//#define CONVERT_MODE(x)  (((x)<=4) ? convert_mode_helper(x) : (x))
////#define CHANNEL_ORDER(x) (pgm_read_byte(chout_ar + g_eeGeneral.templateSetup*4 + (x)-1))
//#define CHANNEL_ORDER(x) ( ( (pgm_read_byte(bchout_ar + g_eeGeneral.templateSetup) >> (6-(x-1) * 2)) & 3 ) + 1 )
//#define THR_STICK       (2-(g_eeGeneral.stickMode&1))
//#define ELE_STICK       (1+(g_eeGeneral.stickMode&1))
//#define AIL_STICK       ((g_eeGeneral.stickMode&2) ? 0 : 3)
//#define RUD_STICK       ((g_eeGeneral.stickMode&2) ? 3 : 0)
//
//enum EnumKeys {
//    KEY_MENU ,
//    KEY_EXIT ,
//    KEY_DOWN ,
//    KEY_UP  ,
//    KEY_RIGHT ,
//    KEY_LEFT ,
//    TRM_LH_DWN  ,
//    TRM_LH_UP   ,
//    TRM_LV_DWN  ,
//    TRM_LV_UP   ,
//    TRM_RV_DWN  ,
//    TRM_RV_UP   ,
//    TRM_RH_DWN  ,
//    TRM_RH_UP   ,
//	  BTN_RE,
//    //SW_NC     ,
//    //SW_ON     ,
//    SW_ThrCt  ,
//    SW_RuddDR ,
//    SW_ElevDR ,
//    SW_ID0    ,
//    SW_ID1    ,
//    SW_ID2    ,
//    SW_AileDR ,
//    SW_Gear   ,
//    SW_Trainer
//};
//
#define NUM_CSW  8 //number of custom switches
#define EXTRA_CSW	5
//#define EXTRA_VOICE_SW	8
//
//extern const prog_char APM Str_Switches[] ;
//
//#define CURVE_BASE 7
//
////#define CSW_LEN_FUNC 7
//
//#define CS_OFF       (uint8_t)0
//#define CS_VPOS      (uint8_t)1  //v>offset
//#define CS_VNEG      (uint8_t)2  //v<offset
//#define CS_APOS      (uint8_t)3  //|v|>offset
//#define CS_ANEG      (uint8_t)4  //|v|<offset
//#define CS_AND       (uint8_t)5
//#define CS_OR        (uint8_t)6
//#define CS_XOR       (uint8_t)7
//#define CS_EQUAL     (uint8_t)8
//#define CS_NEQUAL    (uint8_t)9
//#define CS_GREATER   (uint8_t)10
//#define CS_LESS      (uint8_t)11
//#ifdef VERSION3
//#define CS_LATCH  	 (uint8_t)12
//#define CS_FLIP    	 (uint8_t)13
//#else
//#define CS_EGREATER   (uint8_t)12
//#define CS_ELESS      (uint8_t)13
//#endif
//#define CS_TIME	     (uint8_t)14
//#define CS_MAXF      14  //max function
//
//#define CS_VOFS       (uint8_t)0
//#define CS_VBOOL      (uint8_t)1
//#define CS_VCOMP      (uint8_t)2
//#define CS_TIMER			(uint8_t)3
//uint8_t CS_STATE( uint8_t x) ;
////#define CS_STATE(x)   (((uint8_t)x)<CS_AND ? CS_VOFS : (((uint8_t)x)<CS_EQUAL ? CS_VBOOL : (((uint8_t)x)<CS_TIME ? CS_VCOMP : CS_TIMER)))
//
//
//const prog_char APM s_charTab[]=" ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789_-.";
//#define NUMCHARS (sizeof(s_charTab)-1)
//
//
////#define SW_BASE      SW_NC
//#define SW_BASE      SW_ThrCt
//#define SW_BASE_DIAG SW_ThrCt
//#define MAX_PSWITCH   (SW_Trainer-SW_ThrCt+1)  // 9 physical switches
////#define SWITCHES_STR "  NC  ON THR RUD ELE ID0 ID1 ID2 AILGEARTRNR"
//#if defined(CPUM128) || defined(CPUM2561)
//#define MAX_DRSWITCH (1+SW_Trainer-SW_ThrCt+1+NUM_CSW+EXTRA_CSW)
//#else
//#define MAX_DRSWITCH (1+SW_Trainer-SW_ThrCt+1+NUM_CSW)
//#endif
//
//#define SWP_RUD (SW_RuddDR-SW_BASE)
//#define SWP_ELE (SW_ElevDR-SW_BASE)
//#define SWP_AIL (SW_AileDR-SW_BASE)
//#define SWP_THR (SW_ThrCt-SW_BASE)
//#define SWP_GEA (SW_Gear-SW_BASE)
//
//#define SWP_RUDB (1<<SWP_RUD)
//#define SWP_ELEB (1<<SWP_ELE)
//#define SWP_AILB (1<<SWP_AIL)
//#define SWP_THRB (1<<SWP_THR)
//#define SWP_GEAB (1<<SWP_GEA)
//
//#define SWP_ID0 (SW_ID0-SW_BASE)
//#define SWP_ID1 (SW_ID1-SW_BASE)
//#define SWP_ID2 (SW_ID2-SW_BASE)
//#define SWP_ID0B (1<<SWP_ID0)
//#define SWP_ID1B (1<<SWP_ID1)
//#define SWP_ID2B (1<<SWP_ID2)
//
////Switch Position Illigal states
//#define SWP_IL1 (0)
//#define SWP_IL2 (SWP_ID0B | SWP_ID1B)
//#define SWP_IL3 (SWP_ID0B | SWP_ID2B)
//#define SWP_IL4 (SWP_ID1B | SWP_ID2B)
//#define SWP_IL5 (SWP_ID0B | SWP_ID1B | SWP_ID2B)
//
//#define SWP_LEG1	(SWP_ID0B)
//#define SWP_LEG2	(SWP_ID1B)
//#define SWP_LEG3	(SWP_ID2B)
//
//#define NUM_STICKS	4
//
//
//#define SWASH_TYPE_120   1
//#define SWASH_TYPE_120X  2
//#define SWASH_TYPE_140   3
//#define SWASH_TYPE_90    4
//#define SWASH_TYPE_NUM   4
//
//#define MIX_P1    5
//#define MIX_P2    6
//#define MIX_P3    7
//#define MIX_MAX   8
//#define MIX_FULL  9
//#define MIX_CYC1  10
//#define MIX_CYC2  11
//#define MIX_CYC3  12
//
//#define DR_HIGH   0
//#define DR_MID    1
//#define DR_LOW    2
//#define DR_EXPO   0
//#define DR_WEIGHT 1
//#define DR_RIGHT  0
//#define DR_LEFT   1
//#define DR_BOTH   2
//#define DR_DRSW1  99
//#define DR_DRSW2  98
//
//#define DSW_THR  1
//#define DSW_RUD  2
//#define DSW_ELE  3
//#define DSW_ID0  4
//#define DSW_ID1  5
//#define DSW_ID2  6
//#define DSW_AIL  7
//#define DSW_GEA  8
//#define DSW_TRN  9
//#define DSW_SW1  10
//#define DSW_SW2  11
//#define DSW_SW3  12
//#define DSW_SW4  13
//#define DSW_SW5  14
//#define DSW_SW6  15
//#define DSW_SW7   16
//#define DSW_SW8   17
//#define DSW_SW9   18
//#define DSW_SWA   19
//#define DSW_SWB   20
//#define DSW_SWC   21
//
//#define SCROLL_TH 64
//#define INACTIVITY_THRESHOLD 256
//#define THRCHK_DEADBAND 16
//#define SPLASH_TIMEOUT  (4*100)  //400 msec - 4 seconds
//
//#ifdef FIX_MODE
//#define IS_THROTTLE(x)  ((x) == 2) // (((2-(g_eeGeneral.stickMode&1)) == x) && (x<4))
//#else
//uint8_t IS_THROTTLE( uint8_t x ) ;
//#endif
//uint8_t IS_EXPO_THROTTLE( uint8_t x ) ;
//
//#define NUM_KEYS BTN_RE+1
//#define TRM_BASE TRM_LH_DWN
//
////#define _MSK_KEY_FIRST (_MSK_KEY_REPT|0x20)
////#define EVT_KEY_GEN_BREAK(key) ((key)|0x20)
//#define _MSK_KEY_REPT    0x40
////#define _MSK_KEY_DBL     0x10
//#define IS_KEY_BREAK(key)  (((key)&0xf0)        ==  0x20)
//#define EVT_KEY_BREAK(key) ((key)|                  0x20)
//#define EVT_KEY_FIRST(key) ((key)|    _MSK_KEY_REPT|0x20)
//#define EVT_KEY_REPT(key)  ((key)|    _MSK_KEY_REPT     )
//#define EVT_KEY_LONG(key)  ((key)|0x80)
//#define EVT_KEY_DBL(key)   ((key)|_MSK_KEY_DBL)
////#define EVT_KEY_DBL(key)   ((key)|0x10)
//#define EVT_ENTRY               (0xff - _MSK_KEY_REPT)
//#define EVT_ENTRY_UP            (0xfe - _MSK_KEY_REPT)
//#define EVT_KEY_MASK             0x0f
//
#define HEART_TIMER_PULSES 1;
#define HEART_TIMER_10ms 2;
//
//#define TMRMODE_NONE     0
//#define TMRMODE_ABS      1
//#define TMRMODE_THR      2
//#define TMRMODE_THR_REL  3
//#define MAX_ALERT_TIME   60
//

#define PROTO_PPM        0
#define PROTO_PPM16	   	 1
#define PROTO_PPMSIM     2		// Always make this the last protocol
#define PROT_MAX         2

#define PROT_STR "\006PPM   PPM16 PPMSIM"
//
//#define TRIM_EXTENDED_MAX	500
//
//extern uint8_t stickMoved;
//uint16_t stickMoveValue( void ) ;
//
//typedef void (*MenuFuncP)(uint8_t event);
//
///// stoppt alle events von dieser taste bis eine kurze Zeit abgelaufen ist
//void pauseEvents(uint8_t enuk);
///// liefert die Zahl der schnellen Wiederholungen dieser Taste
//uint8_t getEventDbl(uint8_t event);
///// stoppt alle events von dieser taste bis diese wieder losgelassen wird
//void    killEvents(uint8_t enuk);
///// liefert den Wert einer beliebigen Taste KEY_MENU..SW_Trainer
//bool    keyState(EnumKeys enuk);
///// Liefert das naechste Tasten-Event, auch trim-Tasten.
///// Das Ergebnis hat die Form:
///// EVT_KEY_BREAK(key), EVT_KEY_FIRST(key), EVT_KEY_REPT(key) oder EVT_KEY_LONG(key)
//uint8_t getEvent();
//
//extern uint8_t s_evt;
//#define putEvent(evt) (s_evt = evt)
//
//
///// goto given Menu, but substitute current menu in menuStack
//void    chainMenu(MenuFuncP newMenu);
///// goto given Menu, store current menu in menuStack
//void    pushMenu(MenuFuncP newMenu);
/////deliver address of last menu which was popped from
//MenuFuncP lastPopMenu();
///// return to last menu in menustack
///// if uppermost is set true, thenmenu return to uppermost menu in menustack
//void    popMenu(bool uppermost=false);
///// Gibt Alarm Maske auf lcd aus.
///// Die Maske wird so lange angezeigt bis eine beliebige Taste gedrueckt wird.
//void alert(const prog_char * s, bool defaults=false);
//void message(const prog_char * s);
///// periodisches Hauptprogramm
//void    perMain();
///// Bearbeitet alle zeitkritischen Jobs.
///// wie z.B. einlesen aller Eingaenge, Entprellung, Key-Repeat..
//void    per10ms();
///// Erzeugt periodisch alle Outputs ausser Bildschirmausgaben.
//void zeroVariables();
//void mainSequence() ;
//
//#define NO_TRAINER 0x01
//#define NO_INPUT   0x02
//#define FADE_FIRST	0x20
//#define FADE_LAST		0x40
////#define NO_TRIMS   0x04
////#define NO_STICKS  0x08
//
//void perOutPhase( int16_t *chanOut, uint8_t att ) ;
//void perOut(int16_t *chanOut, uint8_t att);
/////   Liefert den Zustand des Switches 'swtch'. Die Numerierung erfolgt ab 1
/////   (1=SW_ON, 2=SW_ThrCt, 10=SW_Trainer). 0 Bedeutet not conected.
/////   Negative Werte  erzeugen invertierte Ergebnisse.
/////   Die Funktion putsDrSwitches(..) erzeugt den passenden Ausdruck.
/////
/////   \param swtch
/////     0                : not connected. Liefert den Wert 'nc'
/////     1.. MAX_DRSWITCH : SW_ON .. SW_Trainer
/////    -1..-MAX_DRSWITCH : negierte Werte
/////   \param nc Wert, der bei swtch==0 geliefert wird.
//bool    getSwitch(int8_t swtch, bool nc, uint8_t level=0);
///// Zeigt den Namen des Switches 'swtch' im display an
/////   \param x     x-koordinate 0..127
/////   \param y     y-koordinate 0..63 (nur durch 8 teilbar)
/////   \param swtch -MAX_DRSWITCH ..  MAX_DRSWITCH
/////   \param att   NO_INV,INVERS,BLINK
/////
//void putsDrSwitches(uint8_t x,uint8_t y,int8_t swtch,uint8_t att);
//void putsTmrMode(uint8_t x, uint8_t y, uint8_t attr, uint8_t type);
//
//extern int16_t get_telemetry_value( int8_t channel ) ;
//
//extern uint8_t  s_timerState;
//#define TMR_OFF     0
//#define TMR_RUNNING 1
//#define TMR_BEEPING 2
//#define TMR_STOPPED 3
//void resetTimer();
//
//struct t_timerg
//{
//	uint16_t s_timeCumTot;
//	uint16_t s_timeCumAbs;  //laufzeit in 1/16 sec
//	uint16_t s_timeCumSw;  //laufzeit in 1/16 sec
//	uint16_t s_timeCumThr;  //gewichtete laufzeit in 1/16 sec
//	uint16_t s_timeCum16ThrP; //gewichtete laufzeit in 1/16 sec
//	uint8_t  s_timerState;
//  // Statics
//	uint16_t s_time;
//  uint16_t s_cnt;
//  uint16_t s_sum;
//  uint8_t sw_toggled;
//	uint8_t lastSwPos;
//	int16_t s_timerVal[2];
//	uint8_t Timer2_running ;
//	uint8_t Timer2_pre ;
//	int16_t last_tmr;
//} ;
//
//extern struct t_timerg TimerG ;
//void resetTimer2() ;
//
//extern uint8_t heartbeat;
//
//uint8_t char2idx(char c);
//char idx2char(uint8_t idx);
//
//
//#define EE_GENERAL 1
//#define EE_MODEL   2
//#define EE_TRIM    4           // Store model because of trim
//#define INCDEC_SWITCH   0x08
//
//extern bool    checkIncDec_Ret;//global helper vars
//extern uint8_t s_editMode;     //global editmode
//
///// Bearbeite alle events die zum gewaehlten mode passen.
///// KEY_LEFT u. KEY_RIGHT
///// oder KEY_UP u. KEY_DOWN falls _FL_VERT in i_flags gesetzt ist.
///// Dabei wird der Wert der Variablen i_pval unter Beachtung der Grenzen
///// i_min und i_max veraendet.
///// i_pval hat die Groesse 1Byte oder 2Bytes falls _FL_SIZE2  in i_flags gesetzt ist
///// falls EE_GENERAL oder EE_MODEL in i_flags gesetzt ist wird bei Aenderung
///// der Variablen zusaetzlich eeDirty() aufgerufen.
///// Als Bestaetigung wird beep() aufgerufen bzw. audio.warn() wenn die Stellgrenze erreicht wird.
//int16_t checkIncDec16( int16_t i_pval, int16_t i_min, int16_t i_max, uint8_t i_flags);
//int8_t checkIncDec( int8_t i_val, int8_t i_min, int8_t i_max, uint8_t i_flags);
//int8_t checkIncDec_hm( int8_t i_val, int8_t i_min, int8_t i_max);
////int8_t checkIncDec_vm(uint8_t event, int8_t i_val, int8_t i_min, int8_t i_max);
//int8_t checkIncDec_hg( int8_t i_val, int8_t i_min, int8_t i_max);
//int8_t checkIncDec_hg0( int8_t i_val, int8_t i_max) ;
//int8_t checkIncDec_hm0(int8_t i_val, int8_t i_max) ;
//int16_t checkIncDec_hmu0(int16_t i_val, uint8_t i_max) ;
//
//#define CHECK_INCDEC_H_GENVAR( var, min, max)     \
//    var = checkIncDec_hg(var,min,max)
//
//#define CHECK_INCDEC_H_GENVAR_0( var, max)     \
//    var = checkIncDec_hg0( var, max )
//
//#define CHECK_INCDEC_H_MODELVAR( var, min, max)     \
//    var = checkIncDec_hm(var,min,max)
//
//#define CHECK_INCDEC_H_MODELVAR_0( var, max)     \
//    var = checkIncDec_hm0(var,max)
//
//#if defined(CPUM128) || defined(CPUM2561)
//#define CHECK_INCDEC_MODELSWITCH( var, min, max) \
//  var = checkIncDec(var,min,max,EE_MODEL|INCDEC_SWITCH)
//
//#define CHECK_INCDEC_GENERALSWITCH( var, min, max) \
//  var = checkIncDec(var,min,max,EE_GENERAL|INCDEC_SWITCH)
//#else
//#define CHECK_INCDEC_MODELSWITCH( var, min, max) \
//    var = checkIncDec_hm(var,min,max)
//
//#define CHECK_INCDEC_GENERALSWITCH( var, min, max) \
//    var = checkIncDec_hg(var,min,max)
//#endif
//#define STORE_MODELVARS_TRIM   eeDirty(EE_MODEL|EE_TRIM)
//#define STORE_MODELVARS   eeDirty(EE_MODEL)
//#define STORE_GENERALVARS eeDirty(EE_GENERAL)
//
////extern uint8_t Backlight ;
//extern volatile uint8_t LcdLock ;
//
//#define SPY_ON    //PORTB |=  (1<<OUT_B_LIGHT)
//#define SPY_OFF   //PORTB &= ~(1<<OUT_B_LIGHT)
//
//
//#ifdef CPUM2561
//#define PULSEGEN_ON     TIMSK1 |=  (1<<OCIE1A)
//#define PULSEGEN_OFF    TIMSK1 &= ~(1<<OCIE1A)
//#else
//#define PULSEGEN_ON     TIMSK |=  (1<<OCIE1A)
//#define PULSEGEN_OFF    TIMSK &= ~(1<<OCIE1A)
//#endif
//
//#define BITMASK(bit) (1<<(bit))
//
////#define PPM_CENTER 1200*2
////extern int16_t PPM_range ;
////extern uint16_t PPM_gap;
////extern uint16_t PPM_frame ;
//
///// liefert Dimension eines Arrays
//#define DIM(arr) (sizeof((arr))/sizeof((arr)[0]))
//
///// liefert Betrag des Arguments
//template<class t> inline t abs(t a){ return a>0?a:-a; }
///// liefert das Minimum der Argumente
//template<class t> inline t min(t a, t b){ return a<b?a:b; }
///// liefert das Maximum der Argumente
//template<class t> inline t max(t a, t b){ return a>b?a:b; }
//template<class t> inline int8_t sgn(t a){ return a>0 ? 1 : (a < 0 ? -1 : 0); }
//template<class t> inline t limit(t mi, t x, t ma){ return min(max(mi,x),ma); }
//
///// Markiert einen EEPROM-Bereich als dirty. der Bereich wird dann in
///// eeCheck ins EEPROM zurueckgeschrieben.
//void eeWriteBlockCmp(const void *i_pointer_ram, uint16_t i_pointer_eeprom, size_t size);
//void eeWaitComplete();
//void eeDirty(uint8_t msk);
//void eeCheck(bool immediately=false);
//void eeGeneralDefault();
//bool eeReadGeneral();
//void eeWriteGeneral();
////void eeReadAll();
//void eeLoadModelName(uint8_t id,char*buf,uint8_t len);
////uint16_t eeFileSize(uint8_t id);
//void eeLoadModel(uint8_t id);
////void eeSaveModel(uint8_t id);
//bool eeDuplicateModel(uint8_t id);
//bool eeModelExists(uint8_t id);
//
#define NUM_PPM     8
//number of real outputchannels CH1-CH16
#define NUM_CHNOUT  16
///number of real input channels (1-9) plus virtual input channels X1-X4
#define PPM_BASE    MIX_CYC3
#define CHOUT_BASE  (PPM_BASE+NUM_PPM)
//
//
//#define NUM_XCHNRAW (CHOUT_BASE+NUM_CHNOUT) // NUMCH + P1P2P3+ AIL/RUD/ELE/THR + MAX/FULL + CYC1/CYC2/CYC3
////#define NUM_XCHNRAW (CHOUT_BASE+NUM_CHNOUT+1) // NUMCH + P1P2P3+ AIL/RUD/ELE/THR + MAX/FULL + CYC1/CYC2/CYC3 +3POS
/////number of real output channels (CH1-CH8) plus virtual output channels X1-X4
//#define NUM_XCHNOUT (NUM_CHNOUT) //(NUM_CHNOUT)//+NUM_VIRT)
//
#define NUM_SCALERS	4
//
//#define MIX_3POS	(NUM_XCHNRAW+1)
//
////#define MAX_CHNRAW 8
///// Schreibt [RUD ELE THR AIL P1 P2 P3 MAX] aufs lcd
//void putsChnRaw(uint8_t x,uint8_t y,uint8_t idx1,uint8_t att);
////#define MAX_CHN 8
//
///// Schreibt [CH1 CH2 CH3 CH4 CH5 CH6 CH7 CH8] aufs lcd
//void putsChn(uint8_t x,uint8_t y,uint8_t idx1,uint8_t att);
///// Schreibt die Batteriespannung aufs lcd
//void putsVolts(uint8_t x,uint8_t y, uint8_t volts, uint8_t att);
//void putsVBat(uint8_t x,uint8_t y,uint8_t att);
//void putsTime(uint8_t x,uint8_t y,int16_t tme,uint8_t att,uint8_t att2);
//void putsDblSizeName( uint8_t y ) ;
//
//#ifdef FRSKY
//void putsTelemetry(uint8_t x, uint8_t y, uint8_t val, uint8_t unit, uint8_t att);
//uint8_t putsTelemValue(uint8_t x, uint8_t y, uint8_t val, uint8_t channel, uint8_t att, uint8_t scale) ;
//uint16_t scale_telem_value( uint16_t val, uint8_t channel, uint8_t times2, uint8_t *p_att ) ;
//#endif
//
//extern int16_t calc100toRESX(int8_t x) ;
////{
////    return ((x*41)>>2) - x/64;
////}
//
////uint8_t getMixerCount();
//bool reachMixerCountLimit();
//void menuMixersLimit(uint8_t event);
//
//extern int16_t calc1000toRESX(int16_t x) ;  // improve calc time by Pat MacKenzie
////{
////    int16_t y = x>>5;
////    x+=y;
////    y=y>>2;
////    x-=y;
////    return x+(y>>2);
////    //  return x + x/32 - x/128 + x/512;
////}
//
//extern volatile uint16_t g_tmr10ms;
//extern volatile uint8_t g8_tmr10ms;
//
//extern inline uint16_t get_tmr10ms()
//{
//    uint16_t time  ;
//    cli();
//    time = g_tmr10ms ;
//    sei();
//    return time ;
//}
//
//
//
//#define TMR_VAROFS  16
//
//#define SUB_MODE_V     1
//#define SUB_MODE_H     2
//#define SUB_MODE_H_DBL 3
////uint8_t checkSubGen(uint8_t event,uint8_t num, uint8_t sub, uint8_t mode);
//
//void menuProcLimits(uint8_t event);
//void menuProcMixOne(uint8_t event);
//void menuProcMix(uint8_t event);
//void menuProcCurve(uint8_t event);
//void menuProcTrim(uint8_t event);
//void menuProcExpoOne(uint8_t event);
//void menuProcExpoAll(uint8_t event);
//void menuProcModel(uint8_t event);
//void menuModelPhases(uint8_t event) ;
//void menuProcHeli(uint8_t event);
//void menuProcDiagCalib(uint8_t event);
//void menuProcDiagAna(uint8_t event);
//void menuProcDiagKeys(uint8_t event);
//void menuProcDiagVers(uint8_t event);
//void menuProcTrainer(uint8_t event);
//void menuProcSetup(uint8_t event);
//void menuProcMain(uint8_t event);
//void menuProcModelSelect(uint8_t event);
//void menuProcTemplates(uint8_t event);
//void menuProcSwitches(uint8_t event);
//void menuProcSafetySwitches(uint8_t event);
//#ifdef FRSKY
//void menuProcTelemetry(uint8_t event);
//void menuProcTelemetry2(uint8_t event);
//#endif
//
//void menuProcStatistic2(uint8_t event);
//void menuProcStatistic(uint8_t event);
//void menuProc0(uint8_t event);
//void menuProcGlobals(uint8_t event) ;
//
//extern void setupPulses();
//
//void initTemplates();
//
//extern int16_t intpol(int16_t, uint8_t);
//
////extern uint16_t s_ana[8];
//extern uint16_t anaIn(uint8_t chan);
//extern int16_t calibratedStick[7];
//extern int8_t phyStick[4] ;
//extern int16_t ex_chans[NUM_CHNOUT];
//
//void getADC_osmp();
////void getADC_filt();
//
////void checkTHR();
//
//
//#ifdef JETI
//// Jeti-DUPLEX Telemetry
//extern uint16_t jeti_keys;
//#include "jeti.h"
//#endif
//
//#ifdef FRSKY
//// FrSky Telemetry
//#include "frsky.h"
//#endif
//
//#ifdef ARDUPILOT
//// ArduPilot Telemetry
//#include "ardupilot.h"
//#endif
//
//#ifdef NMEA
//// NMEA Telemetry
//#include "nmea.h"
//#endif
//
////extern TrainerData g_trainer;
////extern uint16_t           g_anaIns[8];
//extern uint8_t            g_vbat100mV;
//extern volatile uint8_t   g_blinkTmr10ms;
//extern uint8_t            g_beepCnt;
////extern uint8_t            g_beepVal[5];
////extern const PROGMEM char modi12x3[];
//extern union p2mhz_t pulses2MHz ;
extern volatile int16_t            g_ppmIns[8];
extern volatile uint8_t ppmInValid;
extern volatile int16_t            g_chans[NUM_CHNOUT];
//extern volatile uint8_t   tick10ms;
//
//extern int16_t BandGap ; // VccV ;
//extern uint8_t Ee_lock ;
//
//// Bit masks in Ee_lock
//#define EE_LOCK      1
//#define EE_TRIM_LOCK 2
//
//#include "lcd.h"
//extern const char stamp1[];
//extern const char stamp2[];
//extern const char stamp3[];
//extern const char stamp4[];
//extern const char stamp5[];
//#include "myeeprom.h"
//
//extern const prog_uchar APM s9xsplashMarker[] ;
//extern const prog_uchar APM s9xsplash[] ;
//
//extern const prog_char APM Str_telemItems[] ;
//extern const prog_int8_t APM TelemIndex[] ;
//extern int16_t convertTelemConstant( int8_t channel, int8_t value) ;
//extern int16_t getValue(uint8_t i) ;
//
//#ifdef FRSKY
//#if defined(CPUM128) || defined(CPUM2561)
//#define NUM_TELEM_ITEMS 41
//#else
//#define NUM_TELEM_ITEMS 37
//#endif
//#else
//#define NUM_TELEM_ITEMS 10
//#endif
//
//#define FLASH_DURATION 50
//
////extern uint8_t  beepAgain;
//extern uint16_t g_LightOffCounter;
//
//struct t_inactivity
//{
//	uint8_t inacPrescale ;
//	uint16_t inacCounter ;
//	uint16_t inacSum ;
//} ;
//
//extern struct t_inactivity Inactivity ;
//
//#define STICK_SCROLL_TIMEOUT		9
//
//#define sysFLAG_OLD_EEPROM (0x01)
//extern uint8_t sysFlags;
//
////audio settungs are external to keep out clutter!
//#include "audio.h"
//
//extern uint8_t CurrentVolume ;
//
//extern void setVolume( uint8_t value ) ;
//extern void putVoiceQueue( uint8_t value ) ;
//extern void putVoiceQueueLong( uint16_t value ) ;
//extern void	putVoiceQueueUpper( uint8_t value ) ;
//void voice_numeric( int16_t value, uint8_t num_decimals, uint8_t units_index ) ;
//extern void voice_telem_item( int8_t index ) ;
//
//struct t_alarmControl
//{
//	uint8_t AlarmTimer ;		// Units of 10 mS
////	uint8_t AlarmCheckFlag ;
//	uint8_t OneSecFlag ;
//	uint8_t VoiceFtimer ;		// Units of 10 mS
//	uint8_t VoiceCheckFlag ;
//} ;
//extern struct t_alarmControl AlarmControl ;
//
//NOINLINE void resetTimer1(void) ;
//NOINLINE int16_t getTelemetryValue( uint8_t index ) ;
//
//// Fiddle to force compiler to use a pointer
//#ifndef SIMU
//#define FORCE_INDIRECT(ptr) __asm__ __volatile__ ("" : "=e" (ptr) : "0" (ptr))
//#else
//#define FORCE_INDIRECT(ptr)
//#endif
//
//#ifdef PHASES
//extern uint8_t getFlightPhase( void ) ;
//extern int16_t getRawTrimValue( uint8_t phase, uint8_t idx ) ;
//extern int16_t getTrimValue( uint8_t phase, uint8_t idx ) ;
//extern void setTrimValue(uint8_t phase, uint8_t idx, int16_t trim) ;
//#endif
//
//extern uint8_t StickScrollAllowed ;
//
//extern uint8_t telemItemValid( uint8_t index ) ;
//extern uint8_t Main_running ;
//extern const prog_char *AlertMessage ;
//extern int16_t m_to_ft( int16_t metres ) ;
//
//uint8_t getCurrentSwitchStates( void ) ;
//
//// Rotary encoder movement states
//#define	ROTARY_MENU_LR		0
//#define	ROTARY_MENU_UD		1
//#define	ROTARY_SUBMENU_LR	2
//#define	ROTARY_VALUE			3
//
//extern uint8_t RotaryState ;		// Defaults to ROTARY_MENU_LR
//
//extern uint8_t Tevent ;
//
//#if GVARS
//extern int8_t REG(int8_t x, int8_t min, int8_t max) ;
//extern int8_t REG100_100(int8_t x) ;
//#endif
//
//extern uint16_t evalChkSum( void ) ;
//
//struct t_calib
//{
//	int16_t midVals[7];
//	int16_t loVals[7];
//	int16_t hiVals[7];
//	uint8_t idxState;
//} ;
//
//struct t_p1
//{
//	int16_t p1val ;
//	int16_t p1valdiff ;
//  int16_t p1valprev ;
//	int16_t p2valprev ;
//	int16_t p3valprev ;
//} ;
//
//extern struct t_p1 P1values ;
//
//union t_xmem
//{
////	struct MixTab s_mixTab[MAX_MIXERS+NUM_XCHNOUT+1] ;
//	struct t_calib Cal_data ;
//	char buf[sizeof(g_model.name)+5];
//	ExpoData texpoData[4] ;
////#if defined(CPUM128) || defined(CPUM2561)
////  uint8_t file_buffer[256];
////#else
////  uint8_t file_buffer[128];
////#endif
//} ;
//
//extern union t_xmem Xmem ;
//
//extern uint8_t CurrentPhase ;
//

struct t_latency
{
	uint8_t g_tmr1Latency_min ;
	uint8_t g_tmr1Latency_max ;
	uint16_t g_timeMain ;
} ;


#endif // art6_h

/*eof*/


