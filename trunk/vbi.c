/*****************************************************************************
 * Description       : VBI timing control for VBIT/XMEGA
 * Compiler          : GCC
 *
 * This module handles vbi timing in this sequence:
 *  1) An interrupt on both edges detects a change in the field signal. 
 *  2) This starts a 1ms timer which is the period where the vbi is transmitted.
 *  3) When the timer completes it sets a flag indicating that the fifo is available.
 *  4) Another timer is then started for 18ms. When this completes it indicates
 *     that the FillFIFO routine must stop and ready the FIFO to transmit.
 *
 * Copyright (c) 2010 Peter Kwan
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following
 * conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.

 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 * The name(s) of the above copyright holders shall not be used in
 * advertising or otherwise to promote the sale, use or other
 * dealings in this Software without prior written authorization.
 *
 *****************************************************************************/
#include "vbi.h"

/* hic sunt globals */
volatile uint8_t vbiDone; // Set when the timer reckons that the vbi is over. Cleared by main.
volatile uint32_t UTC=36000; // 10:00am
volatile uint8_t FIFOBusy;	// When set, the FillFIFO process is required to release the FIFO.
volatile uint8_t fifoReadIndex; /// maintains the tx block index 0..43
volatile uint8_t fifoWriteIndex; /// maintains the load index 0..43

 /* Instantiate pointer to fieldPort. */
static PORT_t *fieldPort = &PORTC;
static TC0_t *timerVBIControl = &TCD0;
static TC1_t *timerFIFOBusyControl = &TCE1;
/*! Field interrupt
 * This maintains the real time clock. Sadly, if the video stops, the clock goes wrong
 * It also starts two timers that coordinate when the FIFO may be written to,.
 */
void FieldInterruptHandler(void)
{
	const uint32_t day=(uint32_t)60*60*24;
	LED_On( LED_3 ); // Got a field interrupt (video OK)
	static int count=0;
	count++;
	/*if ((count%50)==0)
		xputs(PSTR("."));
	*/
	// Maintain the UTC.
	if ((count%50)==0)
	{
		UTC++;
		if (UTC>=day)
			UTC=0;
		// SISCom databroadcast fader
		// This is only one command sent once a second to exercise the receiver
		// We need a command to change the string
		putringstring("\016fade,0,1\n");
	}
	
	// What is the state of PINC.2

	// Start the vbi timer
	// We want a preset 15625 samples at fosc/1 for 1024us
	// Because 1ms * 16MHz = 15625
	/* Set period/TOP value. */

	timerVBIControl->PER=15625+1024; // Add a bit on just to make sure that we clear the vbi
	/* Select clock source. */
	timerVBIControl->CTRLA = ( timerVBIControl->CTRLA & ~TC0_CLKSEL_gm ) | TC_CLKSEL_DIV1_gc;	
	/* Set a low level overflow interrupt.*/
	timerVBIControl->INTCTRLA|=TC_OVFINTLVL_LO_gc;	

	// Also start the Window-of-access timer, the time while the FIFO may be written to
	// This is about 18ms. TODO: Measure this on a scope and get it more accurate. We can squeeze better performance.
	// The preset at fosc/64. for 18ms is (16000000 * 0.018s)/64 = 4500.
	timerFIFOBusyControl->PER=8500; // Ummm. No idea why x2. The scope shows it to be correct: !9000 worked!
	/* Select clock source. */
	timerFIFOBusyControl->CTRLA = ( timerFIFOBusyControl->CTRLA & ~TC1_CLKSEL_gm ) | TC_CLKSEL_DIV64_gc;	
	/* Set a low level overflow interrupt.*/
	timerFIFOBusyControl->INTCTRLA|=TC_OVFINTLVL_LO_gc;

	PMIC.CTRL |= PMIC_LOLVLEN_bm;		
} // FieldInterruptHandler

// TCC0, TCC1, TCD1 are all in use
/** VBI Timer done. Signal to the main code (FillFIFO) that it is clear to load more vbi
 */
ISR(TCD0_OVF_vect)
{
	// At this point we want to kill the clock so as not to let it bother us
	timerVBIControl->CTRLA = ( timerVBIControl->CTRLA & ~TC0_CLKSEL_gm ) | TC_CLKSEL_OFF_gc;
	LED_Off( LED_4 ); // 
	if (vbiDone)
	{
		LED_On( LED_4 );
		// xputs(PSTR("ERR: vbi overrun\n")); // Nice to have a message but we don't have enough time
		// work out why we never made it
	}
	vbiDone=1;
	LED_Off( LED_3 );
	LED_Off( LED_6 ); // Start the Window of access here.
	FIFOBusy=0;	
} // ISR: vbi done
 
/*! Set up the FLD interrupt */
void InitVBI(void)
{

	LEDs_Init();
	/* Configure Interrupt0 to have medium interrupt level, triggered by pin 2. */
	fieldPort->INTCTRL = ( fieldPort->INTCTRL & ~PORT_INT0LVL_gm ) | PORT_INT0LVL_MED_gc;
	// pin mask, pin 2 only
	fieldPort->INT0MASK = VBIT_FLD;

	/* Enable medium level interrupts in the PMIC. */
	PMIC.CTRL |= PMIC_MEDLVLEN_bm;	

	/* Build pin control register value. */
	uint8_t temp = PORT_OPC_TOTEM_gc | PORT_ISC_BOTHEDGES_gc;

	/* Configure the pins in one atomic operation. */

	/* Save status register. */
	uint8_t sreg = SREG;
	cli();
	PORTCFG.MPCMASK = VBIT_FLD; // Only pin 2
	fieldPort->PIN0CTRL = temp;

	/* Restore status register. */
	SREG = sreg;
	sei();
} //  InitVBI

 
 /*! PINC.2 FLD Interrupt vector. 
  * Interrupt on changing between the odd and even fields.
  */
ISR(PORTC_INT0_vect)
{
	FieldInterruptHandler();
}
 /// ISR for vbi timer
 
/*! Timer Interrupt vector. FIFOBusy Timer
 * The timer is started when the vbi ends
 * And stops after 18ms, shortly before the vbi resumes
 * This gives enough time for fillFIFO to terminate and release the FIFO. 
 * The timer is reloaded with 1ms and when this terminates, it also sets the FIFO to tx 
 * C0 is common bridge, * C1 is LEDs, * D1 is used by the SD card, * E0 is the audio demo
 * 
 * The read pointer is incremented on each field EXCEPT:
 * 1) When the odd/even phase is wrong
 * 2) When there is no data ready
 */
ISR(TCE1_OVF_vect)
{
	uint16_t fifoReadAddress=fifoReadIndex*FIFOBLOCKSIZE;
	uint8_t nextBlock;
	uint8_t field=PORTC.IN&VBIT_FLD?0:1;	// High on the even field
	if (FIFOBusy) // Second time we need to set the FIFO to tx
	{
		// kill the clock so as not to let it bother us
		// CTRLA means Control register A, NOT Port A
		timerFIFOBusyControl->CTRLA = ( timerFIFOBusyControl->CTRLA & ~TC1_CLKSEL_gm ) | TC_CLKSEL_OFF_gc;
		// Reset the FIFO ready to clock out TTX
		fifoReadAddress=(fifoReadIndex*FIFOBLOCKSIZE); // move the buffer pointer to the next field's worth
		SetSerialRamAddress(SPIRAM_READ, fifoReadAddress); // Set the FIFO to read from the current address
		PORTC.OUT|=VBIT_SEL; // Set the mux to DENC.
		// 1) Are we wrapping round to 0?
		nextBlock=(fifoReadIndex+1)%MAXFIFOINDEX;
		// 2) Is the odd/even phase correct?
		if (nextBlock%2 != field) // Not correct? Wait for the next field. TODO: Check that the phase is correct! 
		{
			xputc('y');		// phase wrong
			return;
		}
		// 3) Does the FIFO have a packet ready?
		if (nextBlock==fifoWriteIndex)
		{
			xputc('Y'); // no data available
			return;
		}
		fifoReadIndex=nextBlock;	
	}
	else
	{
		// Prime the clock for 1ms
		timerFIFOBusyControl->PER=500; // 1ms
		/* Select clock source. */
		timerFIFOBusyControl->CTRLA = ( timerFIFOBusyControl->CTRLA & ~TC1_CLKSEL_gm ) | TC_CLKSEL_DIV64_gc;	
		/* Set a low level overflow interrupt.*/
		timerFIFOBusyControl->INTCTRLA|=TC_OVFINTLVL_LO_gc;
	}
	LED_On( LED_6 ); // Set FIFOBusy
	FIFOBusy=1;
} // ISR: FIFOBusy
