/* Packet 31 Databroadcast*/

// Parts of this code are
// Copyright 2004-2006 (c) MRG Systems Ltd.

#include "databroadcast.h"

// Globals
char nic1[6];
char nic2[6];

char ChecksumFormat='A';// Or B

int g_GSM[2];           /// Format 1 and 2 values for the GSM command

int nServicePacketAddress;  /// Address can be 0 to 15 (default 2)

char strLabel830[40];       /// The current packet 830 label, eg "BBC ONE"
char strNormalLabel830[40]; /// The packet 830 label from configuration

// These are not used
int m_nNIC1=0xfa6f; /// 16 bit network identifier code 8/30 format 1 (BBC1)
int m_nNIC2=0x2c2f; /// 16 bit network identifier code 8/30 format 2



/* Finds the absolute value of a number */
int abs(int no)
{
  return (no<0)?(no*-1):no;
} 

static RingBuff_t buffer;

/** Send a string to the ring buffer
 * \param str : String to send to the ring buffer
 * \result 0 if the string was accepted. A pointer to the last char sent. 
 * A non zero result should be used to resend the 
 */
char* putringstring(char* str)
{
	char *c;
	for (c=&(str[0]);*c;c++)
	{
		Buffer_StoreElement(&buffer, *c);
		if (1<0) // TODO: Check full!
		{
			return c;
		}
	}
	return 0;
} // putringstring

void InitDataBroadcast(void)
{
	Buffer_Initialize(&buffer);
}

//--------------------------------------------------------
/** Send databroadcast as packet 8/31 
 * This routine takes data from the ring buffer and places it into a packet.
 * \param pkt : pointer to a char buffer that must be at least 45 characters long
 * \return 1 if there is nothing to send, or 0 if there is a valid packet.
 */
int SendDataBroadcast(char* pkt)
{
	int i;
	int packet;

	static unsigned char continuity = 0;

	// Do we have any data to send?
	if (Buffer_IsEmpty(&buffer))
		return 1;

	
	// Assemble the basic packet 8/31
	// Note that the comment numbers are one higher than the array index. 
	char* p=pkt;
	*(p++)=0x55;						// 1: clock run in
	*(p++)=0x55;                        // 2: clock run in
	*(p++)=27;							// 3: framing code
	*(p++)=HamTab[11];					// 4: mrag for 8/31. data channel. designation code.
	*(p++)=HamTab[0x0f];				// 5:
	*(p++)=HamTab[6];					// 6: Format type set to format 1 with explicit continuity and repeats
	*(p++)=HamTab[1];					// 7: address length of 1 (IAL)
	*(p++)=HamTab[9];					// 8: address 9. (For SISCom)
	*(p++)=0x80;						// 9: first repeat
	*(p++)=0;							// 10: continuity indicator
	for (i=9;i<43;i++)					// 11-43: 33 payload bytes
		*(p++)=' ';
	*(p++)=0xfe;						// 44: Hi checksum
	*(p++)=0xff;						// 45: Lo checksum

	// initialise the checksum
	ClearCRC();

	// add first set of data into the packet
	pkt[8]  = 0x80;
	pkt[9]  = continuity;		// Ummm, what about repeats? Worry about that later 

	// Copy as much of the payload as we can
	
	// Calculate the CRCs 
	for (i=9;i<43;i++)
	{
		pkt[i]=Buffer_GetElement(&buffer); 	// What about partial lines?
		AddCRC(pkt[i]);
	}

	// add it to the end
	EndPacket(&pkt[43],&pkt[44]);

	// TODO: Repeats
	
	return 0;
}//SendDataBroadcast
