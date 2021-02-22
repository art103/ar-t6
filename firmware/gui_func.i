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

	// Play the key tone.
	if (g_eeGeneral.beeperVal > BEEPER_NOKEY)
		sound_play_tone(500, 10);

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
void gui_popup_select(GUI_MSG msg, uint8_t first_line) {
	g_new_msg = msg;
	g_current_msg = 0;
	g_popup_selected_line = first_line;
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
	BOX_L_X + BOX_W / 2 + 1, BOX_Y + BOX_H / 2 + 1, LCD_OP_SET, RECT_ROUNDED);
	lcd_draw_rect(BOX_R_X + BOX_W / 2 - 1, BOX_Y + BOX_H / 2 - 1,
	BOX_R_X + BOX_W / 2 + 1, BOX_Y + BOX_H / 2 + 1, LCD_OP_SET, RECT_ROUNDED);

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
static void gui_show_timer(int x, int y, int seconds) {
	// Timer
	lcd_set_cursor(x, y);
	if( seconds < 0 ) {
		lcd_write_string("-", LCD_OP_SET, CHAR_4X);
		seconds = -seconds;
	}
	else {
		lcd_write_string(" ", LCD_OP_SET, CHAR_4X);
	}

	lcd_write_int(seconds / 60, LCD_OP_SET, INT_PAD10 | CHAR_4X);
	lcd_write_string(":", LCD_OP_SET, CHAR_4X);
	lcd_write_int(seconds % 60, LCD_OP_SET, INT_PAD10 | CHAR_4X);
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
static void gui_draw_trim(int x, int y, uint8_t h_v, int value) {
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
static void gui_string_edit(MenuContext* pCtx, char *string, uint32_t keys) {
	static int8_t char_index = 0;
	static int8_t edit_mode = 1;
	int i;

	if (keys & KEY_SEL) {
		pCtx->menu_mode = MENU_MODE_EDIT_S;
		edit_mode = 1 - edit_mode;
	} else if (keys & (KEY_CANCEL | KEY_OK)) {
		char_index = 0;
		edit_mode = 1;
		pCtx->menu_mode = MENU_MODE_LIST;
	}

	if (edit_mode) {
		string[char_index] += pCtx->inc;
		if (string[char_index] < 32)
			string[char_index] = 32;
		if (string[char_index] > 127)
			string[char_index] = 127;
	} else {
		char_index += pCtx->inc;
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
static uint32_t gui_bitfield_edit(MenuContext* pCtx, char *string,
		uint32_t field, int8_t delta, uint32_t keys) {
	static int8_t char_index = 0;
	static int8_t edit_mode = 1;
	int i;
	uint32_t ret = field;
	uint16_t lcd_flags = FLAGS_NONE;

	if (pCtx->edit) {
		if (keys & KEY_SEL) {
			edit_mode = 1 - edit_mode;
			pCtx->menu_mode = MENU_MODE_EDIT_S;
		} else if (keys & (KEY_CANCEL | KEY_OK)) {
			char_index = 0;
			edit_mode = 1;
			pCtx->menu_mode = MENU_MODE_LIST;
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
static int32_t gui_int_edit(int32_t data, int32_t delta,
		                    int32_t min, int32_t max)
{
	int32_t ret = data;

	ret += delta;

	if (ret > max)
		ret = max;
	if (ret < min)
		ret = min;

	return ret;
}

/**
 * @brief  scale a value of curve (max,min) into (b,a) range
 * @param  min X
 * @param  max X
 * @param  a target min
 * @param  b target max
 * @param  x - point value
 * @param  selectedPoint a selected point on a curve (-1 for none)
 * @retval None
 */
static uint8_t gui_scale_curve_point(int8_t max, int8_t min,
		                             int8_t a, int8_t b, int8_t x)
{
	long coord = (((b - a) * (x - min)) / (max - min)) + a;
	return (int8_t) coord;
}


/**
 * @brief  Draw a curve
 * @param  curveNumber indext to curve in g_Model; 0..MAX_CURVES
 * @param  selectedPoint a selected point on a curve (-1 for none)
 * @retval None
 */
static void gui_draw_curve(uint8_t curveNumber, int8_t selectedPoint)
{
	int8_t* pCurve =
			curveNumber < MAX_CURVE5 ?
					(int8_t*)&g_model.curves5[curveNumber] :
					(int8_t*)&g_model.curves9[curveNumber - MAX_CURVE5];

	const uint8_t NPTS = curveNumber < MAX_CURVE5 ? 5 : 9;
	// draw curve
	const uint8_t min_y = 10;
	const uint8_t max_y = LCD_HEIGHT;
	const uint8_t min_x = 7 * 6; // 7 chars to the right
	const uint8_t max_x = LCD_WIDTH;
	const int16_t W = max_x - min_x;
	const int16_t H = max_y - min_y;

	const int16_t MX = W / 2 + min_x;
	const int16_t MY = H / 2 + min_y;

	for (int8_t x = -W / 2; x < W / 2; x += 2) {
		//lcd_draw_line( x-1+MX, MY, x+1+MX, MY, LCD_OP_SET);
		lcd_set_pixel(x + MX, MY, LCD_OP_SET);
	}
	for (int8_t y = -H / 2; y < H / 2; y += 2) {
		//lcd_draw_line( MX, y-1+MY, MX, y+1+MY, LCD_OP_SET);
		lcd_set_pixel(MX, y + MY, LCD_OP_SET);
	}
	const uint8_t xstep = W / NPTS;
	//lcd_draw_line(min_x+xstep*NPTS/2, max_y, min_x+xstep*NPTS/2, min_y, LCD_OP_SET);

	for (int8_t pt = 0, yprev; pt < NPTS; pt++) {
		int8_t x = (pt - NPTS / 2) * xstep + MX;
		int8_t y = gui_scale_curve_point(-100, 100, -H / 2, H / 2, pCurve[pt])
				+ MY;
		//lcd_set_pixel(x, y, LCD_OP_SET);
		const int R = 1;
		lcd_draw_rect(x - R, y - R, x + R, y + R, LCD_OP_SET, FLAGS_NONE);
		if (pt > 0)
			lcd_draw_line(x - xstep, yprev, x, y, LCD_OP_SET);
		yprev = y;
	}
	if (selectedPoint >= 0) {
		int8_t x = (selectedPoint - NPTS / 2) * xstep + MX;
		int8_t y = gui_scale_curve_point(-100, 100, -H / 2, H / 2,
				pCurve[selectedPoint]) + MY;
		const int R = 3;
		lcd_draw_rect(x - R, y - R, x + R, y + R, LCD_OP_SET, FLAGS_NONE);
	}
}

/**
 * @brief  Prefill string with space up to length
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
