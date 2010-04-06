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
extern volatile uint8_t vbiDone; /// Goes high when the vbi has been transmitted
extern volatile uint32_t UTC; /// Universal Coordinated Time
void InitVBI(void);
extern volatile uint8_t FIFOBusy; /// High when the FIFO is due to be transmitted
// A block is 45 bytes * 17 or 18 lines = 810 bytes (max)
// The SPIRAM is 32kb. ie 0x40000/8=32768
// There is enough space in the SPIRAM for 32768/810 = 40 blocks
// But we also use a block number to denote the odd/even phase 
// so we need an even number of blocks which gives us the same 40.
#define FIFOBLOCKSIZE 810
#define MAXFIFOINDEX 40
extern volatile uint8_t fifoReadIndex; /// maintains the tx block index 0..39
extern volatile uint8_t fifoWriteIndex; /// maintains the load index 0..39
#endif
