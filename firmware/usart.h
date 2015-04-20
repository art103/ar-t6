/*
 * usart.c
 *
 *  Created on: Mar 22, 2015
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
 * Author: Michal Krombholz (michkrom@github.com)
 */
#ifndef USART_H_
#define USART_H_

void usart_init();
void usart_register_rx_handler(void (*rx_handler)(uint8_t data));

void usart_putc(char c);
void usart_puts(char* s);
void usart_put(char* s, uint8_t len);
void usart_putc_nb(char c);

uint16_t usart_getc();
uint8_t usart_peekc(uint8_t c);

#endif /* USART_H_ */
