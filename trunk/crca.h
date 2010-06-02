// Class for calculating IDL format A CRCs
// See also crcb.h
#ifndef CRCA_H
#define CRCA_H
  unsigned char m_HiByte;
  unsigned char m_LoByte;
  
  void ClearCRC(void);
  unsigned char GetHi(void);
  unsigned char GetLo(void);
  void AddCRC(unsigned char c);  
  void EndPacket(unsigned char *Hi,unsigned char *Lo);

#endif
