A Teletext inserter that is based on the VBIT hardware. Computing power is provided by the Mattair MT-X1 board which has an Atmel AVR XMega and another AVR for USB connectivity. Everything is written in C in Notepad++. Once loaded with pages it can run stand-alone, except for the fact that it has no real time clock so the time will be wrong without a host PC to set it.

If you want an easy plug and play solution then this project isn't ready for you. It is still experimental and I'll say right away that you need a mix of hardware and software skills. If you have a good project for a VBIT then I have a very small stock of completed units. If you have an application and can order 10 or more units then I can develop VBIT to a commercial standard.

One application for VBIT is for private TV networks such as hotels. It can generate Packet 8/30 signals for setting TVs that have clocks built in or add an ID string to a channel. It can generate a service of thousands of pages so it can be used to carry general information for guests.

The hardware is an analogue video inserter, powered by USB with pages stored on an SD card. It has an 8 bit video path instead of the usual 10 and it still looks very good but of course this isn't intended for broadcast use. The teletext signal itself is excellent, thanks to those clever chaps at Philips/NXP/Trident/Sigma Solutions. You could always feed it into a databridge if you want broadcast standard video. The Kicad project files and source files are all up so you can build your own hardware. I made a batch of boards at Beta Layout and as it turns out Kicad exports make good PCBs. However, all the boards are now gone so you'll have to make your own. This is what it looks like at the moment.

http://lh5.ggpht.com/_p2yA4mmkq0o/S5A4wYAvATI/AAAAAAAAIOg/jNyK4X9Yp2M/s400/IMG_0365.JPG

![http://lh6.ggpht.com/_p2yA4mmkq0o/TCDWYU4MpYI/AAAAAAAAIuw/LAuCuDvZPyU/s800/twitfax.jpg](http://lh6.ggpht.com/_p2yA4mmkq0o/TCDWYU4MpYI/AAAAAAAAIuw/LAuCuDvZPyU/s800/twitfax.jpg) The source is on SVN and is not going to be stable as there is no official release yet. Go to Source->Browse if you just want to look at the source.

This project is aimed at software developers and you'll need to be familiar with the ETSI documents. To insert your own packets, just write a routine that fills a transmission packet, and hook this into packet.c. Once your packet is filled then the Parity() routine will add the parity and reverse the bit order of all the bytes into transmission order. Packet types already implemented are full magazines, filler, quiet, databroadcast and opt-out encoding.
# Latest News #

01 August 2013

Raspberry Pi hosted inserter VBIT-Pi is up and running now. Code is at https://code.google.com/p/vbit-pi/ and general website at http://teastop.co.uk/vbit-pi/.

27 June 2013

VBIT is now running with a Raspberry Pi host. You can use an existing VBIT with an adapter board but soon there will be a new layout that connects to Pi direct. The software has a different structure because it can use threads and now has 512MB instead of 8kB of memory! There is no interface at all yet, it just runs whatever pages you have put on the SD card. Future plans include a command port and conf file and maybe also add a web interface but this is a long way off.

Twitter is no longer possible on VBIT. Twitter dropped the old RSS feed and lots of applications stopped working. VBIT Twitfax was one of them. It is just too much bother to extract data from social media for this sort of application.

15 April 2013

@possan has made a web service for vbit with some ingenious javascript and php. It is possible to control VBIT by http commands. There is also code to upload complete teletext pages. https://github.com/possan/node-teletext

14 February 2013

New feature for VBIT, fast updating from the host. Put up pages, newsflashes and subtitles  really fast. The idea is to make things simple for people doing live shows rather than running a normal teletext service. The serial RAM has been restructured to make the FIFO smaller. 256kbits of FIFO was complete overkill. So now it is split into 14 page of teletext with the rest allocated to the FIFO. A special page command RD in a page redirects the teletext stream to one of these RAM pages. VBIT has a new commands JA and JW which lets the host write directly to these RAM pages.

To use the fast updating, put in a different SD card that has a special set of pages. Each page has a redirect to RAM and there are very few pages so that the cycle time is very short.

For fast updating you need a program that sends VBIT lots of JA and JW commands. I made a  Windows program called "LiveText". This can put pages up and run a carousel. It can also do newsflash boxes and run subtitles.

This is completely compatible with the previous system. To revert to a full service just put the original SD card back in.

21 October 2012

I note that Mr Mattair has brought out a new version of the dev board. I expect that it will still work with VBIT. The main difference seems to be the loss of buttons and the gain of a serial port. A serial port could make VBIT a much more traditional sort of inserter. It might also make it easier to control as USB has a habit of going off line if there is a watchdog crash. It would also be possible to run NEWFOR subtitles, given a software update.
In other news, I'll release Minited and see if it is any use to any one.
5 August 2012

Working on getting a complete teletext system. It will consist of a text editor written in VB6. If you are a teletext enthusiast who knows VB6 then call me to get involved. A scheduler (written in C++Builder) that interfaces the editor to a VBIT or any one of a  dozen other inserters made by MRG or Softel. And VBIT (in WinAVR gcc) is getting a major overhaul to enable dynamic updating.

9 July 2012

Updating pages has been a bit tedious because you need to take out the SD card, copy the  the new pages from a PC, put the card back in and let VBIT rebuild all the display list. Also it can't support carousels. It turns out that the CPU board has enough serial ram to run a display list of over 5,000 pages. It is a bit complicated so it will take a while to implement but it will give full control over a live service. It should be able to add and remove pages on the fly and also support carousels. I'll also experiment with parallel transmission because at the moment there are quite a few wasted VBI lines.

20 June 2012

Now I'm in the process of constructing the last few VBITs. When these are done I'll be thinking about the next generation VBIT. How about a ready made unit in a metal box that you can plug and play? The least that it will add is a few MB of RAM and use one of the newer USB AVR XMegas. Apart from that what hardware would you want? Touch panel interface? Bypass relay? Onboard SPG? Text bridging? Real-time clock?

It turns out that people don't like making VBIT. You really want a VBIT but don't have the hardware building skills? Drop me an email saying how many units you could use. If you have any special requirements then I can build it into the next design. If enough people commit to purchasing about 50 units then I can fire up a production line.


11/11/11

I am starting to port the code to the new Mattair board. This should not be difficult. The new Mattair board has an RTC crystal. I wonder if there is a way of adding a battery in order to maintain this clock. It would be ever so useful.
Also thinking about the next version. Should it still be connected to the AVR dev board or be integrated into a single board? It should have something like 16MB of RAM so that live updating of pages is possible. It would be nice to upgrade the digital path to 10 bits but this would mean chips with lots more pins and are harder to solder.

Also the phono sockets from Farnell are no longer manufactured. Sorry, but you'll have to improvise and hack a socket to fit or just add flying leads to some BNCs. The Farnell 1280702 might fit with a bit of cutting and bending. In fact I might just put the BNCs off board in any case so it is easier to fit in a random case.

01 July 2011

The new Mattair MT-X1 board arrived and it is a little different. This time it has no headers attached so instead of cutting off unwanted pins I'll be connecting just the pins that I want. The configuring of this board is in the wiki.

16 May 2011

Now looking at whether the rare SAA7113 can be replaced with a TVP5146M2. Apart from having more pins, and different voltage, and using twice the power, and being more difficult for the amateur to attach it should be possible. But the good news is, the SAA7113 seems to be readily available on Ebay as long as you want it shipped from Hong Kong. The SAA7120 seems harder to get but fortunately it is possible to use the SAA7121 instead without needing any changes. This is available from Hong Kong.

Subtitles were the mainstay of teletext. Subtitles need to bypass the normal page scheduling system. Subtitles need to be on a magazine all on their own with no other pages to get in the way. Traditionally this is magazine 8. They also need to take priority over any other packet. I will implement this by stealing two fields of data from the text FIFO for a subtitle buffer. A new command will allow you to send a complete page to this buffer and then flip a ready bit to tell the inserter to transmit this buffer. In this way a subtitle page can be full frame if required. If this is used for animation we may need to double-buffer so there will be a total of four fields stolen from the FIFO.

# Older News #

VBIT now generates some Softel codes so you can test Opt-out automation without a Softel SE 3067 or MRG ATP612. With a little more work you could add contact closures to make it into a replacement for these devices.

VBIT now does databroadcast just like a single channel MRG ATP650. So now you can embed any data that you like. At the moment it sends out a SISCom control message every second but you can probably do something more entertaining. As usual there is stuff missing. The transmission parameters are burned into the firmware, and repeats are not implemented.