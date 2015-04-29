# Introduction #
Pages are in MRG Systems tti format. If I can get permission then I will post the specification. You can probably work most of it out just by looking at pages.all.


# Details #

Unfortunately the SD card software is very restricted. In order to fit into the AVR it is restricted to MSDOS style 8.3 filenames, no unicode and the file open routine gets slower the more files that there are. Also file objects use a lot of ram so we can only afford to have two open when we might like to use 16 or more.

So bearing all that in mind this is the current solution:
The individual pages are all bundled up into a single big file called pages.all. This is all of the pages concatenated into one big file. There is also an index file which has pointers to the locations inside pages.all.

To make pages.all use a PC to make an onair folder in the root of the SD card. Then put all of your tti files in there. Then back on on the XMEGA with the card installed send the C command. This is with the usual command method which you would type into the console as


&lt;CTRL-N&gt;

0C

&lt;Enter&gt;


This process takes a few minutes on a big service but when it finishes you may restart the inserter and your service will be transmitted. It also generates an index file called mag1.lst. The idea is that in future each magazine and carousel will have a lst file. At the moment all of the pages are indexed in mag1.lst. One tip is to disconnect the video in while you are preparing a card. This prevents field interrupts from slowing down the AVR.

Obviously this makes live updates a little tricky. I'm thinking that if we allocate each page a fixed amount of memory, then we could write modified pages without having to remake pages.all each time. Or we could use the SPIRAM on the XMEGA board to store dynamic pages so we wouldn't need to mess with the file system.

Another solution which has been done in the past is to use a PC to generate the text data and just play it immediately. So you would send packets down the USB. These would get buffered in the FIFO. There would be no other local storage. The downside of this is that you would need a PC working all the time to deliver the service. The upside is that the inserter firmware would be very simple and the service very flexible. It is easier to get a PC to generate the service.

You are free to write a better solution and are most welcome to post it back here.
