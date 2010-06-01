// Filename: escape.c
// Created: 24 April 2004
// Author: Peter Kwan
// 

#include "stdio.h"
#include "string.h"
#include "asciidef.h"
//#include "paritytab.h"
#include "escape.h"

/*
 * $Log: escape.c,v $
 * Revision 1.5  2004/09/02 16:45:09  bryan
 * Nightly commit
 *
 */

// DeEscape:
// Takes packetLength characters from szSource
// and removes viewdata escapes into szPacket.
// Returns the resulting length of the string
int DeEscape(char *szPacket,char *szSource,int packetLength)
{
  int i,j;
  unsigned char ch;
  for (i=0, j=0; j<packetLength; j++, i++)
  {
    ch=(unsigned char)szSource[j];
    if (ch==(unsigned char)ESC)
      szPacket[i]=((unsigned char)szSource[++j])-0x40;
    else
      szPacket[i]=ch; // not escaped
  }
  szPacket[i]=0;
  return i; // the resulting string length
} // DeEscape

// Escape:
// Takes packetLength characters from szSource
// and does viewdata escapes into szPacket.
// Returns the resulting length of the string
int Escape(char *szPacket,char *szSource,int packetLength)
{
  unsigned char ch;
  int i,j;

  // Escape ESC, CR and SI, the others we don't fuss with
  for (i=0, j=0; i<packetLength; j++)
  {
    ch=(unsigned char)szSource[j];
    if ((ch==(unsigned char)ESC) ||
        (ch=='\r') ||
        (ch<' ') || // HACKY HACK. All characters under space get escaped
        (ch==(unsigned char)SI) ||
        (ch==(unsigned char)SO))
    {
      //printf("Escape!!!: ch=%02x\n",ch);
      szPacket[i++]=(unsigned char)ESC;
      szPacket[i++]=(unsigned char)ch+0x40;
    }
    else
      szPacket[i++]=ch; // not escaped
  }
  szPacket[i]=0;
  return i; // the resulting string length
} // Escape

// SendMRG adds the outer packet to a string
// Consisting of Shift in, address, contents, checksum and terminator.
// Returns the length of the resulting string
int PacketiseMRG(char *szPacket,char *szSource) {
  char szBuf[100];
  unsigned char ch;
  unsigned uChecksum=0;
  unsigned i;
  unsigned j;

  
  sprintf(szBuf,"%c%c0",SO,SO); // Twice for luck
  uChecksum=szBuf[0] + szBuf[1] + szBuf[2];
  
  //check the string for double heights + generate checksum
  for (i=3, j=0; j<strlen(szSource); j++) { // WARNING! strlen should not be used
    ch=(unsigned char)szSource[j];
    
    szBuf[i++]=ch;// add the character to the string
    uChecksum+=ch;// add into checksum
  }
  
  // tack  on checksums
  char msg[10];
  sprintf(msg,"%02x",uChecksum & 0xff);

  szBuf[i++]=msg[0];
  szBuf[i++]=msg[1];

  // end the string
  szBuf[i++]='\r';
  szBuf[i++]='\0';
  strcpy(szPacket,szBuf);
  return i;
} // PacketiseMRG



