/*
 * uart.c
 *
 * Created: 05.04.2017 14:11:44
 *  Author: Stone
 */ 

 #include <avr/io.h>
 #include "uart.h"
 #define F_CPU 12000000UL


 volatile unsigned char transmissionBuffer[6];
 volatile unsigned char loopPos = 6;

 void uart_init()
 {
	//set baudrate to 9600
	unsigned int ubrr = F_CPU/16/9600 - 1;
	UBRRH = (unsigned char)(ubrr>>8);
	UBRRL = (unsigned char)ubrr;
	//enable transmitter pin and hardware
	DDRD |= 1<<PD1;
	UCSRB |= 1<<TXEN;
 }

 unsigned char uart_putc(unsigned char c)
 {
	//only send next character if uart is ready
	if(UCSRA & (1<<UDRE))
	{
		UDR = c;
		return 1;
	}
	return 0;
 }


 unsigned char uart_loop()
 {
	if(loopPos < 6)
	{
		if(uart_putc(transmissionBuffer[loopPos]))
		{
			loopPos++;
		}
		return 0;
	}
	return 1;
 }

 void uart_debugInt(int8_t toDebug)
 {
	if(loopPos == 6)
	{
		if(toDebug >= 0)
		{
			transmissionBuffer[0] = '+';
		}
		else
		{
			transmissionBuffer[0] = '-';
			toDebug = 0 - toDebug;
		}
		transmissionBuffer[1] = toDebug/100 + '0';
		transmissionBuffer[2] = (toDebug/10)%10 + '0';
		transmissionBuffer[3] = toDebug%10 + '0';
		transmissionBuffer[4] = '\n';
		transmissionBuffer[5] = '\r';
		loopPos = 0;
	}
 }

  void uart_debugUint(uint8_t toDebug)
  {
	  if(loopPos == 6)
	  {
		  transmissionBuffer[0] = ' ';
		  transmissionBuffer[1] = toDebug/100 + '0';
		  transmissionBuffer[2] = (toDebug/10)%10 + '0';
		  transmissionBuffer[3] = toDebug%10 + '0';
		  transmissionBuffer[4] = '\n';
		  transmissionBuffer[5] = '\r';
		  loopPos = 0;
	  }
  }