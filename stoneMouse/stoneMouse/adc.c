/*
 * adc.c
 *
 * Created: 28.03.2017 22:29:26
 *  Author: Stone
 */ 

 #include <avr/io.h>
 #include "adc.h"

 void adc_init()
 {
	ADMUX |= (1<<REFS0) | (1<<ADLAR); //voltage referenc aref, left adjust result
	ADMUX &= 0xF0; //set MUX3:0 to 0 => Channel 0
	ADCSRA |= (1<<ADEN) | (6<<ADPS0); //ADEN enable adc, ADPS0 set prescaler to 64, 12MHz/64 = 187,5kHz
	ADCSRA |= (1<<ADSC); //start first conversation
 }

 uint8_t adc_updateValues(uint8_t* x, uint8_t* y)
 {
	static unsigned char ready = 0;
	static unsigned char s_current = 0;
	static unsigned char x_tmp, y_tmp;
	if(!(ADCSRA & (1<<ADSC))) // if conversion finished
	{
		//execute handling
		if(s_current == 0)
		{
			x_tmp = ADCH;
			ADMUX |= (1<<MUX0); //set adc to channel 1
			s_current = 1;
		}
		else
		{
			y_tmp = ADCH;
			ADMUX &= 0xF0; //set adc to channel 0
			s_current = 0;
			ready = 1;
		}
		ADCSRA |= (1<<ADSC); //start next conversion
	}

	if(ready)
	{
		ready = 0;
		*x = x_tmp;
		*y = y_tmp;
		return 1;
	}
	return 0;
 }