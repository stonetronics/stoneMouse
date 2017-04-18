/*
 * uart.h
 *
 * Created: 05.04.2017 14:09:45
 *  Author: Stone
 */ 


#ifndef UART_H_
#define UART_H_

void uart_init();

unsigned char uart_loop();

void uart_debugInt(int8_t toDebug);

void uart_debugUint(uint8_t toDebug);



#endif /* UART_H_ */