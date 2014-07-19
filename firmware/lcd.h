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

#ifndef _LCD_H
#define _LCD_H

#define LCD_WIDTH		128
#define LCD_HEIGHT		64

typedef enum {
	LCD_CHAR_SIZE_SMALL,
	LCD_CHAR_SIZE_MEDIUM
} LCD_CHAR_SIZE;

typedef enum {
	RECT_NONE = 0x00,
	RECT_ROUNDED = 0x01,
	RECT_FILL = 0x02,

	INT_SIGN = 0x04,
	INT_DIV10 = 0x08,

} LCD_FLAGS;

void lcd_init(void);
void lcd_backlight(bool state);
void lcd_adj_contrast(int8_t val);
void lcd_update(void);
void lcd_set_pixel(uint8_t x, uint8_t y, uint8_t colour);
void lcd_set_cursor(uint8_t x, uint8_t y);
void lcd_set_char_size(LCD_CHAR_SIZE s);
void lcd_write_char(uint8_t c, uint8_t colour);
void lcd_write_string(char *s, uint8_t colour);
void lcd_write_int(int32_t val, uint8_t colour, uint8_t flags);
void lcd_write_float(float val, uint8_t colour, bool show_sign);
void lcd_draw_line(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, uint8_t colour);
void lcd_draw_rect(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, uint8_t colour, uint8_t flags);
void lcd_draw_message(const char *msg, uint8_t colour);

#endif // _LCD_H
