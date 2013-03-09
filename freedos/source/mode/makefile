# Makefile for FreeDOS MODE C, (c) by Eric Auer 2003-2004

# set 
#   UPX=-rem 
# if you dont want to UPX the generated files
# UPX=-rem
UPX=upx --8086 --best

# the assembler:
NASM=nasm

############# WATCOM (untested) ##############
# CC=wcl
# CFLAGS=-oas -s -wx -we -zq -fm -fe=

############# TURBO_C ########################
# -w warn all, -M mapfile -Z optimize registers -O optimize jumps
# -k- no standard stack frame -e... defines exe filename, must be
# last argument. -f- disables FPU usage. -mt uses tiny model
# and -lt passes "create com file" option to linker.
# IMPORTANT: Depending on your compiler configuration, you will have
#   to add path info to your CFLAGS, e.g.: -IC:\TC\INCLUDE -LC:\TC\LIB
CC=tcc
CFLAGS=-w -M -f- -mt -lt -e


# When you build with prf.c, printf is replaced by a smaller
# version (e.g. without floating point support). This saves
# about 0.9k raw mode.com size, 0.7k after UPXing.
# Why not :-).
# CFILES=mode.c modeopt.c   modecon.c modevga.c modecp.c   modeser.c modepar.c modetsr.c
CFILES=mode.c modeopt.c prf.c   modecon.c modevga.c modecp.c   modeser.c modepar.c modetsr.c

######## targets: ########

# if you include DISPLAY sources with this: all: mode.com display.com

all: mode.com

# display.com: display.asm
#	$(NASM) display.asm -o display.com
#	$(UPX) display.com

# modeint.asm gets compiled and converted to modeint.h with help
# of the bin2c.c program. mode_int17 is an array with the TSR code.
modeint.h:	modeint.asm bin2c.c makefile
	$(CC) $(CFLAGS)bin2c bin2c.c
	$(NASM) -o modeint.bin modeint.asm
	-bin2c modeint.bin modeint.h mode_int17

# the cpitrick part is written in assembly language and deUPXes
# UPX (sic!) compressed CPI files: rename a CPI to COM, compress,
# then rename the result to CPX. Compress with --8086 option.
cpitrick.obj:	cpitrick.asm makefile
	$(NASM) -fobj -o cpitrick.obj cpitrick.asm

# Guess what - the main part is written in all C.
mode.com: $(CFILES) modeint.h cpitrick.obj makefile
	$(CC) $(CFLAGS)mode $(CFILES) cpitrick.obj
	$(UPX) mode.com

clean:
	-del *.obj
	-del bin2c.com
	-del *.bin
	-del bin2c.map
