/*****************************************************************************
 * Description       : packet generation for VBIT/XMEGA
 * Compiler          : GCC
 *
 * This module is used to fill the FIFO with teletext packets.
 * Packets are filled according to the output action list
 * The main packet types are:
 *  1) Header
 *  2) Row
 *  3) Filler
 *  4) Packet 8/30 format 1
 *  5) Quiet (not a WST packet)
 *  There may be other types added later
 *  6) Databroadcast
 *  7) Other packet 8/30 formats
 *
 * Copyright (c) 2010 Peter Kwan
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following
 * conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.

 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 * The name(s) of the above copyright holders shall not be used in
 * advertising or otherwise to promote the sale, use or other
 * dealings in this Software without prior written authorization.
 *
 *****************************************************************************/ 
 /** \brief General teletext packet management
*/
#ifndef _PACKET_H_ 
#define _PACKET_H_ 

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "avr_compiler.h"
#include <avr/pgmspace.h>
#include "xitoa.h"
#include "vbit.h"
#include "fifo.h"
#include "vbi.h"
#include "databroadcast.h"
#include "../SDCard/ff.h"
#include "../SDCard/diskio.h"
#include "sdfilemanager.h"

#define PACKETSIZE 45

/// Defines for SetHeaderControl parameter bit masks. C numbers refer to WST
#define CTRL_C5_NEWFLASH_bm				0x0001	/* C5 */
#define CTRL_C6_SUBTITLE_bm  			0x0002
#define CTRL_C7_SUPPRESSHEADER_bm 		0x0004
#define CTRL_C8_UPDATE_bm 				0x0008
#define CTRL_C9_INTERRUPTEDSEQUENCE_bm 	0x0010
#define CTRL_C10_INHIBITDISPLAY_bm 		0x0020
#define CTRL_C11_MAGAZINESERIAL_bm 		0x0040
#define CTRL_C12_LANGUAGE0_bm 			0x0080
#define CTRL_C13_LANGUAGE1_bm 			0x0100
#define CTRL_C14_LANGUAGE2_bm 			0x0200	
#define CTRL_LANGUAGE_0_bm				0x0000	/* English */
#define CTRL_LANGUAGE_1_bm				0x0080	/* German */
#define CTRL_LANGUAGE_2_bm				0x0100	/* Swedish/Finnish/Hungarian */
#define CTRL_LANGUAGE_3_bm				0x0180	/* Italian */
#define CTRL_LANGUAGE_4_bm				0x0200	/* French */
#define CTRL_LANGUAGE_5_bm				0x0280	/* Portuguese/Spanish */
#define CTRL_LANGUAGE_6_bm				0x0300	/* Czech/Slovak */
#define CTRL_LANGUAGE_7_bm				0x0380	/* ? */
/// Additional VBIT specific control bits. Same as MRG S: Set Page Status
#define CTRL_TIMEDPAGE_bm				0x0400	/* ? Probably will do this a different way */
#define CTRL_REPLACEINCOMING_bm			0x0800	/* N/A Used for bridging */
#define CTRL_RESERVED0_bm				0x1000	/* ? */
#define CTRL_RESERVED1_bm				0x2000	/* ? */
#define CTRL_C4_ERASEBIT_bm				0x4000	/* Erase all rows of the previous transmission of the page */
#define CTRL_ENABLETX_bm				0x8000	/* Set this to enable transmission */

#define OPTOUT_PREROLL 0
#define OPTOUT_START 1
#define OPTOUT_STOP 2


/**\brief Writes the CRI, FC , MRAG to a standard text packet
 * \param packet : Buffer to add the MRAG
 */
void WriteMRAG(char *packet, unsigned char mag, unsigned char row);

/**\brief Loads a buffer with a filler packet
 * \param buffer : Buffer to hold the packet
 */
void FillerPacket(char *buffer);

/**\brief Loads the FIFO with the next field of packets
 */
void FillFIFO(void);

/// Dataline Output Actions array. 
extern char g_OutputActions[2][18];
extern char g_Header[32];

extern void put_rc (FRESULT rc);
extern FATFS Fatfs[1];

// Why are these shared?
// Because they are so huge we can't afford not to.
// These also get accessed during startup
extern FIL pagefileFIL, listFIL;

extern int OptRelays;			/* Holds the current state of the opt out relay signals */

extern unsigned char OptOutMode;	// Which ATP950 mode
extern unsigned char OptOutType;	// One of the OPTOUT values
#endif