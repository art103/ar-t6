#define FOREACH_ROW \
for (int row = context.top_row;\
		prepare_context_for_list_rowcol(&context, row, COL_IGNORE),\
	   (row < context.top_row + LIST_ROWS) && (row <= context.item_limit);  \
	 ++row )

//#define FOREACH_ROW(BODY) FOREACH_ROW_BARE{ BODY }

#define FOREACH_COL \
for(int col = 0;\
	prepare_context_for_list_rowcol(&context, row, col),col <= context.col_limit;\
	++col )

// #define GUI_EDIT_INT_EX2( VAR, MIN, MAX, UNITS, FLAGS, EDITACTION ) \
// 		if (context.edit) { VAR = gui_int_edit((int)VAR, context.inc, MIN, MAX); EDITACTION; } \
// 		lcd_write_int((int)VAR, context.op_item, FLAGS); \
// 		if(UNITS != 0) lcd_write_string(UNITS, context.op_item, FLAGS_NONE);

// #define GUI_EDIT_INT_EX( VAR, MIN, MAX, UNITS, EDITACTION ) \
// 		GUI_EDIT_INT_EX2( VAR, MIN, MAX, UNITS, FLAGS_NONE, EDITACTION )

// #define GUI_EDIT_INT( VAR, MIN, MAX ) GUI_EDIT_INT_EX(VAR, MIN, MAX, 0, {})

// #define GUI_EDIT_ENUM( VAR, MIN, MAX, LABELS ) \
// 		if (context.edit) VAR = gui_int_edit(VAR, context.inc, MIN, MAX); \
// 		lcd_write_string(LABELS[VAR], context.op_item, FLAGS_NONE);

// #define GUI_EDIT_STR( VAR ) \
// 			prefill_string((char*)VAR, sizeof(VAR));\
// 			if (!context.edit) lcd_write_string((char*)VAR, LCD_OP_SET, FLAGS_NONE); \
// 			else       gui_string_edit(&context, (char*)VAR, g_key_press);

#define GUI_CASE( CASE, ACTION ) \
case CASE: \
	{ ACTION } \
	break; \

#define GUI_CASE_OFS( CASE, OFFSET, ACTION ) \
case CASE: \
	lcd_set_cursor(OFFSET, context.cur_row_y); \
	ACTION;   \
	break; \

// Message Popup
#define MSG_X	6
#define MSG_Y	8
#define MSG_H	46

// Stick boxes
#define BOX_W	22
#define BOX_H	22
#define BOX_Y	34
#define BOX_L_X	32
#define BOX_R_X	72
// POT bars
#define POT_W	2
#define POT_Y	(BOX_Y + BOX_H)
#define POT_L_X 59
#define POT_R_X 65
// timer value column
#define TIMER_X 26
// Switch Labels
#define SW_Y	(BOX_Y + 6)
#define SW_L_X	(BOX_L_X - 20)
#define SW_R_X	(BOX_R_X + BOX_W + 4)

#define LIST_ROWS	7

#define PAGE_LIMIT	((g_current_layout == GUI_LAYOUT_SYSTEM_MENU)?5:9)
#define COL_IGNORE 255
