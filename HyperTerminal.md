# Hyperterminal #

VBIT is controlled via a USB virtual serial port. You can connect to VBIT by using a terminal emulator. HyperTerminal is one such terminal program and used to be supplied with Windows XP.

# Installing HyperTerminal on Windows Vista/Windows 7 or later #
HyperTerminal is no longer supplied with Windows. This is a nuisance that you need to fix. HyperTerminal is a simple program that doesn't need complicated installation. To copy Hyperterminal from a Windows XP machine just find and copy these files:
**hypertrm.exe** hypertrm.dll
**hypertrm.chm** hypertrm.hlp

Put them all into one folder. You'll be able to run hypertrm.exe by double clicking on it and it will work normally.

# Installing the Virtual Com Port #

Plug the processor board into a USB port. Windows should ask for a driver. Point Windows at the LUFA virtual serial port INF file. It should install.

# Finding the COM port number #
When you plug in a VBIT a virtual com port will appear. Go to the System control panel and find the device manager. Under COM and LPT ports you will see a list of COM ports. The COM port is the one with the LUFA name in the ID string.

# Running Hyperterminal #
Open a new session and select the COM number that you discovered in the last step. All commands to VBIT must be preceded by 

&lt;CTRL&gt;

N0 and end with 

&lt;Enter&gt;



Example:

To find the VBIT version number type


&lt;CTRL&gt;

N0Y

&lt;Enter&gt;



To see the rest of the commands, look in the source code!