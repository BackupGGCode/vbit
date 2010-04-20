/** ***************************************************************************
 * Description       : VBIT SD File Manager for XMEGA
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
#include "sdfilemanager.h"

extern FATFS Fatfs[1];			/* File system object for each logical drive */
FILINFO Finfo;
#if _USE_LFN
char Lfname[_MAX_LFN+1];
#endif
void put_rc (FRESULT rc)
{
	const prog_char *p;
	static const prog_char str[] =
		"OK\0" "DISK_ERR\0" "INT_ERR\0" "NOT_READY\0" "NO_FILE\0" "NO_PATH\0"
		"INVALID_NAME\0" "DENIED\0" "EXIST\0" "INVALID_OBJECT\0" "WRITE_PROTECTED\0"
		"INVALID_DRIVE\0" "NOT_ENABLED\0" "NO_FILE_SYSTEM\0" "MKFS_ABORTED\0" "TIMEOUT\0";
	FRESULT i;

	for (p = str, i = 0; i != rc && pgm_read_byte_near(p); i++) {
		while(pgm_read_byte_near(p++));
	}
	xprintf(PSTR("rc=%u FR_%S\n"), (WORD)rc, p);	// TODO
}

/** Create the magazine and carousel list files
 * \param mag : approximate magazine count
 */
void SDCreateLists(int mag, unsigned int pagecount)
{
	long p1,p2;
	// FIL file[MAXMAG];		/* Array of output files */
	FIL myfile;
	FIL pagesfile;
	FIL currentpage;
	DIR dir;			/* Directory object */
	UINT s1, s2;	
	char filename[80];
	char str[80];
	char *ptr=0;
	FRESULT res;	
	FATFS *fs;
	PAGE page;
	BYTE drive=0;
	PAGE *pageptr=&page;
	
	res=(WORD)disk_initialize(drive);	// di0
	put_rc(f_mount(drive, &Fatfs[drive]));	// fi0
	put_rc(f_chdir("onair"));	
	filename[0]='.';
	filename[1]=0;
	res = f_opendir(&dir, filename);
	if (res) { put_rc(res); return; }
	p1 = s1 = s2 = 0;
	
	// Set up the output list files
	strcpy(filename,"mag1.lst");
	filename[3]=mag+'0';
	
	
	// The index file
	res=f_open(&myfile,filename,FA_CREATE_ALWAYS|FA_WRITE);
	if (res)
	{
		xprintf(PSTR("[sdifilemanager]Epic Fail 1a\n"));			
		put_rc(res);
	}	
	
	// The pages file
	res=f_open(&pagesfile,"pages.all",FA_CREATE_ALWAYS|FA_WRITE);
	// Now we extend the file to approximately the space that we need
	// TODO: Check that the allocation succeeded
	f_lseek(&pagesfile,pagecount*0x500);	// allocate the memory (about 1200 bytes per page)
	f_lseek(&pagesfile,0);					// and set the pointer back to the start
	if (res)
	{
		xprintf(PSTR("[sdifilemanager]Epic Fail 1b\n"));			
		put_rc(res);
	}	
	
	for(;;) {
		res = f_readdir(&dir, &Finfo);
		if ((res != FR_OK) || !Finfo.fname[0]) break;
		if (Finfo.fattrib & AM_DIR) {
			s2++;
		} else {
			s1++; p1 += Finfo.fsize;
		}

		xprintf(PSTR("%c%c%c%c%c %u/%02u/%02u %02u:%02u %9lu  %s"), 
					(Finfo.fattrib & AM_DIR) ? 'D' : '-',
					(Finfo.fattrib & AM_RDO) ? 'R' : '-',
					(Finfo.fattrib & AM_HID) ? 'H' : '-',
					(Finfo.fattrib & AM_SYS) ? 'S' : '-',
					(Finfo.fattrib & AM_ARC) ? 'A' : '-',
					(Finfo.fdate >> 9) + 1980, (Finfo.fdate >> 5) & 15, Finfo.fdate & 31,
					(Finfo.ftime >> 11), (Finfo.ftime >> 5) & 63,
					Finfo.fsize, &(Finfo.fname[0]));
		for (p2 = strlen(Finfo.fname); p2 < 14; p2++)
			xputc(' ');
		xprintf(PSTR("\n"));

		if (strstr(Finfo.fname,".TTI")) // To do: ignore the case
		{
			page.page=99;
			page.mag=10;
			ParsePage(pageptr, Finfo.fname);
			sprintf(str,"%s",Finfo.fname);
			xprintf(PSTR("Parsed page M=%d, P=%02X, S=%02X : %s\n"),pageptr->mag,pageptr->page,pageptr->subpage,Finfo.fname);
			if (page.mag==mag || 1) // Put all the pages in the list for now
			{
				// Write to the index file
				sprintf(str,"%s,%lx,%x\n",Finfo.fname,pagesfile.fptr,page.filesize);
				res=f_puts(str,&myfile);				
				// res=f_puts(Finfo.fname,&myfile);
				// f_putc((int)'\n',&myfile);
				// Copy to the pages file. (just a big file of ALL the pages)
				f_open(&currentpage,Finfo.fname,FA_READ);
				while (!f_eof(&currentpage))
				{
					ptr=f_gets(str,80,&currentpage);
					f_puts(str,&pagesfile);
				}
				f_close(&currentpage);
			}
		}
	}
	f_close(&myfile);
	f_close(&pagesfile);
	xprintf(PSTR("%4u File(s),%10lu bytes total\n%4u Dir(s)"), s1, p1, s2);
	if (f_getfree(ptr, &p1, &fs) == FR_OK)
		xprintf(PSTR(", %10luK bytes free\n"), p1 * fs->csize / 2);
}

