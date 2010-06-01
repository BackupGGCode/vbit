// Include for PDC output manager
#ifndef _DATABROADCAST_H_
#define _DATABROADCAST_H_

// C libraries
#include <stdio.h>
#include <string.h>

// PDC libraries
#include "databroadcast.h"

#include "asciidef.h"
#include "escape.h"
#include "tables.h"
#include "crca.h" // IDL Format A checksums

#include "Lib/RingBuff.h"

// forward declarations
void SetStatusLabel(unsigned char *pkt, char *str);   // set the status label
int copypacket(unsigned char *cmd, unsigned char *pkt);        // copy the packet into an output buffer using escapes and return checksum

void SetInitialPage(unsigned char *pkt,const char *mppss,const char *subcode);
int transact(char *buf, char *resp);
void SetNIC1(unsigned char *pkt,char *nic); // format 1
int SendPresfax(void);
int SendNewPresfax(void);
void SendTime(void); // Send the time to the inserter

int  TagToEvent(char* tag);
int  SetRTI(int nLabel);
int  SetINT(int nLabel);
int  SetTimer(int nLabel);
int  ValidPILTime(char *piltime);
void CheckStatusLabel(char *label);

void MakeMRGPacket(unsigned char *dest,unsigned char *src);
int MakeOffset(int off);

/** Set up the databroadcast buffer
 */
void InitDataBroadcast(void);


#endif
