/* This file has been prepared for Doxygen automatic documentation generation.*/
/*! \file *********************************************************************
 *
 * \brief  XMEGA TWI driver.
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
 * Parts Copyright (c) 2010, Peter Kwan
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
/*! SAA7113 and SAA7121 address */
#define SAA7113_ADDRESS   	0x4a
#define SAA7121_ADDRESS		0x88


/*! CPU speed 16MHz, BAUDRATE 100kHz and Baudrate Register Settings */
#define CPU_SPEED       16000000
#define BAUDRATE	100000
#define TWI_BAUDSETTING TWI_BAUD(CPU_SPEED, BAUDRATE)

/* Global variables */
static TWI_t *twi=&TWIC;    /*!< TWI master module. */

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

void waitIdle(long int delay)
{
	long int n;
	for (n=0;n<delay;n++)
	{
		if ((twi->MASTER.STATUS & 0x03)==TWI_MASTER_BUSSTATE_IDLE_gc)
			return; // if idle
		else
		{
			if (!(n%1000))
			{
				xprintf(PSTR("%02X "),twi->MASTER.STATUS);
			}
		}
	}
	xputs(PSTR("\ntimed out\n"));
	twi->MASTER.STATUS = TWI_MASTER_BUSSTATE_IDLE_gc;			
}
	
	/*! /brief Example code
 * This code is based on the Atmel code. Since I2C is only accessed during startup
 * and rarely during changes in configuration, there is no point in unrolling
 * and optimising. Except that we do because the interrupt code is way to complicated.
 */
void i2c_init(void)
{	
	uint8_t BufPos;
	/* Initialize TWI master. */

	twi->MASTER.CTRLA  = TWI_MASTER_ENABLE_bm;
	twi->MASTER.BAUD   = TWI_BAUDSETTING;
	twi->MASTER.STATUS = TWI_MASTER_BUSSTATE_IDLE_gc;
	waitIdle(50000);
	// Read the version ID
	for (int i=0;i<10;i++)
	{
		waitIdle(50000);
		twi->MASTER.ADDR=SAA7113_ADDRESS | 0x01; // Set the read address

		twi->MASTER.DATA=i;        // Send the subaddress
			while (!(twi->MASTER.STATUS&TWI_MASTER_RIF_bm)) {
				xputs(PSTR("."));
				// Wait until transaction is complete.
			}
		BufPos=twi->MASTER.DATA;
		xprintf(PSTR(" ID returned=%02X\n"),BufPos);
	}
	twi->MASTER.STATUS = TWI_MASTER_BUSSTATE_IDLE_gc;		
	
	for(BufPos=0;saa7113InitData[BufPos];BufPos+=2)
	{
		waitIdle(50000);
		twi->MASTER.STATUS = TWI_MASTER_BUSSTATE_IDLE_gc;		
		xprintf(PSTR("\nAddr=%02X, Value=%02X "),saa7113InitData[BufPos],saa7113InitData[BufPos+1]);
		twi->MASTER.ADDR=SAA7113_ADDRESS; // Set the address

		twi->MASTER.DATA=saa7113InitData[BufPos];        // Send the subaddress
		while (!(twi->MASTER.STATUS&TWI_MASTER_WIF_bm)) {
			xputs(PSTR("*"));
			// Wait until transaction is complete.
		}
		twi->MASTER.DATA=saa7113InitData[BufPos+1];        // Send the value
		while (!(twi->MASTER.STATUS&TWI_MASTER_WIF_bm)) {
			xputs(PSTR("#"));
			// Wait until transaction is complete.
		}	
	}
	twi->MASTER.STATUS = TWI_MASTER_BUSSTATE_IDLE_gc;		
}
