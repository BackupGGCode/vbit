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

#include "stdint.h"
#include "xitoa.h"
typedef uint16_t NODEPTR;

#include "..\Serial_RAM\Serial_RAM_Demo.h"
#include "packet.h"

// What is the size of the serial ram? (32kBytes)
#define MAXSRAM (0x8000) 

/* A pageindex record as stored in pages.idx */
typedef struct
{
	uint32_t seekptr;	// This is the pointer to the start of a page
	uint16_t pagesize;  // And the number of bytes in the page
} PAGEINDEXRECORD;

/** defines a display list node. However...
 * This is only used to sort subpages.
 * The actual mag and page are in PageArray
 */
typedef struct
{
NODEPTR pageindex; // Not sure that this is a NODEPTR type but we can worry about that later
// pageindex is an index to page.idx. It is either a file offset value for l_seek, or an index.
// Which one is it?
NODEPTR next;
// uint8_t mag;	// 0..7 where 0 is mapped to 8. mag is implicit
// uint8_t page;	// Page number 0x00 to 0xff
uint8_t subpage; // 00 to 99 (not part of teletext standard).
// Value of subpage also defines the node type. N=00..99, R=100, J=101, F=102   
} DISPLAYNODE; 

// extra subpage values
#define ROOTNODE 100
#define JUNCTIONNODE 101
#define FREENODE 102
#define NULLNODE 103

// For Display List pointers that don't go anywhere
#define NULLPTR 0xffff

/**
We don't have the luxury of a compiler that can manage the serial ram allocation.
We must track where we put stuff ourself.
The data is in two parts, a big index
1) 2D array of NODEPTR arranged mag x page. Each cell in this array points to either
	a NULLPTR or a pointer to a page list
2) A set of linked nodes one for each page. A static page only has one node. A carousel
	of subpages can have up to 99 nodes.
*
* A declaration would ordinarily look like this:
* NODEPTR PageArray[8][256];
* DISPLAYNODE PageList[(MAXSRAM-sizeof(PageArray))/sizeof(DISPLAYNODE)];
*
* The memory allocation map is computed thus:
* PageArray has 8 magazines, each of which can have 256 pages. Each cell is a 2 byte NODEPTR.
* 8x256x2=4kB or 0x1000
* So PageArray is 0x0000 to 0x1000 (16 bit index)
*
* The maximum number of nodes that can fit in the PageList are:
* (0x8000-0x1000)/2=0x1666 or 5734
*/

// Should be array size 4096 and node count 5734
// PAGEARRAY is the lower 4k words of the serial ram
#define PAGEARRAYSIZE (8 * 256 * sizeof (NODEPTR))
// Nodes are in the remainder of the serial ram
#define MAXNODES ((MAXSRAM - PAGEARRAYSIZE)/sizeof(DISPLAYNODE))

/*
// How to find a value?
A cell is accessed by this formula:
cell address = (mag*0x100+page)*sizeof(NODEPTR);

A node number is in the range 0..MAXNODES
node address = (node * sizeof(node)) + PAGEARRAYSIZE
*/

/** clear out the whole display list
*/
void InitDisplayList(void);
// ??void AddPage(??,??);
// uint8_t DeleteRange(char *range);

NODEPTR GetNodePtr(uint16_t *addr);
void GetNode(DISPLAYNODE *node,NODEPTR i);
void DumpNode(NODEPTR np);

/** Given a newly added page appended to page.all and page.idx
 *  constructs the page index and node so that it can be displayed
 */
void LinkPage(uint8_t mag, uint8_t page, uint8_t subpage, uint16_t ix);
 
#endif