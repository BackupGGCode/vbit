/** ***************************************************************************
 * Description       : Display List for VBIT teletext inserter
 * Compiler          : GCC
 *
 * Copyright (C) 2012, Peter Kwan
 *
 * Permission to use, copy, modify, and distribute this software
 * and its documentation for any purpose and without fee is hereby
 * granted, provided that the above copyright notice appear in all
 * copies and that both that the copyright notice itand thisboard
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
 ***************************************************************************
 * Hardware : The platform is a Mattair JD200 or MT-X1 XMega development board.
 *
 * Display List Manager
 * ====================
 * The display list controls the order of page transmission.
 * The display list:
 * Lives in the SRAM.
 * Uses a linked list.
 * Handles page updates such as add, replace, remove.
 * Maintains a sorted list of pages
 * Has special nodes for carousels and dynamically generated pages.
 * The display list points to the pages.all file and the page index.
 * Maintains magazine lists for parallel transmission.
 * 
 * Each node is a fixed size structure which contains:
 * Page pointer
 * Next node
 * Magazine
 * Page
 * Subpage 
 * Node type
 *
 * The page is a pointer [or an index?] to a page
 * Next node indexes the next node in the display list.
 * Magazine is 1..8 [If we ran separate mag lists then the mag would be implied]
 * Page is 0x00..0xFF
 * Subpage is 00 to 99
 * Node type can be N=normal, J=junction, 0=null C=carousel list.
 * A junction node is created if there is more than one subpage. It is used for carousels.
 * A null is used for the last item in a list. There should be a null at the end of each magazine.
 * However, a carousel list needs to be looped.
 * Unused nodes are cleared of data and placed in a free list.
 *
 * How are carousels handled?
 * The Junction node represents a page. It points to a list of the subpages in that page.
 * The subpages form a circular list. The current subpage is pointed to in the Junction node.
 * There is a sub-list used for carousels. Each item in the carousel list points to the junction and
 * has a count down timer. 
 * Anyway. When a carousel page is put up we need to push it to a carousel list.
 * Do we need one for each mag? Probably not. This can be another list of nodes
 * Each node has a pointer to its J node and a timer. If the timer reaches zero then
 * the carousel is flagged to be transmitted. The J node is loaded. The page extracted,
 * and the next page pointer loaded, the carousel list timer reset, (or deleted if the carousel is gone),
 * the J node updated with the next carousel page and finally the state machine primed to
 * transmit the page.
 * The carousel will also be refreshed in the normal transmission cycle. In this case it just
 * retransmits the page that it is currently pointing to.
 *
 * 
 * Note that we can't do true parallel transmission because we would need more file handles
 * than we can afford, given 8k of RAM.
 
 
 */
 
 #include "displaylist.h"
 
 static uint16_t FreeList; // FreeList is an index to the DisplayList. It points to the first free node.
 
 void SetNode(DISPLAYNODE node, uint16_t i)
 {
	i=i*sizeof(DISPLAYNODE);	// Find the actual serial ram address
	// TODO MAYBE. Check that i is less than MAXSRAM
	// put out all the values
	SetSPIRamAddress(SPIRAM_WRITE, i); // Write this node
	WriteSPIRam((char*)&node, sizeof(DISPLAYNODE)); // Assuming data is in same order as declaration with no byte alignment padding
	DeselectSPIRam();
 }
 
 void initDisplayList(void)
 {
	int i;
	DISPLAYNODE node; 
	xprintf(PSTR("Display list can contain up to %d nodes \n\r"),MAXNODES);
	
	// Initialise the serial RAM
	spiram_init();
	SetSPIRamStatus(SPIRAM_MODE_SEQUENTIAL);	

	// Set values common to all free nodes
	node.pageIndex=0;
	node.page=0;
	node.subpage=FREENODE;
	for (i=0;i<MAXNODES;i++)
	{
		if (i<MAXNODES-1)
			node.next=i+1;
		else
			node.next=0;	// Last node has no next
		SetNode(node,i);
	}
	FreeList=0;
	// The FreeList is now ready
	// Now scan the pages list and make a sorted list, creating nodes for Root, Node and Junction
 }