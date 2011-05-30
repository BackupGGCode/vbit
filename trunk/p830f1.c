/** ***************************************************************************
 * Description       : VBIT: Maintains Packet 8/30 format 1
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
#include "p830f1.h"

/* Globals */
unsigned char pkt830[46];		/* The packet.40 chars + 2 CRI + WST+terminator */

//---------------------------------------------------------------
/** Given a page number in mppss format fills in a packet for tx
 *
 * Only uses the first three digits mpp 
 */
void SetInitialPage(unsigned char *pkt,const char *mpp,const char *subcode)
{
	int mag,pu,pt,s1,s2,s3,s4;

	// sscanf(mpp,"%1d%1x%1x",&mag,&pt,&pu);
	mag=1;pt=0;pu=0; // XYZZY TODO
	pkt[6] =HamTab[pu]; // Page units
	pkt[7] =HamTab[pt]; // Page tens
	// sscanf(subcode,"%1x%1x%1x%1x",&s4,&s3,&s2,&s1);
	s4=1;s3=1;s2=1;s1=1;
	pkt[8] =HamTab[s1]; // subcode 1
	pkt[9]=HamTab[(s2 & 0x07) +((mag & 0x01)<<3)]; // subcode 2 and mag bit 0
	pkt[10]=HamTab[s3]; // subcode 3
	pkt[11]=HamTab[(s4 & 0x03) + ((mag & 0x06)<<1)]; // subcode 3 and mag bits 2+3
} 

//---------------------------------
/** set status label on packet 8/30
 * Up to 20 text characters
 */
void SetStatusLabel(unsigned char *pkt, char *str)
{
	int i;
	for(i=25;i<45;i++)
	{
		// if we haven't run out of string
		if(*str != 0)
			pkt[i] = ParTab[(int)*str++];
		else
			pkt[i] = ' ';
	}
} // SetStatusLabel
	
//-----------------------------------------------------
/**Set the NIC code in packet 830 (Format 1)
 * nic is four hex characters
 */
void SetNIC1(unsigned char *pkt,char *nic)
{
	char buf[4];
	int i;

	for (i=0;i<4;i++)
	{
		if((nic[i]>='0') && (nic[i]<='9'))
			buf[i]=nic[i]-'0';
		else
		if((nic[i]>='a') && (nic[i]<='f'))
			buf[i]=nic[i]+10-'a';
		else
		if((nic[i]>='A') && (nic[i]<='F'))
			buf[i]=nic[i]+10-'F';
	}

	pkt[12]=(BitRev[(int)buf[1]]<<4)+BitRev[(int)buf[0]];
	pkt[13]=(BitRev[(int)buf[3]]<<4)+BitRev[(int)buf[2]];

} // SetNIC1

// Give an offset in half hours, returns an offset formatted for packet 8/30 format 1 transmission
int MakeOffset(int off)
{
  int result=0x81;   // These are usually set because to keep the bit clock spinning
  if (off<0)         // Check if it's a negative offset (ie. absolutely nowhere)
  {
    off=-off;
    result|=0x40;
  }
  result|=(off&0x1f)<<1; // Mask off to 5 bits and shift in
  return result;
}//MakeOffset

//--------------------------------------------------------
/** Initial Load Packet 8/30 Format 1 (Time and Status)
* Needs to load the packet with settings from the INI file
 */
void Init830F1(void)
{
	int i,n;

	char str[MAXSTR];
	unsigned char *p;
	p=pkt830;
	// Assemble the basic packet 8/30 format 1
	*p++=0;                               // line number (?)
	*p++=27;                              // framing code
	*p++=HamTab[0];           // mrag  
	*p++=HamTab[0xf];
	*p++=HamTab[0];         // 6     designation code  
	*p++=0x15;   			// 7-12  initial page
	*p++=0x15;
	*p++=0xea;
	*p++=0xea;
	*p++=0xea;
	*p++=0x5e;
	*p++=0xa2;              // 13,14 network ID
	*p++=0xfe;
	*p++=0x11;              // 15,   time offset in half hours starting from bit 2 (inserter overwrites)
	*p++=0x11;*p++=0x11;*p++=0x11;         // 16-18 MJD for date 0 (inserter overwrites)
	*p++=0x11;*p++=0x11;*p++=0x11;        // 19-21 UTC for time 0 (inserter overwrites)
	*p++=HamTab[0];                       // 22 alert/recall/auto/manual (BBC Alerts)
	*p++=HamTab[0];                       // 23 other bits
	*p++=HamTab[0];                       // 24 other bits
	*p++=HamTab[0];                       // 25 other bits
	for (i=0;i<20;i++)					// 26-45 20 chars of status text
		*p++=' ';

	// update the packet with the settings we have
	// (Get the following settings from the INI file)
	n = ini_gets("p830f1", "initialpage", "003F7F", str, MAXSTR, inifile); // ppssss
	// SetInitialPage(pkt830,str); NEED TO FIX!!!!
	n=ini_gets("p830f1", "label", PSTR("VBITFax             "), str, MAXSTR, inifile);		
	xputs(str);
	SetStatusLabel(pkt830,PSTR("Your status label   "));
	n = ini_gets("p830f1", "nic", "2a2f", str, MAXSTR, inifile); // need the drive already mounted
	xputs(str);
	SetNIC1(pkt830,str);
	// Local time offset
	pkt830[14]=MakeOffset(0); // 0=GMT, 2=BST

	// set the GPIs
	// Request 25Jan2005 from Geoff Bluestone. Common C1+D1 and C2+D2
	//  i = (lastbits[0] & 0xf0) >> 4); // Original code
/*
	i = ((lastbits[0] & 0xf0) >> 4) | ((lastbits[0] & 0x03) << 2); // This wastes bits, by sharing D1+C1 and D2+C2
	pkt830[21] = HamTab[i];
	i = lastbits[0] & 0xf;
	pkt830[22] = HamTab[i];
	i = (lastbits[1] & 0xf0) / 16;
	pkt830[23] = HamTab[i];
	i = lastbits[1] & 0xf;
	pkt830[24] = HamTab[i];
*/

} // Init830F1 

