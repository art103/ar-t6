	{
		// @formatter:off
		static MenuContext return_context = { 0 };
		static uint8_t sub_edit_item;

		#define MAX_INC_CNT 10 // ignore 0
		#define INC_CNT_SPEEDUP 4
		#define MAX_TIME_SPEEDUP 800 // speedup is erased after x system_ticks
		static uint8_t inc_cnt = MAX_INC_CNT; // used for speed up the increment
		static int32_t last_inc = 0;
		static uint32_t same_inc_time = 0;

		// Clear the screen.
		lcd_draw_rect(0, 0, LCD_WIDTH - 1, LCD_HEIGHT - 1, LCD_OP_CLR, RECT_FILL);
		lcd_set_cursor(0, 0);

		context.edit = 0;
		context.inc = 0;
		if (g_key_press & KEY_LEFT) {
			if (context.menu_mode == MENU_MODE_PAGE) {
				if (context.page > 0) {
					context.page--;
					context.item = 0;
					context.top_row = 0;
					context.form = 0;
				}
			} else if (context.menu_mode == MENU_MODE_LIST) {
				if (context.item > 0) {
					context.item--;
				}
			} else if (context.menu_mode == MENU_MODE_COL) {
				if (context.col > 0) {
					context.col--;
				}
			} else {
				context.inc = -1;
			}
		} else if (g_key_press & KEY_RIGHT) {
			if (context.menu_mode == MENU_MODE_PAGE) {
				if (context.page < PAGE_LIMIT) {
					context.page++;
					context.item = 0;
					context.top_row = 0;
					context.form = 0;
				}
			} else if (context.menu_mode == MENU_MODE_LIST) {
				if (context.item < context.item_limit) {
					context.item++;
				}
			} else if (context.menu_mode == MENU_MODE_COL) {
				if (context.col < context.col_limit) {
					context.col++;
				}
			} else {
				context.inc = 1;
			}
		} else if (g_key_press & (KEY_SEL | KEY_OK)) {
			switch (context.menu_mode) {
			case MENU_MODE_PAGE:
				context.menu_mode = MENU_MODE_LIST;
				g_menu_mode_dir = 1;
				break;
			case MENU_MODE_LIST:
				if (context.submenu_page) {
					return_context = context;
					sub_edit_item = context.item;
					context.page = context.submenu_page;
					context.menu_mode = MENU_MODE_LIST;
					context.item = 0;
					context.top_row = 0;
				} else {
					if (context.col_limit == 0)
						context.menu_mode = MENU_MODE_EDIT;
					else {
						context.menu_mode = MENU_MODE_COL;
						context.col = 0;
					}
				}
				break;
			case MENU_MODE_COL:
				context.menu_mode = MENU_MODE_EDIT;
				break;
			case MENU_MODE_EDIT:
				if (context.col_limit == 0)
					context.menu_mode = MENU_MODE_LIST;
				else
					context.menu_mode = MENU_MODE_COL;
				break;
			default:
				break;
			}
		} else if (g_key_press & KEY_CANCEL) {
			if (return_context.page) {
				// we were in sub menu, so return (pop up the context)
				context = return_context;
				return_context.page = 0;
			} else {
				if (context.menu_mode > 0) {
					context.menu_mode--;
					if (context.col_limit == 0
						&& context.menu_mode == MENU_MODE_COL) {
						context.menu_mode = MENU_MODE_LIST;
					}
				} else {
					context.page = 0;
					context.item = 0;
					context.top_row = 0;
					gui_navigate(g_main_layout);
				}
			}
		} else if (g_key_press & KEY_MENU) {
			if (context.popup_menu) // menu popup
				gui_popup_select(context.popup_menu, context.popup_menu_first_line);
			if (context.popup) // OK / exit popup
				gui_popup(context.popup, 0); // timeout
		}

		// speed up the increment after MAX_INC_CNT of the same direction change
		// speed up reset after MAX_TIME_SPEEDUP system_ticks
		if (context.inc != 0) {
			if(system_ticks - same_inc_time > MAX_TIME_SPEEDUP) {
				inc_cnt = MAX_INC_CNT;
			}
			if (context.inc == last_inc) {
				same_inc_time = system_ticks;

				if (inc_cnt > 0) {
					inc_cnt--;
				}
				else {
					if (context.inc > 0) {
						context.inc = INC_CNT_SPEEDUP;
					} else {
						context.inc = -INC_CNT_SPEEDUP;
					}
				}
			} else {
				inc_cnt = MAX_INC_CNT;
				last_inc = context.inc;
			}
		}

		if (context.form)
			context.top_row = 0;
		else {
			if (context.item < context.top_row)
				context.top_row = context.item;
			if (context.item >= context.top_row + LIST_ROWS)
				context.top_row++;
		}

		// defaults - can be changed by particular menu later for next call of gui_process
		context.submenu_page = 0;
		context.col_limit = 0;
		context.popup_menu = 0;
		context.popup_menu_first_line = 0;

		// actual handling of particular layouts/menus

		if (g_current_layout == GUI_LAYOUT_SYSTEM_MENU) {
			/**********************************************************************
			 * System Menu
			 *
			 * This is the main system menu with 6 pages.
			 *
			 */

			lcd_write_string((char*) msg[GUI_HDG_RADIO_SETUP + context.page],
					LCD_OP_CLR, FLAGS_NONE);
			lcd_set_cursor(110, 0);
			lcd_write_int(context.page + 1,
					(context.menu_mode == MENU_MODE_PAGE) ?
							LCD_OP_CLR : LCD_OP_SET, FLAGS_NONE);
			lcd_write_string("/6",
					(context.menu_mode == MENU_MODE_PAGE) ?
							LCD_OP_CLR : LCD_OP_SET, FLAGS_NONE);

			/**********************************************************************
			 * System Menu Pages
			 */
			switch (context.page) {
			case SYS_PAGE_SETUP: {
				context.item_limit = SYS_MENU_LIST1_LEN - 1;
				context.popup_menu = GUI_MSG_OK_TO_PRESET_ALL;
				context.popup_menu_first_line = 4;
				char popupRes = gui_popup_get_result();
				if (popupRes == 5) {
					settings_preset_all();
				}

				for (uint8_t row = context.top_row;
						(row < context.top_row + LIST_ROWS)
								&& (row <= context.item_limit); ++row) {

					prepare_context_for_list_row(&context, row);

					lcd_write_string((char*) system_menu_list1[row],
							context.op_list, FLAGS_NONE);
					lcd_write_string(" ", LCD_OP_SET, FLAGS_NONE);

					switch (row) {
					GUI_CASE_OFS(0, 74, gui_edit_str(g_eeGeneral.ownerName))
					GUI_CASE_OFS(1, 92, {g_eeGeneral.beeperVal = gui_edit_enum(g_eeGeneral.beeperVal, BEEPER_SILENT, BEEPER_NORMAL, system_menu_beeper );})
					GUI_CASE_OFS(2, 110, {g_eeGeneral.volume = gui_edit_int_ex2(g_eeGeneral.volume, 0, 15, 0, ALIGN_RIGHT,sound_set_volume);} )
					GUI_CASE_OFS(3, 110, {g_eeGeneral.contrast = gui_edit_int_ex( g_eeGeneral.contrast, LCD_CONTRAST_MIN, LCD_CONTRAST_MAX, NULL, lcd_set_contrast );})
					GUI_CASE_OFS(4, 102, {g_eeGeneral.vBatWarn = gui_edit_int_ex2( g_eeGeneral.vBatWarn, BATT_MIN, BATT_MAX, "V", INT_DIV10, NULL );})
					GUI_CASE_OFS(5, 110, {g_eeGeneral.inactivityTimer = gui_edit_int_ex2( g_eeGeneral.inactivityTimer, 0, 99, "m", ALIGN_RIGHT, NULL );})
					GUI_CASE_OFS(6, 110, {g_eeGeneral.throttleReversed = gui_edit_enum(g_eeGeneral.throttleReversed, 0, 1, menu_on_off);})
					GUI_CASE_OFS(7, 110, {g_eeGeneral.minuteBeep = gui_edit_enum(g_eeGeneral.minuteBeep, 0, 1, menu_on_off);})
					GUI_CASE_OFS(8, 110, {g_eeGeneral.preBeep = gui_edit_enum(g_eeGeneral.preBeep, 0, 1, menu_on_off);}) // Beep Countdown
					GUI_CASE_OFS(9, 110, {g_eeGeneral.flashBeep = gui_edit_enum(g_eeGeneral.flashBeep, 0, 1, menu_on_off);}) // Flash on beep
					GUI_CASE_OFS(10, 110, {g_eeGeneral.lightSw = gui_edit_enum(g_eeGeneral.lightSw, 0, NUM_SWITCHES, switches);}) 

			/* how to with oposit switch direction ???
					case 10: // Light Switch
					{
						int8_t sw = g_eeGeneral.lightSw;
						lcd_set_cursor(110, context.cur_row_y);
						if (context.edit) {
							g_eeGeneral.lightSw = gui_int_edit(
									g_eeGeneral.lightSw, context.inc,
									-NUM_SWITCHES,
									NUM_SWITCHES);
						}
						if (sw < 0) {
							sw = -sw;
							lcd_write_string("!", context.op_item, FLAGS_NONE);
						}
						lcd_write_string((char*) switches[sw], context.op_item,
								FLAGS_NONE);
					}
						break;
			*/
					GUI_CASE_OFS(11, 110, {g_eeGeneral.blightinv = gui_edit_enum(g_eeGeneral.blightinv, 0, 1, menu_on_off);}) // what???

					GUI_CASE_OFS(12, 110, {g_eeGeneral.lightAutoOff = gui_edit_int_ex2( g_eeGeneral.lightAutoOff, 0, 99, "s", ALIGN_RIGHT, NULL );})
					GUI_CASE_OFS(13, 110, {g_eeGeneral.lightOnStickMove = gui_edit_enum(g_eeGeneral.lightOnStickMove, 0, 1, menu_on_off);})
					GUI_CASE_OFS(14, 110, {g_eeGeneral.disableSplashScreen = gui_edit_enum(g_eeGeneral.disableSplashScreen, 0, 1, menu_on_off);})
					GUI_CASE_OFS(15, 110, {g_eeGeneral.disableThrottleWarning = gui_edit_enum(g_eeGeneral.disableThrottleWarning, 0, 1, menu_on_off);})
					GUI_CASE_OFS(16, 110, {g_eeGeneral.disableSwitchWarning = gui_edit_enum(g_eeGeneral.disableSwitchWarning, 0, 1, menu_on_off);})

					case 17: // Default Sw
						lcd_set_cursor(104, context.cur_row_y);
						g_eeGeneral.switchWarningStates = gui_bitfield_edit(
								&context, "ABCD",
								g_eeGeneral.switchWarningStates, context.inc,
								g_key_press);
						break;
					GUI_CASE_OFS(18, 110, {g_eeGeneral.disableMemoryWarning = gui_edit_enum(g_eeGeneral.disableMemoryWarning, 0, 1, menu_on_off);})
					GUI_CASE_OFS(19, 110, {g_eeGeneral.disableAlarmWarning = gui_edit_enum(g_eeGeneral.disableAlarmWarning, 0, 1, menu_on_off);})
					//GUI_CASE_OFS(20, 110, gui_edit_enum(g_eeGeneral.enablePpmsim, 0, 1, menu_on_off))

					case 20: // Channel Order & Mode
						lcd_set_cursor(116, context.cur_row_y);
						gui_edit_int(g_eeGeneral.stickMode, 1, 4);
						
						lcd_set_cursor(0, context.cur_row_y);

						// ???
						{
							int j;
							for (j = STICK_R_H; j <= STICK_L_H; ++j) {
								gui_draw_stick_icon(j, j == g_eeGeneral.stickMode);
								lcd_write_char(' ', LCD_OP_SET, FLAGS_NONE);
							}
						}
						// ???
						break;
						/*
						 case 22: // Channel Order & Mode
						 if (context.edit)
						 g_eeGeneral.stickMode = gui_int_edit(g_eeGeneral.stickMode, context.inc, CHAN_ORDER_ATER, CHAN_ORDER_RETA);
						 lcd_write_int(g_eeGeneral.stickMode + 1, context.op_item, FLAGS_NONE);
						 lcd_write_string("   ", LCD_OP_SET, FLAGS_NONE);
						 break;
						 */
					}
				}
				break; // SYS_PAGE_SETUP
			}

			case SYS_PAGE_TRAINER: {
				context.item_limit = 6;
				context.col_limit = context.item != 5 ? 3 : 0;
				for (uint8_t row = context.top_row;
						(row < context.top_row + LIST_ROWS)
								&& (row <= context.item_limit); ++row) {

					// First row isn't editable
					if (context.item == 0)
						context.item = 1;

					prepare_context_for_list_row(&context, row);

					switch (row) {
					case 0:
						lcd_set_cursor(24, context.cur_row_y);
						lcd_write_string((char*) mix_mode_hdr, LCD_OP_SET,
								FLAGS_NONE);
						lcd_set_cursor(66, context.cur_row_y);
						lcd_write_string("% src sw", LCD_OP_SET, FLAGS_NONE);
						break;
					case 1:
					case 2:
					case 3:
					case 4:
						// Mode
						if (context.edit)
							switch (context.col) {
							case 0:
								g_eeGeneral.trainer.mix[row - 1].mode =
										gui_int_edit(
												g_eeGeneral.trainer.mix[row - 1].mode,
												context.inc, 0, 2);
								break;
							case 1:
								g_eeGeneral.trainer.mix[row - 1].studWeight =
										gui_int_edit(
												g_eeGeneral.trainer.mix[row - 1].studWeight,
												context.inc, -100, 100);
								break;
							case 2:
								g_eeGeneral.trainer.mix[row - 1].srcChn =
										gui_int_edit(
												g_eeGeneral.trainer.mix[row - 1].srcChn,
												context.inc, 0, 7);
								break;
							case 3:
								g_eeGeneral.trainer.mix[row - 1].swtch =
										gui_int_edit(
												g_eeGeneral.trainer.mix[row - 1].swtch,
												context.inc, -NUM_SWITCHES,
												NUM_SWITCHES);
							}

						lcd_write_string((char*) sticks[row - 1],
								context.op_list, FLAGS_NONE);
						lcd_write_string(" ", LCD_OP_SET, FLAGS_NONE);
						lcd_write_string(
								(char*) mix_mode[g_eeGeneral.trainer.mix[row - 1].mode],
								(context.col == 0) ?
										context.op_item : LCD_OP_SET,
								FLAGS_NONE);

						lcd_set_cursor(66, context.cur_row_y);
						lcd_write_int(
								g_eeGeneral.trainer.mix[row - 1].studWeight,
								(context.col == 1) ?
										context.op_item : LCD_OP_SET,
								ALIGN_RIGHT);

						// Source
						lcd_set_cursor(78, context.cur_row_y);
						lcd_write_string("ch", LCD_OP_SET, FLAGS_NONE);
						lcd_write_int(
								g_eeGeneral.trainer.mix[row - 1].srcChn + 1,
								(context.col == 2) ?
										context.op_item : LCD_OP_SET,
								FLAGS_NONE);

						// Switch
						{
							int8_t sw = g_eeGeneral.trainer.mix[row - 1].swtch;
							lcd_set_cursor(102, context.cur_row_y);
							if (sw < 0) {
								lcd_write_char('!', LCD_OP_SET, FLAGS_NONE);
								sw = -sw;
							}
							lcd_write_string((char*) switches[sw],
									(context.col == 3) ?
											context.op_item : LCD_OP_SET,
									FLAGS_NONE);
						}
						break;
					case 5:
						lcd_write_string("Multiplier ", context.op_list,
								FLAGS_NONE);
						if (context.edit)
							g_eeGeneral.PPM_Multiplier = gui_int_edit(
									g_eeGeneral.PPM_Multiplier, context.inc, 10,
									50);
						lcd_write_int(g_eeGeneral.PPM_Multiplier,
								context.op_item, INT_DIV10);
						break;
					case 6: {
						int j;
						lcd_write_string("Cal", context.op_list, FLAGS_NONE);
						for (j = 0; j < 4; ++j) {
							lcd_set_cursor(43 + j * 25, context.cur_row_y);
							lcd_write_int(
									g_ppmIns[j] - g_eeGeneral.trainer.calib[j],
									LCD_OP_SET, ALIGN_RIGHT | CHAR_CONDENSED);
						}

						if (context.edit
								&& (g_key_press & (KEY_OK | KEY_SEL))) {
							for (j = 0; j < 4; ++j) {
								g_eeGeneral.trainer.calib[j] = g_ppmIns[j];
							}
							context.menu_mode = MENU_MODE_LIST;
						}
					}
						break;
					}
				}
				break; // SYS_PAGE_TRAINER
			}

			case SYS_PAGE_VERSION: {
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

				// count to differ entering the bootloader so screen can get updated
				static uint8_t deffercount = 0;
				// FW upgrade popup
				context.popup_menu = GUI_MSG_FW_UPGRADE;
				context.popup_menu_first_line = 3;
				// check popup result
				char popupRes = gui_popup_get_result();
				if (popupRes == 4) {
					deffercount = 3;
				}
				if (deffercount > 0) {
					lcd_set_cursor(0, 5 * 8);
					lcd_write_string("!!!!!!!!!!!!!!!!!!!!!", LCD_OP_SET,
							TRAILING_SPACE | NEW_LINE);
					lcd_write_string("!!FIRMWARE UPGRADE!!!", LCD_OP_SET,
							TRAILING_SPACE | NEW_LINE);
					lcd_write_string("!!DO NOT INTERRUPT!!!", LCD_OP_SET,
							TRAILING_SPACE | NEW_LINE);
				}
				// defer bootloader so the scr can get updated
				// count down when count still non-zero
				if (deffercount > 0) {
					// call bootloader when count _reaches_ 0
					if (--deffercount == 0)
						enter_bootloader();
				}
				break; // SYS_PAGE_VERSION
			}

			case SYS_PAGE_DIAG: {
				uint8_t sw = keypad_get_switches();
				uint8_t i;
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
				// i++;

				break;// SYS_PAGE_DIAG
			}

			case SYS_PAGE_ANA: {
				context.op_list = LCD_OP_SET;
				context.item_limit = 1;

				if (context.menu_mode == MENU_MODE_EDIT
						|| context.menu_mode == MENU_MODE_LIST) {
					g_eeGeneral.vBatCalib = gui_int_edit(g_eeGeneral.vBatCalib,
							context.inc, 80, 120);
					context.op_list = LCD_OP_CLR;
				}

				for (uint8_t i = 0; i < STICK_ADC_CHANNELS; ++i) {
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
						lcd_write_int(sticks_get_battery(), context.op_list,
								INT_DIV10);
						lcd_write_string("v", context.op_list, FLAGS_NONE);
					}

				}
				break; // SYS_PAGE_ANA
			}

			case SYS_PAGE_CAL:
				lcd_set_cursor(5, 16);
				lcd_draw_message(msg[GUI_MSG_CAL_OK_START], LCD_OP_SET, 0, 0);

				if (g_update_type & UPDATE_STICKS) {
					gui_show_sticks();
				}

				if (g_key_press & KEY_OK) {
					context.page = 0;
					context.item = 0;
					context.top_row = 0;
					gui_navigate(GUI_LAYOUT_STICK_CALIBRATION);
				}
				break; // SYS_PAGE_CAL
			}
		} else // GUI_LAYOUT_MODEL_MENU
		{
			/**********************************************************************
			 * Model Menu
			 *
			 * This is the model editing menu with 10 pages.
			 *
			 */

			lcd_write_string((char*) msg[GUI_HDG_MODELSEL + context.page],
					LCD_OP_CLR, FLAGS_NONE);
			if (context.page == MOD_PAGE_SETUP)
				lcd_write_int(g_eeGeneral.currModel, LCD_OP_CLR, FLAGS_NONE);
			lcd_set_cursor(104, 0);
			lcd_write_int(context.page + 1,
					(context.menu_mode == MENU_MODE_PAGE) ?
							LCD_OP_CLR : LCD_OP_SET, ALIGN_RIGHT);
			lcd_set_cursor(110, 0);
			lcd_write_string("/10",
					(context.menu_mode == MENU_MODE_PAGE) ?
							LCD_OP_CLR : LCD_OP_SET, FLAGS_NONE);

			/**********************************************************************
			 * System Menu Pages
			 */
			switch (context.page) {
			case MOD_PAGE_SELECT: {
				context.item_limit = MAX_MODELS - 1;
				context.popup_menu = GUI_MSG_OK_TO_PRESET_MODEL;
				context.popup_menu_first_line = 3;
				for (uint8_t row = context.top_row;
						(row < context.top_row + LIST_ROWS)
								&& (row <= context.item_limit); ++row) {
					prepare_context_for_list_row(&context, row);

					lcd_write_string(g_eeGeneral.currModel == row ? "* " : "  ",
							context.op_list, FLAGS_NONE);
					char name[MODEL_NAME_LEN];
					name[0] = '0' + row / 10;
					name[1] = '0' + row % 10;
					name[2] = ' ';
					name[3] = 0;
					lcd_write_string(name, context.op_list, FLAGS_NONE);
					settings_read_model_name(row, name);
					lcd_write_string(name, context.op_list, FLAGS_NONE);
					lcd_write_string(" ", LCD_OP_SET, FLAGS_NONE);
				}
				// check if popup was finished, if so the result would show up
				char popupRes = gui_popup_get_result();
				if (popupRes > 0) {
					// "preset" popup was answered "OK" so preset the model
					if (popupRes == 4) {
						g_eeGeneral.currModel = context.item;
						settings_preset_current_model();
					}
				} else {
					/*if (context.menu_mode == MENU_MODE_LIST) {
						if (g_key_press & KEY_MENU) {
							g_eeGeneral.currModel = context.item;
							gui_popup(GUI_MSG_OK_TO_RESET_MODEL, 0);
						}
					}*/
					if (context.menu_mode == MENU_MODE_EDIT) {
						// select the model to use
						g_eeGeneral.currModel = context.item;
						// pop up from model selection to PAGE nav
						context.menu_mode = MENU_MODE_PAGE;
						gui_navigate(GUI_LAYOUT_MODEL_MENU);
					}
				}
			}
				break;

			case MOD_PAGE_SETUP:
				context.item_limit = MOD_MENU_LIST1_LEN - 1;
				FOREACH_ROW
				{
					lcd_write_string((char*) model_menu_list1[row],
							context.op_list, FLAGS_NONE);
					lcd_write_string(" ", LCD_OP_SET, FLAGS_NONE);
					switch (row) {
						GUI_CASE_OFS(0, 74, gui_edit_str(g_model.name))
						GUI_CASE_OFS(1, 90, {g_model.tmrMode = gui_edit_enum( g_model.tmrMode, 0, 3, timer_modes );})
						GUI_CASE_OFS(2, 96, {g_model.tmrDir = gui_edit_enum( g_model.tmrDir, 0, 1, dir_labels );})
						GUI_CASE_OFS(3, 96, {g_model.tmrVal = gui_edit_int_ex( g_model.tmrVal, 0, 3600, "s", NULL );})
						GUI_CASE_OFS(4, 96, {g_model.traineron = gui_edit_enum( g_model.traineron, 0, 1, menu_on_off );})
						GUI_CASE_OFS(5, 96, {g_model.thrTrim = gui_edit_enum( g_model.thrTrim, 0, 1, menu_on_off );})
						GUI_CASE_OFS(6, 96, {g_model.thrExpo = gui_edit_enum( g_model.thrExpo, 0, 1, menu_on_off );})
						GUI_CASE_OFS(7, 96, {g_model.trimInc = gui_edit_int( g_model.trimInc, 0, 7 );})
						GUI_CASE_OFS(8, 96, {g_model.extendedLimits = gui_edit_enum( g_model.extendedLimits, 0, 1, menu_on_off );})
						GUI_CASE_OFS(9, 96, {g_model.ppmNCH = gui_edit_int( g_model.ppmNCH, 1, NUM_CHNOUT );})
						GUI_CASE_OFS(10, 96, {g_model.ppmDelay = gui_edit_int( g_model.ppmDelay, 0, 7 );})
						GUI_CASE_OFS(11, 96, {g_model.ppmFrameLength = gui_edit_int( g_model.ppmFrameLength, 0, 7 );})
						GUI_CASE_OFS(12, 90, {
						g_model.beepANACenter = gui_bitfield_edit(
							&context, "123456",
							g_model.beepANACenter, context.inc,
							g_key_press);
						}
						);
					}
				}
				break;

			case MOD_PAGE_HELI_SETUP:
				context.item_limit = HELI_MENU_LIST_LEN-1;
				FOREACH_ROW
				{ 
					lcd_write_string((char*)heli_menu_list[row], context.op_list, FLAGS_NONE);

					switch (row) {
						GUI_CASE_OFS(0, 90, {g_model.swashType = gui_edit_enum(g_model.swashType, 0, SWASH_TYPE_MAX-1, swash_type_labels);})
						GUI_CASE_OFS(1, 102, {g_model.swashRingValue = gui_edit_int_ex2(g_model.swashRingValue, 0, 100, 0, ALIGN_RIGHT,NULL);})
						GUI_CASE_OFS(2, 90, {g_model.swashCollectiveSource = gui_edit_enum(g_model.swashCollectiveSource, 0, MIX_SRCS_MAX-1, mix_src);})
						GUI_CASE_OFS(3, 90, {g_model.swashInvertELE = gui_edit_enum(g_model.swashInvertELE, 0, 1, inverse_labels);})
						GUI_CASE_OFS(4, 90, {g_model.swashInvertAIL = gui_edit_enum(g_model.swashInvertAIL, 0, 1, inverse_labels);})
						GUI_CASE_OFS(5, 90, {g_model.swashInvertCOL = gui_edit_enum(g_model.swashInvertCOL, 0, 1, inverse_labels);})
					}
				}

				break;

			case MOD_PAGE_EXPODR: {
				static uint8_t curr_chan = 0;
				if (curr_chan < 0)
					curr_chan = 0;
				if (curr_chan >= DIM(g_model.expoData))
					curr_chan = DIM(g_model.expoData) - 1;
				ExpoData* ed = &g_model.expoData[curr_chan];
				uint8_t dr = GET_DR_STATE(curr_chan);
				context.form = 1;
				context.item_limit = 7 - 1; // 7 fields all together
				// print labels
				for (int row = 0; row < 7; row++) {
					prepare_context_for_list_row(&context, row);
					lcd_write_string(expodr[row], LCD_OP_SET, TRAILING_SPACE);
					if (row == 1)
						lcd_write_string(drlevel[dr], LCD_OP_SET, 0);
				}
				// print/edit fields
				for (int fld = 0; fld <= context.item_limit; fld++) {
					int8_t row = (fld == 1 || fld == 3) ? fld + 1 : fld;
					prepare_context_for_list_row(&context, row);
					prepare_context_for_field(&context, fld);
					switch (fld) {
					GUI_CASE_OFS(0, 4 * 6,
						{curr_chan = gui_edit_enum( curr_chan, 0, DIM(g_model.expoData), sticks );});
					GUI_CASE_OFS(1, 4 * 6,
						{ed->expo[dr][DR_EXPO][DR_RIGHT] = gui_edit_int_ex2(ed->expo[dr][DR_EXPO][DR_RIGHT], -100, 100, 0, ALIGN_RIGHT,NULL);});
					GUI_CASE_OFS(2, 8 * 6,
						{ed->expo[dr][DR_EXPO][DR_LEFT] = gui_edit_int_ex2(ed->expo[dr][DR_EXPO][DR_LEFT], -100, 100, 0, ALIGN_RIGHT,NULL);});
					GUI_CASE_OFS(3, 4 * 6,
						{ed->expo[dr][DR_WEIGHT][DR_RIGHT] = gui_edit_int_ex2(ed->expo[dr][DR_WEIGHT][DR_RIGHT], -100, 100, 0, ALIGN_RIGHT,NULL);});
					GUI_CASE_OFS(4, 8 * 6,
						{ed->expo[dr][DR_WEIGHT][DR_LEFT] = gui_edit_int_ex2(ed->expo[dr][DR_WEIGHT][DR_LEFT], -100, 100, 0, ALIGN_RIGHT,NULL);});
					GUI_CASE_OFS(5, 4 * 6,
						{ed->drSw1 = gui_edit_enum( ed->drSw1, 0, NUM_SWITCHES, switches);});
					GUI_CASE_OFS(6, 4 * 6,
						{ed->drSw2 = gui_edit_enum( ed->drSw2, 0, NUM_SWITCHES, switches);});
					}
				}
				// draw curve
				const int X0 = 9 * 6;
				const int Y0 = 1 * 8;
				const int W = LCD_WIDTH - X0;
				const int H = LCD_HEIGHT - Y0;
				//lcd_draw_line( X0, Y0+H/2, X0+W, Y0+H/2, LCD_OP_SET);
				for (int x = X0; x < X0 + W; x += 2)
					lcd_set_pixel(x, Y0 + H / 2, LCD_OP_SET);
				for (int y = Y0; y < Y0 + H; y += 2)
					lcd_set_pixel(X0 + W / 2, y, LCD_OP_SET);
				//lcd_draw_line( X0+W/2, Y0, X0+W/2, Y0+H, LCD_OP_SET);
				for (int v = -1024; v <= 1024; v += 2 * 1024 / W) {
					int e = expo(v,
							ed->expo[dr][DR_EXPO][v > 0 ? DR_RIGHT : DR_LEFT]);
					int y = -(e * H) / (2 * 1024) + Y0 + H / 2;
					int x = (v * W) / (2 * 1024) + X0 + W / 2;
					lcd_set_pixel(x, y, LCD_OP_SET);
				}
				break;
			}

			case MOD_PAGE_MIXER: {
				context.item_limit = MAX_MIXERS - 1;
				context.submenu_page = MOD_PAGE_MIX_EDIT;
				context.popup_menu = GUI_MSG_ROW_MENU;
				context.popup_menu_first_line = 1;
				FOREACH_ROW
				{
					MixData* const mx = &g_model.mixData[row];
					if ((mx->destCh == 0) || (mx->destCh > NUM_CHNOUT)) {
						context.item_limit = row - 1;
						if (context.item >= row - 1)
							context.item = row - 1;
						break;
					}

					if (row == 0
							|| (mx->destCh
									&& g_model.mixData[row - 1].destCh
											!= mx->destCh)) {
						char s[4] = "CH0";
						s[2] += mx->destCh;
						lcd_write_string(s, context.op_list, FLAGS_NONE);
					} else {
						lcd_write_string(mix_mode[mx->destCh ? mx->mltpx : 0],
								context.op_list, FLAGS_NONE);
					}
					lcd_set_cursor(4 * 6, context.cur_row_y);

					// TODO: mix_src must be changed according to stick modes!?
					lcd_write_string(mix_src[mx->srcRaw], LCD_OP_SET,
							FLAGS_NONE);
					lcd_set_cursor(10 * 6, context.cur_row_y);
					lcd_write_int(mx->weight, LCD_OP_SET, ALIGN_RIGHT);
					lcd_set_cursor(12 * 6, context.cur_row_y);
					//lcd_write_string(switches_mask[mx->swtch], LCD_OP_SET,FLAGS_NONE);
					gui_bitfield_edit( &context, "ABCD", mx->swtch, context.inc, 0);
				}
				// if we were in the popup then the result would show up, once
				char popupRes = gui_popup_get_result();
				if (popupRes) {
					context.copy_row = -1;
					// todo - handle selected line: Preset, Insert, Delete, Copy, Paste
					switch (popupRes) {
					// preset
					case 1:
						settings_preset_current_model_mixers();
						break;
						// insert (duplicate?)
					case 2:
						// make sure we are not pushing out the last row of the last outchan
						if (g_model.mixData[MAX_MIXERS - 1].destCh == 0
								|| g_model.mixData[MAX_MIXERS - 1].destCh
										== g_model.mixData[MAX_MIXERS - 2].destCh) {
							memmove(&g_model.mixData[context.item + 1],
									&g_model.mixData[context.item],
									(MAX_MIXERS - context.item - 1)
											* sizeof(MixData));
						}
						break;
						// delete
					case 3:
						// delete only if not removing the last of rows for given output channel (at least one must stay)
						if (g_model.mixData[context.item].destCh
								== g_model.mixData[context.item + 1].destCh
								|| (context.item > 0
										&& g_model.mixData[context.item - 1].destCh
												== g_model.mixData[context.item].destCh)) {
							memmove(&g_model.mixData[context.item],
									&g_model.mixData[context.item + 1],
									(MAX_MIXERS - context.item - 1)
											* sizeof(MixData));
						}
						break;
						// copy
					case 4:
						context.copy_row = context.item;
						break;
						// paste
					case 5:
						if (context.copy_row >= 0) {
							// copy the "copy" row but retain the destCh
							int ch = g_model.mixData[context.item].destCh;
							g_model.mixData[context.item] =
									g_model.mixData[context.copy_row];
							g_model.mixData[context.item].destCh = ch;
						}
						break;
					default:
						// canceled
						break;
					}
				}
			}
				break;

			case MOD_PAGE_LIMITS:
				context.item_limit = NUM_CHNOUT - 1;
				context.col_limit = 4 - 1;
				FOREACH_ROW
				{
					char s[4] = "CH1";
					s[2] += row;
					lcd_write_string(s, context.op_list, CHAR_NOSPACE);

					volatile LimitData* const p = &g_model.limitData[row];

					FOREACH_COL
					{
						switch (col) {
						GUI_CASE_OFS(0, (3 + 6 - 1) * 6 + 2,
								{p->offset = gui_edit_int_ex2(p->offset,-1000, 1000, 0 , INT_DIV10|ALIGN_RIGHT, NULL);})
							;
						GUI_CASE_OFS(1, (3 + 6 + 4 - 1) * 6 + 2,
								{p->min = gui_edit_int_ex2(p->min, -100, 100, 0, ALIGN_RIGHT, NULL);})
						GUI_CASE_OFS(2, (3 + 6 + 4 + 4 - 1) * 6 + 2,
								{p->max = gui_edit_int_ex2(p->max, -100, 100, 0, ALIGN_RIGHT,NULL);})
						GUI_CASE_OFS(3, (3 + 6 + 4 + 4 + 2 - 1) * 6 + 2,
								{p->reverse = gui_edit_enum(p->reverse, 0, 1, inverse_labels);})
						}
					}
				}
				break;

			case MOD_PAGE_CURVES:
				context.item_limit = MAX_CURVE5 + MAX_CURVE9 - 1;
				context.submenu_page = MOD_PAGE_CURVE_EDIT;
				FOREACH_ROW
				{
					char s[4] = "CV0";
					s[2] = '1' + row;
					lcd_write_string(s, context.op_list, TRAILING_SPACE);
					int8_t ncv = row >= MAX_CURVE5 ? 9 : 5;
					lcd_write_int(ncv, LCD_OP_SET, TRAILING_SPACE);
				}
				gui_draw_curve(context.item, -1);
				break;

			case MOD_PAGE_CUST_SW:
				// ToDo implement!
				lcd_set_cursor(2*CHAR_WIDTH, 3*CHAR_HEIGHT);
				lcd_write_string("ToDo...", LCD_OP_SET, FLAGS_NONE);
				break;

			case MOD_PAGE_SAFE_SW:
				context.item_limit = DIM(g_model.safetySw) - 1;
				context.col_limit = 3 - 1;
				FOREACH_ROW
				{
					char s[4] = "CH0";
					s[2] = '1' + row;
					lcd_write_string(s, context.op_list, TRAILING_SPACE);
					FOREACH_COL
					{
						SafetySwData* d = &g_model.safetySw[row];
						switch (col) {
						GUI_CASE_OFS(0, 4 * 6,
								{d->opt.ss.swtch = gui_edit_enum(d->opt.ss.swtch, 0, 4, switches);})
						GUI_CASE_OFS(1, 13 * 6,
								{d->opt.ss.val = gui_edit_int(d->opt.ss.val, -100, 100);})
						/* GUI_CASE_OFS(2, 9 * 6,
								gui_edit_enum(d->opt.ss.mode, 0, 3, safety_switch_mode_labels )) */
						}
					}
				}
				break;

			case MOD_PAGE_TEMPLATES:
				// ToDo: Implement!
				lcd_set_cursor(2*CHAR_WIDTH, 3*CHAR_HEIGHT);
				lcd_write_string("ToDo...", LCD_OP_SET, FLAGS_NONE);
				break;

				// SubMenus --- Not navigable through left / right scrolling.

			case MOD_PAGE_MIX_EDIT: {
				MixData* const mx = &g_model.mixData[sub_edit_item];
				// print label for this mix on header row (top)
				{
					char s[4] = "CH0";
					s[2] += mx->destCh;
					lcd_set_cursor(9 * 6, 0);
					lcd_write_string(s, LCD_OP_CLR, FLAGS_NONE);
				}

				context.item_limit = MIXER_EDIT_LIST1_LEN - 1;
				context.col_limit = 0;
				FOREACH_ROW
				{
					lcd_write_string((char*) mixer_edit_list1[row],
							context.op_list, FLAGS_NONE);
					lcd_write_string(" ", LCD_OP_SET, FLAGS_NONE);

					switch (row) {
					GUI_CASE_OFS(0, 96, {mx->srcRaw = gui_edit_enum( mx->srcRaw, 0, MIX_SRCS_MAX-1, mix_src );});
					GUI_CASE_OFS(1, 96, {mx->weight = gui_edit_int( mx->weight, -125, 125 );});
					GUI_CASE_OFS(2, 96, {mx->sOffset = gui_edit_int( mx->sOffset, -125, 125 );});
					GUI_CASE_OFS(3, 96, {mx->carryTrim = gui_edit_enum( mx->carryTrim, 0, 1, menu_on_off );});
					GUI_CASE_OFS(4, 96, {mx->curve = gui_edit_enum( mx->curve, 0, MIX_CURVE_MAX-1, mix_curve);});
					GUI_CASE_OFS(5, 96,
							{ mx->swtch = gui_bitfield_edit( &context, "ABCD", mx->swtch ,
									context.inc, g_key_press); }
					);
					// #6 phase
					GUI_CASE_OFS(7, 96, {mx->mixWarn = gui_edit_enum( mx->mixWarn, 0, 1, menu_on_off );});
					GUI_CASE_OFS(8, 96, {mx->mltpx = gui_edit_enum( mx->mltpx, 0, 3, mix_mode );});
					GUI_CASE_OFS(9, 96, {mx->delayUp = gui_edit_int( mx->delayUp, 0, 255 );});
					GUI_CASE_OFS(10, 96, {mx->delayDown = gui_edit_int( mx->delayDown, 0, 255 );});
					GUI_CASE_OFS(11, 96, {mx->speedUp = gui_edit_int( mx->speedUp, 0, 255 );});
					GUI_CASE_OFS(12, 96, {mx->speedDown = gui_edit_int( mx->speedDown, 0, 255 );});
					}
				}
				break;
			}

			case MOD_PAGE_CURVE_EDIT: {
				const int NPTS = sub_edit_item < MAX_CURVE5 ? 5 : 9;
				// print label
				{
					char s[4] = "CV1";
					s[2] = '1'+sub_edit_item;
					lcd_set_cursor(6 * 6, 0);
					lcd_write_string(s, LCD_OP_CLR, FLAGS_NONE);
				}
				context.item_limit = NPTS - 1;
				context.col_limit = 0;
				gui_draw_curve(sub_edit_item, context.item);

				int8_t* pCurve =
						sub_edit_item < MAX_CURVE5 ?
								(int8_t*)&g_model.curves5[sub_edit_item] :
								(int8_t*)&g_model.curves9[sub_edit_item - MAX_CURVE5];

				FOREACH_ROW{
					lcd_write_int(row, context.op_list, FLAGS_NONE);
				}


				FOREACH_ROW{
					const int X = 5*6;
					switch (row) {
					GUI_CASE_OFS(0, X, {pCurve[0] = gui_edit_int_ex2( pCurve[0], -100, 100, "", ALIGN_RIGHT, NULL );});
					GUI_CASE_OFS(1, X, {pCurve[1] = gui_edit_int_ex2( pCurve[1], -100, 100, "", ALIGN_RIGHT, NULL );});
					GUI_CASE_OFS(2, X, {pCurve[2] = gui_edit_int_ex2( pCurve[2], -100, 100, "", ALIGN_RIGHT, NULL );});
					GUI_CASE_OFS(3, X, {pCurve[3] = gui_edit_int_ex2( pCurve[3], -100, 100, "", ALIGN_RIGHT, NULL );});
					GUI_CASE_OFS(4, X, {pCurve[4] = gui_edit_int_ex2( pCurve[4], -100, 100, "", ALIGN_RIGHT, NULL );});
					GUI_CASE_OFS(5, X, {pCurve[5] = gui_edit_int_ex2( pCurve[5], -100, 100, "", ALIGN_RIGHT, NULL );});
					GUI_CASE_OFS(6, X, {pCurve[6] = gui_edit_int_ex2( pCurve[6], -100, 100, "", ALIGN_RIGHT, NULL );});
					GUI_CASE_OFS(7, X, {pCurve[7] = gui_edit_int_ex2( pCurve[7], -100, 100, "", ALIGN_RIGHT, NULL );});
					GUI_CASE_OFS(8, X, {pCurve[8] = gui_edit_int_ex2( pCurve[8], -100, 100, "", ALIGN_RIGHT, NULL );});
					}
				}
				break;
			}
			} // switch (context.page)
		} // else // GUI_LAYOUT_MODEL_MENU
	}
