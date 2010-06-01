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
			return *c;
		}
	}
	return 0;
} // putringstring

void InitDataBroadcast(void)
{
	Buffer_Initialize(&buffer);
}


//--------------------------------------------------------
/** Send Presfax data as packet 8/31 
 * new routine that sends each event as fixed length strings in three packets
 * with repeats
 * The last packet contains the schedule filename
 * \return value is not used
 */
int SendNewPresfax(void)
{
	int i;
	//CRC *HorizontalChecksum;
	char resp[100];
	int event;
	int packet;

	static unsigned char continuity = 0;

	// Assemble the basic packet 8/31
	unsigned char pkt[46] = 
	{
	    44,                              // 1: data length
		0,                               // 2: line number
		27,                              // 3: framing code
		HamTab[11],HamTab[0xf],          // 4,5: mrag for packet 8/31. data channel, desig. code
		HamTab[6],                       // 6 Format type set to format 1 
									   // with explicit continuity and repeats
		HamTab[1],                       // 7 address length of 1 (IAL)
		HamTab[nServicePacketAddress],   // 8 address of 2 (Single nibble SPA default=2)
		0x80,                            // 9 First repeat 
		0,                               // 10 Continuity indicator
		' ',' ',' ',' ',' ',' ',' ',' ', // 11-43 33 payload bytes
		' ',' ',' ',' ',' ',' ',' ',' ', 
		' ',' ',' ',' ',' ',' ',' ',' ', 
		' ',' ',' ',' ',' ',' ',' ',' ', 
		' ',
		0xfe,0xff,                     // 44,45 CRC Bytes
		0
	};                            // string terminator
	unsigned char cmd[80] = {0xe,'0','d',0x80}; // 0x80 Data Broadcast Packet 8/31

	// initialise the checksum
	// HorizontalChecksum=new CRCA();

	// add first set of data into the packet
	pkt[8]  = 0x80;
	pkt[9]  = continuity;

	// TODO: Copy the as much payload as we can
	uint8_t ch=Buffer_GetElement(&buffer); // Something like this
	
	// Calculate the CRCs 
	for (i = 9; i < 43; i++)
		Add(pkt[i]);

	// add it to the end
	EndPacket(&pkt[43],&pkt[44]);

	// copy the packet into the output command, escaping characters as we go and adding to the command checksum
	copypacket(cmd,pkt);

	// send it to the inserter
	transact((char*)cmd,resp);

	// TODO: Repeats
	
	return(0);
}//SendNewPresfax
