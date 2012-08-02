/*****************************************************************************
 * Description       : stream manager for VBIT/XMEGA
 * Compiler          : GCC (WinAVR)
 *
 * This module manages streams.
 * A stream is a sequence of pages.
 * A stream has these properties:
 * One stream per magazine.
 * Streams have priority so some streams can transmit faster than others.
 * Each magazine has its own stream.
 * Streams sort pages so they are presented in ascending order except...
 * When streams manage carousels they are presented by schedule which can break the sequence 
 * Streams know nothing about the transmission line or field.
 * ... therefore something else must manage overlapping headers etc. 
 * 
 * The only global function is
 * GetNextPage(MAGMASK mask) - return a page node for transmission
 * 
 * 
 * 
 * 
 * 
 *
 * Copyright (c) 2012 Peter Kwan
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
 /** \brief Teletext magazine stream management
*/
#ifndef _STREAM_H_ 
#define _STREAM_H_ 

#include "avr_compiler.h"
#include <avr/pgmspace.h>
#include "xitoa.h"
#include "vbit.h"
#include "displaylist.h"
//#include "fifo.h"
//#include "vbi.h"
//#include "databroadcast.h"
//#include "../SDCard/ff.h"
//#include "../SDCard/diskio.h"
//#include "sdfilemanager.h"


// Each bit 0..7 is a magazine number, where mag 8 is bit 0.
// If a bit is not set then GetNextPage can not return a page from that mag
typedef uint8_t MAGMASK;

/**\brief Returns a pointer to the next page for transmission
 * \param packet : Set of bits which define which magazines may be transmitted.
 */
NODEPTR GetNextPage(MAGMASK mask);

/**\brief Returns a page, consisting of a seek pointer to page.all and the page size
 * \return 0 if OK, >0 if problem
*/
uint8_t GetPage(uint32_t *pageptr,uint32_t *pagesize, MAGMASK mask);

/** \brief Initialise the streams
 */
void InitStream(void);

#endif
