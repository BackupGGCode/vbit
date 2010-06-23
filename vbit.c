/** ***************************************************************************
 * Description       : VBIT teletext inserter program for XMEGA
 * Compiler          : GCC
 *
 * Copyright (C) 2010, Peter Kwan
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
Assume echo mode is always on, but can add it later
*/
static void get_line (char *buff, int len)
{
	unsigned char c;
	int idx = 0;
	
	unsigned char started=0;

	for (;;) {
		while (!getBridgeByte(&c, false)) // Any characters?
			if (vbiDone) // do the next field?
			{			
				FillFIFO();
				vbiDone=0; // Reset the flag
			}
		if (c == '\r') break;
		if ((c == '\b') && idx) {
			idx--;
			sendBridgeByte(c, true);
		}

		if (started && ((unsigned char)c >= ' ') && (idx < len - 1)) {
			buff[idx++] = c;
			sendBridgeByte(c, true);
		}
		if (c==0x0e) started=1;

	}
	buff[idx] = 0;
	sendBridgeByte(c, true);
	sendBridgeByte('\n', true);
}

/* SPI ram test */
void test2(void)
{
	char packet[PACKETSIZE];
	FillerPacket(packet);
	// VBIT Mux needs setting as SCK is shared
	GPIO_Off(VBIT_SEL);	// Low=AVR, High=VBIT
	xprintf(PSTR("Status=%03d\n\r"),GetSerialRamStatus());	
	int i;
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
	for (i=0;i<6;i++)
	{
		SetSerialRamAddress(SPIRAM_READ, i);
		ReadSerialRam(test,8);
		DeselectSerialRam();
	}
	xputs(PSTR("Test ended Have a Nice Day\n\r"));	
	for (i=0;i<4;i++)
	{
		xprintf(PSTR("%03d "),test[i]);
		if (test[i]<' ' || test[i]>0x7f) test[i]='!';
	}
	xprintf(PSTR("\n\r%s\n\r"),test);
}

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
	long n;
	unsigned char i;
	char str[80];
	char *ptr;
	str[0]='?';
	str[1]='\0';
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
	case 'I': // III or I2
		// I20xnnmm
		if (Line[2]=='2') // SAA7113 I2C. value. 0xnnmm where nn=address mm=value to write 
		{
			xputs(PSTR("Setting SAA7113 I2C register\n"));
			ptr=&Line[3];
			xatoi(&ptr,&n);
			xprintf(PSTR("Blah=%04X\n"),n);
			i2c_SetRegister((n>>8)&0xff,n&0xff);			
			xprintf(PSTR("Done\n"));
		}		
		break;
	case 'Y': /* Y - Version */
		xputs(PSTR("VBIT620 Version 0.00\n"));
		break;		
	case 'T': // TEST
		// testIni();
		test2();
		xputs(PSTR("OK\n"));
		break;
	case 'U': // TEST
		Init830F1();
		break;
	case 'X':	/* X - Exit */
		return 2;	
	case 'O':	/* O - Opt out. Example: O1c*/
		/* Two digit hex number. Only 6 bits are used so the valid range is 0..3f */
			ptr=&Line[0];
			Line[0]='0';Line[1]='x';
			xatoi(&ptr,&n);
			OptRelays=n & 0x3f;
		break;		
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
		default:
			returncode=1;	
		}
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
	default:
		xputs(PSTR("Unknown command\n"));
		returncode=1;
	}
	xprintf(PSTR("%d\n"),returncode);
	return 0;
}

/* Load settings from SD card
 * inserter.ini
*/
int LoadINISettings(void)
{
	int n;
	n = ini_gets("service", "outputodd", "111Q2233P44556678Q", &(g_OutputActions[0][0]), 18, inifile);	
	n = ini_gets("service", "outputeven", "111Q2233P44556678Q", &(g_OutputActions[1][0]), 18, inifile);	
}

int RunVBIT(void)
{
	/* Join xitoa module to USB-SPI bridge module */
	xfunc_out = (void (*)(char))sendBridgeByteSD;
	Term_Erase_Screen();
	xputs(PSTR("VBIT620 Inserter 0.01 Started\nKings Road Applications\n"));
	GPIO_Init();	// Set up the ports
	// Configure the spiram port and set the spiram to sequential mode
	spiram_initialise();
	SetSerialRamStatus(SPIRAM_MODE_SEQUENTIAL);	
	disk_initialize(0); // Set up the SD memory card
	i2c_init();			// Start the video processors
	InitVBI();			// Set up the video timing
	f_mount(0,&Fatfs[0]);
	LoadINISettings();
	InitDataBroadcast();
	for (;;)
	{
		xputc('>');
		get_line((char*)Line, sizeof(Line));
		if (vbit_command((char*)Line)==2) break;
	}
	xputs(PSTR("VBIT Terminated. Have a Nice Day\n"));
	return 0;
}