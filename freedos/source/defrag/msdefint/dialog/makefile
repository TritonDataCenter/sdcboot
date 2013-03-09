#
#  Makefile for files in this directory.
#

!include "..\..\config.mak"

all: dialog.lib
        @echo Dialogs compiled. 

dialog.lib: menu.obj copyrigt.obj maplegnd.obj sortbox.obj \
            methods.obj seldrvbx.obj msgbxs.obj defrdone.obj recoment.obj \
            hlpdialg.obj stopdfrg.obj AskFrXMS.obj
        @-del dialog.lib
        @$(libman) dialog.lib + menu.obj
        @$(libman) dialog.lib + copyrigt.obj
        @$(libman) dialog.lib + maplegnd.obj
        @$(libman) dialog.lib + sortbox.obj
        @$(libman) dialog.lib + methods.obj
        @$(libman) dialog.lib + seldrvbx.obj
        @$(libman) dialog.lib + msgbxs.obj
        @$(libman) dialog.lib + defrdone.obj
        @$(libman) dialog.lib + recoment.obj
        @$(libman) dialog.lib + hlpdialg.obj
        @$(libman) dialog.lib + stopdfrg.obj
	@$(libman) dialog.lib + AskFrXMS.obj
        @-del dialog.bak

menu.obj: menu.c menu.h dialog.h msgbxs.h ..\event\event.h \
          ..\screen\screen.h ..\mouse\mouse.h ..\keyboard\keyboard.h \
          ..\winman\window.h ..\winman\winman.h
        @$(compiler) $(options) menu.c

copyrigt.obj: copyrigt.c copyrigt.h dialog.h ..\screen\screen.h \
              ..\event\event.h ..\winman\winman.h ..\winman\cmdbtn.h \
              ..\winman\control.h ..\winman\controls.h ..\winman\timlabel.h \
              ..\..\misc\bool.h ..\..\misc\version.h
        @$(compiler) $(options) copyrigt.c

maplegnd.obj: maplegnd.c maplegnd.h dialog.h ..\screen\screen.h \
              ..\winman\control.h ..\winman\controls.h ..\winman\winman.h \
              ..\winman\window.h ..\winman\cmdbtn.h ..\..\misc\bool.h
        @$(compiler) $(options) maplegnd.c 

sortbox.obj: sortbox.c dialog.h sortbox.h ..\screen\screen.h \
             ..\winman\control.h ..\winman\controls.h \
             ..\winman\slctbtn.h ..\winman\window.h ..\winman\winman.h \
             ..\..\misc\bool.h ..\..\modlgate\defrpars.h
        @$(compiler) $(options) sortbox.c 

methods.obj: methods.c dialog.h methods.h ..\screen\screen.h \
             ..\winman\control.h ..\winman\window.h ..\winman\winman.h \
             ..\winman\controls.h ..\winman\slctbtn.h \
             ..\..\misc\bool.h ..\..\modlgate\defrpars.h
        @$(compiler) $(options) methods.c

seldrvbx.obj: seldrvbx.c dialog.h ..\screen\screen.h \
              ..\..\misc\misc.h ..\..\misc\bool.h \
	      ..\event\event.h ..\mouse\mouse.h  \
              ..\winman\winman.h ..\winman\control.h \
              ..\winman\controls.h ..\winman\cmdbtn.h \
              ..\winman\window.h ..\winman\winman.h \
              ..\winman\lolstbx.h ..\winman\vscrctrl.h  \
              ..\..\modlgate\defrpars.h
        @$(compiler) $(options) seldrvbx.c

msgbxs.obj: msgbxs.c dialog.h ..\screen\screen.h \
	    ..\event\event.h ..\mouse\mouse.h  \
            ..\..\misc\bool.h ..\winman\winman.h ..\winman\window.h \
            ..\winman\control.h ..\winman\controls.h ..\winman\cmdbtn.h
        @$(compiler) $(options) msgbxs.c

defrdone.obj: defrdone.c dialog.h ..\screen\screen.h ..\winman\control.h \
              ..\winman\window.h ..\winman\winman.h ..\winman\controls.h \
              ..\winman\slctbtn.h ..\..\misc\bool.h
        @$(compiler) $(options) defrdone.c

recoment.obj: recoment.c dialog.h ..\screen\screen.h ..\winman\control.h \
              ..\winman\window.h ..\winman\winman.h ..\winman\controls.h \
              ..\..\misc\bool.h
        @$(compiler) $(options) recoment.c

hlpdialg.obj: hlpdialg.c ..\screen\screen.h ..\dialog\dialog.h \
              ..\winman\control.h ..\winman\controls.h ..\winman\loview.h \
              ..\winman\window.h ..\winman\winman.h ..\winman\vscrctrl.h \
              ..\..\misc\bool.h
        @$(compiler) $(options) hlpdialg.c

stopdfrg.obj: stopdfrg.c dialog.h ..\screen\screen.h ..\winman\control.h \
              ..\winman\window.h ..\winman\winman.h ..\winman\controls.h \
              ..\winman\slctbtn.h ..\..\misc\bool.h ..\..\modlgate\defrpars.h \
              ..\helpsys\hlpindex.h ..\helpsys\idxstack.h
        @$(compiler) $(options) stopdfrg.c

AskFrXMS.obj: AskFrXMS.c dialog.h ..\screen\screen.h ..\winman\control.h \
              ..\winman\window.h ..\winman\winman.h ..\winman\controls.h \
              ..\winman\slctbtn.h ..\..\misc\bool.h ..\..\modlgate\defrpars.h \
              ..\helpsys\hlpindex.h ..\helpsys\idxstack.h
	@$(compiler) $(options) AskFrXMS.c
	
clean:                      
        @-del menu.obj     
        @-del copyrigt.obj 
        @-del maplegnd.obj 
        @-del sortbox.obj  
        @-del methods.obj  
        @-del seldrvbx.obj 
        @-del msgbxs.obj
        @-del defrdone.obj
        @-del recoment.obj
        @-del hlpdialg.obj
        @-del stopdfrg.obj
	@-del AskFrXMS.obj
        @-del dialog.lib 
