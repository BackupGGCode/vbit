/* Packet 31 Databroadcast*/

// Parts of this code are
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
	static unsigned char continuity = 0;

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
	*(p++)=HamTab[0x0f];				// 5:
	//  Need to implement DL
	*(p++)=HamTab[0];					// 6: FT Format type set to format 1 with implicit continuity and no repeats
	*(p++)=HamTab[1];					// 7: IAL address length of 1 (IAL)
	*(p++)=HamTab[9];					// 8: SPA address 9. (For SISCom)
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
