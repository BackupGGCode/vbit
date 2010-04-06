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
#define MAXLINE	17

char g_OutputActions[2][18];

/** Check that parity is correct for the packet payload
 * The parity is set to odd for all bytes from offset to the end
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
	char *p=packet;
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

/** All the bits on this line are off
 */
void QuietLine(char * packet)
{
	const unsigned char debug=1;
	for (int i=0;i<PACKETSIZE;i++)
		if (debug)
		{
			packet[i]=(i%10)>5?0xff:0x00;
		}
		else		
			packet[i]=0;
} // QuietLine

/** A header has mag, row=0, page, flags, caption and time
 */
void Header(char *packet ,unsigned char mag, unsigned char page, unsigned int subcode,
			unsigned int control, char *caption)
{
	static int lastsec;
//	char date[10]; TODO: Implement something convincing
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
	packet[12]=HamTab[0]; // TBA C11 to C14
	strncpy(&packet[13],caption,24); // This is dangerously out of order! Need to range check and fill as needed
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
	packet[37]='0'+hour/10; // grrr. Stupid sprintf breaking the CLI
	packet[38]='0'+hour%10;
	packet[39]=':';
	packet[40]='0'+min/10;
	packet[41]='0'+min%10;
	packet[42]='.';
	packet[43]='0'+sec/10;
	packet[44]='0'+sec%10;

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
	xatoi(&textline, &linenumber);
	//xprintf(PSTR("Line number=%d\n"),(int)linenumber);
	// Skip to the comma to get the body of the command
	for (i=0;i<4 && ((*textline++)!=',');i++);
	if (*(textline-1)!=',')
	{
		return 0xff; // failed
	}
	for (char *p=packet+5;*textline && p<(packet+PACKETSIZE);textline++) // Stop on end of file OR packet over run
	{
		// TODO: Also need to check viewdata escapes
		if (*p!=0x0d) // Do not include \r
		{
			if ((*textline && 0x7f)==0x0a)		
			{
				// *p=0x0d; // Translate lf to cr (double height)
				*p='?';
			}
			else
				*p=*textline;
			p++;
		}
	}	
	return linenumber;
} // copyOL

/** Fastext links
 * FL,<link red>,<link green>,<link yellow,<link cyan>,<link>,<link index>
 */
static void copyFL(char *packet, char *textline)
{
	QuietLine(packet);	// TODO: Fastext links	
}

/** This generates the next line
 * So, tell me how this can support parallel streams?
 */
static unsigned char insert(char *packet, uint8_t field)
{
	unsigned char noCarousel=0; // Use to only display the first page of a carousel. TODO. Implement carousels
	static FIL pagefile, list;
	static uint8_t savefield;
	static DWORD fileptr;		// Used to save the file pointer to the body of the ttx file
	char pagename[15];
	char listentry[25];
	char data[80];
	char *str;
	char *p;
	unsigned char row;
	FRESULT res;	
	PAGE page;
	BYTE drive=0;
	DWORD pageptr;				// Pointer to the start of page in pages.all
	DWORD pagesize;				// Size of the page in pages.all
	unsigned char mag=0;	// just one mag for now!
	// xputc(state[mag]+'0');
	switch (state[mag])
	{
	case STATE_BEGIN: // Open the first page and drop through to idle
		xputs(PSTR("B"));
	
		res=(WORD)disk_initialize(drive);	// di0
		put_rc(f_mount(drive, &Fatfs[drive]));	// fi0
		put_rc(f_chdir("onair"));
		res=f_open(&list,"mag1.lst",FA_READ);	
		if (res)
		{
			xprintf(PSTR("[insert]Epic Fail 1\n"));			
			put_rc(res);
			return 1;
		}	
		f_gets(listentry,sizeof(listentry),&list);		
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
			//xprintf(PSTR("page=%lX size=%lX\n\r"),(unsigned long) pageptr,(unsigned long) pagesize);					
		}
		xprintf(PSTR("\n\rf=%s\n\r"),pagename);		
		state[mag]=STATE_IDLE;
		res=f_open(&pagefile,"pages.all",FA_READ);		// Only need to open this once!
	case STATE_IDLE: // If permitted to run we send the header
		// xputs(PSTR("I"));
		noCarousel=0; // Just until we can handle carousels
		savefield=field; // record the field that we are on	
		// Now tx the header
		// Need to do the whole parse and parity bit here 
		// open pagefile
		LED_On( LED_5 );		// LED5 - high while seeking a folder
		f_lseek(&pagefile,pageptr); // Instead of f_open just use lseek
		LED_Off( LED_5 );
		if (res)
		{
			xprintf(PSTR("[insert]Epic Fail 2\n"));			
			put_rc(res);
			return 1;
		}	
		// Loop through the header and parse down to the OL
		while (pagefile.fptr<(pageptr+pagesize))
		{
			fileptr=pagefile.fptr;		// Save the file pointer in case we found "OL"
			f_gets(data,sizeof(data),&pagefile);
			if (data[0]=='O' && data[1]=='L')
			{
				f_lseek (&pagefile, fileptr);	// Step back to the OL line
				break;
			}
			if (ParseLine(&page, data))
			{
				xprintf(PSTR("[insert]file error handler needed:%s\n"),data);			
			}
		}
		// xprintf(PSTR("M%d P%X, "),page.mag,page.page);
		// create the header packet. TODO: Add a system wide header caption
		Header(packet,page.mag,page.page,page.subpage,page.control,"VBIT Pocket Inserter            ");		// 6 - 24 characters plus 8 for clock
		state[mag]=STATE_HEADER;
		break;
	case STATE_HEADER: // We are waiting for the field to change before we can tx
		// xputs(PSTR("H"));
		if (field==savefield)
		{
			QuietLine(packet);	// TODO: We would let the next magazine steal this line
			break;
		}
		state[mag]=STATE_SENDING; // We have the new field. Change state
	case STATE_SENDING:
		if (f_eof(&pagefile) || noCarousel || (pagefile.fptr>=(pageptr+pagesize))) // Page done?
		{
			if (f_eof(&list)) // List All done? Start again at the top of the list
			{
				// f_close(&pagefile);
				f_lseek (&list, 0);
			}
			f_gets(listentry,sizeof(listentry),&list);
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
			//xprintf(PSTR("page=%lX size=%lX\n\r"),(unsigned long) pageptr,(unsigned long) pagesize);									
			}
			// xprintf(PSTR("L-%s "),pagename);				
			state[mag]=STATE_IDLE;
		}
		else
		{
			// Get the next line
			str=f_gets(data,sizeof(data),&pagefile);
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
					Parity(packet,5);	
				}
				if (str[0]=='F' && str[1]=='L')		// Fastext links X26
				{
					noCarousel=1; // Indicate the end of this page. Kill it now. TODO: Implement carousels
					copyFL(packet,str);	
				}
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
		xputc('!');
	}	
	return 0; // success
} // insert

void TestPage(void)
{
char packet[PACKETSIZE];
static uint8_t counter=0;
uint8_t i;
int control;
	control=CTRL_LANGUAGE_0_bm | CTRL_ENABLETX_bm;
	if (PORTC.IN&VBIT_FLD) // Odd field
	{
		Header(packet,1,0,0,control,"VBIT Pocket Inserter            ");		// 6 - 24 characters plus 8 for clock
		WriteSerialRam(packet, PACKETSIZE);
		Row(packet,2,1,"                                        ");	// 7
		WriteSerialRam(packet, PACKETSIZE);
		Row(packet,2,2,"\177 Analog inserter with digital path     ");	// 8
		WriteSerialRam(packet, PACKETSIZE);
		Row(packet,2,3,"\177 USB controlled and powered            ");	// 9
		WriteSerialRam(packet, PACKETSIZE);
		Row(packet,2,4,"\177 SD card for a huge number of pages    ");	// 10
		WriteSerialRam(packet, PACKETSIZE);
		Row(packet,2,5,"\177 Can operate standalone with ext. power");	// 10
		WriteSerialRam(packet, PACKETSIZE);
		Row(packet,2,6,"\177 Inserts all WSS modes                 ");	// 10
		WriteSerialRam(packet, PACKETSIZE);
		Row(packet,2,7,"\177 Can handle PAL and NTSC video         ");	// 10
		WriteSerialRam(packet, PACKETSIZE);
		Row(packet,2,24,"\001\035\007Index \002Next  \003Up  \006Down              ");	// 10
		WriteSerialRam(packet, PACKETSIZE);
		FillerPacket(packet);
		if (counter>=25)
		{
			counter=0;
			// At this point generate packet 8/30/1
			FillerTest(packet,0xff);
		}
		WriteSerialRam(packet, PACKETSIZE);		
		FillerPacket(packet);
		for (i=11;i<21;i++)
			WriteSerialRam(packet, PACKETSIZE);

		counter++;
	}
	else // even field
	{
		Header(packet,2,0,0,control,"VBIT Test page                  ");		
		WriteSerialRam(packet, PACKETSIZE);	
		Row(packet,1,1,"This is a line of text, MAG 1, ROW 1    ");	// 7
		WriteSerialRam(packet, PACKETSIZE);
		Row(packet,1,2,"This is a line of text, MAG 1, ROW 2    ");	// 8
		WriteSerialRam(packet, PACKETSIZE);
		Row(packet,1,3,"This is a line of text, MAG 1, ROW 3    ");	// 9
		WriteSerialRam(packet, PACKETSIZE);
		Row(packet,1,4,"This is a line of text, MAG 1, ROW 4    ");	// 10
		WriteSerialRam(packet, PACKETSIZE);
		FillerPacket(packet);
		for (i=11;i<22;i++)
			WriteSerialRam(packet, PACKETSIZE);
	}
} // TestPage

/** Loads the FIFO with text packets
 *  until either the FIFO is full
 *  or the FIFO is busy when we want to write to it.
 *  FillFIFO is called after a vbi field has been transmitted.  
 */
void FillFIFO(void)
{
	char action;
	static char packet[PACKETSIZE];
	uint16_t fifoWriteAddress;
	static uint8_t packetToWrite=0; // Flags a left over packet that we need to send

	// xputc(PORTC.IN&VBIT_FLD?'O':'E'); // Odd even indicator (just debug nonsense)
	uint8_t oddfield; //=PORTC.IN&VBIT_FLD?1:0;	// Odd or even?

	// Get the FIFO ready for new data
	PORTC.OUT&=~VBIT_SEL; // Set the mux to MPU so that we are in control
	fifoWriteAddress=fifoWriteIndex*FIFOBLOCKSIZE+fifoLineCounter*PACKETSIZE;
	SetSerialRamAddress(SPIRAM_WRITE, fifoWriteAddress); 	// Set FIFO address to write to the current write address
	// xputs(PSTR("i"));	
	while(1) // loop until we hit a FIFO access conflict or the FIFO is full.
	{
		if (!packetToWrite) // If we have a packet to write then it is already in the buffer
		{
			oddfield=fifoWriteIndex%2;	// TODO: Check that this is odd or even

			action=g_OutputActions[oddfield][fifoLineCounter];
			// xputc(action);
			switch (action)
			{
			case 'F' : // Filler 825
				FillerPacket(packet);
				break;
			case 'P' :;	// Pass through does nothing. It is configured in I2C Lines
						// Also it is not implemented yet. TODO
			case 'Q' : 	// Quiet Line
				QuietLine(packet);
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
				//xputs(PSTR("i"));			
				insert(packet,oddfield);
				break;
			default: // Error! Don't know what to do. Make it quiet.
				QuietLine(packet);
				xputc('?');				
			}
		}
		packetToWrite=0;
		// Sometimes we can not put out the next line
		// because the FIFO is busy or full so we return
		if (FIFOBusy) // Can not write because the FIFO is busy
		{
			packetToWrite=1; // Flag that packet has something we need to send the next time
			//xputs(PSTR("x"));			
			return;
		}

		// Work out the next line
		fifoLineCounter++;
		// The odd field is 17 while even is 18 lines
		if (fifoLineCounter>=(MAXLINE+(oddfield?1:0))) // End of block? Start next field
		{
			uint8_t index;
			index=(fifoWriteIndex+1)%MAXFIFOINDEX;
			if (index==fifoReadIndex)
			{
				packetToWrite=1; // Flag that packet has something we need to send the next time
				// xputs(PSTR("X"));	
				// xprintf(PSTR("X%d,%d "),fifoWriteIndex,fifoReadIndex);
				return;	// FIFO Full
			}
			fifoLineCounter=0;
			fifoWriteIndex=index;
		}	
		// xputc(fifoLineCounter+'a');	// show the current line number
		
		WriteSerialRam(packet, PACKETSIZE);	// Now we can put out the packet
	}
	// Reset the FIFO ready to clock out TTX (vbi.c now does the switch)
	//SetSerialRamAddress(SPIRAM_READ, 0); // Set the FIFO to read from address 0
	//PORTC.OUT|=VBIT_SEL; // Set the mux to DENC.
} // FillFIFO