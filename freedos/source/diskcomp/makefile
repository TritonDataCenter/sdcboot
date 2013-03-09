!include "config.mak"


SOURCE = diskcomp.c misc.c md5.c drive.c
HEADER = drive.h global.h md5.h
BINARY = diskcomp.com

all:    $(BINARY)

clobber:        clean
		del ..\..\bin\$(BINARY)
		del $(BINARY)

clean:
		del *.obj
		del *.bak 
		del *.crf *.xrf *.map *.lst


$(BINARY): $(SOURCE) $(HEADER)
	$(CC) $(CFLAGS) -ediskcomp $(SOURCE)
	$(UPX) $(BINARY)
	copy $(BINARY) ..\..\bin

