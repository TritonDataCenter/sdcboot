#
#  Makefile for files in this directory.
#

!include "..\..\config.mak"

all: main.lib
        @echo Main routines compiled.

main.lib: msdefint.obj keepdos.obj actions.obj chkargs.obj
        @-del main.lib
        @$(libman) main.lib + msdefint.obj
        @$(libman) main.lib + keepdos.obj
        @$(libman) main.lib + actions.obj
        @$(libman) main.lib + chkargs.obj
        @-del main.bak

msdefint.obj: msdefint.c ..\..\misc\misc.h ..\keyboard\keyboard.h     \
            ..\mouse\mouse.h ..\event\event.h                     \
            ..\screen\screen.h ..\dialog\dialog.h                 \
            ..\dialog\menu.h ..\dialog\msgbxs.h                   \
            ..\dialog\copyrigt.h ..\dialog\maplegnd.h             \
            keepdos.h                                             \
            ..\..\misc\version.h ..\winman\winman.h               \
            ..\winman\window.h ..\winman\control.h chkargs.h      \
            ..\..\modlgate\modlgate.h ..\..\misc\reboot.h         \
            ..\..\misc\bool.h ..\dialog\defrdone.h                \
            ..\logman\logman.h 
        @$(compiler) $(options) -I..\..\misc;..\..\engine\header msdefint.c

keepdos.obj: keepdos.c ..\screen\screen.h
        @$(compiler) $(options) keepdos.c

actions.obj: actions.c ..\screen\screen.h ..\dialog\methods.h \
             ..\dialog\sortbox.h ..\dialog\msgbxs.h ..\dialog\seldrvbx.h \
             ..\..\modlgate\defrpars.h actaspct.h
        @$(compiler) $(options) -I..\..\misc;..\..\engine\header actions.c

chkargs.obj: chkargs.c chkargs.h ..\..\misc\bool.h ..\..\modlgate\defrpars.h \
             ..\screen\screen.h
        @$(compiler) $(options) chkargs.c

clean:
        @-del msdefint.obj
        @-del keepdos.obj
        @-del actions.obj
        @-del chkargs.obj
        @-del main.lib