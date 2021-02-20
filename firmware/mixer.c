/*
 * Author: Richard Taylor <richard@artaylor.co.uk>
 *
 * Based on er9x: Erez Raviv <erezraviv@gmail.com>
 * Based on th9x -> http://code.google.com/p/th9x/
 * Original Author not known.
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

/* Description:
 *
 * This is where the magic happens.
 * The er9x based mixer logic is contained in here.
 *
 */

#include <stdlib.h>
#include <stdbool.h>

#include "system.h"
#include "tasks.h"

#include "art6.h"
#include "myeeprom.h"
#include "pulses.h"
#include "sticks.h"
#include "mixer.h"
#include "sound.h"
#include "keypad.h"
#include "strings.h"
//#include "lcd.h"

static int16_t trim_increment;
static void perOut(volatile int16_t *chanOut, uint8_t att);

/**
  * @brief  Initialise the mixer.
  * @note
  * @param  None
  * @retval None
  */
void mixer_init(void)
{
	// Coarse trim
	trim_increment = 10;
}

/**
  * @brief  The main mixer function
  * @note	This is called from the DMA completion routine,
  *         so keep stack usage and duration to a minimum!
  * @param  None.
  * @retval None.
  */
void mixer_update(void)
{
	// Input data is in stick_data[].
	// Values are scaled to +/- RESX

	// =================================
	// Output Channel Data
	// =================================
	perOut(g_chans, 0);
}

/**
  * @brief  Receive key presses and update the trim data
  * @note
  * @param  key: Which trim key was pressed.
  * @retval None
  */
void mixer_input_trim(KEYPAD_KEY key)
{
	uint8_t channel = 0;
	int8_t increment = 0;
	uint8_t endstop = 0;

	switch (key)
	{
	case KEY_CH1_UP:
		channel = 0; increment = 1;	break;
	case KEY_CH2_UP:
		channel = 1; increment = 1;	break;
	case KEY_CH3_UP:
		channel = 2; increment = 1;	break;
	case KEY_CH4_UP:
		channel = 3; increment = 1;	break;

	case KEY_CH1_DN:
		channel = 0; increment = -1; break;
	case KEY_CH2_DN:
		channel = 1; increment = -1; break;
	case KEY_CH3_DN:
		channel = 2; increment = -1; break;
	case KEY_CH4_DN:
		channel = 3; increment = -1; break;

	default:
		break;
	}

	if (increment > 0)
	{
		if (g_model.trim[channel] < MIXER_TRIM_LIMIT)
			g_model.trim[channel] += trim_increment;
		else
			endstop = 1;
	}
	else
	{
		if (g_model.trim[channel] > -MIXER_TRIM_LIMIT)
			g_model.trim[channel] -= trim_increment;
		else
			endstop = 1;
	}

	if (g_model.trim[channel] == 0)
		endstop = 1;

	if (endstop != 0)
	{
		keypad_cancel_repeat();
		sound_play_tone(500 + 250*g_model.trim[channel]/MIXER_TRIM_LIMIT, 200);
	}
	else
	{
		sound_play_tone(500 + 250*g_model.trim[channel]/MIXER_TRIM_LIMIT, 100);
	}
}

/**
  * @brief  Return the current value for the specified input.
  * @note
  * @param  stick: The input to return trim for
  * @retval int16_t trim value between +/- MIXER_TRIM_LIMIT.
  */
int16_t mixer_get_trim(STICK stick)
{
	if (stick >= STICK_R_H && stick <= STICK_L_H)
		return g_model.trim[stick];
	return 0;
}

// Sticks that are scaled to +/- RESX and in the correct mode order.
static int16_t calibratedStick[STICK_ADC_CHANNELS];
static int16_t anas[NUM_XCHNRAW];
static int32_t chans[NUM_CHNOUT];
static int16_t ex_chans[NUM_CHNOUT]; // Outputs + intermidiates
static uint8_t swOn[MAX_MIXERS] = {0};
static int32_t act[MAX_MIXERS] = {0};
static uint16_t sDelay[MAX_MIXERS] = {0};

// Inactivity Timer
static uint8_t inacPrescale;
static uint16_t inacCounter = 0;
static uint16_t inacSum = 0;

static uint8_t  bpanaCenter = 0;

static uint8_t stickMoved = 0;

static int8_t *TrimPtr[4] =
{
    &g_model.trim[0],
    &g_model.trim[1],
    &g_model.trim[2],
    &g_model.trim[3]
};


static uint16_t isqrt32(uint32_t n)
{
    uint16_t c = 0x8000;
    uint16_t g = 0x8000;

    for(;;) {
        if((uint32_t)g*g > n)
            g ^= c;
        c >>= 1;
        if(c == 0)
            return g;
        g |= c;
    }
    return 0;
}

uint16_t expou(uint16_t x, uint16_t k)
{
    // k*x*x*x + (1-k)*x
    return ((unsigned long)x*x*x/0x10000*k/(RESXul*RESXul/0x10000) + (RESKul-k)*x+RESKul/2)/RESKul;
}

// expo-funktion:
// ---------------
// kmplot
// f(x,k)=exp(ln(x)*k/10) ;P[0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20]
// f(x,k)=x*x*x*k/10 + x*(1-k/10) ;P[0,1,2,3,4,5,6,7,8,9,10]
// f(x,k)=x*x*k/10 + x*(1-k/10) ;P[0,1,2,3,4,5,6,7,8,9,10]
// f(x,k)=1+(x-1)*(x-1)*(x-1)*k/10 + (x-1)*(1-k/10) ;P[0,1,2,3,4,5,6,7,8,9,10]

int16_t expo(int16_t x, int16_t k)
{
    if(k == 0) return x;
    int16_t   y;
    bool    neg =  x < 0;
    if(neg)   x = -x;
    if(k<0){
        y = RESXu-expou(RESXu-x,-k);
    }else{
        y = expou(x,k);
    }
    return neg? -y:y;
}

static inline int16_t calc100toRESX(int8_t x)
{
	// ((int32_t)x * RESX)/100
    return ((x*41)>>2) - x/64;
}

static inline int16_t calc1000toRESX(int16_t x)  // improve calc time by Pat MacKenzie
{
    int16_t y = x>>5;
    x+=y;
    y=y>>2;
    x-=y;
    return x+(y>>2);
    //  return x + x/32 - x/128 + x/512;
}

int16_t intpol(int16_t x, uint8_t idx) // -100, -75, -50, -25, 0 ,25 ,50, 75, 100
{
 #define D9 (RESX * 2 / 8)
 #define D5 (RESX * 2 / 4)
    bool    cv9 = idx >= MAX_CURVE5;
    int8_t *crv = cv9 ? g_model.curves9[idx-MAX_CURVE5] : g_model.curves5[idx];
    int16_t erg;

    x+=RESXu;
    if(x < 0) {
        erg = (int16_t)crv[0] * (RESX/4);
    } else if(x >= (RESX*2)) {
        erg = (int16_t)crv[(cv9 ? 8 : 4)] * (RESX/4);
    } else {
        int16_t a,dx;
        if(cv9){
            a   = (uint16_t)x / D9;
            dx  =((uint16_t)x % D9) * 2;
        } else {
            a   = (uint16_t)x / D5;
            dx  = (uint16_t)x % D5;
        }
        erg  = (int16_t)crv[a]*((D5-dx)/2) + (int16_t)crv[a+1]*(dx/2);
    }
    return erg / 25; // 100*D5/RESX;
}

const int16_t stick_map[4][4] = { // define some constants ???
    {0, 2, 1, 3}, // mode 1 
    {0, 1, 2, 3}, // mode 2
    {3, 2, 1, 0}, // mode 3 
    {3, 1, 2, 0}, // mode 4 
};

// translate logical stick numbers to physical acording to mode
// src can be sticks or mix_src (sticks only)
// log is number from mix_src variable - sticks are 1-4 
int16_t log2physSticks(int16_t log, int16_t mode){
    if (log > 0 && log < 5) {
        return stick_map[mode-1][log-1];
    } else {
        return log-1;
    }
}

static void perOut(volatile int16_t *chanOut, uint8_t att)
{
    int16_t trimA[4];
    uint8_t anaCenter = 0;
    uint8_t mixWarning = 0;
    uint16_t d = 0;
    uint8_t i;
    static uint32_t last10ms = 0;
    uint8_t tick10ms;

    if (last10ms < system_ticks && (system_ticks % 10) == 0)
    {
    	tick10ms = 1;
    	last10ms = system_ticks;
    }
    else
    	tick10ms = 0;

    if(tick10ms)
    {
        uint16_t tsum = 0;
        for(i=0;i<4;i++) tsum += anas[i];
        if(abs((int16_t)(tsum-inacSum))>INACTIVITY_THRESHOLD){
            inacSum = tsum;
            stickMoved = 1;  // reset in perMain
        }
        if( (g_eeGeneral.inactivityTimer + 10) && (sticks_get_battery() > 49))
        {
            if (++inacPrescale > 15 )
            {
                inacCounter++;
                inacPrescale = 0 ;
            }
            uint16_t tsum = 0;
            for(i=0;i<4;i++) tsum += anas[i];
            if(stickMoved) inacCounter=0;
            if(inacCounter>((uint16_t)(g_eeGeneral.inactivityTimer+10)*(100*60/16)))
            {
                if((inacCounter&0x3)==1)
                {
                	sound_play_tune(AU_INACTIVITY);
                }
            }
        }
    }
    // mode depend stick numbers
    uint8_t ail_stick_m = log2physSticks(AIL_STICK+1, g_eeGeneral.stickMode);
    uint8_t ele_stick_m = log2physSticks(ELE_STICK+1, g_eeGeneral.stickMode);
    uint8_t thr_stick_m = log2physSticks(THR_STICK+1, g_eeGeneral.stickMode);

    { 
        // int8_t rud_stick_m = log2physSticks(RUD_STICK, g_eeGeneral.stickMode); // no need for the heli mix
    
        //===========Swash Ring================
        if(g_model.swashType)
        {
            uint32_t v = ((int32_t)(calibratedStick[ele_stick_m])*calibratedStick[ele_stick_m] +
                          (int32_t)(calibratedStick[ail_stick_m])*calibratedStick[ail_stick_m]);
            uint32_t q = (int32_t)(RESX)*g_model.swashRingValue/100;
            q *= q;
            if(v>q)
                d = isqrt32(v);
        }
        //===========Swash Ring================

        // Calc Sticks
        for(i=0; i<STICK_INPUT_CHANNELS; i++)
        {
            // Stick_data already normalized: [0..2048] -> [-1024..1024]
            int16_t v = stick_data[i];

            if ( g_eeGeneral.throttleReversed )
            {
                if ( i == thr_stick_m )
                {
                    v = -v ;
                }
            }

            calibratedStick[i] = v; //for show in expo

            if(!(v/64))
            	anaCenter |= 1<<i;


            if(i<STICKS_TO_TRIM)
            { //only do this for sticks

                //===========Trainer mode================
                if (!(att&NO_TRAINER) && g_model.traineron)
                {
                    TrainerMix* td = &g_eeGeneral.trainer.mix[i];
                    if (td->mode && keypad_get_switch(td->swtch))
                    {
                        uint8_t chStud = td->srcChn;
                        int16_t vStud  = (g_ppmIns[chStud]- g_eeGeneral.trainer.calib[chStud]) /* *2 */ ;
                        vStud /= 2 ;		// Only 2, because no *2 above
                        vStud *= td->studWeight ;
                        vStud /= 31 ;
                        vStud *= 4 ;
                        switch ((uint8_t)td->mode) {
                        case MLTPX_ADD: v += vStud;   break; // add-mode
                        case MLTPX_REP: v  = vStud;   break; // subst-mode
                        }
                    }
                }

                //===========Swash Ring================
                if(d && (i==ele_stick_m || i==ail_stick_m))
                    v = (int32_t)(v)*g_model.swashRingValue*RESX/((int32_t)(d)*100);
                //===========Swash Ring================

                uint8_t expoDrOn = GET_DR_STATE(i);
                uint8_t stkDir = v>0 ? DR_RIGHT : DR_LEFT;

                if(i == thr_stick_m && g_model.thrExpo){
                    v  = 2*expo((v+RESX)/2,g_model.expoData[i].expo[expoDrOn][DR_EXPO][DR_RIGHT]);
                    stkDir = DR_RIGHT;
                }
                else
                    v  = expo(v,g_model.expoData[i].expo[expoDrOn][DR_EXPO][stkDir]);

                int32_t x = (int32_t)v * (g_model.expoData[i].expo[expoDrOn][DR_WEIGHT][stkDir]+100)/100;
                v = (int16_t)x;
                if (i == thr_stick_m && g_model.thrExpo) v -= RESX;

                //do trim -> throttle trim if applicable
                int32_t vv = 2*RESX;
				if(i == thr_stick_m && g_model.thrTrim)
				{
					int8_t ttrim ;
					ttrim = *TrimPtr[i] ;
					if(g_eeGeneral.throttleReversed)
					{
						ttrim = -ttrim ;
					}
					vv = ((int32_t)ttrim+125)*(RESX-v)/(2*RESX);
				}
                // if(i == thr_stick_m && g_model.thrTrim) vv = ((int32_t)*TrimPtr[i]+125)*(RESX-v)/(2*RESX);

                //trim
                trimA[i] = (vv==2*RESX) ? *TrimPtr[i]*2 : (int16_t)vv*2; //    if throttle trim -> trim low end
            }
            anas[i] = v; //set values for mixer
        }

        //===========BEEP CENTER================
        anaCenter &= g_model.beepANACenter;
        if(((bpanaCenter ^ anaCenter) & anaCenter)) sound_play_tune(AU_POT_STICK_MIDDLE);
        bpanaCenter = anaCenter;

        //===========setup rest of ANAS (input to mixer)================
        anas[MIX_MAX-1]  = RESX;     // MAX
        anas[MIX_FULL-1] = RESX;     // FULL
        for(i=0; i<STICK_INPUT_CHANNELS; i++) 		anas[i+PPM_BASE] = (g_ppmIns[i] - g_eeGeneral.trainer.calib[i])*2; //add ppm channels
        for(i=STICK_INPUT_CHANNELS; i<NUM_PPM; i++) anas[i+PPM_BASE]   = g_ppmIns[i]*2; //add ppm channels
        for(i=0; i<NUM_CHNOUT; i++) 				anas[i+CHOUT_BASE] = chans[i]; //other mixes previous outputs

        //===========Swash Ring================
        // recalculated swash ring values for ele and ail
        int32_t sw_ring_ele = anas[ele_stick_m];
        int32_t sw_ring_ail = anas[ail_stick_m];
        if(g_model.swashRingValue)
        {
            uint32_t v = ((int32_t)anas[ele_stick_m]*anas[ele_stick_m] + (int32_t)anas[ail_stick_m]*anas[ail_stick_m]);
            uint32_t q = (int32_t)RESX*g_model.swashRingValue/100;
            q *= q;
            if(v>q)
            {
                uint16_t d = isqrt32(v);
                sw_ring_ele = (int32_t)anas[ele_stick_m]*g_model.swashRingValue*RESX/((int32_t)d*100);
                sw_ring_ail = (int32_t)anas[ail_stick_m]*g_model.swashRingValue*RESX/((int32_t)d*100);
            }
        }

 #define REZ_SWASH_X(x)  ((x) - (x)/8 - (x)/128 - (x)/512)   //  1024*sin(60) ~= 886
 #define REZ_SWASH_Y(x)  ((x))   //  1024 => 1024

        if(g_model.swashType)
        {
            int16_t vp = sw_ring_ele + trimA[ele_stick_m];
            int16_t vr = sw_ring_ail + trimA[ail_stick_m];

            if(att&NO_INPUT)  //zero input for setStickCenter()
            {
                vp = vr = 0;
            }

            int16_t vc = 0;
            if(g_model.swashCollectiveSource)
                vc = anas[log2physSticks(g_model.swashCollectiveSource, g_eeGeneral.stickMode)];

            if(g_model.swashInvertELE) vp = -vp;
            if(g_model.swashInvertAIL) vr = -vr;
            if(g_model.swashInvertCOL) vc = -vc;

            switch (( uint8_t)g_model.swashType)
            {
            case (SWASH_TYPE_120):
                vp = REZ_SWASH_Y(vp);
                vr = REZ_SWASH_X(vr);
                anas[MIX_CYC1-1] = vc - vp;
                anas[MIX_CYC2-1] = vc + vp/2 + vr;
                anas[MIX_CYC3-1] = vc + vp/2 - vr;
                break;
            case (SWASH_TYPE_120X):
                vp = REZ_SWASH_X(vp);
                vr = REZ_SWASH_Y(vr);
                anas[MIX_CYC1-1] = vc - vr;
                anas[MIX_CYC2-1] = vc + vr/2 + vp;
                anas[MIX_CYC3-1] = vc + vr/2 - vp;
                break;
            case (SWASH_TYPE_140):
                vp = REZ_SWASH_Y(vp);
                vr = REZ_SWASH_Y(vr);
                anas[MIX_CYC1-1] = vc - vp;
                anas[MIX_CYC2-1] = vc + vp + vr;
                anas[MIX_CYC3-1] = vc + vp - vr;
                break;
            case (SWASH_TYPE_90):
                vp = REZ_SWASH_Y(vp);
                vr = REZ_SWASH_Y(vr);
                anas[MIX_CYC1-1] = vc - vp;
                anas[MIX_CYC2-1] = vc + vr;
                anas[MIX_CYC3-1] = vc - vr;
                break;
            default:
                break;
            }
        }
    }

    memset(chans,0,sizeof(chans));        // All outputs to 0

    if(att&NO_INPUT) { //zero input for setStickCenter()
        for(i=0;i<4;i++) {
            if(i != thr_stick_m) {
                anas[i]  = 0;
                trimA[i] = 0;
            }
        }
        for(i=0;i<4;i++) anas[i+PPM_BASE] = 0;
    }

    //========== MIXER LOOP ===============

    // Set the trim pointers back to the master set
    TrimPtr[0] = &g_model.trim[0] ;
    TrimPtr[1] = &g_model.trim[1] ;
    TrimPtr[2] = &g_model.trim[2] ;
    TrimPtr[3] = &g_model.trim[3] ;

    for(i=0; i<MAX_MIXERS; i++){
        MixData *md = &g_model.mixData[i];

        // first unused entry (channel==0) marks an end
        if((md->destCh==0) || (md->destCh>NUM_CHNOUT) || md->srcRaw == 0) break;

        /* srcRaw
        STK1..STK4
        VRA, VRB
        ??? non existent VRC/VRD
		MIX_MAX   8
		MIX_FULL  9
		MIX_CYC1  10
		MIX_CYC2  11
		MIX_CYC3  12
		CH1...CH8
		*/

        //Notice 0 = NC switch means not used -> always on line
        int16_t v  = 0;
        uint8_t swTog;

        //swOn[i]=false;
        if(!keypad_get_switch(md->swtch)) { // switch on?  if no switch selected => on
            swTog = swOn[i];
            swOn[i] = 0;
            //            if(md->srcRaw==MIX_MAX) act[i] = 0;// MAX back to 0 for slow up
            //            if(md->srcRaw!=MIX_FULL) continue;// if not FULL - next loop
            //            v = -RESX; // switch is off  => FULL=-RESX

            if(md->srcRaw!=MIX_MAX && md->srcRaw!=MIX_FULL) continue;// if not MAX or FULL - next loop
            if(md->mltpx==MLTPX_REP) continue; // if switch is off and REPLACE then off
            v = (md->srcRaw == MIX_FULL ? -RESX : 0); // switch is off and it is either MAX=0 or FULL=-512
        }
        else {
            swTog = !swOn[i];
            swOn[i] = 1;
            // uint8_t k = md->srcRaw-1;
            uint8_t k = log2physSticks(md->srcRaw, g_eeGeneral.stickMode); // function handles 0 as no input (returns  -1)
            v = anas[k]; //Switch is on. MAX=FULL=512 or value.
            if(k>=CHOUT_BASE && (k<i)) v = chans[k-CHOUT_BASE]; // if we've already calculated the value - take it instead // anas[i+CHOUT_BASE] = chans[i]
            if(md->mixWarn) mixWarning |= 1<<(md->mixWarn-1); // Mix warning
 #ifdef FMODE_TRIM
            if ( md->enableFmTrim )
            {
                if ( md->srcRaw <= 4 )
                {
                    TrimPtr[md->srcRaw-1] = &md->sOffset ;		// Use the value stored here for the trim
                }
            }
 #endif
        }

        //========== INPUT OFFSET ===============
 #ifdef FMODE_TRIM
        if ( md->enableFmTrim == 0 )
 #endif
        {
            if(md->sOffset) v += calc100toRESX(md->sOffset);
        }

        //========== DELAY and PAUSE ===============
        if (md->speedUp || md->speedDown || md->delayUp || md->delayDown)  // there are delay values
        {
 #define DEL_MULT 256

            //if(init) {
            //act[i]=(int32_t)v*DEL_MULT;
            //swTog = false;
            //}
            int16_t diff = v-act[i]/DEL_MULT;

            if(swTog) {
                //need to know which "v" will give "anas".
                //curves(v)*weight/100 -> anas
                // v * weight / 100 = anas => anas*100/weight = v
                if(md->mltpx==MLTPX_REP)
                {
                    act[i] = (int32_t)anas[md->destCh-1+CHOUT_BASE]*DEL_MULT;
                    act[i] *=100;
                    if(md->weight) act[i] /= md->weight;
                }
                diff = v-act[i]/DEL_MULT;
                if(diff) sDelay[i] = (diff<0 ? md->delayUp :  md->delayDown) * 100;
            }

            if(sDelay[i]){ // perform delay
                if(tick10ms)
                {
                  sDelay[i]-- ;
                }
                if (sDelay[i] > 0)
                { // At end of delay, use new V and diff
                  v = act[i]/DEL_MULT;   // Stay in old position until delay over
                  diff = 0;
                }
            }

            if(diff && (md->speedUp || md->speedDown)){
                //rate = steps/sec => 32*1024/100*md->speedUp/Down
                //act[i] += diff>0 ? (32768)/((int16_t)100*md->speedUp) : -(32768)/((int16_t)100*md->speedDown);
                //-100..100 => 32768 ->  100*83886/256 = 32768,   For MAX we divide by 2 sincde it's asymmetrical
                if(tick10ms) {
                    int32_t rate = (int32_t)DEL_MULT*2048*100;
                
                if(md->weight) rate /= abs(md->weight);

    // The next few lines could replace the long line act[i] = etc. - needs testing
    //										int16_t speed ;
    //                    if ( diff>0 )
    //										{
    //											speed = md->speedUp ;
    //										}
    //										else
    //										{
    //											rate = -rate ;
    //											speed = md->speedDown ;
    //										}
    //										act[i] = (speed) ? act[i]+(rate)/((int16_t)100*speed) : (int32_t)v*DEL_MULT ;

					act[i] = (diff>0) ? ((md->speedUp>0)   ? act[i]+(rate)/((int16_t)100*md->speedUp)   :  (int32_t)v*DEL_MULT) :
                                        ((md->speedDown>0) ? act[i]-(rate)/((int16_t)100*md->speedDown) :  (int32_t)v*DEL_MULT) ;
                }
								
				int32_t tmp = act[i]/DEL_MULT ;
                
                if(((diff>0) && (v<tmp)) || ((diff<0) && (v>tmp))) act[i]=(int32_t)v*DEL_MULT; //deal with overflow
								
                v = act[i]/DEL_MULT;
            }
            else if (diff)
            {
              act[i]=(int32_t)v*DEL_MULT;
            }
        }


        //========== CURVES ===============
        switch(md->curve){
        case 0: // symmetric (normal)
            break;
        case 1: // positive only
            if(md->srcRaw == MIX_FULL) //FUL
            {
                if( v<0 ) v=-RESX;   //x|x>0
                else      v=-RESX+2*v;
            }else{
                if( v<0 ) v=0;   //x|x>0
            }
            break;
        case 2: // negative only
            if(md->srcRaw == MIX_FULL) //FUL
            {
                if( v>0 ) v=RESX;   //x|x<0
                else      v=RESX+2*v;
            }else{
                if( v>0 ) v=0;   //x|x<0
            }
            break;
        case 3:       // x|abs(x)
            v = abs(v);
            break;
        case 4:       //f|f>0
            v = v>0 ? RESX : 0;
            break;
        case 5:       //f|f<0
            v = v<0 ? -RESX : 0;
            break;
        case 6:       //f|abs(f)
            v = v>0 ? RESX : -RESX;
            break;
        default: //c1..c8
            v = intpol(v, md->curve - 7);
        }


        //========== TRIM ===============
        if((md->carryTrim==0) && (md->srcRaw>0) && (md->srcRaw<=4)) v += trimA[md->srcRaw-1];  //  0 = Trim ON  =  Default

        //========== MULTIPLEX ===============
        int32_t dv = (int32_t)v*md->weight;
        // Save calculating address several times
        int32_t *ptr = &chans[md->destCh-1] ;
        switch((uint8_t)md->mltpx){
        case MLTPX_REP:
            *ptr = dv;
            break;
        case MLTPX_MUL:
        	dv /= 100 ;
			dv *= *ptr ;
            dv /= RESXl;
            *ptr = dv ;
        //    chans[md->destCh-1] *= dv/100l;
        //    chans[md->destCh-1] /= RESXl;
            break;
        default:  // MLTPX_ADD
            *ptr += dv; //Mixer output add up to the line (dv + (dv>0 ? 100/2 : -100/2))/(100);
            break;
        }
    }

    //========== MIXER WARNING ===============
    //1= 00,08
    //2= 24,32,40
    //3= 56,64,72,80
    {
        uint16_t tmr10ms ;
        tmr10ms = system_ticks / 10;

        if(mixWarning & 1) if(((tmr10ms&0xFF)==  0)) sound_play_tune(AU_MIX_WARNING_1);
        if(mixWarning & 2) if(((tmr10ms&0xFF)== 64) || ((tmr10ms&0xFF)== 72)) sound_play_tune(AU_MIX_WARNING_2);
        if(mixWarning & 4) if(((tmr10ms&0xFF)==128) || ((tmr10ms&0xFF)==136) || ((tmr10ms&0xFF)==144)) sound_play_tune(AU_MIX_WARNING_3);


    }

    //========== LIMITS ===============
    for(i=0; i<NUM_CHNOUT; i++) {
        // chans[i] holds data from mixer.   chans[i] = v*weight => 1024*100
    	// chans[i] value is -1024..1024 mapped here to full output range (limits)
        // later we multiply by the limit (up to 100) and then we need to normalize
        // at the end chans[i] = chans[i]/100 =>  -1024..1024
        // interpolate value with min/max so we get smooth motion from center to stop
        // this limits based on v original values and min=-1024, max=1024  RESX=1024

        int32_t q = chans[i];// + (int32_t)g_model.limitData[i].offset*100; // offset before limit

        ex_chans[i] = q/100; //for getswitch back to -1024..1024

        int16_t ofs = g_model.limitData[i].offset;
        int16_t lim_p = 10*(g_model.limitData[i].max);//+100);
        int16_t lim_n = 10*(g_model.limitData[i].min);//-100); //multiply by 10 to get same range as ofs (-1000..1000)
        if(ofs>lim_p) ofs = lim_p;
        if(ofs<lim_n) ofs = lim_n;

        if(q) q = (q>0) ?
                    q*((int32_t)lim_p-ofs)/100000 :
                    -q*((int32_t)lim_n-ofs)/100000 ; //div by 100000 -> output = -1024..1024

        q += calc1000toRESX(ofs);
        lim_p = calc1000toRESX(lim_p);
        lim_n = calc1000toRESX(lim_n);
        if(q>lim_p) q = lim_p;
        if(q<lim_n) q = lim_n;
        if(g_model.limitData[i].reverse) q=-q;// finally do the reverse.

        if(g_model.safetySw[i].opt.ss.swtch)  //if safety sw available for channel check and replace val if needed
            if(keypad_get_switch(1<<(g_model.safetySw[i].opt.ss.swtch-1)))
            	q = calc100toRESX(g_model.safetySw[i].opt.ss.val);

        chanOut[i] = q; //copy consistent word to int-level
    }
}

