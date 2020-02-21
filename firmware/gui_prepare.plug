/**
 * @brief  sets up menu context parameters for menu and given list row
 * @note
 * @param  pCtx pointer to menu context
 * @param  row list row we are working on
 * @param  col list col we are working on
 * @retval None
 */
static void prepare_context_for_list_rowcol(
		MenuContext* pCtx, uint8_t row,
		uint8_t col) {
	if (pCtx->col_limit == 0)
		pCtx->col = 0;
	pCtx->cur_row_y = (row - pCtx->top_row + 1) * 8;
	pCtx->op_list = LCD_OP_SET;
	pCtx->op_item = LCD_OP_SET;
	pCtx->edit = 0;
	if (row == pCtx->item) {
		switch (pCtx->menu_mode) {
		case MENU_MODE_LIST:
			pCtx->op_list = LCD_OP_CLR;
			break;
		case MENU_MODE_COL:
			if (pCtx->col == col)
				pCtx->op_item = LCD_OP_CLR;
			break;
		case MENU_MODE_EDIT:
		case MENU_MODE_EDIT_S:
			if (pCtx->col == col || pCtx->col_limit == 0 || col == COL_IGNORE) {
				pCtx->edit = 1;
				pCtx->op_item = LCD_OP_CLR;
			}
			break;
		default:
			break;
		}
	}

	lcd_set_cursor(0, pCtx->cur_row_y);
}

/**
 * @brief  sets up menu context parameters for menu and given list row in no col context
 * @note
 * @param  pCtx pointer to menu context
 * @param  row list row we are working on
 * @retval None
 */
static void prepare_context_for_list_row(MenuContext* pCtx, uint8_t row) {
	pCtx->col_limit = 0;
	prepare_context_for_list_rowcol(pCtx, row, 0);
}

/**
 * @brief  sets up menu context parameters for a given field we are on
 * @note
 * @param  pCtx pointer to menu context
 * @param  field field number
 * @retval None
 */
static void prepare_context_for_field(MenuContext* pCtx, uint8_t field) {
	pCtx->edit = 0;
	if (field == pCtx->item) {
		switch (pCtx->menu_mode) {
		case MENU_MODE_LIST:
		case MENU_MODE_EDIT:
		case MENU_MODE_EDIT_S:
			pCtx->edit = 1;
			pCtx->op_item = LCD_OP_CLR;
			break;
		default:
			break;
		}
	}
}
