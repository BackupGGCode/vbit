# 05 June 2010 #
Found a mistake in packet.c that could cause VBIT to lock up.
Added databroadcast. Currently this sends short text messages that a special decoder (SISCom) can pick up. It needs extending to generalise it for any service packet address, repeats, continuity flags, and USB commands to enter external data etc.

## 21 April 2010 ##
Finally found the crazy animated page fault. There was a page structure that should have been static but wasn't. Now I can concentrate on implementing the missing features such as:
  * more commands including page updating
  * carousels
  * packet 8/30/1
  * proper parallel mode magazine output.
## 20 April 2010 ##
Found one of the big bugs that was messing up pages. There is one more big bug left that is either dropping headers or sending them out with magazine 1 instead of their proper number. Page 199 has a crazy animated display.
## 13 April 2010 ##
The bad news is that the JM-XD200-USB board is now sold out and no more will be made. The good news is that the JM-XD300-USB board will be out towards the end of the month and will be mostly compatible with the XD200. Even better news is that it will be cheaper and the USB interface will have a more powerful chip. Hopefully the difference in software will only be a single line to change in the Makefile.
## 09 April 2010 ##
Some mistakes in the FIFO loading code are now fixed and it is producing a reasonably convincing service of 300 pages. But it sometimes takes two passes to catch all the lines so there is still some buffering fault. Also the fastext isn't working yet.
## 06 April 2010 ##
Source code is up but just so that I can take advantage of proper version control. It builds on the JM-XD200 demo code which you can download from http://xmega.mattair.net/code.html
## 04 April 2010 ##
A little hospital break means that the development is halted. The only prototype is locked up for Easter. The results were very poor and there is clearly some problem in the order that the lines are being transmitted. Maybe an evening with the TXA1 would sort it out.
## 28 March 2010 ##
f\_lseek() is indeed very much faster. f\_open() would take 5 to 30ms to find a file and get it ready for reading. f\_lseek() on one big file takes about 50us. The FIFO is always full so now the elaborate hold off and resync scheme is never triggered. I just need to take it to work to see the results on an actual TV because don't actually possess a TV.
## 27 March 2010 ##
It turns out that f\_open() is taking 30ms to execute when there are 300 pages in the service. This is obviously way too slow. The answer at the moment is to hold off the inserting while the filing system is busy and then re-sync later. You miss an whole number of frames so as not to swap odd and even fields. This unfortunately makes a big service even slower. The other idea is to copy all of the pages into one big file and use f\_seek() to go to pre-indexed pages. This should be much faster than f\_open and you only get to use one file pointer.
## 24 March 2010 ##
The SD card interface uses FatFS from ChaN. While it is well written and fits nicely into the memory available, f\_open() is very slow. This is probably inevitable given the limited memory. VBIT may be required to open one or two pages per field. This is OK for the first 20 pages. By the time I got to insert 300 pages then the file opening time overruns the time slot available. Extending the FIFO from a field buffer of 720 bytes into a 262080 byte circular buffer should give the FIFO enough leeway.

When this bit of code is done then I'll measure the improvement. Ideally VBIT should be able to handle several thousand pages. Then I'll run through the TODO list and the first software release.