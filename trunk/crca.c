// CRC class for doing IDL format A

#include "tables.h" // change to table.h
#include "crca.h"

// Returns the accumulated Hi byte of the checksum
unsigned char GetHi()
{
  return m_HiByte;
}//GetHi

// Returns the accumulated Lo byte of the checksum
unsigned char GetLo()
{
  return m_LoByte;
}//GetLo

// End of a row. Hi is checksum S0, Lo is checksum S1 (or vice versa)
void EndPacket(unsigned char *Hi,unsigned char *Lo)
{
  *Hi=GetHi();
  *Lo=GetLo();
}//EndPacket

// Add a character to the checksum
void Add(unsigned char c)
{
  c^=m_HiByte;
  m_HiByte=m_LoByte^pgm_read_byte(TH+c); // hi byte from progmem
  m_LoByte=pgm_read_byte(TL+c); // lo byte from progmem
}//Add


