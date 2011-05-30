// Include for PDC output manager
#ifndef _DATABROADCAST_H_
#define _DATABROADCAST_H_

// C libraries
#include <stdio.h>
#include <string.h>

// databroadcast libraries
#include "xitoa.h"
#include "asciidef.h"
#include "escape.h"
#include "tables.h"
#include "crca.h" // IDL Format A checksums

// A bit of LUFA
#include "Lib/RingBuff.h"

// forward declarations
int copypacket(unsigned char *cmd, unsigned char *pkt);        // copy the packet into an output buffer using escapes and return checksum


int MakeOffset(int off);

/** Set up the databroadcast buffer
 */
void InitDataBroadcast(void);

/** Send data from the ring buffer if there is any
 */
int SendDataBroadcast(char* pkt);

char* putringstring(char* str);

#endif
