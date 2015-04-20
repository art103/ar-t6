/*
 * Author: Michal Krombholz michkrom@github.com
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
 */

/* Description:
 *
 * This module contains debug utilities for printing to serial port.
 *
 */


#define DEBUG_UTILS
#include "stdint.h"
#include "debug.h"
#include "usart.h"

/**
  * @brief  Send hex char of lower 4 bits of b to usart
  * @param  b byte
  * @retval None
  */
void puts_hex1(uint8_t b)
{
	b &= 0xF;
	usart_putc( b < 10 ? '0'+b : 'A'+b-10 );
}

/**
  * @brief  Send hex chars of a byte to usart
  * @param  b byte
  * @retval None
  */
void puts_hex2(uint8_t b)
{
	puts_hex1(b >> 4);
	puts_hex1(b);
}

/**
  * @brief  Send 4 hex char of 16 bit v to usart
  * @param  v value
  * @retval None
  */
void puts_hex4(uint16_t v)
{
	puts_hex2(v >> 8);
	puts_hex2(v);
}

/**
  * @brief  Send 8 hex char of 32 bit v to usart
  * @param  v value
  * @retval None
  */
void puts_hex8(uint32_t v)
{
	puts_hex4(v >> 16);
	puts_hex4(v);
}


/**
  * @brief  Send decimal value to usart
  * @param  v value
  * @retval None
  */
void puts_dec(int32_t v)
{
	if( v < 0 ) {
		v = -v;
		usart_putc('-');
	}
	uint32_t d = 1000000000; // 10^9 biggest power of 10 fitting in 32 bits
	uint8_t cnt = 0;
	while(d > 1)
	{
		uint32_t n = v / d;
		v -= n * d;
		d /= 10;
		if( n > 0 || cnt > 0 )
		{
			usart_putc('0'+n);
			cnt++;
		}
	}
	usart_putc('0'+v);
}


/**
  * @brief  Dump memory in hex to usart
  * @param  adr address
  * @param  len length to dump
  * @retval None
  */
void puts_mem(void* adr, int len)
{
	for(int i = 0; i < len; i++)
	{
		puts_hex2(((uint8_t*)adr)[i]);
		if( i % 16 == 15 ) usart_puts("\r\n");
	}
}
