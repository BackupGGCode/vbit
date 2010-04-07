/** ***************************************************************************
 * Description       : VBIT: Serial SRAM FIFO
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
 * The author disclaim all warranties with regard to this
 * software, including all implied warranties of merchantability
 * and fitness.  In no event shall the author be liable for any
 * special, indirect or consequential damages or any damages
 * whatsoever resulting from loss of use, data or profits, whether
 * in an action of contract, negligence or other tortious action,
 * arising out of or in connection with the use or performance of
 * this software.
 *************************************************************************** **/
#ifndef _FIFO_H_
#define _FIFO_H_

#include "../Drivers/SPI/spi_driver.h"
#include "xitoa.h"
 
/* SPI SRAM Instruction set */
#define SPIRAM_READ		(0x03)
#define SPIRAM_WRITE	(0x02)
#define SPIRAM_RDSR		(0x05)
#define SPIRAM_WRSR		(0x01)

/* These are status byte values, ignoring HOLD which we do not implement.
The mode VBIT uses is sequential */
#define SPIRAM_MODE_BYTE		(0x00)
#define SPIRAM_MODE_PAGE		(0x80)
#define SPIRAM_MODE_SEQUENTIAL	(0x40)
	
/* Sets up the ports and settings required */
void spiram_initialise(void);

/* Set the status register */
void SetSerialRamStatus(unsigned char status);

/** Set the start address for reading or writing
* \param RWMode : One of the SPIRAM instructions
* \param Address : 0..32k
* After a call to this you can write or read 
* To finish, DeselectSerialRam
*/
void SetSerialRamAddress(unsigned char RWMode, uint16_t address);
/** Read the status word
 * The status word has mode bits in 6,7 and hold is bit 0
 * \return Status word
 */
int GetSerialRamStatus(void);
/** Sends length bytes from buffer to the SPIRAM
 * You must first call SetSerialRamAddress with SPIRAM_WRITE
 * After the last transfer you should call DeselectSerialRam
 * \param buffer : Array to send
 * \param length : Number of bytes to send
 */
void WriteSerialRam(char *buffer, int length);
/** Reads 'length' bytes into 'buffer' from the SPIRAM
 * You must first call SetSerialRamAddress with SPIRAM_READ
 * \param buffer : Array to store data
 * \param length : Number of bytes to read
 */
void ReadSerialRam(char *buffer, int length);
/** Just pulls the slave select high to release it
 * After the last transfer you should call DeselectSerialRam
 */
 void DeselectSerialRam(void);

#endif