/* Packet 31 Databroadcast*/

// Parts of this code were originally
// Copyright 2004-2006 (c) MRG Systems Ltd.

#include "databroadcast.h"

int nServicePacketAddress;  /// Address can be 0 to 15 (default 2). TODO: Make more flexible?

static RingBuff_t DBbuffer;

/** Send a string to the ring buffer
 * \param str : String to send to the ring buffer
 * \result 0 if the string was accepted. A pointer to the last char sent. 
 * A non zero result should be used to resend the 
 */
char* putringstring(char* str)
{
	// xputs(PSTR("P"));
	char *c=str;
	for (;*c;c++)
	{
		// xputc(*c);
		if (Buffer_IsFull(&DBbuffer))
		{
			// TODO: do handshaking? maybe XON/XOFF?
			// xputs(PSTR("full\n"));	
			return c;
		}
		else
			Buffer_StoreElement(&DBbuffer, *c);
	}
	return 0;
} // putringstring

void InitDataBroadcast(void)
{
	Buffer_Initialize(&DBbuffer);
}

//--------------------------------------------------------
/** Send databroadcast as packet 8/31 using IDL type A checksums
 * This routine takes data from the ring buffer and places it into a packet.
 * \param pkt : pointer to a char buffer that must be at least 45 characters long
 * \return 1 if there is nothing to send, or 0 if there is a valid packet.
 * TODO: Repeats, Datalength. 
 */
int SendDataBroadcast(char* pkt)
{
	int i;
	// static unsigned char continuity = 0; // TODO

	//xputc('D');

	// Do we have any data to send?
	if (Buffer_IsEmpty(&DBbuffer))
	{
		return 1;
	}
	
	// Assemble the basic packet 8/31
	// Note that the comment numbers are one higher than the array index. 
	char* p=pkt;
	*(p++)=0x55;						// 1: clock run in
	*(p++)=0x55;                        // 2: clock run in
	*(p++)=0x27;						// 3: framing code
	*(p++)=HamTab[0x08];				// 4: mrag for 8/31. data channel. designation code.
	*(p++)=HamTab[0x0f];				// 5: DCG
	//  Need to implement DL
	*(p++)=HamTab[0];					// 6: FT Format type set to format 1 with implicit continuity and no repeats
	*(p++)=HamTab[1];					// 7: IAL address length of 1 (IAL)
	*(p++)=HamTab[9];					// 8: SPA address=9 for SISCom, 8 for BTVC
	// *(p++)=0x00; //						// 9: no repeat
	//*(p++)=continuity++;				// 10: continuity indicator
	// The rest of the packet doesn't need to be filled with dummy values,
	// except for debugging
	/*
	for (i=8;i<43;i++)					// 11-43: 33 payload bytes
		*(p++)=' ';
	*(p++)=0xfe;						// 44: Hi checksum
	*(p++)=0xff;						// 45: Lo checksum
	*/
	// initialise the checksum
	ClearCRC();
	
	// insert the payload and set even parity,
	// and clear out the rest of the packet.
	// TODO: Add the DL value. This will reduce the overheads
	for (i=8;i<43;i++)
	{
		if (Buffer_IsEmpty(&DBbuffer))
			pkt[i]=0x80;
		else
			pkt[i]=pgm_read_byte(&ParTab[Buffer_GetElement(&DBbuffer)]); 	// What about partial lines? DL!
		// xputs(PSTR(" char="));
		// xputc(pkt[i]);
	}
	// Calculate the CRCs 
	for (i=8;i<43;i++)
	{
		AddCRC(pkt[i]);
	}

	// add it to the end
	EndPacket(&pkt[43],&pkt[44]);

	// TODO: Repeats
	
	return 0;
}//SendDataBroadcast

// SendOpt14 - Sends a mode 14 start packet
int SendOpt14(char* pkt)
{
// Softel opt-outs have 6 address bytes (24 bit)
#define AddressLength 6
	unsigned int i;
	char* p;
		
	WritePrefix(pkt,8,31);	// CRI,FC,MRAG
	p=&pkt[5];	
	
	//  Need to implement DL
	*(p++)=HamTab[0];					// 6: FT Format type set to format 1 with implicit continuity and no repeats
	*(p++)=HamTab[AddressLength];		// 7: IAL address length of 6 (IAL)
	*(p++)=HamTab[0];					// Address 000242 (Eurosport 2)
	*(p++)=HamTab[0];					// 
	*(p++)=HamTab[0];					// 
	*(p++)=HamTab[2];					// 
	*(p++)=HamTab[4];					// 
	*(p++)=HamTab[2];					// 
	*(p++)=HamTab[0];					// Checksum computing starts here.
	// RI is not included because FT=0

	//1538151515496449155001B0B0B03134B6B0B932B020204332B334B5B920202020D3CDD351E3BF11
//0 6 0 0 0 2 4 2 0.............. d..............................................
//ö  8ö ö ö  I d Iö  Pö  0 0 0 1 4 6 0 9 2 0     C 2 3 4 5 9         S M S Q c ?ö 
	*(p++)=0x50; 
	*(p++)=0x01;*(p++)=0xB0;*(p++)=0xB0;*(p++)=0xB0;*(p++)=0x31;	// 15
	*(p++)=0x34;*(p++)=0xB6;*(p++)=0xB0;*(p++)=0xB9;*(p++)=0x32;	// 20
	*(p++)=0xB0;*(p++)=0x20;*(p++)=0x20;*(p++)=0x43;*(p++)=0x32;	// 25
	*(p++)=0xB3;*(p++)=0x34;*(p++)=0xB5;*(p++)=0xB9;*(p++)=0x20;	// 30
	*(p++)=0x20;*(p++)=0x20;*(p++)=0x20;*(p++)=0xD3;*(p++)=0xCD;	// 35
	*(p++)=0xD3;*(p++)=0x51;*(p++)=0xE3;*(p++)=0xBF;*(p++)=0x11;	// 40
	
	// Calculate the CRCs (Parity is already done on these values)
	ClearCRC();
	// Strictly speaking, we should also look at FT and add as required CI, RI, DL.
	for (i=8+AddressLength;i<43;i++) // Where the count is 7+Address Length. (BTVC=1, Softel trigger=6.)
	{
		AddCRC(pkt[i]);
	}
	// add it to the end
	// Oh dear. It doesn't match. Let's not do this at the moment
	// EndPacket(&pkt[43],&pkt[44]);

	return 0; // TODO. Return something useful?
} // SendOpt14
