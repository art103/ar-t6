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

#define LCD_CONTRAST_MIN	20
#define LCD_CONTRAST_MAX	60

#define CHAR_HEIGHT				7
#define CHAR_WIDTH				5

// LCD Operation type.
typedef enum {
	LCD_OP_NONE = 0x00,
	LCD_OP_CLR,
	LCD_OP_SET,
	LCD_OP_XOR,
} LCD_OP;

typedef enum {
	FLAGS_NONE = 0x00,	// No flags
	RECT_ROUNDED = 0x01,// Round the rectangle corners
	RECT_FILL = 0x02,	// Fill the rectangle

	INT_SIGN = 0x04,	// Draw the +/- sign
	INT_DIV10 = 0x08,	// Put a decimal point 1 digit from the end
	INT_PAD10 = 0x10,	// Put a decimal point 1 digit from the end

	CHAR_2X = 0x100,	// Draw double size
	CHAR_4X = 0x200,	// Draw quadrouple size
	CHAR_CONDENSED = 0x400,	// Draw with 4th column missing
	CHAR_UNDERLINE = 0x800,	// Underline each char
	CHAR_NOSPACE = 0x1000,

	ALIGN_RIGHT = 0x2000,
} LCD_FLAGS;

void lcd_init(void);
void lcd_backlight(bool state);
void lcd_set_contrast(uint8_t val);
void lcd_update(void);
void lcd_set_pixel(uint8_t x, uint8_t y, LCD_OP op);
void lcd_set_cursor(uint8_t x, uint8_t y);
void lcd_write_char(uint8_t c, LCD_OP op, uint16_t flags);
void lcd_write_string(char *s, LCD_OP op, uint16_t flags);
void lcd_write_int(int32_t val, LCD_OP op, uint16_t flags);
void lcd_write_hex(uint32_t val, LCD_OP op, uint16_t flags);
void lcd_draw_line(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, LCD_OP op);
void lcd_draw_rect(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, LCD_OP op, uint16_t flags);
void lcd_draw_message(const char *msg, LCD_OP op);

#endif // _LCD_H
