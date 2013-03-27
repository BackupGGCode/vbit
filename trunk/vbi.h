/*****************************************************************************
 * Description       : VBI timing control for VBIT/XMEGA
 * Compiler          : GCC
 *
 * This module handles vbi timing in this sequence:
 *  1) An interrupt on both edges detects a change in the field signal. 
 *  2) This starts a 1ms timer which is the period where the vbi is transmitted.
 *  3) When the timer completes it sets a flag indicating that the fifo is available.
 *  4) Another timer is then started for 18ms. When this completes it indicates
 *     that the FillFIFO routine must stop and ready the FIFO to transmit.
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
#ifndef _VBI_H_
#define _VBI_H_
#include "avr_compiler.h"
#include "xitoa.h"
#include "vbit.h"
#include "databroadcast.h"

extern volatile uint8_t vbiDone; /// Goes high when the vbi has been transmitted
extern volatile uint32_t UTC; /// Universal Coordinated Time
uint8_t InitVBI(void);
extern volatile uint8_t FIFOBusy; /// High when the FIFO is due to be transmitted
// A block is 45 bytes * 17 or 18 lines = 810 bytes (max)
// The SPIRAM is 32kBytes. ie 0x40000/8=32768
// There is enough space in the SPIRAM for 32768/810 = 40 blocks
// But we also use a block number to denote the odd/even phase 
// so we need an even number of blocks which gives us the same 40.
// We will grab some of this memory for dynamic pages
// [Idea: However, we want to allow for queue jumping to go on with subtitles
// and any other "real time" page insertion. We will do this by
// reserving one pair of blocks. A flag will signify if one or both
// of these blocks has data (subtitles or live pages) that need to go out. ]
#define SPIRAMSIZE 32768
#define FIFOBLOCKSIZE 810

// 40 is the maximum that we can fit.
// We want a big buffer of 40 blocks because if the CPU falls behind then it has 0.8 seconds 
// to catch up. Things that slow the AVR down. File handling on the SD card. Writing to EEPROM.
// However it makes live updates tardy and makes clocks delayed.
// The smallest buffer that is possible is two blocks. 
// #define MAXFIFOINDEX 40
// So lets cut it down to 20 ahead.
#define MAXFIFOINDEX 20

// Now lets do some other calculations for what we are going to do with the spare space.
// Assuming that we are storing whole pages in the form of raw packets.
// These will be the dynamic pages, ones that play out direct from RAM.

// The base address of these pages above the FIFO area is 810*20 = 16200.
#define SRAMPAGEBASE (FIFOBLOCKSIZE*MAXFIFOINDEX)

#define SRAMPAGEPACKETS 26
// 24 lines + header + fastext = 26 x 45 = 1170 bytes per page.
// Probably we should generate row 0 from the stub file or it will mess up the packet sequencer.
// [what is stub file? It is a tti page that only has a header and a redirect. The idea is
// that the file is handled like any other page so the packet generation is not affected.
// It will also supply row 0 so we probably can save a row 0 here by not storing it]
// This works out at 1170 bytes per dynamic page
#define SRAMPAGESIZE (SRAMPAGEPACKETS*PACKETSIZE)
// How many of these pages can we have?
// I make it (32768-810*20)/1170=14
// So this is RD,0 to RD,d
#define SRAMPAGECOUNT ((SPIRAMSIZE-SRAMPAGEBASE)/SRAMPAGESIZE)

// So to access the page block n, the equation is
// SRAMPAGEBASE+n*SRAMPAGESIZE
// where n=0..SRAMPAGECOUNT-1

extern volatile uint8_t fifoReadIndex; /// maintains the tx block index 0..MAXFIFOINDEX-1
extern volatile uint8_t fifoWriteIndex; /// maintains the load index 0..MAXFIFOINDEX-1
#endif
