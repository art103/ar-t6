void gui_edit_int_ex2(int8_t var, int8_t min, int8_t max, char units[], LCD_FLAGS FLAGS, void (*EDITACTION)(int8_t))
{
    if (context.edit)
    {
        var = gui_int_edit((int)var, context.inc, min, max);
        if (EDITACTION) (*EDITACTION)(var);
    }
    lcd_write_int((int)var, context.op_item, FLAGS);
    if (units != 0)
        lcd_write_string(units, context.op_item, FLAGS_NONE);
}

void gui_edit_int_ex(int8_t var, int8_t min, int8_t max, char units[], void (*EDITACTION)(int8_t))
{
    gui_edit_int_ex2(var, min, max, units, FLAGS_NONE, EDITACTION);
}

void gui_edit_int(int8_t var, int8_t min, int8_t max)
{
    gui_edit_int_ex(var, min, max, 0, NULL);
}

void gui_edit_enum(int8_t var, int8_t min, int8_t max, char * const labesl[])
{
    if (context.edit)
        var = gui_int_edit(var, context.inc, min, max);
    lcd_write_string(labesl[var], context.op_item, FLAGS_NONE);
}

void gui_edit_str(char var[])
{
    prefill_string((char *)var, sizeof(var));
    if (!context.edit)
        lcd_write_string((char *)var, LCD_OP_SET, FLAGS_NONE);
    else
        gui_string_edit(&context, (char *)var, g_key_press);
}
