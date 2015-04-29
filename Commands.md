# VBIT Commands #

VBIT runs a simple command line interpreter.

All commands start with



&lt;CTRL-N&gt;

0

and end with 

&lt;ENTER&gt;



# Commands summary #
Note that these commands are not properly defined yet. Many are not implemented. Look at the source code for their behaviour.
## C - Create Transmission File ##
This command creates magazine lists.
## III - Clear out all settings and reload defaults ##
## I2 - Set an SAA7113 I2C register ##
## Y - Read version string ##
## T - Debugging test. The behaviour is not defined ##
## G - Packet 8/30 stuff ##
### GUC - Set format type ###
### GUL - Set link ###
### GUN - Set nic ###
### GUT - Set time ###
### GUD - Set F1 caption ###
## QM - Set miscellaneous flags ##
The read version is EM
## QO - Set output lines ##
The parameter is 18 characters where
**1..8 = magazine. However, in the current software, any number can output any magazine.** P=Pass through
**Q=Quiet** F=Filler
**Z=databroadcast
Example:**

QO111Q2233P445566778
EO will read the output lines
### ? - System Health ###
This reports on the basic functions of each chip. The video chips must respond correctly using I2C. The FIFO must store and return data.
## Lots more commands and documentation to follow ##
