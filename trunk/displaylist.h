/** ***************************************************************************
 * Description       : VBIT: Display List in Serial RAM
 * Compiler          : GCC
 *
 * Copyright (C) 2012, Peter Kwan
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
 * The author disclaim all warranties with regard to this
 * software, including all implied warranties of merchantability
 * and fitness.  In no event shall the author be liable for any
 * special, indirect or consequential damages or any damages
 * whatsoever resulting from loss of use, data or profits, whether
 * in an action of contract, negligence or other tortious action,
 * arising out of or in connection with the use or performance of
 * this software.
 *************************************************************************** **/
#ifndef _DISPLAYLIST_H_
#define _DISPLAYLIST_H_

#include "xitoa.h"
#include "..\Serial_RAM\Serial_RAM_Demo.h"

// What is the size of the serial ram? (32kBytes)
#define MAXSRAM (0x8000) 

typedef struct
{
uint16_t pageIndex;
uint16_t next;
// uint8_t mag;	// 0..7 where 0 is mapped to 8. mag is implicit
uint8_t page;	// Page number 0x00 to 0xff
uint8_t subpage; // 00 to 99 (not part of teletext standard).
// Value of subpage also defines the node type. N=00..99, R=100, J=101, F=102   
} DISPLAYNODE; 

#define ROOTNODE 100
#define JUNCTIONNODE 101
#define FREENODE 102


#define MAXNODES (MAXSRAM/sizeof(node))

// Copy node to address i in SRAM
void SetNode(DISPLAYNODE node, uint16_t i);

/** clear out the whole display list
*/
void initDisplayList(void);

#endif