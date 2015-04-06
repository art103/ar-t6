#ifndef USART_H_
#define USART_H_

void usart_init();
void usart_register_rx_handler(void (*rx_handler)(uint8_t data));

void usart_putc(char c);
void usart_puts(char* s);
void usart_put(char* s, uint8_t len);

uint16_t usart_getc();

#endif /* USART_H_ */
