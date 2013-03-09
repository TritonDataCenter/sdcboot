#
# Makefile for the checks on the directories
#

!include "..\config.mak"

includepath = ..\engine\header;..\misc

all: dirs.lib
        @echo DIR checks compiled

dirs.lib: dtdtpnt.obj dblfiles.obj chklfns.obj chkentrs.obj scanfils.obj \
          scnsbdrs.obj scnrtdir.obj invchar.obj invfclst.obj
       @-del dirs.lib
       @$(libman) dirs.lib + dtdtpnt.obj
       @$(libman) dirs.lib + dblfiles.obj
       @$(libman) dirs.lib + chklfns.obj
       @$(libman) dirs.lib + chkentrs.obj
       @$(libman) dirs.lib + scanfils.obj
       @$(libman) dirs.lib + scnsbdrs.obj
       @$(libman) dirs.lib + scnrtdir.obj
       @$(libman) dirs.lib + invchar.obj
       @$(libman) dirs.lib + invfclst.obj
       @-del dirs.bak

dtdtpnt.obj: dtdtpnt.c
       @$(compiler) $(options) -I$(includepath) $(logging) dtdtpnt.c

dblfiles.obj: dblfiles.c
       @$(compiler) $(options) -I$(includepath) $(logging) dblfiles.c

chklfns.obj: chklfns.c
       @$(compiler) $(options) -I$(includepath) $(logging) chklfns.c 

chkentrs.obj: chkentrs.c
       @$(compiler) $(options) -I$(includepath) $(logging) chkentrs.c 

scanfils.obj: scanfils.c
       @$(compiler) $(options) -I$(includepath) $(logging) scanfils.c
       
scnsbdrs.obj: scnsbdrs.c
       @$(compiler) $(options) -I$(includepath) $(logging) scnsbdrs.c
       
scnrtdir.obj: scnrtdir.c
       @$(compiler) $(options) -I$(includepath) $(logging) scnrtdir.c       

invchar.obj: invchar.c
       @$(compiler) $(options) -I$(includepath) $(logging) invchar.c       
       
invfclst.obj: invfclst.c
       @$(compiler) $(options) -I$(includepath) $(logging) invfclst.c             
       
clean:
        @-del dtdtpnt.obj
        @-del dblfiles.obj
        @-del chklfns.obj
        @-del chkentrs.obj
        @-del scanfils.obj
        @-del scnsbdrs.obj
        @-del scnrtdir.obj
        @-del invchar.obj
        @-del invfclst.obj
        @-del dirs.lib