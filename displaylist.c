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
 * Lives in the SRAM therefore it must be constructed at the start of each run
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
 
 static NODEPTR sFreeList; // FreeList is an index to the DisplayList. It points to the first free node.
 static NODEPTR sDisplayList; // Root of the display list
 
 /* Write node to slot i in the serial ram */
 void SetNode(DISPLAYNODE node, NODEPTR i)
 {
	i=i*sizeof(DISPLAYNODE);	// Find the actual serial ram address
	// TODO MAYBE. Check that i is less than MAXSRAM
	// put out all the values
	SetSPIRamAddress(SPIRAM_WRITE, i); // Write this node
	WriteSPIRam((char*)&node, sizeof(DISPLAYNODE)); // Assuming data is in same order as declaration with no byte alignment padding
	DeselectSPIRam();
	//if (node.subpage==FREENODE)
	//	xprintf(PSTR("Ouch! SetNode just stored a FREENODE\n\r"));
	
 } // SetNode
 
 /* Fetch a node from the slot in serial ram
  * Would this be better being passed by reference?
  */ 
 DISPLAYNODE GetNode(NODEPTR i)
 {
	DISPLAYNODE node;
	i=i*sizeof(DISPLAYNODE);	// Find the actual serial ram address
	SetSPIRamAddress(SPIRAM_READ, i); // Write this node
	ReadSPIRam((char *)&node, sizeof(DISPLAYNODE));
	DeselectSPIRam();
	if (node.subpage==FREENODE)
		xprintf(PSTR("[GetNode]Ouch! Just got a FREENODE %d\n\r"),i/sizeof(DISPLAYNODE));
	return node;
 } // GetNode
 
 void DumpNode(NODEPTR np)
 {
	DISPLAYNODE n;
	n=GetNode(np);
	xprintf(PSTR("Node (%d) pageindex=%d next=%d page=%d subpage=%d\n\r"),np,n.pageindex,n.next,n.page,n.subpage);
 } // DumpNode
 
 static void Dump(void)
 {
	uint16_t i;
		xprintf(PSTR("[Dump] ... \n\r"));	
	for (i=0;i<10;i++)
		DumpNode(i);
 }
 
 /** Grab a node from the free list
  * \return a node pointer
  */
 NODEPTR NewNode(void)
 {
	NODEPTR ix=sFreeList;	// The first node in the free list is what we are going to grab
	DISPLAYNODE node=GetNode(sFreeList); // So we need to update the FreeList pointer
	// TODO: Check that we didn't empty the free list
	if (node.subpage==NULLNODE)
	{
		// TODO: Oh dear. What can we do now? We are out of nodes
		xprintf(PSTR("[NewNode] NULLNODE error\n\r"));
	}
	else
		sFreeList=node.next;
	xprintf(PSTR("[NewNode] Returns %d\n\r"),ix);
	return ix;
 } // NewNode

 /** Given a serial ram slot i, 
  * It clears out slot i and links it into the free list
  * We probably should call this from initDisplayList too as the code is duplicated
  * WARNING. You must unlink this node or the display list will get chopped
  */
 void ReturnToFreeList(NODEPTR i)
 {
	DISPLAYNODE node;
	// Set the values in this node
 	node.pageindex=0;
	node.page=0;
	node.subpage=FREENODE;
	node.next=sFreeList; // This node points to the rest of the list
	SetNode(node,i);	// TODO: Check that i is in range
	sFreeList=i;		// And the free list now points to this node
 } // ReturnToFreeList
 
 /* Sets the initial value of all the display list slots
  * and joins them together into a freelist.
  */
 void MakeFreeList(void)
 {
	int i;
	DISPLAYNODE node; 
	xprintf(PSTR("Display list can contain up to %d nodes \n\r"),MAXNODES);

	sFreeList=0;
	// We need to make one node to start with
	node.pageindex=0;
	node.page=0;
	node.next=0;
	node.subpage=NULLNODE;
	SetNode(node,0);
	for (i=MAXNODES-1;i>=0;i--)
	{
		ReturnToFreeList(i);
	}
	// The FreeList is now ready
 } // MakeFreeList
 
 /** Given a mag, this adds the mag to the Root list
  *  \return Node pointer to the root node of the newly created magazine.
  * We call this if findmag failed
  */
 NODEPTR CreateRoot(NODEPTR rootNode,uint8_t mag)
 {
	DISPLAYNODE node,n;
	NODEPTR np,lastnp,newrootnp,savenp;
	xprintf(PSTR("Creating Root for mag %d\n\r"),mag);
	np=NULLPTR;
	node.pageindex=NULLPTR; // Node is the new map root
	node.next=NULLPTR;
	node.page=mag;	// for the root nodes, page is used to hold mag
	node.subpage=ROOTNODE;	// Identify this as a root node.
	newrootnp=NewNode();	// Our new mag will be written to here
	// If the display list is completely empty we need to start one now
	if (sDisplayList==NULLPTR)
	{
		// create the node
		sDisplayList=newrootnp;
		SetNode(node,sDisplayList);
	}
	else
	{
		// We have a list to traverse
		// Find where to insert the new mag, keep it sorted by mag
		lastnp=np=sDisplayList;
		while (1)
		{
			n=GetNode(np);
			if (n.subpage!=ROOTNODE)
				xprintf(PSTR("[CreateRoot] ERROR. Not a Root node"));
			if (n.page<mag) // our node is not found yet
			{
				if (n.next==NULLPTR) // Last node? Link our node
				{
					xprintf(PSTR("[CreateRoot] Adding to the end"));
					// Set the new root and put the node back
					n.next=newrootnp; // This node has a new child
					SetNode(n,np);	// Update the last root node
					SetNode(node,newrootnp);	// And save the new mag root
					break;
				}
				else
				{
					lastnp=np;	// Save the last node in case we want to insert.
					np=n.next;
				}
			}
			if (n.page>mag) // We have gone past.
			{
				// Do an insert.
					xprintf(PSTR("[CreateRoot] Doing an insert\n\r"));
				// First update the last node by pointing next to our new mag root				
				n=GetNode(lastnp);
				if (n.subpage==FREENODE)
					xprintf(PSTR("[CreateRoot] ERROR, got a FREENODE\n\r"));
				
				savenp=n.next;
				n.next=newrootnp;
				SetNode(n,lastnp);
				// And update the new map root with the moved next link
				node.next=savenp;
				SetNode(node,newrootnp);
				break;	// Done 
			}
		} // while
	}
	xprintf(PSTR("[CreateRoot] exits\n\r"));	
	return newrootnp; // We always return what we just created
 } // CreateRoot
 
 /** Insert a page into a magazine
  * \param np - Pointer to the mag root node
  * \param node - node to insert
  *  \return Might be useful to return something 
  * This is called with a mag root pointer returned by FindMag
  */
 void LinkPage(NODEPTR np,DISPLAYNODE newnode)
 {
	DISPLAYNODE node,n;
	NODEPTR lastnp,savenp,newnodeptr;
	xprintf(PSTR("[LinkPage] enters with np=%d\n\r"),np);
	uint8_t pp; // Page number to insert
	
	// Validate the magazine root node
	if (np==NULLPTR)
	{
		xprintf(PSTR("[LinkPage] null root error\n\r"));
		return;
	}
	
	node=GetNode(np);
	
	if (node.subpage!=ROOTNODE)
	{
		xprintf(PSTR("[LinkPage] ERROR, Initial node is not a root node (%d)\n\r"),node.subpage);
		return;
	}
	
	// Validations done. Allocate a place for our inserted node.
	newnodeptr=NewNode();
	
	// First time the mag will be empty.
	if (node.pageindex==NULLPTR)
	{
		node.pageindex=newnodeptr;	// Point the root node to our new node
		SetNode(node,np);
		SetNode(newnode,newnodeptr);	// Tack the new node on the end
		return;
	}
	
	// The magazine list is not empty. We traverse it looking for the insert point
	pp=newnode.page;
	// Note that for a ROOTNODE, pageindex is the pointer to the first page in the mag 
	lastnp=np=node.pageindex; // The first pointer comes from the mag root
	for (;1;)
	{
		// We got to the end of the list without finding it?
		// Tack our new node onto the end
		if (node.next==NULLPTR)
		{
			node.next=newnodeptr;
			SetNode(node,np);
			SetNode(newnode,newnodeptr);
			break;
		}
		// Not got there yet?
		if (node.page>pp)
		{
			lastnp=np;			// Save this, we might need it
			np=node.next;		// Traverse to the next node
			node=GetNode(np);
			if (node.subpage>100)
				xprintf(PSTR("Oops. This node should be a subpage\n\r"),node.subpage);
		}
		// Did we get there?		// TODO: Consider subpages
		if (node.page<=pp)
		{
			// Go back one link and point it to the new node			
			n=GetNode(lastnp);
			n.next=newnodeptr;
			SetNode(n,lastnp);
			// Special case. Page exists already. Dump the previous page we don't want
			if (node.page==pp)
			{
				newnode.next=node.next;	// Save the next pointer before we dispose of the node
				ReturnToFreeList(np);
			}
			else
				newnode.next=np;	// Link to the node 
			SetNode(newnode,newnodeptr);
			
			break;
		}
	} // for
	xprintf(PSTR("[LinkPage] exits\n\r"));
 } // LinkPage
 
/** FindMag - Find a magazine in the root list
 * \return Pointer to the magazine or NULLPTR if failed
 */
 
NODEPTR FindMag(NODEPTR dispList,uint8_t mag)
{
	xprintf(PSTR("[FindMag]Enters\n\r"));
	DISPLAYNODE node;
	// The display list has to be set to something
	while (dispList!=NULLPTR)
	{
		// Fetch the display List Item
		node=GetNode(dispList);
		xprintf(PSTR("[FindMag]"));DumpNode(dispList);
		// Is not a root node? Then return NULL
		if (node.subpage!=ROOTNODE)
		{
			xprintf(PSTR("[FindMag] Not a root node\n\r"));
			break;
		}
		// Is the magazine the one that we seek?
		if (node.page==mag) // !! For Root Nodes, page contains mag. !!
		{
			xprintf(PSTR("[FindMag] Found displist=%d mag=%d!\n\r"),dispList,mag);
			return dispList;	// YAY! return dispList
		}
		// Not found yet? Get the next one.
		dispList=node.next;
	}
	xprintf(PSTR("[FindMag]Failed to find mag %d\n\r"),mag);
	return NULLPTR; // We failed. Sorry.
} // FindMag

 
 /** This takes the page.idx list and makes a sorted display list out of it
  * We need to look at all the pages and extract their MPPSS
  */
 void ScanPageList(void)
 {
	FIL PageIndex;		// page.idx
	FIL PageAll;	// page.all	
	BYTE drive=0;
	FRESULT res;	
	PAGEINDEXRECORD ixRec;
	UINT charcount;	
	PAGE page;
	uint8_t ix;
	const unsigned char MAXLINE=80;
	
	char line[MAXLINE];
	char *str;
	
	NODEPTR root;
	DISPLAYNODE node;
	
	// Open the drive and navigate to the correct location
	res=(WORD)disk_initialize(drive);	// di0
	put_rc(f_mount(drive, &Fatfs[drive]));	// fi0
	put_rc(f_chdir("onair"));	
	
	res=f_open(&PageIndex,"pages.idx",FA_READ);
	if (res)
	{
		xprintf(PSTR("[displaylist]Epic Fail. Can not open pages.idx\n"));			
		put_rc(res);
		// At this point we might try to create page.all and pages.idx
		return;
	}
	
	res=f_open(&PageAll,"pages.all",FA_READ);
	if (res)
	{
		xprintf(PSTR("[displaylist]Epic Fail. Can not open page.all\n"));			
		put_rc(res);
		f_close(&PageAll);
		return;
	}
	
	for (ix=0;!f_eof(&PageIndex);ix++)
	{
		f_read(&PageIndex,&ixRec,sizeof(ixRec),&charcount);
		xprintf(PSTR("seek %ld size %d \n\r"),ixRec.seekptr,ixRec.pagesize);
		// TODO: Use seekptr on page.all and parse the page
		f_lseek(&PageAll,ixRec.seekptr);	// and set the pointer back to the start
		// TODO: Extract the M PP SS fields
		page.mag=9;
		while (page.mag==9) // TODO: Prevent this from going badly wrong!!!
		{
			str=f_gets(line,MAXLINE,&PageAll);		
			ParseLine(&page,str);
			//TODO: Check that we didn't read past the end of this page
		}
		xprintf(PSTR("M PP SS %1d %02X %02d\n\r"),page.mag,page.page,page.subpage);
		// TODO: Find or create the root of the mag M 
		// Something like 
		root=FindMag(sDisplayList,page.mag);
		// HOWEVER, sDisplayList should probably be implied! TODO. Simplify
		if (root==NULLPTR)
		{
			// If there is no root for this map, lets go and make one
			root=CreateRoot(sDisplayList,page.mag); // Probably drop sDisplayList
		}
		node.pageindex=ix;
		node.page=page.page;
		node.subpage=page.subpage;
		node.next=NULLPTR;
		LinkPage(root,node);
		// Scan the mag to find the PP, else create PP
		// If page already exists and has a different subcode...
		// then convert to a junction node and make carousel list.
		// If Junction node already exists, insert the new page at the correct sorted position.
	}
	f_close(&PageIndex);
	f_close(&PageAll);
 } // ScanPageList
 
 /** Set up all the lists.
  * Scan all the existing pages and make a sorted list
  */
 void InitDisplayList(void)
 {
 	// Initialise the serial RAM
	spiram_init();
	SetSPIRamStatus(SPIRAM_MODE_SEQUENTIAL);
	
	sDisplayList=NULLPTR;
	sFreeList=NULLPTR;
	// Put all the slots into the free list
	MakeFreeList();
	Dump();
	// Now scan the pages list and make a sorted list, creating nodes for Root, Node and Junction
	ScanPageList();
 } // initDisplayList
 
 
