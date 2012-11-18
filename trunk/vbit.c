/** ***************************************************************************
 * Description       : VBIT teletext inserter program for XMEGA
 * Compiler          : GCC
 *
 * Copyright (C) 2010-2012, Peter Kwan
 *
 * Permission to use, copy, modify, and distribute this software
 * and its documentation for any purpose and without fee is hereby
 * granted, provided that the above copyright notice appear in all
 * copies and that both that the copyright notice itand this
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
#include "vbit.h"

#define sizearray(a)  (sizeof(a) / sizeof((a)[0]))
 
/* Globals */
unsigned char Line[120];			/* Console input buffer */

extern FATFS Fatfs[1];			/* File system object for the only logical drive */

const char inifile[] = "test.ini";

static unsigned char statusI2C;
static unsigned char statusVBI;
static unsigned char statusFIFO;
static unsigned char statusDisk;

static uint8_t echoMode;

static uint8_t passBackspace=false; // Prevent backspace from operating during page load


static char pageFilter[6]; 
// Variables to do with stepping through the directory
static uint16_t FirstEntry;// last entry in a directory listing (DF and D+ commands)
static uint16_t LastEntry;// last entry in a directory listing (DF and D+ commands)
static uint16_t currentPage; // The current page being iterated

FIL PageF;	// A file object if we need one outside of inserting.	


/** pageFilterToArray
 *  Takes the page filter and finds the page array index
 * \param if high is set, it converts * to the high value.
 * \return An index into the page array
 */
uint16_t pageFilterToArray(uint8_t high)
{
	char value[4];
	uint8_t i;
	char ch;
	uint16_t result;
		// Scan the pageFilter string, making a high and low bound value
	for (i=0;i<3;i++)
	{
		ch=pageFilter[i];	// Get the character from the page filter
		if (i==0 && ch!='*') ch--; // because mag 1..8 maps to 0..7 in the page array
		if (ch=='*')
		{
			switch (i) // Set high/low bounds
			{
			case 0: // mag
				if (high) value[i]='7'; else value[i]='0';				
				break;
			case 1:; // page
			case 2:
				if (high) value[i]='f'; else value[i]='0';
				break;
			}
		}
		else // Just copy the digit
		{
			value[i]=ch;
		}
	}
	// cap the string
	value[3]=0;
	// convert hex digits to uint
	sscanf(value,"%3X",&result);
	return result+result; // Because the page array has 2 byte cells	
} // pageFilterToArray

/** Directory first. This also initialises the directory iterator
 * returns the index to an item in the page array. This is the first page that the filter selects.
 */
 uint16_t DirectoryFirst(void)
 {
	LastEntry=pageFilterToArray(1);	// The last value
	currentPage=pageFilterToArray(0);
	FirstEntry=currentPage;
	return currentPage;			// The first value
 }
/** Directory last. This also initialises the directory iterator
 * returns the index to an item in the page array. This is the first page that the filter selects.
 */
 uint16_t DirectoryLast(void)
 {
	currentPage=LastEntry=pageFilterToArray(1);	// The last value
	return LastEntry;
 }
 
/** Call this after calling Directory first.
 *  Call until the return value indicates the end of directory list.
 * [which is yet to be defined]
 */
uint16_t DirectoryNext(void)
{
	// TODO: What we should do is skip all empty page slots
	// TODO: We also need to increment by amounts other than 2.
	if (currentPage<=LastEntry)
		currentPage+=2;	// TODO: Store this up in case a number parameter comes next
	return currentPage;
}
uint16_t DirectoryPrev(void)
{
	// TODO: What we should do is skip all empty page slots
	// TODO: We also need to increment by amounts other than 2.
	currentPage-=2;	// TODO: Limit this to the start of the page filter
	return currentPage;}


/** Give the currentPage and a step value, iterates to find the page.
 * This is to implement the DF/DL/D+/D-
 * \param step - Number of pages to step
 */
uint16_t LocatePage(int8_t step)
{
	uint16_t stepCount;
	uint16_t saveCurrent;
	NODEPTR np;
	saveCurrent=currentPage;
	if (step==0)	// We didn't iterate. Just return the page that we are on
		return currentPage;
	if (step>0)	// stepcount=abs(step)
		stepCount=step;
	else
		stepCount=-step;
	while (stepCount)
	{
		// If we hit the page filter limits we return failure. (Actually return the page that we entered with)
		if ((currentPage+step)<FirstEntry || (currentPage+step)>LastEntry)
		{
			currentPage=saveCurrent;	// Revert currentPage
			return NULLPTR;
		}
		if (step>0)		// Iterate
			currentPage+=2;
		else
			currentPage-=2;
		np=GetNodePtr(&currentPage);	// Get the current page
		if (np!=NULLPTR)			// Only count actual pages
		{
			stepCount--;			// If there is a page, then count it
		}
	}
	return currentPage;
} // LocatePage

void testIni(void)
{
	xputs(PSTR("TestIni\n"));	
  char str[100];
  long n;
  int s, k;
  char section[50];

	xputs(PSTR("Test 1\n"));
  /* string reading */
  n = ini_gets("first", "string", "aap", str, sizearray(str), inifile);
	xprintf(PSTR("n=%d\n"),n);
	xprintf(PSTR("Test 1 str=%s (supposed to say noot)\n"),str);

//  assert(n==4 && strcmp(str,"noot")==0);
  n = ini_gets("second", "string", "aap", str, sizearray(str), inifile);
	xprintf(PSTR("Test 1 str=%s (supposed to say mies)\n"),str);
//  assert(n==4 && strcmp(str,"mies")==0);
  n = ini_gets("first", "dummy", "aap", str, sizearray(str), inifile);
//  assert(n==3 && strcmp(str,"aap")==0);
  xputs(PSTR("1. String reading tests passed\n"));

  /* value reading */
  n = ini_getl("first", "val", -1, inifile);
//  assert(n==1);
  n = ini_getl("second", "val", -1, inifile);
//  assert(n==2);
  n = ini_getl("first", "dummy", -1, inifile);
//  assert(n==-1);
  xputs(PSTR("2. Value reading tests passed\n"));

  /* string writing */
  n = ini_puts("first", "alt", "flagged as \"correct\"", inifile);
//  assert(n==1);
  n = ini_gets("first", "alt", "aap", str, sizearray(str), inifile);
//  assert(n==20 && strcmp(str,"flagged as \"correct\"")==0);
  /* ----- */
  n = ini_puts("second", "alt", "correct", inifile);
//  assert(n==1);
  n = ini_gets("second", "alt", "aap", str, sizearray(str), inifile);
//  assert(n==7 && strcmp(str,"correct")==0);
  /* ----- */
  n = ini_puts("third", "alt", "correct", inifile);
//  assert(n==1);
  n = ini_gets("third", "alt", "aap", str, sizearray(str), inifile);
//  assert(n==7 && strcmp(str,"correct")==0);
  /* ----- */
  xputs(PSTR("3. String writing tests passed\n"));

  /* section/key enumeration */
  for (s = 0; ini_getsection(s, section, sizearray(section), inifile) > 0; s++) {
    xprintf(PSTR("[%s]\n"), section);
    for (k = 0; ini_getkey(section, k, str, sizearray(str), inifile) > 0; k++) {
      xprintf(PSTR("\t%s\n"), str);
    } /* for */
  } /* for */

  /* string deletion */
  n = ini_puts("first", "alt", NULL, inifile);
//  assert(n==1);
  n = ini_puts("second", "alt", NULL, inifile);
//  assert(n==1);
  n = ini_puts("third", NULL, NULL, inifile);
//  assert(n==1);
  xputs(PSTR("All done\n"));

}

/* A line starts with <ctrl-n>0 and ends with \r 
Default to echo mode off.
This code pinched from SD_Card_Demo
*/
static void get_line (char *buff, int len)
{
	unsigned char c;
	int idx = 0;
	
	unsigned char started=0;

	for (;;)
	{
		while (!USB_Serial_GetNB(&c)) // Any characters?
			if (vbiDone) // do the next field?
			{			
				FillFIFO();
				vbiDone=0; // Reset the flag
			}
		if (c == '\r') break;
		// killing \b will kill flashing. Probably need to make something more foolproof and resetting, We can get into a trap with passBackspace
		if ((c == '\b') && idx && !passBackspace) {
			idx--;
			if (echoMode & 0x01) USB_Serial_Send(c);
		}
		// For ee command to work, all control characters should have bit 7 set EXCEPT 0x10.
		if (started && (((unsigned char)c >= ' ') || (unsigned char)c==0x10 ) && (idx < len - 1))
		{
			buff[idx++] = c;
			if (echoMode & 0x01) USB_Serial_Send(c);
		}
		if (c==0x0e) started=1;

	}
	buff[idx++]='\r';	// Preserve the end of line
	buff[idx] = 0;		// and cap it off
	if (echoMode & 0x01) 
	{
		USB_Serial_Send(c);
		USB_Serial_Send('\n');
	}
} // get_line

/** Test that page sequencing is working
 */
uint8_t test3(void)
{
	uint8_t i;
	xprintf(PSTR("Magazine streaming test\n\r"));
	for (i=0;i<64;i++)
		GetNextPage(0x8f); // This returns a node pointer which we would normally use to get the page.
	xprintf(PSTR("\n\r"));
	return 0; // nothing to return?
}

/* SPI ram test */
uint8_t test2(void)
{
	// VBIT Mux needs setting as SCK is shared
	GPIO_Off(VBIT_SEL);	// Low=AVR, High=VBIT
	xprintf(PSTR("Status=%03d\n\r"),GetSerialRamStatus());	
	char test[40];
	test[0]=0x7f;
	test[1]=0x7e;
	test[2]=0x7d;
	test[3]=0x7c;
	test[4]=0x7b;
	test[5]=0x7a;
	test[6]=0x79;
	test[7]=0x78;
	test[8]=0x77;
	test[9]=0x76;
	test[10]=1;
	SetSerialRamStatus(SPIRAM_MODE_SEQUENTIAL);
	xprintf(PSTR("Status=%03d\n\r"),GetSerialRamStatus());		
	SetSerialRamAddress(SPIRAM_WRITE, 0);
	WriteSerialRam(test, 10);
	DeselectSerialRam();
	test[0]='x';
	test[1]='y';
	test[2]='z';
	test[3]=0;
		SetSerialRamAddress(SPIRAM_READ, 0);
		ReadSerialRam(test,10);
		DeselectSerialRam();
	xputs(PSTR("Test ended Have a Nice Day\n\r"));	
	xprintf(PSTR("\n\r test0=%02X test1=%02X test2=%02X\n\r\n\r"), test[0],test[1],test[2]);
	if (test[0]!=0x7f || test[1]!=0x7e || test [2]!=0x7d)
		return 1; // fail
	return 0; // OK
}

// report good or bad status.
static void report(uint8_t ok)
{
	if (ok)
		xputs(PSTR("bad\n\r"));
	else
		xputs(PSTR("good\n\r"));
} // report

/** Given the page count string in pageFilter
 * \return The page count in that range. Or -1, if specific page (no wildcards) doesn't exist.
 */
int FindPageCount(void)
{
	char lower[6];
	char upper[6];
	char ch;
	unsigned int high,low;
	uint16_t i; // Don't insert more than 64k pages!!!
	NODEPTR np;
	int pageCount;
	uint16_t addr;
	uint8_t hasRange=0;	// There is a wildcarded range (used to signal an empty slot!)
	// Scan the pageFilter string, making a high and low bound value
	for (i=0;i<3;i++)
	{
		ch=pageFilter[i];	// Get the character from the page filter
		if (i==0 && ch!='*') ch--; // because mag 1..8 maps to 0..7 in the page array
		if (ch=='*')
		{
			hasRange=true;	// Not a unique page, but instead a range
			switch (i) // Set high/low bounds
			{
			case 0: // mag
				lower[i]='0';
				upper[i]='7';
				break;
			case 1:; // page
			case 2:
				lower[i]='0';
				upper[i]='f';
				break;
			case 3:; // subpage
			case 4:
				lower[i]='0';
				upper[i]='9';
				break;
			}
		}
		else // Just copy the digit
		{
			lower[i]=ch;
			upper[i]=ch;
		}
	}
	// cap the string
	lower[3]=0;
	upper[3]=0;
	// convert hex digits to uint
	sscanf(lower,"%5X",&low);
	sscanf(upper,"%5X",&high);
	// Fix for mag 8 to map to 0
	if (low>=0x800) low-=0x800;
	if (high>=0x800) high-=0x800;	
	//xprintf(PSTR("%s - From %5X to %5X\n\r"),pageFilter,low,high);
	pageCount=0;
	// Iterate looking through the page array for pages.
	if (hasRange)
	{
		for (i=low;i<high;i++)
		{
			addr=i+i;
			np=GetNodePtr(&addr);
			if (np!=NULLPTR)
				pageCount++;
		}
	}
	else
	{
		addr=low+low;
		if (GetNodePtr(&addr)==NULLPTR)
			pageCount=-1;	// page doesn't exist
		else
			pageCount=1;	// page exists
	}
	return pageCount;
} // FindPageCount

/** Dump the first 19 lines of the current page
 */
void dumpPage(void)
{
	DISPLAYNODE n;
	NODEPTR np;
	FRESULT res;
	uint16_t idx;
	uint16_t charcount;		
	PAGEINDEXRECORD ixRec;
	char data[80];
	//int i;
	FIL Page;	// hope we have enough memory for this!
	xprintf(PSTR("[dumpPage]\n\r"));
	idx=pageFilterToArray(0);
	np=GetNodePtr(&idx);	// np points to the displaynode
	xprintf(PSTR("index=%u node number=%u\n\r"),idx,np);
	GetNode(&n,np);
	DumpNode(np);
	// At this point we need to get the pageindex out of pages.idx
	// NB. Shared file access won't be a problem, I hope.
	// xprintf(PSTR("Now we need to look up pages.idx[%d]\n\r"),n.pageindex);

	f_lseek(&listFIL,(n.pageindex)*sizeof(PAGEINDEXRECORD));	// Seek the page index
	f_read(&listFIL,&ixRec,sizeof(PAGEINDEXRECORD),&charcount);	// and read it	
	
	res=f_open(&Page,"pages.all",FA_READ);					// Now look for the relevant page
	f_lseek(&Page,ixRec.seekptr);	// Seek the actual page
	// Now we need to parse the page.
	while (Page.fptr<(ixRec.seekptr+ixRec.pagesize)) // Need to actually parse the data? Don't think so
	{
		if (!f_gets(data,sizeof(data),&Page)) break;
		xprintf(PSTR("%s"),data);
	}

	f_close(&Page);
	// and finally parse the page and dump 19 lines of text
} // dumpPage

/* Command interpreter for VBIT,
	The leading SO and trailing carriage return are already removed
	The X command returns 2
	Other good commands return 0 and bad commands return 1.
	Also after a P command if selecting a single page fails, the page is created and 8 is the return code 
	*/
static int vbit_command(char *Line)
{
	static uint8_t firstLine=true;
	unsigned char rwmode;
	unsigned char returncode=0;
	int pagecount;
	uint8_t directorySteps;
	int8_t sign; // 1=plus -1=minus
	char ch;
	unsigned char valid;
	long n;
	unsigned char i;
	char str[80];
	char *ptr;
	char *dest;
	str[0]='O';
	str[1]='K';
	str[2]='\0';
	// char data[80];
	
	// This stuff is to do with locating pages in the display list (Directory command)
	// (also shared with ee/ea command for uploading pages)
	NODEPTR np;
	DISPLAYNODE node;
	PAGEINDEXRECORD ixRec;
	uint16_t charcount;	
	uint8_t res;
	DWORD fileptr;		// Used to save the file pointer to the body of the ttx file	
	PAGE page;
	// ee/ea specific variable
	static DWORD StartOfPage;
	DWORD EndOfPage;	// Records the start and end of the new page
	PAGEINDEXRECORD pageindex;
	uint16_t ix;	
	// tba
	
	// Read, Update or not
	switch (Line[2])
	{
	case 'R' : rwmode=CMD_MODE_READ; break;
	case 'U' : rwmode=CMD_MODE_WRITE;break;
	default:
		rwmode=CMD_MODE_NONE;
	}
	/* Is there actually any data */
	if (*Line==0)
		returncode=1;
	else
	switch (Line[1])
	{
	case 'b': // Dump the current page
		dumpPage();
		break;
	case 'C': // Create magazine lists
		// TODO: Kill video
		// TODO: C<pages to pre-allocate> // this would speed up the process immensely
		// It only needs to be approximate as FatFS will extend the file as needed.
		
		cli();
		pagecount=300;
		for (i=1;i<=1;i++) // TODO: Do we need more lists? Probably can find a way around it.
			SDCreateLists(i,pagecount);
		sei();
		break;
	case 'D': // Directory - D[<F|L>][<+|->][<n>]
		// Where F=first, L=Last, +=next, -=prev, n=number of pages to step (default 1)
		//xprintf(PSTR("D Command needs to be written"));
		// It would probably be a good idea to save the seek pointer
		// do the reading required
		// and then reset it. This would save memory.
		// If the ee command is active, don't allow D to mess the page variable.
		if (firstLine)
		{
			returncode=1; // ee command is busy. 
			break;
		}
		directorySteps=0;
		sign=1;
		for (i=2;Line[i];i++)
		{
			ch=Line[i];
			// xprintf(PSTR("Processing Line[%d]=%c\n\r"),i,Line[i]);
			switch (ch)
			{
			case 'F' : ; // Set the first item
				DirectoryFirst();
				break;
			case 'L' : ; // Set the last item
				DirectoryLast();
				break;
			case '+' : ; // Next item
				directorySteps=1;
				sign=1;
				// xprintf(PSTR("D+ not implemented"));
				break;
			case '-' : ; // Previous item
				directorySteps=1;
				sign=-1;
				// xprintf(PSTR("D- not implemented"));
				break;
			default: // Number of steps (TODO: Extend to a generic decimal)
				if (ch>='0' && ch <='9')
				{
					directorySteps=ch;
				}
				Line[i]=0;// Force this to be the last option
				break;	// Break because this must be the last option
			}
			// Find the page
			if (LocatePage(directorySteps*sign)==NULLPTR)
			{
				// Probably want to set an error value as we failed to iterate
				// But I don't think that we return anything different.
				// We rely on TED scheduler to remember the count returned by the P command and NOT overrun
			}
			// TODO: Work out what to do with the rest of the parameters
			np=GetNodePtr(&currentPage);
			// TODO: Handle sub pages
			GetNode(&node,np);
			// Instead treat the page like a single page
			f_lseek(&listFIL,(node.pageindex)*sizeof(PAGEINDEXRECORD));	// Seek the page index
			f_read(&listFIL,&ixRec,sizeof(PAGEINDEXRECORD),&charcount);	// and read it	
			// Now seek the actual page that we are referencing
			res=f_open(&PageF,"pages.all",FA_READ);					// Now look for the relevant page
			f_lseek(&PageF,ixRec.seekptr);	// Seek the actual page
			// Now we have the page, we need to seek through it to get
			// the data
			while (PageF.fptr<(ixRec.seekptr+ixRec.pagesize))
			{
				fileptr=PageF.fptr;		// Save the file pointer in case we found "OL"
				f_gets(str,sizeof(str),&PageF);
				if (str[0]=='O' && str[1]=='L')
				{
					f_lseek (&PageF, fileptr);	// Step back to the OL line
					break;
				}
				if (ParseLine(&page, str))
				{
					xprintf(PSTR("[insert]file error handler needed:%s\n"),str);
					// At this point we are stuffed.
					f_close(&PageF);
					returncode=1;
					break; // what else should we do if we get here?
				}
			}	
			// bb mpp qq cc tttt ssss n xxxxxxx
			// Leading zeros rely on PRINTF_LIB_FLOAT in makefile!!!
			sprintf_P(str,PSTR("%02X %03X %02d %02X %04X 0000 %1d 00000000"),
			3, // seconds (hex)
			0x100+(currentPage/2), // mpp
			page.subpage, // ss
			page.control, // S
			page.time,  // Cycle time (secs)
			(currentPage>>8)+1); // Mag
			f_close(&PageF);
		}
		// str[0]=0;	// might return the directory paramaters here
		break;
	case 'E' : // EO, ES, EN, EP, EL, EM - examine 
		switch (Line[2])
		{
		case 'M': // EM - Return Miscellaneous flags
			{
				// These are BFLSU. The code below is not correct
				n = ini_getl("service", "serialmode", 0, inifile);	
				if (n)
					xputs(PSTR("20"));
				else
					xputs(PSTR("00"));
				break;
			}
				break;
			default:
				returncode=1;
		case 'O': // EO - Output dataline actions set by QO
			// 18 characters on a line, but odd ignores last action
			n = ini_gets("service", "outputodd", "111Q2233P445566778", str, sizearray(str), inifile);	
			n = ini_gets("service", "outputeven", "111Q2233P445566778", str, sizearray(str), inifile);	
			xprintf(PSTR("%s"),str);
			break;
		}		
		break; // E commands
	case 'e' : // ea or ee : Upload page(s). Warning. "page" is shared with the directory command
		// directory calls are blocked until you do ee.
		switch (Line[2])
		{
		case 'a' : // Add a page, line at a time.
			passBackspace=true;
			if (firstLine)
			{
				firstLine=false;
				ClearPage(&page); // Clear out our Page object
				res=f_open(&PageF,"pages.all",FA_READ| FA_WRITE);	// Ready to write
				// TODO: check the value of res
				//xprintf(PSTR("Size of pages.all=%ul\n\r"),PageF.fsize);
				/* Move to end of pages.all to append data */
				res=f_lseek(&PageF, PageF.fsize);				
				//xprintf(PSTR("lseek res=%d\n\r"),res);
				StartOfPage=PageF.fptr;  // We need the Start Of Page for the index
			}
			// uh fellows, Although we get \r => Ctrl-P, we need to map it to 0x8d which is the file format.
			for (ptr=&Line[4];*ptr;ptr++)
				if (*ptr==0x10)
					*ptr=0x8d;
			f_puts(&Line[4],&PageF);	// The rest of the line is the file contents
			f_putc('\n',&PageF);	// Add the LF that the interpreter strips out			
			xprintf(PSTR("Now writing:%s\n\r"),&Line[4]);
			// Don't unencode the line, pages.all should follow MRG \r => ctrl-P substitutions
			// Probably can ignore Viewdata escapes.
			// Write the rest of the line to pages.all
			// Parse the line so we have all the page details so we know where to put it in the array/node
			if (ParseLine(&page, &Line[4]))
			{
				xprintf(PSTR("Your page sucks. Unable to parse this nonsense\n\r"));
				returncode=1;	// failed
				// TODO: implement the break
				// break;
			}
			break; // a
		case 'e' : // We finished. End the update
			passBackspace=false;
			EndOfPage=PageF.fptr;
			f_close(&PageF);		// We are done with pages.all
			// We need to refresh the transmission file object so close and re-open it
			f_close(&pagefileFIL);
			res=f_open(&pagefileFIL,"pages.all",FA_READ);		// Only need to open this once!			
			// Or are we all done? We need to write the page to Pmpp.TTI so that we can rebuild the index later
			pageindex.seekptr=StartOfPage;
			pageindex.pagesize=(uint16_t)(EndOfPage-StartOfPage); // Warning! 16 bit file size limits to about 50 subpages.
		    xprintf(PSTR("seek %ld size %d \n\r"),pageindex.seekptr,pageindex.pagesize);
			// 1: Append pages.idx with the file start/end (append pageindex)
			f_close(&listFIL); // TODO: Perhaps we should check that this is actually opened first?
			res=f_open(&listFIL,"pages.idx",FA_READ| FA_WRITE);	// Ready to write // TODO: check the value of res
			ix=listFIL.fsize;		// This is the address in the file
			res=f_lseek(&listFIL, ix);	// Locate the end of the file
			// Now append the page index
			f_write(&listFIL,&(pageindex.seekptr),4,&charcount);	// 4 byte seek pointer
			f_write(&listFIL,&(pageindex.pagesize),2,&charcount);	// 2 byte file size 			
			// Now restore the file to the previous opened readonly state
			f_close(&listFIL);
		    res=f_open(&listFIL,"pages.idx",FA_READ);			
			// 2: Add the page to the page array. (Or we could rebuild just by doing a restart)
			ix=ix/sizeof(pageindex);
			xprintf(PSTR("New page mag=%d page=%02X --> added at ix=%d\n\r"),page.mag,page.page,ix);
			LinkPage(page.mag, page.page, page.subcode, ix);
			// TODO:
			// 3: Add the page to the node list.  (ditto)
			// TODO:
			firstLine=true;		// and reset ready for the next file
			break; // e
		default:
			str[0]=0;
			returncode=1;
		}
		break; // e commands
	case 'G': /* G - Packet 8/30 format 1 [p830f1]*/
		if (rwmode==CMD_MODE_NONE)
		{
			str[0]=0;
			returncode=1;
			break;
		}
		/* C, L N, T, D */
		switch (Line[3])
		{
		case 'C' : /* Code. 1=format 1 */
			//xputs(PSTR("GUC command not implemented. Why would we need it\n"));
			// It should always default to 0
			/** Where is the initial page done? The code below is wrong */
			//SetInitialPage(pkt830,str1,str2); // nb. Hard coded to 100
			break;
		case 'D' : /* up to 20 characters label*/
			if (rwmode==CMD_MODE_READ)
			{			
				n = ini_gets("p830f1", "label", "VBITFax             ", str, sizearray(str), inifile);			
				xprintf(PSTR("%s"),str);
			}
			if (rwmode==CMD_MODE_WRITE)
			{
				n = ini_puts("p830f1", "label", &Line[4], inifile);	
				SetStatusLabel(pkt830,&Line[4]);
			}			
			break;
		case 'L' : /* Link */
			xputs(PSTR("GUL command\n"));
			/* Alrighty. The MAG is already in the MRAG. All we actually need is 
			ppssss where pp=hex page number and ssss=hex subcode */
			n = ini_gets("p830f1", "initialpage", "003F7F", str, MAXSTR, inifile);
			break;
		case 'N' : /* Net IC  */
			if (rwmode==CMD_MODE_READ)
			{
				n = ini_gets("p830f1", "nic", "fa6f", str, sizearray(str), inifile);			
				xprintf(PSTR("%s"),str);
			}
			if (rwmode==CMD_MODE_WRITE)
			{
				n = ini_puts("p830f1", "nic", &Line[4], inifile);	
				SetNIC1(pkt830,&Line[4]);
			}
			break;
		case 'T' : /* Time */
			xputs(PSTR("GUT command\n"));
			i2c_init();			
			break;
		default:
			str[0]=0;
			returncode=1;	
		}
		break;
	case 'H': // H or HO. Set header
		if (Line[3]=='\0' || Line[5]=='\0') // Don't get confused by checksums
		{
			strcpy_P(str,PSTR("      "));
			strncat(str,g_Header,32);	// Just readback the header. TODO. Get the correct length
			str[40]=0;
		}
		else
		{
			strncpy(g_Header,&Line[9],32); // accept new header
			//g_Header[32]=0;					// Make sure it is capped
			n = ini_puts("service", "header", g_Header, inifile);	// Save this value back to the INI file.
		}
		break;
	case 'I': // III or I2
		// I20xnnmm
		if (Line[2]=='2') // SAA7113 I2C. value. 0xnnmm where nn=address mm=value to write 
		{
			strcpy_P(str,PSTR("Setting SAA7113 I2C register\n"));
			ptr=&Line[3];
			xatoi(&ptr,&n);
			xprintf(PSTR("Blah=%04X\n"),n);
			i2c_SetRegister((n>>8)&0xff,n&0xff);			
			xprintf(PSTR("Done\n"));
		}		
		break;
	case 'J' : // J<h>,DATA - Send a packet to SRAM address
		// Probably want a whole family of J commands.
		// JA<h> - Set the address pointer to SRAM page <h> where <h> is 0..e
		// JW<data> - Write a complete 45 byte packet to the current address and increment
		// JR<data> - Read back the next block of data and increment the pointer.
		// [JT<h> - Retransmit page <h> immediately. (can't work. You must Tx the parent page) ]
		// JT<mpp> - Transmit page <mpp> immediately. (probably need to set a flag in the magazine stream
		// to insert the page in the next transmission slot
		switch (Line[2])
		{
		case 'A':
			xprintf(PSTR("JA set SRAM address\n"));
			// Read the SRAM page value 0..e. There are 14 pages 
			// Do we need finer control than setting the pointer to the start?
			// For the lulz we probably want to have complete random access.
			break;
		case 'W':
			xprintf(PSTR("JW Write SRAM data\n"));
			// Write a single packet
			// Not sure how we are going to map control codes but probably the same as OL 
			break;
		case 'R':
			xprintf(PSTR("JR Read back SRAM data\n"));
			// Read back a single packet (translated back into OL format)
			break;
		case 'T':
			xprintf(PSTR("JT Transmit mpp\n"));
			// Set a flag to transmit the selected page ASAP.
			break;
		}
		break;
	case 'L': // L<nn>,<line data>
		// We don't use L in vbit. Because it would require RAM buffering or more file writing,
		// instead we use the e command which writes the file directly.
		xprintf(PSTR("L command not implemented. Use 'e'\n"));
		str[0]=0;
		returncode=1;
		break;
	case 'M': // MD - Delete all the pages selected by the last P command.
		xprintf(PSTR("Delete...\n"));
		// Iterate down all the pages
		// Traverse down subpages and release nodes
		// Go into the pages index and null out the entries in pages.idx
		break;
	case 'O':	/* O - Opt out. Example: O1c*/
		/* Two digit hex number. Only 6 bits are used so the valid range is 0..3f */
			ptr=&Line[0];
			Line[0]='0';Line[1]='x';
			xatoi(&ptr,&n);
			OptRelays=n & 0x3f;
		break;		
	case 'P': // P<mppss>. An invalid character will set null. P without parameters will return the current value
		ptr=&Line[2];
		if (!*ptr)
		{
			sprintf_P(str,PSTR("%s\n\r"),pageFilter);
			break;
		}
		dest=pageFilter;
		for (i=0;i<5;i++)
		{
			ch=*ptr++;
			if (ch=='*')
				valid=1;
			else
			{
				switch (i)
				{
				case 0 : // M
					valid=(ch>'0' && ch<'9' );break;
				case 1 :; // PP
				case 2 :
					valid=((ch>='0' && ch<='9') || (ch>='A' && ch<='F'));break;
				case 3 :; // SS
				case 4 :
					valid=(ch>='0' && ch<='9');break;
				}
			}
			if (valid)
				*dest++ = ch;
			else
			{
				dest[0]=0;
				break;
			}
		}
		*dest=0;	// terminate the string
		// TODO: Find out how many pages are in this page range
		pagecount=FindPageCount();
		if (pagecount<0)
		{
			pagecount=1;
			returncode=8;
			// TODO: At this point we must create the page, as we are about to receive the contents.
			xprintf(PSTR("Page has been created. Please send some data to fill the page\n\r"));
		}
		sprintf_P(str,PSTR("%04d"),pagecount); // Where nnn is the number of pages in this filter. 099 is a filler ack and checksum 
		// str[0]=0;
		break;
	case 'Q' : // QO, QM
		if (Line[2]=='M') // QMnn
		{
			ptr=&Line[3];
			xatoi(&ptr,&n);
			xprintf(PSTR("QM command, n=%d\n"),n);
			n = ini_putl("service", "serialmode", n, inifile);	
			// And at this point put it into the vbi configuration
			// vbi_mode_serial=0 or CTRL_C11_MAGAZINESERIAL_bm
			/** TBA. This setting needs to be in the VBI section
			if (n) 
				vbi_mode_serial=0;
			else
				vbi_mode_serial=CTRL_C11_MAGAZINESERIAL_bm;
			*/
			break;
		}
		// QO sets both odd and even lines
		// QD only sets the odd.
		if (Line[2]=='O' || Line[2]=='D') // QO[18 characters <P|Q|1..8|F>]. QD is the odd line and has 18 lines
		{
			int i;
			char ch;
			ptr=&Line[3];

			xputc('0');
			// Validate it.
			for (i=0;i<18;i++)
			{
				ch=*ptr++;
				switch (ch)
				{
				case '1':;case'2':;case'3':;case'4':;case'5':;case'6':;case'7':;case'8':;case'I':;
				case'F':;
				case'P':;
				case'Q':;
				case'Z':;
					break;
				default:
					returncode=1;
				}
				g_OutputActions[0][i]=ch;		// odd field
				if (Line[2]=='O')
					g_OutputActions[1][i]=ch;	// even field (QO only)
			}
			*ptr=0;
			if (returncode) break;
			n = ini_puts("service", "outputodd", &Line[3], inifile);	
			if (Line[2]=='O')
				n = ini_puts("service", "outputeven", &Line[3], inifile);	// QO only
			break;
		}
		returncode=1;
		break;
	// We could probably use the S command to encapsulate Newfor. We aren't going to be setting the page status much.
	case 'S' : ; // Newfor. This should be a Newfor command. Hmm, but how to escape SO and SI?
		// Work out how to escape data. The parity and reserved characters will break the CI
		break;
	case 'T': // T <hhmmss> (this syntax was superceded by GUT and GUt)
		// testIni();
		// test3();
		// test2();
		// UTC is the time of day in seconds
		Line[8]=0;
		ptr=&(Line[2]);
		UTC=0; // Maybe save this. We need to revert if it fails.
		for (i=2;i<8;i++)
		{
			// First multiply according to which digit
			switch (i)
			{
			case 3:;
			case 5:;
			case 7:
				UTC*=10;break;
			case 4:;
			case 6:
				UTC*=6;break; // (already *10!)
			}
			ch=*(ptr++)-'0';
			UTC+=ch;
		}
		
		// xprintf(PSTR("UTC=%d\n\r"),UTC); // This upsets the protocol!
		
		/**
		UTC=Line[7]-'0';				// s units
		UTC=UTC+(Line[6]-'0')*10;		// s tens
		UTC=UTC+(Line[5]-'0')*60;		// m units
		UTC=UTC+(Line[4]-'0')*60*10;	// m tens
		UTC=UTC+(Line[3]-'0')*60*60;	// h units
		UTC=UTC+(Line[2]-'0')*60*60*10;	// h tens
		*/
		// strcpy_P(str,PSTR("OK\n"));
		str[0]=0;
		break;
	case 'U': // TEST
		Init830F1();
		break;
	case 'V': // Communication settings. 2 hex chars (bit=(on/off) 7=Viewdata/Text 1=CRLF/CR 0=Echo/Silent
		// VBIT seems a bit fussy. When using TED Scheduler it is best to sat V00
		pagecount=sscanf(&(Line[2]),"%2X",&i);
		// xprintf(PSTR("sscanf returns %d. Parameter is %X0\n\r"),pagecount,i);
		if (pagecount>0)
		{
			echoMode=i;		// Yes, it was OK
			// TODO: Save the result in the INI
		}
		else
			returncode=1;	// No, it failed
		break;
	case 'W': // TEST Ad-tec opt outs
		// W14 - Send a mode 14 opt out
		// We want to be able to test various ATP950 modes.
		// read the parameter
		ptr=&Line[2];	
		xatoi(&ptr,&n);
		OptOutMode=n;
		xprintf(PSTR("W=%04X\n"),OptOutMode);
		// Which command? Set the appropriate opt out mode
		switch (OptOutMode)
		{
		case 14:
			xprintf(PSTR("Mode 14 shenanigans\n"),n);
			// Or just flag that we want an opt-out packet
			OptOutMode=14;
			OptOutType=OPTOUT_START;
			// Assemble a mode 14 packet
			break;
		default:
			OptOutMode=0;
			returncode=1;
		}
		// Assemble the packet and 
		break;
	case 'X':	/* X - Exit */
		return 2;	
	case 'Y': /* Y - Version. Y2 should return a date string */
		strcpy_P(str,PSTR("VBIT620 Version 0.04"));
		break;		
	case '?' :; // Status TODO
		xprintf(PSTR("STATUS %02X\n\r"),statusI2C);
		// Want to know if the chips check out and the file system is OK
		// Video Input:
		xprintf(PSTR("Video input: "));report(statusI2C & 0x01); // chip responds, generating field interrupts
		// Digital Encoder
		xprintf(PSTR("Digital encoder: "));report(statusI2C & 0x02); // chip responds
		// FIFO
		statusFIFO=test2();
		xprintf(PSTR("FIFO R/W verified: "));report(statusFIFO); // we can read and write to it
		// File system
		xprintf(PSTR("File system: "));report(statusDisk); // There is a card, it is formatted, it has onair/pages.all
		break;
	default:
		xputs(PSTR("Unknown command\n"));
		returncode=1;
	}
	xprintf(PSTR("%d%s0\r"),returncode,str); // always add address 0.
	return 0;
}

/* Load settings from SD card
 * inserter.ini
*/
int LoadINISettings(void)
{
	int n;
	n = ini_gets("service", "outputodd",  "111Q2233P44556678Q", &(g_OutputActions[0][0]), 18, inifile);	
	n = ini_gets("service", "outputeven", "111Q2233P44556678Q", &(g_OutputActions[1][0]), 18, inifile);	
	n = ini_gets("service", "header",     "mpp MRG DAY dd MTH", g_Header, 33, inifile);
	return 0; // TODO: Return success or otherwise
}

int RunVBIT(void)
{
	/* Join xitoa module to USB-Serial bridge module */
	xfunc_out = (void (*)(char))USB_Serial_Send;
	// Term_Erase_Screen();
	BUTTON_Init( BUTTON_ALL );
	xputs(PSTR("VBIT620 Inserter Started"));
	GPIO_Init();	// Set up the ports
	if (disk_initialize(0)==FR_OK) // Set up the SD memory card
		statusDisk=0;
	else statusDisk=1;
	f_mount(0,&Fatfs[0]);
	InitStream();
	InitDisplayList();				// Do this before we start interrupts!!!
	
	pageFilter[0]=0;		// I think that statics get zeroed anyway.

	// Configure VBIT's spiram port and set the spiram to sequential mode
	spiram_initialise();
	SetSerialRamStatus(SPIRAM_MODE_SEQUENTIAL);	
	statusI2C=i2c_init();			// Start the video processors
	statusVBI=InitVBI();			// Set up the video timing
	LoadINISettings();
	InitDataBroadcast();
	for (;;)
	{
		// xputc('>'); // no room for a prompt
		get_line((char*)Line, sizeof(Line));
		if (vbit_command((char*)Line)==2) break;
	}
	xputs(PSTR("VBIT Terminated. Have a Nice Day\n"));
	return 0;
}