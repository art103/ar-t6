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

/**
  * @brief  Initialise the lcd panel.
  * @note   Sets up the controller and displays our logo.
  * @param  None
  * @retval None
  */
void lcd_init(void);

/**
  * @brief  Turn the backlight on / off.
  * @note
  * @param  on: Whether the backlight should be on or off.
  * @retval None
  */
void lcd_backlight(bool on);

/**
  * @brief  Transfer frame buffer to LCD.
  * @note
  * @param  None
  * @retval None
  */
void lcd_update(void);

void lcd_set_pixel(uint8_t x, uint8_t y, uint8_t colour);
void lcd_set_cusor(uint8_t x, uint8_t y);
void lcd_write_char(uint8_t c, uint8_t colour);
void lcd_write_string(char *s, uint8_t colour);

#endif // _LCD_H
