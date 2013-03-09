COMPILERPATH=c:\tc

CC=$(COMPILERPATH)\tcc

TLINK=$(COMPILERPATH)\tlink

# -lt and -mt are for .com files
# add -M to create a map file...
CFLAGS=-M -I$(COMPILERPATH)\include\ -L$(COMPILERPATH)\lib\ -lt -mt -w -a- -f- -Z

LIBS=$(COMPILERPATH)\lib\

# enable either the line with real upx or the -rem line.
# UPX is a compressor for executable files: http://upx.sourceforge.net/ ...
UPX=upx --8086
# UPX=-rem
