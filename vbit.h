/** ***************************************************************************
 * Description       : VBIT teletext program for XMEGA
 * Compiler          : GCC
 *
 * Copyright (C) 2010, Peter Kwan
 *
 * Permission to use, copy, modify, and distribute this software
 * and its documentation for any purpose and without fee is hereby
 * granted, provided that the above copyright notice appear in all
 * copies and that both that the copyright notice and this
 * permission notice and warranty disclaimer appear in supporting
 * documentation, and that the name of the author not be used in
 * advertising or publicity pertaining to distribution of the
 * software without specific, written prior permission.
 *
 * The author disclaims all warranties with regard to this
 * software, including all implied warranties of merchantability
 * and fitness.  In no event shall the author be liable for any
 * special, indirect or consequential damages or any damages
 * whatsoever resulting from loss of use, data or profits, whether
 * in an action of contract, negligence or other tortious action,
 * arising out of or in connection with the use or performance of
 * this software.
 *************************************************************************** **/

#ifndef _VBIT_H_
#define _VBIT_H_

 
 
/** ************** Includes ************** **/
#include <avr/io.h>
#include <avr/pgmspace.h>
/*
#include <avr/interrupt.h>
	*/
//#include <assert.h>
#include <stdio.h>

#include "../minini/minIni.h"		
#include <string.h>
#include "xitoa.h"
//#include "../Common/Bridge.h"
#include "../Drivers/Port/port_driver.h"
#include "../SDCard/ff.h"
#include "../SDCard/diskio.h"
#include "sdfilemanager.h"
#include "packet.h"
#include "p830f1.h"
#include "fifo.h"
#include "i2c.h"
#include "vbi.h"
#include "databroadcast.h"
#include "displaylist.h"
		
#include "../../Common/Terminal/TerminalDriver.h"
#include "../../Common/Terminal/TerminalEvents.h"	

#include "../USB_Serial/USB_Serial.h"	
/*
#include "../RTC/rtc.h"
*/

/* Command mode */
#define CMD_MODE_NONE  0
#define CMD_MODE_READ  1
#define CMD_MODE_WRITE 2

/* Ports */

/** Macros **/
#ifndef _INLINE_
#define _INLINE_ static inline __attribute__((always_inline))
#endif

/* VBIT specific GPIO on Port C */
#define VBIT_SDA		(PIN0_bm)
#define VBIT_SCL		(PIN1_bm)
#define VBIT_FLD		(PIN2_bm)
#define VBIT_SEL		(PIN3_bm)
#define VBIT_SS			(PIN4_bm)
#define VBIT_MOSI		(PIN5_bm)
#define VBIT_MISO		(PIN6_bm)
#define VBIT_SCK		(PIN7_bm)

/** Inline Functions. Why do this? It is only executed once **/
_INLINE_ void GPIO_Init(void)	// default off (low)
{
	PORT_SetPinsAsOutput( &PORTC, VBIT_SEL | VBIT_SCK | VBIT_SS );
	PORT_ClearPins( &PORTC, VBIT_SEL | VBIT_SCK );
}

_INLINE_ void GPIO_On(uint8_t gpiobits)
{
	PORT_SetPins( &PORTC, gpiobits );
}

_INLINE_ void GPIO_Off(uint8_t gpiobits)
{
	PORT_ClearPins( &PORTC, gpiobits );
}	

#define NDEBUG

	

/** ************** Typedefs ************** **/



/** ************** Function Prototypes ************** **/
int RunVBIT(void);


#endif