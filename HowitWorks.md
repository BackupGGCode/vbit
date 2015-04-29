# VBIT #

The aims of vbit is to make an inserter with the minimum of components, that does not need any calibration and has an open source or at least a free tool chain.


## Processor Interface ##

The processor and VBIT circuit are connected with these signals:
  * I2C - Used to configure the video decoder and encoder.
  * SPI - Used to control the serial RAM
  * ODDFLD - A bit from the decoder that indicates which is the current field
  * AVRSEL - A bit from the processor that switches the multiplex between AVR clock and the encoder teletext clock.
## Video Path ##
The video path is as follows. The PAL video input is digitised and turned into parallel CCIR601. The parallel video is immediately encoded back into PAL and output. The encoder also has a port for teletext consisting of a clock out and a data input. When text is required, the clock sends out strobes to the FIFO which returns each bit to be encoded. The advantage of doing it this way means that there are none of the usual analog problems of black level clamp etc. There are no adjustments required.
## Inserter Operation ##
Now a FIFO is usually an expensive item but in this case a standard serial ram is used.  The spi ram has enough capacity for 40 fields of text. Conveniently, the serial ram can configured clock out all the bits sequentially. But unlike a FIFO it can not read and write at the same time. Fortunately we do not need to read and write at the same time. This is because out of each field period of 20ms, only 1ms is required for vbi leaving 19ms for the processor to fill the FIFO. As it turns out this is ample and we save a lot of cost and complexity.

But because it isn't a proper FIFO we need to multiplex it between the processor and the text encoder. This is where the fourth chip comes in. Normally this would be some big FPGA controlling all the signal lines and producing all sorts of clocks but in this case it is a humble quad nand gate. It is wired as a multiplexer that takes a clock from either the processor or the encoder. This is the sequence of how everything works:
  * The field bit changes state. This starts a timer that waits until the end of the vbi.
  * The vbi ends and the timer is set to wait until a short while before the next vbi starts.
  * Meanwhile the multiplexer is switched to the processor. The processor then fills up the FIFO with teletext packets. This continues until either the FIFO is full or the next vbi is about to start.
  * The serial ram address is then set up for the next field and it is switched into read mode. Then the multiplexer is switched to the encoder.
  * The encoder clocks out up to 18 teletext packets from the serial ram in the current vbi.
  * While the vbi is being transmitted the field bit changes state and the cycle starts again.

So as you can see, what ever the processor manages to put in the FIFO is what gets transmitted so a full text service of packet 8/30, databroadcast, opt-out signalling etc. is possible. You just have to fill the FIFO with what you want.