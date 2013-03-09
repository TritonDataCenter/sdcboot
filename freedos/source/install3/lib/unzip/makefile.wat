# Memory model: one of s, m, c, l (small, medium, compact, large)
MODEL=l

CC=wcl
LD=$(CC)
AR=wlib -q

# compiler flags
CFLAGS=-oasl -i=../zlib -zq -bt=DOS -fo=.obj -m$(MODEL)
LDFLAGS=-m$(MODEL)

CMN_OBJS = ioapi.obj
ZLIB = ../zlib/zlib_$(MODEL).lib
UNZ_OBJS = miniunz.obj unzip.obj
ZIP_OBJS = minizip.obj zip.obj

MZLIB_LIB = unzip.lib

.c.obj:
	$(CC) -c $(CFLAGS) $*.c

all: $(MZLIB_LIB)

# we must cut the command line to fit in the MS/DOS 128 byte limit:
$(MZLIB_LIB):  ioapi.obj munzlib.obj unzip.obj
	@-if exist $(MZLIB_LIB) rm $(MZLIB_LIB)
	$(AR) $(MZLIB_LIB) +ioapi.obj +unzip.obj +munzlib.obj +../zlib/zlib_l.lib

miniunz:  $(CMN_OBJS) $(UNZ_OBJS) $(ZLIB)
	$(CC) $(CFLAGS) $(UNZ_OBJS) $(CMN_OBJS) $(ZLIB)

minizip:  $(CMN_OBJS) $(ZIP_OBJS) $(ZLIB)
	$(CC) $(CFLAGS) -o$@ $(ZIP_OBJS) $(CMN_OBJS) $(ZLIB)

test:	miniunz minizip
	./minizip test readme.txt
	./miniunz -l test.zip
	mv readme.txt readme.old
	./miniunz test.zip

clean: .SYMBOLIC
	-rm *.obj
	-rm minizip.exe
	-rm miniunz.exe
    -rm *.lib


