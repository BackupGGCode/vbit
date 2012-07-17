/** ***************************************************************************
 * Description       : VBIT Teletext Page Parser
 * Compiler          : GCC
 *
 * Copyright (C) 2010, Peter Kwan
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
 * The author disclaims all warranties with regard to this
 * software, including all implied warranties of merchantability
 * and fitness.  In no event shall the author be liable for any
 * special, indirect or consequential damages or any damages
 * whatsoever resulting from loss of use, data or profits, whether
 * in an action of contract, negligence or other tortious action,
 * arising out of or in connection with the use or performance of
 * this software.
 *************************************************************************** **/
#include "page.h"

static void put_rc (FRESULT rc)
{
	const prog_char *p;
	static const prog_char str[] =
		"OK\0" "DISK_ERR\0" "INT_ERR\0" "NOT_READY\0" "NO_FILE\0" "NO_PATH\0"
		"INVALID_NAME\0" "DENIED\0" "EXIST\0" "INVALID_OBJECT\0" "WRITE_PROTECTED\0"
		"INVALID_DRIVE\0" "NOT_ENABLED\0" "NO_FILE_SYSTEM\0" "MKFS_ABORTED\0" "TIMEOUT\0";
	FRESULT i;
	if (rc==0) return; // Not interested in OK
	for (p = str, i = 0; i != rc && pgm_read_byte_near(p); i++) {
		while(pgm_read_byte_near(p++));
	}
	xprintf(PSTR("rc=%u FR_%S\n"), (WORD)rc, p);	// TODO
}


/** Parse a single line of a tti teletext page
 * \param str - String to parse
 * \param page - page to return values in
 * \return 1 if there is an error, otherwise 0
 */
unsigned char ParseLine(PAGE *page, char *str)
{
	int32_t n;
	char *ptr;
	xprintf(PSTR("%s\n"),str);
	if (str[2]!=',')
	{
		xprintf(PSTR("[Parse page]Bad format\n"));
		return 1;
	}
	switch (str[0])
	{
	case 'D':; // DE - description, DT - date or DS - ?
		break;			// This is not implemented - no memory :-(
	case 'P': 	// PN or PS
		if (str[1]=='N') // PN is 3 or 5 hex digits
		{
			page->page=0x98; // Remove these two lines, they are only for testing
			page->mag=0x99;
			str[1]='0';
			str[2]='x';
			ptr=&str[1];
			xatoi(&ptr,&n);
			if (n>0x8ff)
			{
				page->subpage=n%0x100;
				n/=0x100;
			}
			else
				page->subpage=0;
			page->mag=n/0x100;
			page->page=n%0x100;	
			if (page->mag>8)
			{
				xprintf(PSTR("error in line %s\n"),str);
				return 1;
			}
			xprintf(PSTR("[ParseLine]PN mag=%d page=%X, subpage=%X\n"),page->mag,page->page,page->subpage);
		}
		else
		if (str[1]=='S')
		{
			str[1]='0';
			str[2]='x';
			ptr=&str[1];
			xatoi(&ptr,&n);
			page->control=n;
		}
		break;
	case 'C':; // CT,nn,<T|C> - cycle time
		break;
	case 'S':; // SP - filename or SC - subcode
		break;
	case 'M':; // MS - no idea
		break;
	case 'O':; // OL - output line
		break;
	case 'F':; // FL - fastext links
		break;
	case 'R':; // RT - readback time?
		break;			
	default :
		xprintf(PSTR("[Parse page]unhandled page code=%c\n"),str[0]);	
		return 1;
	}
	return 0;
} // ParseLine

/** Parse a teletext page
 * \param filaname - Name of the teletext file
 * \return true if there is an error
 */
unsigned char ParsePage(PAGE *page, char *filename)
{
	FIL file;
	char *str;
	const unsigned char MAXLINE=80;
	char line[MAXLINE];
	BYTE res;
	//xprintf(PSTR("[Parse page]Started looking at %s\n"),filename);
	// open the page
	res=f_open(&file,filename,FA_READ);
	if (res)
	{
		xprintf(PSTR("[Parse page]Epic Fail 1\n"));			
		put_rc(res);
		return 1;
	}
	page->filesize=(unsigned int)file.fsize;
	// Read a line
	while (!f_eof(&file))
	{
		str=f_gets(line,MAXLINE,&file);
		if (ParseLine(page,str))
		{
			f_close(&file);
			return 1;
		}
	}
	//xprintf(PSTR("[ParsePage] mag=%d page=%X, subpage=%X\n"),page->mag,page->page,page->subpage);
	f_close(&file);
	// xprintf(PSTR("[Parse page]Ended\n"));	
	return 0;
}

void ClearPage(PAGE *page)
{
	page->filename[0]=0;
	page->mag=0x99;
	page->page=0xff;
	page->subpage=0xff;
	page->timerMode='T';
	page->control=0;
	page->filesize=0;
}
