# this is a DOS makefile

CC=wcl
CFLAGS=-c -ml -DNDEBUG -fo=.obj -bt=DOS -oasl -zq -i=../../include

all: lsm_desc.obj

lsm_desc.obj : lsm_desc.c
	$(CC) $(CFLAGS) lsm_desc.c

clean:
	del *.obj
