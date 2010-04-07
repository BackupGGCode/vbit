/* This file has been prepared for Doxygen automatic documentation generation.*/
/*! \file *********************************************************************
 *
 * \brief  XMEGA TWI driver example source.
 *
 *      This file contains an example application that demonstrates the TWI
 *      master and slave driver. It shows how to set up one TWI module as both
 *      master and slave, and communicate with itself.
 *
 *      The recommended test setup for this application is to connect 10K
 *      pull-up resistors on PC0 (SDA) and PC1 (SCL). Connect a 10-pin cable
 *      between the PORTD and SWITCHES, and PORTE and LEDS.
 *
 * \par Application note:
 *      AVR1308: Using the XMEGA TWI
 *
 * \par Documentation
 *      For comprehensive code documentation, supported compilers, compiler
 *      settings and supported devices see readme.html
 *
 * \author
 *      Atmel Corporation: http://www.atmel.com \n
 *      Support email: avr@atmel.com
 *
 * $Revision: 2660 $
 * $Date: 2009-08-11 12:28:58 +0200 (ti, 11 aug 2009) $  \n
 *
 * Copyright (c) 2008, Atmel Corporation All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * 3. The name of ATMEL may not be used to endorse or promote products derived
 * from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY ATMEL "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE EXPRESSLY AND
 * SPECIFICALLY DISCLAIMED. IN NO EVENT SHALL ATMEL BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *****************************************************************************/

#include "i2c.h"

/*! Defining an example slave address. */
// #define SLAVE_ADDRESS_7113    0xa4
// #define SLAVE_ADDRESS_7121    0x99
#define SLAVE_ADDRESS_7113    0x4a
#define SLAVE_ADDRESS_7121    0x88

/*! CPU speed 2MHz, BAUDRATE 100kHz and Baudrate Register Settings */
#define CPU_SPEED    16000000
#define BAUDRATE	100000
#define TWI_BAUDSETTING TWI_BAUD(CPU_SPEED, BAUDRATE)

/* Global variables */
TWI_Master_t twiMaster;    /*!< TWI master module. */

/*! Buffer with test data to send, starting from address 1
It might be a wise idea to move this to the SD card*/
uint8_t saa7113InitData[] = {
0x01,0x08, // increment delay - 0x08 recommended 
0x02,0xc0, // analog control 1 - 0xc0 Mode 0 CVBS, input pin 4, 9 bit hyst off, amp and anti-alias active
0x03,0x33, // analog control 2 - AGC fixed during video and  blanking. Probably NOT what we want!
0x04,0x00, // analog control 3 - Static Gain set to 0. Channel 1
0x05,0x00, // analog control 4 - Static Gain channel 2 (Don't care)
0x06,0xe9, // horiz sync start - e9 recommnded
0x07,0x0d, // horiz sync stop - 0d recommended
0x08,0x98, // sync control - vertical noise=normal, PLL closed, Fast locking mode, field toggle on interlace, field detection AUFD 
0x09,0x01, // luminance control - aperture factor = 0.25, update agc per line,active luma (correct?), bandpass 4MHz, prefilter bypassed, chroma trap set for CVBS
0x0a,0x80, // luminance brightness - Set to ITU level
0x0b,0x47, // luminance contrast - Set to ITU level
0x0c,0x40, // chrominance saturation - Set to ITU level
0x0d,0x00, // chrominance hue - Phase control
0x0e,0x01, // chrominance control - Normal bandwidth 800kHz, PAL
0x0f,0x2a, // chrominance gain control ?
0x10,0x00, // format/delay control - Mainly ITU 656
0x11,0x0c, // output control 1
0x12,0xa1, // output control 2 - Default 0x01. Controls RTS0 (don't care) and RTS1 - ODD/EVEN Field
0x13,0x00, // output control 3 - Analog test select (don't care)

0x15,0x00, // VGATE start - Probably don't care
0x16,0x00, // VGATE stop - Probably don't care
0x17,0x00, // MSBs for VGATE control - Probably don't care
// 0x1f,0x, decoder status byte. readonly
0x40,0x02, // slicer control 1 - Probably don't care
// 0x41 to 0x57 // line control register 2 to 24  - Don't care
0x58,0x00, // programmable framing code- Don't care
0x59,0x54, // horizontal offset for slicer - don't care
0x5a,0x07, // vertical offset for slicer - Don't care
0};

/*! Buffer with encoder setup to send, starting from address 1*/
uint8_t saa7121InitData[] = {
0x3a,0x13, // H/V from CCIR, UV2C and Y2C straight binary
0x5a,0x0a, // CHPS. Colour might be better at 00?  where did 0x77 come from?
0x5b,0x7d, // U gain - white to black=100ire
0x5c,0xaf, // V Gain
0x5d,0x40+0x23, // (aa) blckl=23, decoe (use rtci) =1
0x5e,0x35, // default blanking level
0x5f,0x35, // ditto
0x61,0x06, // 00000101. scbw and fise
0x62,0xaf, // rtce=1 bsta=2f
0x63,0xcb, // FSC0 set for PAL
0x64,0x8a, // FSC1
0x65,0x09, // FSC2
0x66,0x2a, // FSC3 - Subcarrier frequency
0x67,0x55, // L21O0 - captioning data (four bytes per field)
0x68,0x56, // L21O1
0x69,0x67, // L21E0
0x6a,0x58, // L21E1
0x6b,0x20, // RCV port control (not used)
0x6c,0x01, // HTRIG
0x6d,0x30, // VTRIG
0x6e,0xa0, // SBLBN=1, PHRES=2 FLC=0
0x6f,0x20, // (00) CCEN=0, TTXEN=1 Enable teletext, SCCLN=0 (was 20)
0x70,0x00, // RCV2S 
0x71,0x00, //
0x72,0x00, // RCV2E
0x73,0x42, // Start of TTX signal. TTXHS=PAL=42 
0x74,0x02, // H delay TTXHD=data delay=4 - minimum=2
0x75,0x00, // VS_S
0x76,0x05, // TTXOVS=05 for PAL V odd start (line 5)
0x77,0x16, // TTXOVE=16 for PAL V odd end (line 22)
0x78,0x05, // TTXEVS=04 for PAL V even start (should be line 4) 
0x79,0x16, // TTXEVE=16 for PAL V even end (line 22)
0x7a,0x00, // FAL
0x7b,0x00, //
0x7c,0x00, // 
0x7d,0x00, //
0x7e,0x00, // (00) Disable TTX line
0x7f,0x00, // (00) Disable TTX line
0};


/*! /brief Example code
 *
 *  Example code that reads the key pressed and show a value from the buffer,
 *  sends the value to the slave and read back the processed value which will
 *  be inverted and displayed after key release.
 */
void i2c_init(void)
{

uint8_t buffer[256] = {
0,
0,0,0};
	unsigned int timeout=0;
	xputs(PSTR("I2C INIT\n"));
	
	/* Initialize TWI master. */
	TWI_MasterInit(&twiMaster,
	               &TWIC,
	               TWI_MASTER_INTLVL_LO_gc,
	               TWI_BAUDSETTING);

	/* Enable LO interrupt level. */
	PMIC.CTRL |= PMIC_LOLVLEN_bm;
	sei();

	uint8_t BufPos = 0;
	for (BufPos=0;saa7113InitData[BufPos];BufPos+=2)
	{
		xprintf(PSTR("\naddr=%02X data=%02X "),saa7113InitData[BufPos],saa7113InitData[BufPos+1]);
		if (!TWI_MasterWrite(&twiMaster,
							SLAVE_ADDRESS_7113,
							&saa7113InitData[BufPos],
							2))
			xputs(PSTR("Could not start transfer :-(\n"));

		timeout=0;
		while (twiMaster.status != TWIM_STATUS_READY)
		{
			/* Wait until transaction is complete. */
			timeout++;
			if (timeout>1000) return; // Or give up all hope
		}
		xprintf(PSTR("result=%02X\n"),twiMaster.result);
	}
	
	buffer[0]=0;
	for (int i=0;i<10;i++)
	{
	buffer[0]=i;
	TWI_MasterWriteRead(&twiMaster,
						SLAVE_ADDRESS_7113,
						buffer,
						1,
						1);
	timeout=0;
	while (twiMaster.status != TWIM_STATUS_READY)
	{
		/* Wait until transaction is complete. */
			timeout++;
			if (timeout>1000) return; // Or give up all hope
	}	
	xprintf(PSTR("\nbuffer[0]=%02X, result=%02X\n"),buffer[0],twiMaster.result); // 5=nack
	}					
	
	for (BufPos=0;saa7121InitData[BufPos];BufPos+=2)
	{
		xprintf(PSTR("\naddr=%02X data=%02X "),saa7121InitData[BufPos],saa7121InitData[BufPos+1]);
		if (!TWI_MasterWrite(&twiMaster,
							SLAVE_ADDRESS_7121,
							&saa7121InitData[BufPos],
							2))
			xputs(PSTR("Could not start transfer :-(\n"));

		timeout=0;
		while (twiMaster.status != TWIM_STATUS_READY)
		{
			/* Wait until transaction is complete. */
			timeout++;
			if (timeout>1000) return; // Or give up all hope
		}
		xprintf(PSTR("result=%02X\n"),twiMaster.result);	// 1=OK	
	}	
}

/** Set a value to addr in SAA7121
 */
void i2c_SetRegister(uint8_t addr, uint8_t value)
{
	uint8_t buffer[2];
	xputs(PSTR("I2C SET REGISTER\n"));
	buffer[0]=addr;
	buffer[1]=value;
	/* Initialize TWI master. */
	TWI_MasterInit(&twiMaster,
	               &TWIC,
	               TWI_MASTER_INTLVL_LO_gc,
	               TWI_BAUDSETTING);

	/* Enable LO interrupt level. */
	PMIC.CTRL |= PMIC_LOLVLEN_bm;
	sei();
	if (!TWI_MasterWrite(&twiMaster,
						SLAVE_ADDRESS_7121,
						buffer,
						2))
	xputs(PSTR("Could not start transfer :-(\n"));

	while (twiMaster.status != TWIM_STATUS_READY)
	{
		/* Wait until transaction is complete. */
		xputs(PSTR("*"));
	}
	xprintf(PSTR("result=%02X\n"),twiMaster.result);
}

/*! TWIC Master Interrupt vector. */
ISR(TWIC_TWIM_vect)
{
	TWI_MasterInterruptHandler(&twiMaster);
}

