///////////////////////////////////////////////////////////////////////////////
// TODO: timers deserve separate module

static uint32_t minute_tick = 0;
static uint32_t timer_tick = 0;
static int16_t timer = 0; // timer in seconds displayed value
static int16_t second = 0;
static uint8_t thr_stick_m = 0;
static uint32_t time_multiplier = 16;


static void timer_update() {
	time_multiplier = 16; // full speed timer (real time)
	
	// works only for "abs" timer
	if ( (g_update_type & UPDATE_KEYPRESS) != 0) {
		if  (g_model.tmrMode == 1   && (g_key_press & ( KEY_OK))) {
			timer_start_stop();
		} else if (g_key_press & (KEY_CANCEL )) {
			timer_restart();
		}
	} 
	
	if (g_model.tmrMode > 1) { // stk x
		thr_stick_m = log2physSticks(THR_STICK+1, g_eeGeneral.stickMode);
		if (g_model.tmrMode == 2 || g_model.tmrMode == 3) { // any Stk start/stop timer
			if (stick_data[thr_stick_m] < -900) { // stick_data[x]  -1024 .. +1024 
				timer_stop();
			} else {
				timer_start();
			}
		}
		if (g_model.tmrMode ==3) {
			time_multiplier = (stick_data[thr_stick_m]+1024) >> 7; // 0-2048 -> 0-16
		}
	}

	if (timer_tick != 0) {
		second += ((system_ticks - timer_tick) * time_multiplier) >> 4;
		if (second > 1000) {
			second = 0;
			
			timer += g_model.tmrDir ? 1 : -1;
			if (g_model.tmrDir == 0) // down
			{
				// reached timer val
				if( timer <= 0  )
					sound_play_tune(AU_INACTIVITY);
				// count down
				if( g_eeGeneral.preBeep ) {
					if( timer > 0 && timer  <= 10 )
					{
						sound_play_tone(700 - timer * 50, 30);
					}
				}
			}
		}
		timer_tick = system_ticks;
	}
	// one mninute beeps
	if( g_eeGeneral.minuteBeep && system_ticks - minute_tick > 1000*60 ) {
		minute_tick = system_ticks;
		sound_play_tone(800, 20);
		//sound_play_tune(AU_INACTIVITY);
	}
}

static void timer_start() {
	if (timer_tick == 0) timer_tick = system_ticks;
}

static void timer_stop() {
	timer_tick = 0;
}

static void timer_start_stop() {
	if (timer_tick == 0)
		timer_tick = system_ticks;
	else
		timer_tick = 0;
}


static void timer_restart() {
	timer = g_model.tmrDir ? 0 : g_model.tmrVal;
	//timer_tick = system_ticks;
}

static void backlight_management() {
	// if key g_eeGeneral.lightSw is on - light on
	// if key g_eeGeneral.lightSw is off - check g_eeGeneral.lightAutoOff 
	//  0- always on
	// > 0 No of seconds to light off - key_inactivity returns system ticks inactivity of keys
	// 
	lcd_backlight(
		(( g_eeGeneral.lightSw && (keypad_get_switches() & (0x01 << (g_eeGeneral.lightSw-1))) ) 	// light switch is defined and switched on
	     || (g_eeGeneral.lightAutoOff == 0)										  	// auto light off is zero - always lit on
		 || (key_inactivity() < (g_eeGeneral.lightAutoOff * 1000))					// last key activity before timeout
	   	)
	);
}
///////////////////////////////////////////////////////////////////////////////
