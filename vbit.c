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

static char pageFilter[6]; 
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
Assume echo mode is always on, but can add it later.
This code pinched from SD_Card_Demo
*/
static void get_line (char *buff, int len)
{
	unsigned char c;
	int idx = 0;
	
	unsigned char started=0;

	for (;;) {
		while (!USB_Serial_GetNB(&c)) // Any characters?
			if (vbiDone) // do the next field?
			{			
				FillFIFO();
				vbiDone=0; // Reset the flag
			}
		if (c == '\r') break;
		if ((c == '\b') && idx) {
			idx--;
			USB_Serial_Send(c);
		}

		if (started && ((unsigned char)c >= ' ') && (idx < len - 1)) {
			buff[idx++] = c;
			USB_Serial_Send(c);
		}
		if (c==0x0e) started=1;

	}
	buff[idx] = 0;
	USB_Serial_Send(c);
	USB_Serial_Send('\n');
}

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
 * \return The page count in that range.
 */
int FindPageCount(void)
{
	char lower[6];
	char upper[6];
	char ch;
	unsigned int high,low;
	uint16_t i; // Don't insert more than 64k pages!!!
	NODEPTR np;
	uint16_t pageCount;
	uint16_t addr;
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
	for (i=low;i<high;i++)
	{
		addr=i+i;
		np=GetNodePtr(&addr);
		if (np!=NULLPTR)
			pageCount++;
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
	// xprintf(PSTR("[dumpPage]\n\r"));
	idx=pageFilterToArray(0);
	np=GetNodePtr(&idx);	// np points to the displaynode
	GetNode(&n,np);
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
	Other good commands return 0 and bad commands return 1
	*/
static int vbit_command(char *Line)
{
	unsigned char rwmode;
	unsigned char returncode=0;
	unsigned int pagecount;
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
	case 'b': // Dump first 19 lines of current page
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
	case 'G': /* G - Packet 8/30 format 1 [p830f1]*/
		if (rwmode==CMD_MODE_NONE)
		{
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
			returncode=1;	
		}
		break;
	case 'H': // H or HO. Set header
		if (Line[2]=='\0' || Line[4]=='\0') // Don't get confused by checksums
		{
			strcpy_P(str,PSTR("      "));
			strncat(str,g_Header,32);	// Just readback the header. TODO. Get the correct length
			str[40]=0;
		}
		else
		{
			strncpy(g_Header,&Line[9],32); // accept new header
		}
		n = ini_puts("service", "header", &(g_Header[0]), inifile);	// Save this value back to the INI file.
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
		sprintf_P(str,PSTR("00%04d"),pagecount); // Where nnn is the number of pages in this filter. 099 is a filler ack and checksum 
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
		
		xprintf(PSTR("UTC=%d\n\r"),UTC);
		
		/**
		UTC=Line[7]-'0';				// s units
		UTC=UTC+(Line[6]-'0')*10;		// s tens
		UTC=UTC+(Line[5]-'0')*60;		// m units
		UTC=UTC+(Line[4]-'0')*60*10;	// m tens
		UTC=UTC+(Line[3]-'0')*60*60;	// h units
		UTC=UTC+(Line[2]-'0')*60*60*10;	// h tens
		*/
		strcpy_P(str,PSTR("OK\n"));
		break;
	case 'U': // TEST
		Init830F1();
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
		strcpy_P(str,PSTR("0VBIT620 Version 0.01\n"));
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
	xprintf(PSTR("0%s%1d\r"),str,returncode); // always add address 0.
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
	n = ini_gets("service", "header",     "mpp MRG DAY dd MTH", g_Header, 32, inifile);
	return 0; // TODO: Return success or otherwise
}

int RunVBIT(void)
{
	/* Join xitoa module to USB-Serial bridge module */
	xfunc_out = (void (*)(char))USB_Serial_Send;
	// Term_Erase_Screen();
	BUTTON_Init( BUTTON_ALL );
	xputs(PSTR("VBIT620 Inserter 0.02 Started\nKings Road Applications\n"));
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