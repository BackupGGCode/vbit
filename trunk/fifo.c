/** ***************************************************************************
 * Description       : VBIT: FIFO Control
 * The FIFO uses SPI. The clock is multiplexed so that control can 
 * be shared between the CPU and the video encoder (DENC)
 *
 * Compiler          : GCC
 *
 * Copyright (C) 2010, Peter Kwan
 * based on Atmel:spi_polled_demo.c
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
#include "fifo.h"
 
/*
see avr/include/avr/iox128a1.h for register names
*/

/* Globals */
/* Set up SPI on port E */
/* Instantiate pointer to ssPort. */
static PORT_t *ssPort = &PORTC;

/*! \brief SPI master module on PORT (E=demo board) C=VBIT. */
SPI_Master_t spiMaster;

void spiram_initialise(void)
{
	/* Init SS pin as output with wired AND and pull-up. */
	PORTC.DIRSET = PIN4_bm;
	PORTC.PIN4CTRL = PORT_OPC_TOTEM_gc; // Only one master, so no pull-up nonsense

	/* Set SS output to high. (No slave addressed). */
	PORTC.OUTSET = PIN4_bm;



	/* Initialize SPI master on port C(or E!). */
	SPI_MasterInit(&spiMaster,
	               &SPIC,
	               &PORTC,
	               false, // false=MSB First to match the spiram
	               SPI_MODE_0_gc, // 
	               SPI_INTLVL_OFF_gc, // Mode 0 - Sample on rising, change on falling
	               true,  // Clock doubler enabled
	               SPI_PRESCALER_DIV4_gc);		// 4 is the fastest? Could switch on clock doubler
}

void SetSerialRamStatus(unsigned char status)
{
	// TBA set the mux here too!
	// VBIT Mux needs setting as SCK is shared
	//GPIO_On(VBIT_SEL);	
	/* MASTER: Pull SS line low. This has to be done since
	 *         SPI_MasterTransceiveByte() does not control the SS line(s). */
	SPI_MasterSSHigh(ssPort, PIN4_bm); // Toggle the chip to reset any mode
	SPI_MasterSSLow(ssPort, PIN4_bm);
	SPI_MasterTransceiveByte(&spiMaster,SPIRAM_WRSR); // Write status register
	SPI_MasterTransceiveByte(&spiMaster,status); // Write status register
	SPI_MasterSSHigh(ssPort, PIN4_bm); // Release the RAM
}

int GetSerialRamStatus(void)
{
	int result;
	// TBA set the mux here too!
	/* MASTER: Pull SS line low. This has to be done since
	 *         SPI_MasterTransceiveByte() does not control the SS line(s). */
	SPI_MasterSSHigh(ssPort, PIN4_bm); // Toggle the chip to reset any mode
	SPI_MasterSSLow(ssPort, PIN4_bm);
	SPI_MasterTransceiveByte(&spiMaster,SPIRAM_RDSR); // Write status register
	result=SPI_MasterTransceiveByte(&spiMaster,'?'); // Write status register
	SPI_MasterSSHigh(ssPort, PIN4_bm); // Release the RAM
	return result;
}

void SetSerialRamAddress(unsigned char RWMode, uint16_t address)
{
	SPI_MasterSSHigh(ssPort, PIN4_bm); // Toggle the chip to reset any mode
	delay_us(1);
	SPI_MasterSSLow(ssPort, PIN4_bm);
	delay_us(1);
	SPI_MasterTransceiveByte(&spiMaster,RWMode); // Read or Write command
	// now the 15 address bits
	SPI_MasterTransceiveByte(&spiMaster,(address>>8)&0xff); // Write address high
	SPI_MasterTransceiveByte(&spiMaster,address&0xff); // Write address low
}

void WriteSerialRam(char *buffer, int length)
{
	unsigned int i;
	for(i=0;i<length;i++)
	{
		SPI_MasterTransceiveByte(&spiMaster,buffer[i]); // Send the bytes
	}
}

void ReadSerialRam(char *buffer, int length)
{
	unsigned int i;
	xputs(PSTR("ReadSerialRam\n\rRX:"));
	for(i=0;i<length;i++)
	{
		buffer[i]=SPI_MasterTransceiveByte(&spiMaster,'?'); // get the bytes
		xprintf(PSTR(" %02X"),buffer[i]);
	}
	xputs(PSTR("\n\r"));
	buffer[length]=0;
}

void DeselectSerialRam(void)
{
	SPI_MasterSSHigh(ssPort, PIN4_bm); // Release the last control command
}