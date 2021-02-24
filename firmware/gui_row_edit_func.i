int gui_edit_int_ex2(int var, int min, int max, char units[], LCD_FLAGS FLAGS, void (*EDITACTION)(int))
{
    if (context.edit)
    {
        var = gui_int_edit(var, context.inc, min, max);
        if (EDITACTION)
            (*EDITACTION)(var);
    }
    lcd_write_int(var, context.op_item, FLAGS);
    if (units != 0)
        lcd_write_string(units, context.op_item, FLAGS_NONE);
        return var;
}

int gui_edit_int_ex(int var, int min, int max, char units[], void (*EDITACTION)(int))
{
    return gui_edit_int_ex2(var, min, max, units, FLAGS_NONE, EDITACTION);
}

int gui_edit_int(int var, int min, int max)
{
    return gui_edit_int_ex(var, min, max, 0, NULL);
}

int gui_edit_enum(int var, int min, int max, char *const labesl[])
{
    if (context.edit)
        var = gui_int_edit(var, context.inc, min, max);
    lcd_write_string(labesl[var], context.op_item, FLAGS_NONE);
    return var;
}

void gui_edit_str(char* var)
{
    // sizeof(var) - 2x a vzdy 10
    //MODEL_NAME_LEN
    //prefill_string((char *)var, sizeof(var));
    prefill_string((char *)var, MODEL_NAME_LEN);
    
    if (!context.edit)
        lcd_write_string((char *)var, LCD_OP_SET, FLAGS_NONE);
    else
        gui_string_edit(&context, (char *)var, g_key_press);
}
