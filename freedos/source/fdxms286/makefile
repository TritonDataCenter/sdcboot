#
# Makefile for FreeDOS XMS Driver for 286s.
#
# You might need to change the following lines if you use another
# assembler, linker or compiler.

# This is for Turbo Assembler Version 3.1.
ASM		=	tasm32 /m9

# This is for Turbo Link Version 5.1.
LNK		=	tlink /t /x 

# This is for bcc from Borland C++ Version 3.1. 
CC 	= 	bcc -O1

#
# That you'd want to change something below this line is unlikely.

all : fdxms286.sys fdxm286t.sys XMStest.exe

fdxms286.sys : fdxms286.asm fdxms.inc
	$(ASM) $* ,$* ,$*
	$(LNK) $*.obj,$*.sys

fdxm286t.sys : fdxms286.asm fdxms.inc
	$(ASM) /dTRACE_CODE fdxms286.asm
	$(LNK) fdxms286.obj,$*.sys

XMStest.exe: XMStest.c
	$(CC) $*.c
