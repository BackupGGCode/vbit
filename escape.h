#ifndef ESCAPE_H
#define ESCAPE_H

// Filename: escape.h
// Created: 24 April 2004
// Author: Peter Kwan
// 
// Viewdata and MRG packetising and checksums.


int Escape(char *szPacket,char *szSource,int packetLength);
int DeEscape(char *szPacket,char *szSource,int packetLength);
int PacketiseMRG(char *szPacket,char *szSource);

#endif
