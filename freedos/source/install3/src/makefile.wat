# this is a DOS makefile

CATS=../lib/cats
UZPHOME=../lib/unzip

CC=wcl
CFLAGS=-zq -d0 -oasl -bt=DOS -fo=.obj -ml -DNDEBUG -i=../include:../lib/lsm:$(UZPHOME):$(CATS):../lib/zlib
LDFLAGS=-ml -bt=DOS -k19k -fm -zq
LDLIBS1=$(UZPHOME)/unzip.lib $(CATS)/catsdb.lib
LDLIBS2=../lib/dat.obj ../lib/lsm/lsm_desc.obj

SRC=install.c
OBJ=install.obj

SOURCES1=bargraph.c cat.c isfile.c inst.c pause.c cdp.c log.c int24.c screen.c
SOURCES2=box.c getkey.c sel_list.c repaint.c unz.c globals.c cchndlr.c
OBJECTS1=bargraph.obj cat.obj isfile.obj inst.obj pause.obj cdp.obj log.obj int24.obj screen.obj
OBJECTS2=box.obj getkey.obj sel_list.obj repaint.obj unz.obj globals.obj cchndlr.obj

.c.obj:
	$(CC) -c $(CFLAGS) $<

# targets:

all: install.exe # cat_file.exe

clean realclean: .SYMBOLIC
	-rm *.obj

distclean: realclean .SYMBOLIC
	-rm *.exe


# the del cat.obj lines are due to cat.obj include or doesn't include a main()
# depending on how CAT_FILE_PROGRAM is defined, install.exe's cat.obj should NOT
# have a main, whereas when compiled standalone is should.
cat_file.exe: cat.c
	del cat.obj
	echo $(CFLAGS) > cflags.tmp
	echo $(LDLIBS1) > ldlibs1.tmp
	$(CC) @cflags.tmp -DCAT_FILE_PROGRAM -ecat_file cat.c @ldlibs1.tmp 
	del cat.obj
	del *.tmp

inst.lib: $(OBJ) $(OBJECTS1) $(OBJECTS2)
	@if exist inst.lib rm inst.lib
	wlib -q inst.lib +bargraph.obj
	wlib -q inst.lib +cat.obj
	wlib -q inst.lib +isfile.obj
	wlib -q inst.lib +inst.obj
	wlib -q inst.lib +pause.obj
	wlib -q inst.lib +cdp.obj
	wlib -q inst.lib +log.obj
	wlib -q inst.lib +int24.obj
	wlib -q inst.lib +box.obj
	wlib -q inst.lib +getkey.obj
	wlib -q inst.lib +sel_list.obj
	wlib -q inst.lib +repaint.obj
	wlib -q inst.lib +unz.obj
	wlib -q inst.lib +globals.obj
	wlib -q inst.lib +cchndlr.obj
	wlib -q inst.lib +screen.obj
	wlib -q inst.lib +../lib/dat.obj
	wlib -q inst.lib +../lib/lsm/lsm_desc.obj

LDLIBS1=$(UZPHOME)\unzip.lib $(CATS)\catsdb.lib
LDLIBS2=..\lib\dat.obj ..\lib\lsm\lsm_desc.obj
OBJECTS1=bargraph.obj cat.obj isfile.obj inst.obj pause.obj cdp.obj log.obj int24.obj
OBJECTS2=box.obj getkey.obj sel_list.obj repaint.obj unz.obj globals.obj cchndlr.obj
install.exe: $(OBJ) $(OBJECTS1) $(OBJECTS2) inst.lib
	$(CC) $(LDFLAGS) $(OBJ) $(UZPHOME)/unzip.lib $(CATS)/catsdb.lib inst.lib @op.el
	upx --best --8086 install.exe

