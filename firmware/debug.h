/*
 * Author: Michal Krombholz michkrom@github.com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#ifndef _DEBUG_H
#define _DEBUG_H

#include "stdint.h"

extern void puts_hex1(uint8_t b);
extern void puts_hex2(uint8_t b);
extern void puts_hex4(uint16_t v);
extern void puts_hex8(uint32_t v);
extern void puts_dec(int32_t v);
extern void puts_mem(void* adr, int len);

#ifdef DPUTS

#include "usart.h"

#define dputs_dec(v) puts_dec(v)
#define dputs_hex1(v) puts_hex1(v)
#define dputs_hex2(v) puts_hex2(v)
#define dputs_hex4(v) puts_hex4(v)
#define dputs_hex8(v) puts_hex8(v)
#define dputs(s) usart_puts(s)
#define dputc(c) usart_putc(c)
// usable from IRQ as it does not block
#define dputcnb(c) usart_putc_nb(c)

#else

#define dputs_hex1(v)
#define dputs_hex2(v)
#define dputs_hex4(v)
#define dputs_hex8(v)
#define dputs(s)
#define dputcnb(c)

#endif

#endif // _DEBUG_H
