// Class for calculating IDL format A CRCs
// See also crcb.h
#ifndef CRCA_H
#define CRCA_H
  unsigned char m_HiByte;
  unsigned char m_LoByte;
  
  void Clear(void){m_HiByte=0;m_LoByte=0;};
  unsigned char GetHi(void);
  unsigned char GetLo(void);
  void Add(unsigned char c);  
  void EndPacket(unsigned char *Hi,unsigned char *Lo);

#endif
