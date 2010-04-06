/** ***************************************************************************
 * Description       : VBIT: Packet 8/30 format 1
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
/*
What is Packet 8/30 format 1?
This packet is transmitted once a second
It contains a network identity code, a status word
an identity string
and time.
The time must be recalculated every time that this is called.
*/

#ifndef _P830F1_H_
#define _P830F1_H_ 

extern const char inifile[];
#define MAXSTR 80
#include "tables.h"
#include "xitoa.h"
#include "../minini/minIni.h"
extern unsigned char pkt830[46];		/* The packet.40 chars + 2 CRI + WST+MRAG+terminator */
	
void Init830F1(void);
void SetNIC1(unsigned char *pkt,char *nic);
void SetStatusLabel(unsigned char *pkt, char *str);
/*
C: Code
L: Link
N: Network
T: Time
D: Display text
*/

#endif