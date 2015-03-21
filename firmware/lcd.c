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

/* Description:
 *
 * This is an LCD driver.
 * There are low, medium and high level functions for drawing text
 * and basic graphics to the display buffer.
 * lcd_update() must be called to send the buffer to the LCD.
 *
 */

#include <string.h>

#include "stm32f10x.h"
#include "tasks.h"
#include "lcd.h"

#include "lcd_font_medium.h"

#define LCD_PIN_MASK	(0x1FFF)

#define LCD_DATA		(0xFF)		// D0-D7
#define LCD_RD			(1 << 8)	// RD / E
#define LCD_WR			(1 << 9)	// RD / #WR
#define LCD_A0			(1 << 10)	// A0 / RS / Data / #CMD
#define LCD_RES			(1 << 11)	// Reset
#define LCD_CS1			(1 << 12)	// Chip Select 1

#define LCD_BACKLIGHT	(1 << 2)

#define KS0713_DISP_ON_OFF		(0xAE)
#define KS0713_DISPLAY_LINE		(0x40)
#define KS0713_SET_REF_VOLTAGE	(0x81)	// 2-byte cmd
#define KS0713_SET_PAGE_ADDR	(0xB0)
#define KS0713_SET_COL_ADDR_MSB	(0x10)
#define KS0713_SET_COL_ADDR_LSB	(0x00)
#define KS0713_ADC_SELECT		(0xA0)
#define KS0713_REVERSE_DISP		(0xA6)
#define KS0713_ENTIRE_DISP		(0xA4)
#define KS0713_LCD_BIAS			(0xA2)
#define KS0713_SET_MOD_READ		(0xE0)
#define KS0713_UNSET_MOD_READ	(0xEE)
#define KS0713_RESET			(0xE2)
#define KS0713_SHL_SELECT		(0xC0)
#define KS0713_POWER_CTRL		(0x28)
#define KS0713_REG_RES_SEL		(0x20)
#define KS0713_STATIC_IND_MODE	(0xAC)	// 2-byte cmd
//#define KS0713_POWER_SAVE		Display off, Entire display ON.

static uint8_t contrast = 0x28;

uint8_t lcd_buffer[LCD_WIDTH * LCD_HEIGHT / 8];

#define FONT_STRIDE				(16 * 5)

static uint8_t cursor_x = 0;
static uint8_t cursor_y = 0;

static const unsigned char *font = font_medium;

/**
 * @brief  Send a command to the LCD.
 * @note	Switch the MPU interface to command mode and send
 * @param  cmd: Command to send
 * @retval None
 */
static void lcd_send_command(uint8_t cmd) {
	uint16_t gpio = GPIO_ReadOutputData(GPIOC);

	gpio &= ~(LCD_WR | LCD_DATA | LCD_A0);
	gpio |= cmd;
	GPIO_Write(GPIOC, gpio);

	// Toggle the Enable lines.
	gpio &= ~LCD_CS1;
	gpio |= LCD_RD;
	GPIO_Write(GPIOC, gpio);
	gpio |= LCD_CS1;
	gpio &= ~LCD_RD;
	GPIO_Write(GPIOC, gpio);
}

/**
 * @brief  Send data to the LCD.
 * @note	Switch the MPU interface to data mode and send
 * @param  data: pointer to LCD data
 * @param  len: Number of bytes of data to send
 * @retval None
 */
static void lcd_send_data(uint8_t *data, uint16_t len) {
	uint16_t i;
	uint16_t gpio = GPIO_ReadOutputData(GPIOC);

	gpio &= ~(LCD_WR | LCD_DATA);
	gpio |= LCD_A0;

	data += (len - 1);

	for (i = 0; i < len; ++i) {
		gpio &= ~LCD_DATA;
		gpio |= *data--;
		GPIO_Write(GPIOC, gpio);

		// Toggle the Enable lines.
		gpio &= ~LCD_CS1;
		gpio |= LCD_RD;
		GPIO_Write(GPIOC, gpio);
		gpio |= LCD_CS1;
		gpio &= ~LCD_RD;
		GPIO_Write(GPIOC, gpio);
	}
}

/**
 * @brief  Initialise the lcd panel.
 * @note   Sets up the controller and displays our logo.
 * @param  None
 * @retval None
 */
void lcd_init(void) {
	GPIO_InitTypeDef gpioInit;

	// Enable the GPIO block clocks and setup the pins.
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOD, ENABLE);

	GPIO_SetBits(GPIOC, LCD_PIN_MASK);
	GPIO_ResetBits(GPIOC, LCD_RD | LCD_WR);

	// Configure the LCD pins.
	gpioInit.GPIO_Speed = GPIO_Speed_2MHz;
	gpioInit.GPIO_Mode = GPIO_Mode_Out_PP;
	gpioInit.GPIO_Pin = LCD_PIN_MASK;
	GPIO_Init(GPIOC, &gpioInit);

	gpioInit.GPIO_Pin = LCD_BACKLIGHT;
	GPIO_Init(GPIOD, &gpioInit);

	// Reset LCD
	GPIO_ResetBits(GPIOC, LCD_RES);
	delay_us(5);
	GPIO_SetBits(GPIOC, LCD_RES);

	// Wait for reset to complete.
	delay_us(50);

	lcd_send_command(KS0713_RESET); // Initialize the internal functions (Reset)
	lcd_send_command(KS0713_DISP_ON_OFF | 0x00); // Turn off LCD panel (DON = 0)
	lcd_send_command(KS0713_ADC_SELECT | 0x01); // Select SEG output direction reversed (ADC = 1)
	lcd_send_command(KS0713_REVERSE_DISP | 0x00); // Select normal / reverse display (REV = 0)
	lcd_send_command(KS0713_ENTIRE_DISP | 0x00); // Select normal display ON (EON = 0)
	lcd_send_command(KS0713_LCD_BIAS | 0x00); 		// Select LCD bias (0)
	lcd_send_command(KS0713_SHL_SELECT | 0x08); // Select COM output direction normal (SHL = 0)
	lcd_send_command(KS0713_POWER_CTRL | 0x07); // Control power circuit operation (VC,VR,VF on)
	lcd_send_command(KS0713_REG_RES_SEL | 0x04); // Select internal resistance ratio (0x05)
	lcd_send_command(KS0713_SET_REF_VOLTAGE); // Set reference voltage Mode (2-part cmd)
	lcd_send_command(contrast); 			// Set reference voltage register
	lcd_send_command(KS0713_DISP_ON_OFF | 0x01); // Turn on LCD panel (DON = 1)

	lcd_update();
	lcd_backlight(TRUE);
}

/**
 * @brief  Turn the backlight on / off.
 * @note
 * @param  on: Whether the backlight should be on or off.
 * @retval None
 */
void lcd_backlight(bool state) {
	if (state)
		GPIO_SetBits(GPIOD, LCD_BACKLIGHT);
	else
		GPIO_ResetBits(GPIOD, LCD_BACKLIGHT);
}

/**
 * @brief  Set the LCD contrast.
 * @note
 * @param  val: Set contrast to this value.
 * @retval None
 */
void lcd_set_contrast(uint8_t val) {
	contrast = val;
	if ((contrast + val) > 0xff)
		contrast = 0xff;
	else if (contrast + val < 0)
		contrast = 0;

	lcd_send_command(KS0713_SET_REF_VOLTAGE); // Set reference voltage Mode (2-part cmd)
	lcd_send_command(contrast); 			// Set reference voltage register
}

/**
 * @brief  Transfer frame buffer to LCD.
 * @note
 * @param  None
 * @retval None
 */
void lcd_update(void) {
	int row;

	// Update CGRAM
	for (row = 0; row < LCD_HEIGHT / 8; ++row) {
		lcd_send_command(KS0713_SET_PAGE_ADDR | row);
		lcd_send_command(KS0713_SET_COL_ADDR_LSB | 0x04); // low col
		lcd_send_command(KS0713_SET_COL_ADDR_MSB | 0x00);
		lcd_send_data(lcd_buffer + (LCD_WIDTH * row), LCD_WIDTH);
	}
}

/**
 * @brief  Set / Clean a specific pixel.
 * @note	Top left is (0,0)
 * @param  x: horizontal pixel location
 * @param  y: vertical pixel location
 * @param  operation: 0 - off, 1 - on, 2 - xor
 * @retval None
 */
void lcd_set_pixel(uint8_t x, uint8_t y, LCD_OP op) {
	switch (op) {
	case LCD_OP_NONE:
		break;
	case LCD_OP_CLR:
		lcd_buffer[x + (y / 8) * LCD_WIDTH] &= ~(1 << (y % 8));
		break;
	case LCD_OP_SET:
		lcd_buffer[x + (y / 8) * LCD_WIDTH] |= (1 << (y % 8));
		break;
	case LCD_OP_XOR:
		lcd_buffer[x + (y / 8) * LCD_WIDTH] ^= (1 << (y % 8));
		break;
	}
}

/**
 * @brief  Set cursor position in pixels.
 * @note	Top left is (0,0)
 * @param  x: horizontal cursor location
 * @param  y: vertical cusros location
 * @retval None
 */
void lcd_set_cursor(uint8_t x, uint8_t y) {
	if ((y + CHAR_HEIGHT) >= LCD_HEIGHT)
		return;
	if ((x + CHAR_WIDTH) >= LCD_WIDTH)
		return;

	cursor_x = x;
	cursor_y = y;
}

/**
 * @brief  Write a character.
 * @note
 * @param  c: ASCII character to write
 * @param  op: LCD_OP
 * @param  flags: LCD_FLAGS (CHAR_*)
 * @retval None
 */
void lcd_write_char(uint8_t c, LCD_OP op, LCD_FLAGS flags) {
	uint8_t x, y;
	uint8_t x1, y1, divX = 1, divY = 1;
	uint8_t height = CHAR_HEIGHT;
	uint8_t width = CHAR_WIDTH;
	uint8_t row = 0;
	LCD_OP op_set = (op == LCD_OP_SET) ? LCD_OP_SET : LCD_OP_CLR;
	LCD_OP op_clr = (op == LCD_OP_SET) ? LCD_OP_CLR : LCD_OP_SET;

	if ((flags & CHAR_2X) != 0) {
		height *= 2;
		width *= 1;
		divY = 2;
	} else if ((flags & CHAR_4X) != 0) {
		height *= 2;
		width *= 2;
		divX = 2;
		divY = 2;
	}

	// CR/LF - note LF == CR+LF
	switch (c) {
	case 10:
		cursor_y += height;
		/* no break */
	case 13:
		cursor_x = 0;
		return;
	}

	if (op == LCD_OP_XOR) {
		op_set = LCD_OP_XOR;
		op_clr = LCD_OP_NONE;
	}

	// condensed chars overlap so do not erase or prev char gets damaged, use CHAR_NOSPACE alternatively
	if (flags & CHAR_CONDENSED)
		op_clr = LCD_OP_NONE;

	if ((cursor_y + height) >= LCD_HEIGHT)
		return;
	else if ((cursor_x + width) >= LCD_WIDTH)
		return;

	for (x = 0; x < width; x++) {
		row = 0;
		for (y = 0; y < height + 1; y++) {
			uint8_t d;
			x1 = x / divX;
			y1 = y / divY;
			d = font[(c * CHAR_WIDTH) + x1 + row * FONT_STRIDE];
			if (flags & CHAR_UNDERLINE)
				d |= 0x80;

			if (d & (1 << y1 % 8))
				lcd_set_pixel(cursor_x + x, cursor_y + y, op_set);
			else
				lcd_set_pixel(cursor_x + x, cursor_y + y, op_clr);

			if (y1 % 8 == 7)
				row++;
		}

		if ((flags & CHAR_CONDENSED) != 0 && x % CHAR_WIDTH == 4)
			cursor_x--;
	}
	for (y = 0; y < height + 1; y++)
		lcd_set_pixel(cursor_x + width, cursor_y + y, op_clr);

	cursor_x += width + 1;
	if ((flags & (CHAR_CONDENSED | CHAR_NOSPACE)) != 0)
		cursor_x--;

	if (cursor_x >= LCD_WIDTH)
		cursor_y += height + 1;
}

/**
 * @brief  Write a string.
 * @note	Iterate through a null terminated string.
 * @param  s: ASCII string to write
 * @param  op: LCD_OP
 * @param  flags: LCD_FLAGS (CHAR_*)
 * @retval None
 */
void lcd_write_string(const char *s, LCD_OP op, LCD_FLAGS flags) {
	const char *ptr = s;
	uint8_t n = strlen(s);

	if (flags & ALIGN_RIGHT) {
		cursor_x -= n
				* ((flags & CHAR_CONDENSED) ? CHAR_WIDTH : (CHAR_WIDTH + 1));
	}

	for (ptr = s; n > 0; ptr++, n--)
		lcd_write_char(*ptr, op, flags);

	if (flags & TRAILING_SPACE)
		lcd_write_char(' ', op, flags);
	if (flags & NEW_LINE)
		lcd_write_char(10, 0, 0);
}

// Reduce stack usage.
static uint8_t tth;
static uint8_t th;
static uint8_t h;
static uint8_t t;
static int16_t u;

/**
 * @brief  Write an int (up to 5 digits)
 * @note
 * @param  val: Signed integer (-99999 to 99999)
 * @param  op: LCD_OP
 * @param  flags: LCD_LCD_FLAGS
 * @retval None
 */
void lcd_write_int(int32_t val, LCD_OP op, LCD_FLAGS flags) {
	int count = 0;

	if (val < 0)
		u = -val;
	else
		u = val;
	tth = u / 10000;
	u -= tth * 10000;
	th = u / 1000;
	u -= th * 1000;
	h = u / 100;
	u -= h * 100;
	t = u / 10;
	u -= t * 10;

	if (flags & ALIGN_RIGHT) {
		if (val < 0)
			count++;
		//if (val >= 0) lcd_write_char('+', op);

		if (tth > 0)
			count++;
		if (tth > 0 || th > 0)
			count++;
		if (tth > 0 || th > 0 || h > 0)
			count++;
		if (tth > 0 || th > 0 || h > 0 || t > 0 || (flags & INT_DIV10)
				|| (flags & INT_PAD10))
			count++;
		if (flags & INT_DIV10)
			count++;

		if (flags & CHAR_CONDENSED)
			cursor_x -= count * CHAR_WIDTH;
		else
			cursor_x -= count * (CHAR_WIDTH + 1);
	}

	if (val < 0)
		lcd_write_char('-', op, flags);
	//if (val >= 0) lcd_write_char('+', op);

	if (tth > 0)
		lcd_write_char(tth + '0', op, flags);

	if (tth > 0 || th > 0)
		lcd_write_char(th + '0', op, flags);

	if (tth > 0 || th > 0 || h > 0)
		lcd_write_char(h + '0', op, flags);

	if (tth > 0 || th > 0 || h > 0 || t > 0 || (flags & INT_DIV10)
			|| (flags & INT_PAD10))
		lcd_write_char(t + '0', op, flags);
	if (flags & INT_DIV10)
		lcd_write_char('.', op, flags);

	lcd_write_char(u + '0', op, flags);

	if (flags & TRAILING_SPACE)
		lcd_write_char(' ', op, flags);
	if (flags & NEW_LINE)
		lcd_write_char(10, 0, 0);
}

/**
 * @brief  Write a value in hex
 * @note
 * @param  val: unsigned Signed integer
 * @param  op: LCD_OP
 * @param  flags: LCD_FLAGS
 * @retval None
 */
void lcd_write_hex(uint32_t val, LCD_OP op, LCD_FLAGS flags) {
	int digits = 4;
	int i;
	if (val > 0xFFFF)
		digits = 8;

	for (i = 0; i < digits; ++i) {
		int digit = (val >> (4 * (digits - i - 1))) & 0xF;

		if (digit > 9)
			lcd_write_char(digit - 10 + 'A', op, flags);
		else
			lcd_write_char(digit + '0', op, flags);
	}
}

/**
 * @brief  Draw a line between two points.
 * @note	Top left is (0,0)
 * @param  x1: First pixel (x)
 * @param  y1: First pixel (y)
 * @param  x2: Second pixel (x)
 * @param  y2: Second pixel (y)
 * @param  op: LCD_OP
 * @retval None
 */
void lcd_draw_line(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1, LCD_OP op) {
	// from http://rosettacode.org/wiki/Bitmap/Bresenham%27s_line_algorithm#C
	int dx = x1 > x0 ? x1 - x0 : x0 - x1;
	int sx = x0 < x1 ? 1 : -1;
	int dy = y1 > y0 ? y1 - y0 : y0 - y1;
	int sy = y0 < y1 ? 1 : -1;
	int err = (dx > dy ? dx : -dy) / 2, e2;

	for (;;) {
		lcd_set_pixel(x0, y0, op);
		if (x0 == x1 && y0 == y1)
			break;
		e2 = err;
		if (e2 > -dx) {
			err -= dy;
			x0 += sx;
		}
		if (e2 < dy) {
			err += dx;
			y0 += sy;
		}
	}
}

/**
 * @brief  Draw a rectangle.
 * @note	Top left is (0,0)
 * @param  x1,y1: 1st corner
 * @param  x2,y2: 2nd corner
 * @param  op: LCD_OP
 * @param  fill: Fill?
 * @retval None
 */
void lcd_draw_rect(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, LCD_OP op,
		uint16_t flags) {
	uint8_t x, y;

	if (x1 > x2 || y1 > y2)
		return;

	for (y = y1; y <= y2; ++y) {
		for (x = x1; x <= x2; ++x) {
			if ((flags & RECT_FILL) || y == y1 || y == y2 || x == x1
					|| x == x2) {
				if (flags & RECT_ROUNDED) {
					if (!((x == x1 && y == y1) || (x == x2 && y == y1)
							|| (x == x1 && y == y2) || (x == x2 && y == y2))) {
						lcd_set_pixel(x, y, op);
					}

				} else
					lcd_set_pixel(x, y, op);
			}
		}
	}
}

/**
 * @brief  Draw a message with line wrapping
 * @note	Starts at the cursor and uses the x offset as a margin.
 * @param  msg: ASCII message to write
 * @param  op: LCD_OP
 * @param: op2: LCD_OP for selected line if any
 * @param: selected line or 0 for none
 * @retval lines drawed
 */
char lcd_draw_message(const char *msg, LCD_OP op, LCD_OP op2, char selectedLine) {
	char line = 1;
	const int width = (LCD_WIDTH - 2 * cursor_x) / (CHAR_WIDTH + 1);
	char line_buffer[LCD_WIDTH / (CHAR_WIDTH + 1)];
	int nchars;
	const char *ptr = msg;
	int x = cursor_x;

	// Iterate through the string to print it
	while (*ptr) {
		const char *p;
		const char *pspace = 0;
		// find next wrap point
		for (p = ptr; *p; ++p) {
			// remember last place with space
			if (*p == ' ')
				pspace = p;
			// out of width so it's a break place
			if (p - ptr > width) {
				p = pspace ? pspace : p;
				break;
			}
			// new line - must break
			if (*p == '\n') {
				break;
			}
		}
		nchars = p - ptr;
		memcpy(line_buffer, ptr, nchars);
		line_buffer[nchars] = 0;
		cursor_x = x + (width - nchars) * (CHAR_WIDTH + 1) / 2;
		lcd_write_string(line_buffer, line == selectedLine ? op2 : op,
				FLAGS_NONE);
		cursor_y += (CHAR_HEIGHT + 1);
		line++;
		ptr = *p ? p + 1 : p;
	}
	return line - 1;
}
