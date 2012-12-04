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
 * Copyright (c) 2010-2012 Peter Kwan
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
 /** \file packet.c
 * Basic packet management
 */

#include "packet.h"

static uint8_t fifoLineCounter=0; // vbi line 0..15

// The state of the 8 magazines
static unsigned char state[8]={0,0,0,0,0,0,0,0};
/// States that each magazine can be in
#define STATE_BEGIN	0
#define STATE_IDLE	1
#define STATE_HEADER	2
#define STATE_SENDING	3

// How many vbi lines per field. Not quite right. Odd field is 17 (6..22), Even is 18 (318..335).
#define VBILINES	17

char g_OutputActions[2][18];
char g_Header[32]; // Add one for luck (or terminator)

int OptRelays;			/** Holds the current state of the opt out relay signals */

unsigned char OptOutMode=0;	// Which ATP950 mode. If 0, then do nothing
unsigned char OptOutType=OPTOUT_START;	// One of the OPTOUT values

// These are global file objects
FIL pagefileFIL, listFIL;

/** Check that parity is correct for the packet payload
 * The parity is set to odd for all bytes from offset to the end
 * Offset should be at least 3, as the first three bytes have even parity
 * The bits are then all reversed into transmission order
 * \param packet : packet to check
 * \param offset : start offset to check. (5 for rows, 13 for header)
 */
void Parity(char *packet, uint8_t offset)
{
	int i;
	uint8_t c;
	for (i=offset;i<PACKETSIZE;i++)
	{
		
		packet[i]=pgm_read_byte(ParTab+(uint8_t)(packet[i]&0x7f)); // Strange syntax because of ParTab in progmem
	}
	for (i=0;i<PACKETSIZE;i++)
	{
		c=(uint8_t)packet[i];
		c = (c & 0x0F) << 4 | (c & 0xF0) >> 4;
		c = (c & 0x33) << 2 | (c & 0xCC) >> 2;
		c = (c & 0x55) << 1 | (c & 0xAA) >> 1;	
		packet[i]=(char)c;
	}
} // Parity

/** The prefix is the first five characters
 * consisting of the clock run in, framing code, mag and row
 */
void WritePrefix(char *packet, uint8_t mag, uint8_t row)
{
	// BEGIN: Special effect. Go to page 100 and press Button 1.
	// Every page becomes P100.
	if (BUTTON_GetStatus(BUTTON_1))
	{
		mag=1;
	}
    // END: Special effect

	char *p=packet; // Remember that the bit order gets reversed later
	*p++=0x55; // cri 
	*p++=0x55; // cri
	*p++=0x27; // fc
	// add MRAG
	*p++=HamTab[mag%8+((row%2)<<3)]; // mag + bit 3 is the lowest bit of row
	*p++=HamTab[((row>>1)&0x0f)];
} // WritePrefix

/** Stuffs a line where all the packet contents is value
 */
void FillerTest(char *packet, uint8_t value)
{
	WritePrefix(packet,8,25);
	for (int i=5;i<PACKETSIZE;i++)
		packet[i]=value;
} // FillerTest

void FillerPacket(char *packet)
{
	int i;
	WritePrefix(packet,8,25);
	for (i=5;i<PACKETSIZE;i++)
		packet[i]=' ';
	Parity(packet,5);
} // FillerPacket

/** All the bits on this line are off, if code is 0
 * Otherwise the low 4 bits are used for a pattern to aid debugging using a 'scope.
 */
void QuietLine(char * packet, uint8_t code)
{
	const unsigned char debug=1;
	code&=0x0f;
	for (int i=0;i<PACKETSIZE;i++)
		if (debug)
		{
			// packet[i]=(i%10)>5?0xff:0x00; // Insert a recognisable waveform for the scope
			if (i%11==0)
				code>>=1;
			if (i%11>5)
				packet[i]=code&1?0xff:0;
			else
				packet[i]=0;
		}
		else		
		{
			packet[i]=0;
		}
} // QuietLine

/** A header has mag, row=0, page, flags, caption and time
 */
void Header(char *packet ,unsigned char mag, unsigned char page, unsigned int subcode,
			unsigned int control, char *caption)
{
	char *p;
	char ch;
	int i,j;
	static int lastsec;
	// BEGIN: Special effect. Go to page 100 and press Button 1.
	// Every page becomes P100.
	if (BUTTON_GetStatus(BUTTON_1))
	{
		page=0;
	}
    // END: Special effect
	uint8_t hour, min, sec;
	uint32_t utc;
	WritePrefix(packet,mag,0);
	packet[5]=HamTab[page%0x10];
	packet[6]=HamTab[page/0x10];
	packet[7]=HamTab[(subcode&0x0f)]; // S1
	subcode>>=4;
	packet[8]=HamTab[(subcode&0x07)]; // S2 TBA add C4
	subcode>>=3;
	packet[9]=HamTab[(subcode&0x0f)]; // S3
	subcode>>=4;
	packet[10]=HamTab[(subcode&0x03)]; // S4 TBA C6, C5
	packet[11]=HamTab[0]; // TBA C7 to C10
	packet[12]=HamTab[0]; // TBA C11 to C14 (0=parallel & language 0 (English))
	strncpy(&packet[13],caption,32); // This is dangerously out of order! Need to range check and fill as needed
	// Stuff the page number in. TODO: make it work with hex numbers etc.
	p=strstr(packet,"mpp"); 
	if (p) // if we have mpp, replace it with the actual page number...
	{
		*p++=mag+'0';
		ch=page>>4; // page tens (note wacky way of converting digit to hex)
		*p++=ch+(ch>9?'7':'0');
		ch=page%0x10; // page units
		*p++=ch+(ch>9?'7':'0');
	}
	// xputc(packet[20]); // Echo the mag for debugging
	// Stick the time in. Need to implement flexible date/time formatting
	utc=UTC;
	sec=utc%60;
	utc/=60;
	min=utc%60;
	hour=utc/60;
	//if (lastsec!=sec)
	//	xprintf(PSTR("%02d:%02d.%02d "),hour,min,sec);
	lastsec=sec;
	//sprintf(&packet[37],"%02d:%02d.%02d",hour,min,sec);
	
	// Format the time string in the last 8 characters of the heading
	j=0;
	for (i=37;i<=44;i++)
	{
		if (packet[i]=='\r')	// Replace spurious double height. This can really destroy a TV!
			packet[i]='?';
		if ((packet[i]>='0') && (packet[i]<='9'))
		{
			packet[i]='0';
			switch (j++)
			{
			case 0:packet[i]+=hour/10;break;
			case 1:packet[i]+=hour%10;break;
			case 2:packet[i]+= min/10;break;
			case 3:packet[i]+= min%10;break;
			case 4:packet[i]+= sec/10;break;
			case 5:packet[i]+= sec%10;break;
			default:
				packet[i]='?';
			}
		}
	}
	Parity(packet,13);		
} // Header

void Row(char * packet, unsigned char mag, unsigned char row, char * str)
{
	WritePrefix(packet,mag,row);
	strncpy(packet+5,str,40);
	Parity(packet,5);		
} // Row

/** Copy a line of teletext in MRG tti format.
 * OL,nn,<line> 
 * Where nn is a line number 1..27
 * <line> has two methods of escaping, which need to be decoded
 * \return The row number
 */
static unsigned char copyOL(char *packet, char *textline)
{
	int i;
	long linenumber;
	// Get the line number
	textline+=3;
	char ch;
	xatoi(&textline, &linenumber);
	//xprintf(PSTR("Line number=%d\n"),(int)linenumber);
	// Skip to the comma to get the body of the command
	for (i=0;i<4 && ((*textline++)!=',');i++);
	if (*(textline-1)!=',')
	{
		xputc('F');
		return 0xff; // failed
	}
	for (char *p=packet+5;p<(packet+PACKETSIZE);p++)*p='P'; // If you see a line full of P, we dun goofed.
	for (char *p=packet+5;*textline && p<(packet+PACKETSIZE);textline++) // Stop on end of file OR packet over run
	{
		// TODO: Also need to check viewdata escapes
		// Only handle MRG mapping atm
		ch=*textline; // Don't strip off the top bit just yet!
		if ((ch!=0x0d) && (ch & 0x7f)) // Do not include \r or null
		{
			if ((ch & 0x7f)==0x0a)		
			{
				// *p=0x0d; // Translate lf to cr (double height)
				*p='?';
			}
			else
				*p=ch;
		}
		else
		{
			// \r is not valid in an MRG page, so it must be a truncated line
			// Fill the rest of the line in with blanks
			char *r;
			for (r=p;r<(packet+PACKETSIZE);r++)
				*r=' '; // fill to end with blanks
				// *r='z'-(char)(PACKETSIZE-(char)((int)packet+(int)r)); // was space but I want it visible
			// *r='E';    // Make the last one visibly different (debugging)
			break;
		}
		// if ((*p & 0x7f)==0) *p=' '; // In case a null sneaked in
		p++;
	}
// if (!*textline) xputc('T'); // Not sure what this means
	return linenumber;
} // copyOL

/** Fastext links
 * FL,<link red>,<link green>,<link yellow,<link cyan>,<link>,<link index>
 */
static void copyFL(char *packet, char *textline, PAGE *page)
{
	long nLink;
	// add the designation code
	char *ptr;
	char *p;
	p=packet+5;
	*p++=HamTab[0];
			 // work out what the magazine number for this page is
	char cCurMag=page->mag;
	
	// add the link control byte
	packet[42]=HamTab[0x0f];

	// and the page CRC
	packet[43]=HamTab[0];
	packet[44]=HamTab[0];

	// for each of the six links
	for (int i=0; (i < 6); i++)
	{
		// TODO: Simplify this. It can't be that difficult to read 6 hex numbers.
		// TODO: It needs to be much more flexible in the formats that it will accept
		// Skip to the comma to get the body of the command
		for (i=0;i<6 && ((*textline++)!=',');i++);
		if (*(textline-1)!=',')
		{
			return; // failed :-(
		}
		// page numbers are hex
		ptr=textline-2;
		*ptr='0';
		*(ptr+1)='x';
		xatoi(&ptr,&nLink);
		//if (page->page==0 && page->mag==1)
		//{
//			xprintf(PSTR("[copyFL]page 100 link:%X\n"),nLink);
		//}
		// int nLink =StrToIntDef("0x" + strParams.SubString(1 + i*4,3),0x100);

			
					 // calculate the relative magazine
		char cRelMag=(nLink/0x100 ^ cCurMag);
		*p++=HamTab[nLink & 0xF];			// page units
		*p++=HamTab[(nLink & 0xF0) >> 4];	// page tens
		*p++=HamTab[0xF];									// subcode S1
		*p++=HamTab[((cRelMag & 1) << 3) | 7];
		*p++=HamTab[0xF];
		*p++=HamTab[((cRelMag & 6) << 1) | 3];
	}	
}

/** This generates the next line
 * So, tell me how this can support parallel streams?
 * Easy! There isn't a lot of "state" needed.
 * We have the state of the magazine. Already got that.
 * Then we will need to look after the file pointers so that we can read from multiple magazines.
 * \param packet : A char array that gets filled with a text packet
 * \param field : A boolean indicating which TV field we are on
 * \return 0 if OK, >0 if there is a file reading problem
 */
static unsigned char insert(char *packet, uint8_t field)
{
	unsigned char noCarousel=0; // Use to only display the first page of a carousel. TODO. Implement carousels
	// static FIL pagefile, list;
	static uint8_t savefield;
	static DWORD fileptr;		// Used to save the file pointer to the body of the ttx file
	static PAGE page;
	//char pagename[15];
	//char listentry[25];
	char data[80];
	char *str;
	static char *redirectPtr;	// When a page is redirected to SRAM we use this pointer
	// char *p;
	unsigned char row;
	static uint8_t myfield;
	FRESULT res=0;	
	BYTE drive=0;
	DWORD pageptr;				// Pointer to the start of page in pages.all
	DWORD pagesize;				// Size of the page in pages.all
	unsigned char mag=0;	// just one mag for now!
	// xputc(state[mag]+'0');
	// DEBUG CODE
	if (myfield!=field)
	{
		myfield=field;
		// xprintf(PSTR("\n\rF%d"),field);		
	}
	// END OF DEBUG CODE
	switch (state[mag])
	{
	case STATE_BEGIN: // Open the first page and drop through to idle
		// xputs(PSTR("B"));
		// Open the onair folder
		res=(WORD)disk_initialize(drive);	// di0
		put_rc(f_mount(drive, &Fatfs[drive]));	// fi0 /*!!!*/
		put_rc(f_chdir("onair"));
		// res=f_open(&listFIL,"mag1.lst",FA_READ);	
		res=GetPage(&pageptr,&pagesize,0xff);	// Get the next transmission page details. TODO: A proper mask. TODO: A return value
		if (res)
		{
			xprintf(PSTR("[insert]Epic Fail: Could not open initial page\r\n"));			
			put_rc(res);
			return 1;
		}	
		/* This legacy stuff extracts the pageptr and ,pagesize 
		f_gets(listentry,sizeof(listentry),&listFIL);		
		str=strchr(listentry,',');
		if (str)
		{
			*str=0;
			strcpy(pagename,listentry); // Get the filename
			// Now we can get the seek pointer and size
			p=--str;
			*(str++)='0';
			*(str++)='x';
			xatoi(&p,&pageptr); // warnings here!
			str=strchr(str,',');
			p=--str;
			*(str++)='0';
			*(str++)='x';
			xatoi(&p,&pagesize);		
			//xprintf(PSTR("page=%lX size=%lX\n\r"),(unsigned long) pageptr,(unsigned long) pagesize);					
		}
		xprintf(PSTR("\n\rf=%s\n\r"),pagename);		
		*/
		state[mag]=STATE_IDLE;
		res=f_open(&pagefileFIL,"pages.all",FA_READ);		// Only need to open this once!
		// xputs(PSTR("STATE_BEGIN\n\n\r"));
		if (res)
			xprintf(PSTR("Failed to open pages.all, res=%d\n\n"),res); 
	case STATE_IDLE: // If permitted to run we send the header
		// xputs(PSTR("I"));
		noCarousel=0; // Just until we can handle carousels
		savefield=field; // record the field that we are on	
		// Now tx the header
		// Need to do the whole parse and parity bit here 
		// open pagefile
		//LED_On( LED_1 );		// LED5 - high while seeking a folder
		res=f_lseek(&pagefileFIL,pageptr); // Instead of f_open just use lseek
		//LED_Off( LED_1 ); // Need to define the correct LED
		if (res)
		{
			xprintf(PSTR("[insert]Epic Fail 2\n"));			
			put_rc(res);
			return 1;
		}	
		ClearPage(&page); // Clear the page parameters (not strictly required)
		// Loop through the header and parse down to the OL
		while (pagefileFIL.fptr<(pageptr+pagesize))
		{
			fileptr=pagefileFIL.fptr;		// Save the file pointer in case we found "OL"
			f_gets(data,sizeof(data),&pagefileFIL);
			if (data[0]=='O' && data[1]=='L')
			{
				f_lseek (&pagefileFIL, fileptr);	// Step back to the OL line
				break;
			}
			if (ParseLine(&page, data))
			{
				xprintf(PSTR("[insert]file error handler needed:%s\n"),data);
				state[mag]=STATE_BEGIN; // TODO. Is this the best thing to do???
				break; // what else should we do if we get here?
			}
		}
		// xprintf(PSTR("MPP: %d%02X\n\r"),page.mag,page.page);
		// create the header packet. TODO: Add a system wide header caption
		// xputs(PSTR("H"));
		Header(packet,page.mag,page.page,page.subpage,page.control,g_Header);		// 6 - 24 characters plus 8 for clock
		state[mag]=STATE_HEADER;
		// TODO: check that page.redirect is indicating a redirect,
		// if so then set up the pointer. (Also see JA/JW commands)
		if (page.redirect<SRAMPAGECOUNT)
			redirectPtr=(char *)SRAMPAGEBASE+page.redirect*SRAMPAGESIZE;
		break;
	case STATE_HEADER: // We are waiting for the field to change before we can tx
		// xputs(PSTR("H"));
		if (field==savefield)
		{
			// xputs(PSTR("W"));
			QuietLine(packet,0x0f);	// TODO: We would let the next magazine steal this line
			break;
		}
		state[mag]=STATE_SENDING; // We have the new field. Change state
	case STATE_SENDING:
		// The pointer is initialised before we get here, at the same time that we set STATE_HEADER
		// Are we in redirect mode?
		// WARNING. The mag must match the header?
		// TODO: We must save the mag from row 0
		// and insert it here
		if (page.redirect!=0xff)
		{
			// Get the next line of SRAM data
			DeselectSerialRam();
			// xprintf(PSTR("Redirect reading from=%04X\n"),redirectPtr);
			SetSerialRamAddress(SPIRAM_READ, (uint16_t)redirectPtr);
			ReadSerialRam(data,PACKETSIZE);	// Could load direct into packet, but it may be a bug?
			DeselectSerialRam();
			for (int i=0;i<PACKETSIZE;i++)
				packet[i]=data[i];
			
			// Validate for CRI/FC
			//if (packet[0]!=0x55 || packet[1]!=0x55 || packet[2]!=0x27)
			if (packet[0]!=0x55) // It isn't a valid packet? The page is ended.
			{
				state[mag]=STATE_IDLE;	// Set the IDLE state and get ready for the next page
				noCarousel=1;
				res=GetPage(&pageptr,&pagesize,0xff);	// Get the next transmission page details. 			
				break;
			}
			redirectPtr+=PACKETSIZE;
			// Put the page's mag number in place of the one we have got.
			// Note that the row is in packet[3]
			WritePrefix(packet, page.mag, packet[3]); // This prefix gets replaced later

			// We can transmit
			Parity(packet,5);
			break;
		}
		// Normal page from SD card.....
		// Do something like copyOL from SRAM location given in page.redirect
		// Update the pointer.
		// If we have copied all the pages then we go to STATE_IDLE.		
		// STATE_IDLE if The ptr has an invalid CRI/MRAG or ptr is at the end of the page
		// else // normal page
		if (f_eof(&pagefileFIL) || noCarousel || (pagefileFIL.fptr>=(pageptr+pagesize))) // Page done?
		{
			res=GetPage(&pageptr,&pagesize,0xff);	// Get the next transmission page details. TODO: A proper mask. TODO: A return value
			if (res)
			{
				xprintf(PSTR("[insert]Epic Fail: Could not open initial page\r\n"));			
				put_rc(res);
				return 1;
			}
			/* legacy code
			if (f_eof(&listFIL)) // List All done? Start again at the top of the list
			{
				// f_close(&pagefile);
				f_lseek (&listFIL, 0);
			}
			f_gets(listentry,sizeof(listentry),&listFIL);
			str=strchr(listentry,',');
			if (str)
			{
				*str=0;
				strcpy(pagename,listentry); // Get the filename
				// Now we can get the seek pointer and size
				p=--str;
				*(str++)='0';
				*(str++)='x';
				xatoi(&p,&pageptr);
				str=strchr(str,',');
				p=--str;
				*(str++)='0';
				*(str++)='x';
				xatoi(&p,&pagesize);
				// xprintf(PSTR("page=%lX size=%lX\n\r"),(unsigned long) pageptr,(unsigned long) pagesize);									
			}
			// xprintf(PSTR("L-%s "),pagename);	
*/			
			state[mag]=STATE_IDLE;
		}
		else
		{
			// Get the next line from SD card
			// xputs(PSTR("S"));
			str=f_gets(data,sizeof(data),&pagefileFIL);
			if (str)
			{
				// Now we need to parse the line and send it to the packet
				// xprintf(PSTR("p=%s\n\r"),data);	 // instead of dumping it!
				if (str[0]=='O' && str[1]=='L')		// Normal text line
				{
					row=copyOL(packet,str);
					if (row==0xff)
						xprintf(PSTR("[insert]Error: Page file has bad line:%s\n"),str);	
					WritePrefix(packet,page.mag,row);
				}
				if (str[0]=='F' && str[1]=='L')		// Fastext links X26
				{
					noCarousel=1; // Indicate the end of this page. Kill it now. TODO: Implement carousels
					copyFL(packet,str,&page);	
					WritePrefix(packet,page.mag,27); // X/27/0	
				}
				Parity(packet,5);	
			}
			else
			{
				xputs("[insert]Error: Page file has empty line in it\n");
				state[mag]=STATE_BEGIN;
			}
			// Obviously need to do the whole parse and parity bit here 
		}
		break;
	default: // Not sure what to do. This can never happen
		state[mag]=STATE_BEGIN;
		xputc('#');	// Oh dear
	}	
	return 0; // success
} // insert

/** dump - Dumps a packet to the console in hex format
\param p : pointer to a packet.
*/
void dump(char* p)
{
	int i;
	int j=0;
	for (i=0;i<5;i++)
	{
		xprintf(PSTR("%02X "),*p++);
					//xprintf(PSTR("page=%lX size=%lX\n\r"),(unsigned long) pageptr,(unsigned long) pagesize);	
	}
	xputc('\n');
	for (j=0;j<4;j++)
	{
		for (i=0;i<10;i++)
		{
			xprintf(PSTR("%02X "),*p++);
						//xprintf(PSTR("page=%lX size=%lX\n\r"),(unsigned long) pageptr,(unsigned long) pagesize);	
		}
		xputc('\n');
	}
}

/** Loads the FIFO with text packets
 *  until either the FIFO is full
 *  or the FIFO is busy when we want to write to it.
 *  FillFIFO is called after a vbi field has been transmitted.  
 */
void FillFIFO(void)
{
#define opt_out_test	
#ifdef opt_out_test	
	static char testOptMode;	// modes are 0..3
	static char testOptRate=0;	// Rate reducer
	static int OldOptRelays=0;	// Tell if the Opts changed
#endif	
	char action;
	char *p;
	static char packet[PACKETSIZE];
	uint16_t fifoWriteAddress;
	static uint8_t packetToWrite=0; // Flags a left over packet that we need to send

	// xputc(PORTC.IN&VBIT_FLD?'O':'E'); // Odd even indicator (just debug nonsense)
	uint8_t evenfield; //=PORTC.IN&VBIT_FLD?1:0;	// Odd or even?
	
	if (fifoWriteIndex==fifoReadIndex)
	{
		return;	// FIFO Full
	}	

	// Get the FIFO ready for new data
	PORTC.OUT&=~VBIT_SEL; // Set the mux to MPU so that we are in control

	fifoWriteAddress=fifoWriteIndex*FIFOBLOCKSIZE+fifoLineCounter*PACKETSIZE; 
	// Don't need to set the address until later. Why do it here?
	//... because the system doesn't seem to work reliably otherwise
	SetSerialRamAddress(SPIRAM_WRITE, fifoWriteAddress); 	// Set FIFO address to write to the current write address	
	evenfield=(fifoWriteIndex)%2;	
	// xputs(PSTR("i"));	
	while(1) // loop until we hit a FIFO access conflict or the FIFO is full.
	{
		if (!packetToWrite) // If we have a packet to write then it is already in the buffer
		{
			evenfield=(fifoWriteIndex)%2;	// TODO: Check that this is odd or even
			//xputc((evenfield&1)+'0');	

			action=g_OutputActions[evenfield][fifoLineCounter];
			//xputc(action);
			switch (action)
			{
			case 'F' : // Filler 825
				FillerPacket(packet);
				break;
			case 'P' :;	// Pass through does nothing. It is configured in I2C Lines
						// Also it is not implemented yet. TODO
			case 'Q' : 	// Quiet Line
				QuietLine(packet,0x0e|(evenfield?1:0));

				break;
			case 'I' :; // Insert
			case '1' :;
			case '2' :;
			case '3' :;
			case '4' :;
			case '5' :;
			case '6' :;
			case '7' :;
			case '8' :;
				// xputs(PSTR("i"));			
				if (insert(packet,evenfield))
					return;	// Oops fail
				break;
			case 'Z' : // How can this even get here when there is no Z?
				if (OptOutMode>0)
				{
					xprintf(PSTR("in ur code, doing ur optout\n"));
					switch (OptOutMode)
					{
					case 14:
						switch (OptOutType)
						{
						case OPTOUT_PREROLL:;
						case OPTOUT_START:;
						case OPTOUT_STOP:;
							SendOpt14(packet);
							dump(packet);
							Parity(packet,14);		// Do the parity and bit reverse
							dump(packet);
							break;
						default:
							xprintf(PSTR("Unknown opt out type\n"));
						}
						break;
					default:
						xprintf(PSTR("Unknown opt out mode\n"));
					}
					OptOutMode=0;	// This is a one shot trigger, so clear it now.					
				}
				else
				if (SendDataBroadcast(packet))
				{
					if (insert(packet,evenfield))	// If there is no databroadcast to send, insert the next normal line
						return;
				}
				else				
				{
				
// #ifdef opt_out_test	
// The contents in here tests Softel opt out signals. It does this by stealing from the databroadcast packet
// So if it is important that you don't lose packets then remove this block.
// See the "O" command for more details
					testOptRate++;	// The existing code should get here at 1Hz
					if ((testOptRate>5 || OldOptRelays!=OptRelays) && false)	// Steal. (Disabled by adding FALSE!)
					{
						OldOptRelays=OptRelays;
						testOptMode=(testOptMode+1)%4;
						testOptRate=0;
						WritePrefix(packet,8,31);
						p=&packet[5];
						*p++=0x64;			// 5 FT=4
						*p++=0x49;			// 6 IAL=2
						*p++=0xb6;			// 7 Address(0)=D
						*p++=0x2f;			// 8 Address(1)=7
						*p++=0x00;			// 9 CI
/*						*p++=HamTab[testOptMode];			// 10 relays 1						
						*p++=HamTab[(testOptMode+1)%4];	// 11 relays 2
						*/
						*p++=HamTab[OptRelays&0x0f];			// 10 relays 1						
						*p++=HamTab[(OptRelays>>4)&0x0f];	// 11 relays 2
						*p++='S';*p++='o';*p++='f';*p++='t';*p++='e';*p++='l';*p++=' '; // 12
						*p++='D';*p++='l';*p++=' '; // 19
						*p++='X';*p++='3';*p++='l';*p++=' '; // 22
						*p++='S';*p++='u';*p++='b';*p++='t';*p++='i';*p++='t';*p++='l';*p++='e';*p++=' '; // 26
						*p++='I';*p++='n';*p++='s';*p++='e';*p++='r';*p++='t';*p++='e';*p++='r'; //35
						// Which leaves two checksum digits
						// dump(packet);
					}
// #endif
					// dump(packet);
					Parity(packet,10);		// Do the parity and bit reverse, as databroadcast doesn't do it
				}
				break;
			default: // Error! Don't know what to do. Make it quiet.
				QuietLine(packet,0x06);
				g_OutputActions[evenfield][fifoLineCounter]='Q'; // Shut it up!
				xputc('>');				
			}
		}
		else
			packetToWrite=0;
			
		// Sometimes we can not put out the next line
		// because the FIFO is busy or full so we return
		if (FIFOBusy) // Can not write because the FIFO is busy
		{
			packetToWrite=1; // Flag that packet has something we need to send the next time
			// xputs(PSTR("x")); // Flag that the line was saved for the next field 			
			return;
		}
		// TODO. This may have been messed up by a SPIRAM_READ in redirect mode
		//PORTC.OUT&=~VBIT_SEL; // Set the mux to MPU so that we are in control [This should be redundant!]

		// REALLY would like to do this here, but it seems to upset the fifo
		fifoWriteAddress=fifoWriteIndex*FIFOBLOCKSIZE+fifoLineCounter*PACKETSIZE; 
		
		SetSerialRamAddress(SPIRAM_WRITE, fifoWriteAddress); 	// Set FIFO address to write to the current write address
		WriteSerialRam(packet, PACKETSIZE);	// Now we can put out the packet

		// Work out the next line
		fifoLineCounter++;
		// The odd field is 17 while even is 18 lines
		if (fifoLineCounter>=(VBILINES+evenfield)) // End of block? Start next field
		{
			fifoWriteIndex=(fifoWriteIndex+1)%MAXFIFOINDEX;
			fifoLineCounter=0;
			if (fifoWriteIndex==fifoReadIndex)
			{
				// xputs(PSTR("f")); // FIFO FULL WARNING
				return;	// FIFO Full
			}	
			else
			{
				// Set the next SPI RAM address
				fifoWriteAddress=fifoWriteIndex*FIFOBLOCKSIZE+fifoLineCounter*PACKETSIZE; 
		//		SetSerialRamAddress(SPIRAM_WRITE, fifoWriteAddress); 	// Set FIFO next address
			}
		}	
		// xputc(fifoLineCounter+'a');	// show the current line number
		
	} // while
	// Reset the FIFO ready to clock out TTX (vbi.c now does the switch)
	//SetSerialRamAddress(SPIRAM_READ, 0); // Set the FIFO to read from address 0
	//PORTC.OUT|=VBIT_SEL; // Set the mux to DENC.
} // FillFIFO
