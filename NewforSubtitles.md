# Introduction #

Newfor is a Softel protocol commonly used for live subtitles. It is normally transmitted over serial links but is sometimes sent over ethernet. Inserters like the MRG DTP600 are still being used for subtitles.  Sorry about the formatting. This needs to be fixed.
# Details #

This is the protocol copied from a Softel document:

# Newfor Protocol #
## Message Type 1 – Set Subtitle Page ##
This message informs the interface which magazine and page the subtitling is to
be performed on. Subtitling is terminated by requesting magazine 9 page 99.
The message consists of 5 bytes:

SO (HEX OE), 0, Mag No., page (tens), Page units) 0

Hamming Encoded

## Message Type 2 – Set Subtitle Buffer ##
This message consists of up to 7 rows of subtitle information preceded by a message identifier.
The message identifier consists of 2 bytes:

SI (HEX 8F), 8 x Clear Bit + No. of subtitle rows

Hamming Encoded

The clear bit is set if the previous subtitle is to be cleared.

Each row of subtitle information consists of:

Top 4 bits of row No., bottom Top 4 bits of row No., 40 data bytes

Hamming Encoded
(Binary)
## Message Type 3 – Display Subtitle ##
This message informs the interface to display the subtitle currently buffered.

The message consists of a single byte.

DLE (HEX 10)
## Message Type 4 – Clear Subtitle ##
This message informs the interface to clear the subtitle currently on display.

The message consists of a single byte.

Conceal Display (HEX 98)
## Specification of Subtitle Interface Protocol ##
There are four types of input data:
<pre>
a)"Set Subtitle page" command<br>
b)"Set Subtitle Buffer"<br>
c)"Display Subtitles" command<br>
d)"Clear Subtitles" command<br>
</pre>
On receipt of a message (Type 2), the interface responds with either a NAK or ACK, signifying its rejection of acceptance of the data.
All non-hamming encoded bytes contain odd parity.
## Hamming Table ##
<pre>
Byte-Encoded<br>
0-15<br>
1-02<br>
2-49<br>
3-5E<br>
4-64<br>
5-73<br>
6-38<br>
7-2F<br>
8-D0<br>
9-C7<br>
A-8C<br>
B-9B<br>
C-A1<br>
D-B6<br>
E-FD<br>
F-EA<br>
</pre>
## Extensions for Multi-Language Subtitling ##
Softel have designed extensions to the basic Newfor protocol to address the problem of sending subtitles in 4 languages on a single RS232 comms line, from a subtitle workstation to a Teletext insertion system.
We have added a 5thth Message Type to the existing 4 of the basic Newfor protocol. This new Message Type is called "Set Channel" and is used to define which of the 4 logical channels will be used by subsequent subtitle messages.
## Message Type 5 – Set Logical Channel ##
Defines the Channel number 1-4 that subsequent Message Types 1-4 above will apply to. Consists of 3 bytes:-
> - ESCAPE (Hex 9B) in ODD Parity as usual.
> - the Channel Number (1 – 4), Hamming Encoded
> e.g. Hex 9B, Hex 5E = Set Channel 3.
Each Logical Channel number has its own Mag/Page numbers. This arrangement makes it compatible with existing Newfor equipment, which of course will never send the Set Channel message. The receiving equipment should default to Channel 1 on power-up and after terminating transmission, so all the type 1-4 messages will refer to Channel 1 and other channels remain unused.
## Specifying the Language ##
The Host system needs to know what language is on which channel, so it can set the appropriate country code in the Row 0 Header – this selects the National Character Options to be used.
This is achieved by sending a Type 1 message with the Magazine set to Zero. The Country code is determined by the Page No., i.e.
SO (hex 0E), 0, 0, 0, CC
<sup>^</sup><sup>^</sup><sup>^</sup>^^   Hamming Encoded
^ _Mag No. = zero
^_ Page No. (tens) = Zero
^ _Country Code (replaces Page No. (Units))
where CC represents the Country Code, as defined below:-_<pre>
0 English<br>
1 German & Dutch<br>
2 Sweden<br>
3 Italian<br>
4 French<br>
5 Spanish & Portugese<br>
6 (not used)<br>
7 Arabic<br>
</pre>
## Using Newfor Protocol over a Network ##
As an alternative to transmission over a Serial line, Newfor commands may be sent over a network, wrapped up in an appropriate network packet, e.g. IPX or SPX.
## Using an Extended Character Set ##
Some languages (e.g. Swedish, Spanish) support an extended Teletext character set, with character codes in the range Hex 80 to FF. In these cases, the 40 data bytes per subtitle row in the Type 2 Message will NOT be odd parity, they will be 8-bit character codes.
Characters in the extended range are transmitted over Teletext using Packet 26 techniques and it is the Host which is expected to assemble these packets.