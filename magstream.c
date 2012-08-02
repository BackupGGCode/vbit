/*****************************************************************************
 * Description       : Stream manager for VBIT/XMEGA
 * Compiler          : GCC
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
 /** \file stream.c
 * Stream management
 */

#include "magstream.h"

static uint8_t MagPriority[8]; // 0..9.
static uint8_t MagLevel[8];		// 0..9

static uint8_t MagPtr[8];

/** MagStreamer. Find the next page from this magazine 
 * \param mag - Magazine number
 * \return NODEPTR to the next page, or NULL
 */
static NODEPTR MagStreamer(uint8_t mag)
{
	uint8_t page, pagestart;
	uint16_t cellAddress;
	NODEPTR np;
	// xprintf(PSTR("[MagStreamer] Enters looking for the next page in mag %d\n\r"),mag);
	// Pointer to the last transmitted page;
	mag&=0x07;	// Wrap mag 8
	pagestart=MagPtr[mag];
	MagPtr[mag]++;
	// Iterate through this magazine looking for a page.
	while (1)
	{
		// Find the cell address of the current page, and get the node pointer
		page=MagPtr[mag];
		cellAddress=(((mag & 0x07)<<8)+page)*sizeof(NODEPTR);
		np=GetNodePtr(&cellAddress);
		if (np==NULLPTR)
			MagPtr[mag]++;	// Iterate through this mag
		else
		{
			// xprintf(PSTR("[MagStreamer] Exits with mag[%d]->%d\n\r"),mag,np);
			return np;
		}
		// double mobius wrap-around
		if (pagestart==page)
			return NULLPTR;
		// TODO: The stuff goes here
		// We need pointers to the current node and a special array for carousels 
		// TODO: check if any carousels are due to go out on this mag
	}
} // MagStreamer

/** MagPrioritiser.
 * \return Magazine number (exceptions: 0 means mag 8. >7 means none 
 * Why so complicated?
 * 1) Need to prioritise magazines.
 * 2) Need to ensure that we don't have infinite loops
 * 3) Need to ensure that streams doesn't completely block others.
 */
static uint8_t MagPrioritiser(MAGMASK mask)
{
	static uint8_t magIndex=0;
	uint8_t indexSave=magIndex;
	// xprintf(PSTR("[MagPrioritiser] Enters\n\r"));
	// TODO: The stuff goes here
	if (!mask)
		return 9; // Fail. This can't happen!
	while (1)
	{
		magIndex++;		// next mag
		magIndex&=0x07;	// wrap it around if needed
		if (mask & (1<<magIndex)) // Do we consider this mag?
		{
			if (!MagLevel[magIndex]) // When we hit zero, the Mag gets selected
			{
				// This is the mag we want
				// Reset the priority level			
				if (MagPriority[magIndex])
					MagLevel[magIndex]=MagPriority[magIndex];
				else
					MagLevel[magIndex]=1;
				return magIndex+1;
			}
			else
				MagLevel[magIndex]--;	// Count down
		}
	}
	return 9; // fail
	//xprintf(PSTR("[MagPrioritiser] Exits\n\r"));
} // MagPrioritiser

/** GetNextPage. Get the nodeptr of the next page to transmit.
 * \param mask - a bit mask which controls which magazines can be returned
 * According to transmission rules, use mask to ensure that only page from a magazine goes out at a time 
 */
NODEPTR GetNextPage(MAGMASK mask)
{
	uint8_t mag;
	NODEPTR node;
	//xprintf(PSTR("[GetNextPage] Enters\n\r"));
	// Get the mag number
	if (mask)
		mag=MagPrioritiser(mask);
	else
		return NULLPTR; // Weird. Nothing to insert
	// xprintf(PSTR("We are going to do mag=%d\n\r"),mag);
	node=MagStreamer(mag);
	return node;
	// This will be called in packet.c
	// from where it will request the next page.
	// It then needs to fetch the node and extract the page index
	// Then set up the page ready to transmit
	// This will be in packet.c where listFIL gets accessed
	//xprintf(PSTR("[GetNextPage] Exits\n\r"));
	return NULLPTR;	// TODO: Return a proper value
} // GetNextPage

/** GetPage - Given a nodeptr, returns the address and size of the page
 * \param pageptr - a DWORD for the seek address
 * \param pagesize - a DWORD for the page size
 * \param mask - a bitmask indicating any mags from which we do not want a page
 */
uint8_t GetPage(uint32_t *pageptr,uint32_t *pagesize, MAGMASK mask)
{
	FRESULT res;
	static uint8_t opened=0;
	NODEPTR np;
	uint16_t charcount;	
	DISPLAYNODE node;
	PAGEINDEXRECORD ixRec;	
	np=GetNextPage(mask); // This is the node pointer of the next page to go out
	GetNode(&node,np);	// And this is the node contents of the page
	// We don't need to keep opening the file, it should be opened once only.
	if (!opened)
	{
		res=f_open(&listFIL,"pages.idx",FA_READ);
		if (res) // GetPage
		{
			xprintf(PSTR("[displaylist]Epic Fail. Can not open pages.idx\n"));			
			put_rc(res);
			// At this point we might try to create page.all and pages.idx
			return res;	
		}
		opened=1;
	}
	// Now we know the page index, lets fetch the index record
	f_lseek(&listFIL,(node.pageindex)*sizeof(PAGEINDEXRECORD));
	f_read(&listFIL,&ixRec,sizeof(PAGEINDEXRECORD),&charcount);
	// TODO: Check that charcount is plausible
	*pageptr=ixRec.seekptr;
	*pagesize=ixRec.pagesize;
	//xprintf(PSTR("[GetPage] exits ptr=%l size=%l\n\r"),*pageptr, *pagesize);
	return 0;
} // GetPage

/** InitStream - sets up default priorities
 *  Call this before starting the video chain
 */
void InitStream(void)
{
	uint8_t i;
	// Set the stream priority all to 1
	for (i=0;i<8;i++)
	{
		MagPriority[i]=MagLevel[1]=2;	// TODO: The Priority level must come from the ATP420 style INI values
	}
	MagPriority[0]=1;	// Give mag 1 priority
} // InitStream