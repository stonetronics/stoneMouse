/* Name: main.c
 * Project: Joystick Mouse (from HID-Mouse Example by Christian Starkjohann)
 * Author: stonetronics
 * Creation Date: 2017.04.18
 * Copyright: (c) 2008 by OBJECTIVE DEVELOPMENT Software GmbH
 * License: GNU GPL v2 (see License.txt), GNU GPL v3 or proprietary (CommercialLicense.txt)
 */

/*
We use VID/PID 0x046D/0xC00E which is taken from a Logitech mouse. Don't
publish any hardware using these IDs! This is for demonstration only!
*/

#define DDR_BUTTONS		DDRC
#define PORT_BUTTONS	PORTC
#define PIN_BUTTONS		PINC
#define ANALOG_X		PC0
#define ANALOG_Y		PC1
#define BUTTON_LMB		PC2
#define BUTTON_RMB		PC3
#define BUTTON_WHEELUP	PC4
#define BUTTON_WHEELDWN	PC5
#define DDR_DPI			DDRD
#define PORT_DPI		PORTD
#define PIN_DPI			PIND
#define BUTTON_DPI		PD0



#define AVG_SIZE	8
#define NORM_THRESHOLD	15
#define F_CPU			12000000

#include <avr/io.h>
#include <avr/wdt.h>
#include <avr/interrupt.h>  /* for sei() */
#include <util/delay.h>     /* for _delay_ms() */

#include <avr/pgmspace.h>   /* required by usbdrv.h */
#include "usbdrv/usbdrv.h"

#include "adc.h" // for adc functions, obviously
//#include "uart.h" // for uart debugging

/* ------------------------------------------------------------------------- */
/* ----------------------------- USB interface ----------------------------- */
/* ------------------------------------------------------------------------- */

PROGMEM const char usbHidReportDescriptor[52] = { /* USB report descriptor, size must match usbconfig.h */
    0x05, 0x01,                    // USAGE_PAGE (Generic Desktop)
    0x09, 0x02,                    // USAGE (Mouse)
    0xa1, 0x01,                    // COLLECTION (Application)
    0x09, 0x01,                    //   USAGE (Pointer)
    0xA1, 0x00,                    //   COLLECTION (Physical)
    0x05, 0x09,                    //     USAGE_PAGE (Button)
    0x19, 0x01,                    //     USAGE_MINIMUM
    0x29, 0x03,                    //     USAGE_MAXIMUM
    0x15, 0x00,                    //     LOGICAL_MINIMUM (0)
    0x25, 0x01,                    //     LOGICAL_MAXIMUM (1)
    0x95, 0x03,                    //     REPORT_COUNT (3)
    0x75, 0x01,                    //     REPORT_SIZE (1)
    0x81, 0x02,                    //     INPUT (Data,Var,Abs)
    0x95, 0x01,                    //     REPORT_COUNT (1)
    0x75, 0x05,                    //     REPORT_SIZE (5)
    0x81, 0x03,                    //     INPUT (Const,Var,Abs)
    0x05, 0x01,                    //     USAGE_PAGE (Generic Desktop)
    0x09, 0x30,                    //     USAGE (X)
    0x09, 0x31,                    //     USAGE (Y)
    0x09, 0x38,                    //     USAGE (Wheel)
    0x15, 0x81,                    //     LOGICAL_MINIMUM (-127)
    0x25, 0x7F,                    //     LOGICAL_MAXIMUM (127)
    0x75, 0x08,                    //     REPORT_SIZE (8)
    0x95, 0x03,                    //     REPORT_COUNT (3)
    0x81, 0x06,                    //     INPUT (Data,Var,Rel)
    0xC0,                          //   END_COLLECTION
    0xC0,                          // END COLLECTION
};
/* This is the same report descriptor as seen in a Logitech mouse. The data
 * described by this descriptor consists of 4 bytes:
 *      .  .  .  .  . B2 B1 B0 .... one byte with mouse button states
 *     X7 X6 X5 X4 X3 X2 X1 X0 .... 8 bit signed relative coordinate x
 *     Y7 Y6 Y5 Y4 Y3 Y2 Y1 Y0 .... 8 bit signed relative coordinate y
 *     W7 W6 W5 W4 W3 W2 W1 W0 .... 8 bit signed relative coordinate wheel
 */
typedef struct{
    uchar   buttonMask;
    char    dx;
    char    dy;
    char    dWheel;
}report_t;

void normXY(uint8_t x, uint8_t y, int8_t* x_norm, int8_t* y_norm);

static report_t reportBuffer;
static uchar    idleRate;   /* repeat rate for keyboards, never used for mice */

/* ------------------------------------------------------------------------- */

usbMsgLen_t usbFunctionSetup(uchar data[8])
{
usbRequest_t    *rq = (void *)data;

    /* The following requests are never used. But since they are required by
     * the specification, we implement them in this example.
     */
    if((rq->bmRequestType & USBRQ_TYPE_MASK) == USBRQ_TYPE_CLASS){    /* class request type */
        if(rq->bRequest == USBRQ_HID_GET_REPORT){  /* wValue: ReportType (highbyte), ReportID (lowbyte) */
            /* we only have one report type, so don't look at wValue */
            usbMsgPtr = (void *)&reportBuffer;
            return sizeof(reportBuffer);
        }else if(rq->bRequest == USBRQ_HID_GET_IDLE){
            usbMsgPtr = &idleRate;
            return 1;
        }else if(rq->bRequest == USBRQ_HID_SET_IDLE){
            idleRate = rq->wValue.bytes[1];
        }
    }else{
        /* no vendor specific requests implemented */
    }
    return 0;   /* default for not implemented requests: return no data back to host */
}

/* ------------------------------------------------------------------------- */

int __attribute__((noreturn)) main(void)
{
	uchar   i;
	uint8_t x_raw;
	uint8_t y_raw;
	int8_t x_norm;
	int8_t y_norm;
	uint8_t dpi_factor = 2;

	uint8_t button_lmb;
	uint8_t button_rmb;
	uint8_t button_wheelup;
	uint8_t button_wheeldwn;
	uint8_t button_dpi;
	uint8_t button_dpi_old;

	/* init button ports & stuff */
	//set all buttons and analog inputs to input
	DDR_BUTTONS &= ~((1<<ANALOG_X) | (1<<ANALOG_Y) | (1<<BUTTON_LMB) | (1<<BUTTON_RMB) | (1<<BUTTON_WHEELUP) | (1<<BUTTON_WHEELDWN));
	DDR_DPI &= ~(1<<BUTTON_DPI);
	//set pullups on all buttons
	PORT_BUTTONS |= (1<<BUTTON_LMB) | (1<<BUTTON_RMB) | (1<<BUTTON_WHEELUP) | (1<<BUTTON_WHEELDWN);
	PORT_DPI |= (1<<BUTTON_DPI);
	button_dpi_old = PIN_DPI & (1<<BUTTON_DPI);

	adc_init();
	//uart_init();
    wdt_enable(WDTO_1S); //needed
    usbInit();
    usbDeviceDisconnect();  /* enforce re-enumeration, do this while interrupts are disabled! */
    i = 0;
    while(--i){             /* fake USB disconnect for > 250 ms */
        wdt_reset();
        _delay_ms(1);
    }

    usbDeviceConnect();
    sei();
    while(1){                /* main event loop */
        wdt_reset();
        usbPoll();
		if(usbInterruptIsReady()){
            /* called after every poll of the interrupt endpoint */
            usbSetInterrupt((void *)&reportBuffer, sizeof(reportBuffer));
        }

		/*button inputs handling*/
		//scan all buttons
		button_rmb = PIN_BUTTONS & (1<<BUTTON_RMB);
		button_lmb = PIN_BUTTONS & (1<<BUTTON_LMB);
		button_wheelup = PIN_BUTTONS & (1<<BUTTON_WHEELUP);
		button_wheeldwn = PIN_BUTTONS & (1<<BUTTON_WHEELDWN);
		button_dpi = PIN_DPI & (1<<BUTTON_DPI);

		if(!button_lmb)
			reportBuffer.buttonMask |= (1<<0); //activate left mousebutton
		else
			reportBuffer.buttonMask &= ~(1<<0); //deactivate left mousebutton

		if(!button_rmb)
			reportBuffer.buttonMask |= (1<<1); //activate right mousebutton
		else
			reportBuffer.buttonMask &= ~(1<<1); //deactivate right mousebutton
		
		//button emulated mousewheel
		if(!button_wheelup)
			reportBuffer.dWheel = 10/dpi_factor;
		else if(!button_wheeldwn)
			reportBuffer.dWheel = -10/dpi_factor;
		else
			reportBuffer.dWheel = 0;

		//if dpi button pressed switch through various dpi factors
		if(button_dpi != button_dpi_old)
		{
			button_dpi_old = button_dpi;
			if(!button_dpi)
			{
				switch(dpi_factor)
				{
					case 1:
						dpi_factor = 2;
						break;
					case 2:
						dpi_factor = 5;
						break;
					case 5:
						dpi_factor = 10;
						break;
					case 10:
						dpi_factor = 1;
						break;
					default:
						dpi_factor = 2;
						break;
				}
			}
		}
		
		//joystick input stuff
		if(adc_updateValues(&(x_raw), &(y_raw)))
		{	
			normXY(x_raw, y_raw, &x_norm, &y_norm );
			reportBuffer.dx = x_norm/dpi_factor;
			reportBuffer.dy = y_norm/dpi_factor;
		}

    }
}


void normXY(uint8_t x, uint8_t y, int8_t* x_norm, int8_t* y_norm)
{
	static uint8_t firstrun = 1;
	static float x_avg = 128;
	static uint8_t x_upperThreshold = 130;
	static uint8_t x_lowerThreshold = 120;
	static float y_avg = 128;
	static uint8_t y_upperThreshold = 130;
	static uint8_t y_lowerThreshold = 120;

	if(firstrun)
	{
		x_avg = (float)x;
		x_upperThreshold = x_avg + NORM_THRESHOLD;
		x_lowerThreshold = x_avg - NORM_THRESHOLD;
		y_avg = (float)y;
		y_upperThreshold = y_avg + NORM_THRESHOLD;
		y_lowerThreshold = y_avg - NORM_THRESHOLD;
		firstrun = 0;
	}

	if ((x > x_lowerThreshold) && (x < x_upperThreshold))
	{
		x_avg = x_avg * ((float)(AVG_SIZE - 1)/AVG_SIZE) + ((float)x) * 1/((float) AVG_SIZE);
	}

	if ((y > (y_lowerThreshold)) && (y < (y_upperThreshold)))
	{
		y_avg = y_avg * ((float)(AVG_SIZE - 1)/AVG_SIZE) + ((float)y) * 1/((float) AVG_SIZE);
	}

	*x_norm =(int8_t)( ((float) x) - x_avg );
	*y_norm =(int8_t)( y_avg - ((float) y) );
}
/* ------------------------------------------------------------------------- */