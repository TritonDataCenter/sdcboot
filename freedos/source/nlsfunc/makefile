# Makefile for building under Linux
#

UPX=/home/eduardo/fdos/upx/upx

INSTDIR=../../bin

all: nlsfunc.exe

install: all
	$(UPX) --8086 -9 -f -o $(INSTDIR)/nlsfunc.exe nlsfunc.exe
	
clean:
	rm -f nlsfunc.exe

nlsfunc.exe: nlsfunc.asm exebin.mac
	nasm -dNEW_NASM -fbin nlsfunc.asm -o $@

