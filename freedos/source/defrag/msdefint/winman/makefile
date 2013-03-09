#
#  Makefile for files in this directory.
#

!include "..\..\config.mak"

all: winman.lib
        @echo Window manager compiled. 

winman.lib: cmdbtn.obj frame.obj hline.obj label.obj winman.obj timlabel.obj \
     ictrlvls.obj extevent.obj cmnctrls.obj slctbtn.obj vscrctrl.obj \
     lolstbx.obj drvsltbx.obj eventbtn.obj wndwclsr.obj loview.obj helpview.obj
        @-del winman.lib
        @$(libman) winman.lib + cmdbtn.obj
        @$(libman) winman.lib + frame.obj
        @$(libman) winman.lib + hline.obj
        @$(libman) winman.lib + label.obj
        @$(libman) winman.lib + winman.obj
        @$(libman) winman.lib + timlabel.obj
        @$(libman) winman.lib + ictrlvls.obj
        @$(libman) winman.lib + extevent.obj
        @$(libman) winman.lib + cmnctrls.obj
        @$(libman) winman.lib + slctbtn.obj
        @$(libman) winman.lib + vscrctrl.obj
        @$(libman) winman.lib + lolstbx.obj
        @$(libman) winman.lib + drvsltbx.obj
        @$(libman) winman.lib + eventbtn.obj
        @$(libman) winman.lib + wndwclsr.obj
        @$(libman) winman.lib + loview.obj
        @$(libman) winman.lib + helpview.obj
        @-del winman.bak

cmdbtn.obj: cmdbtn.c control.h cmdbtn.h ..\event\event.h \
	    ..\mouse\mouse.h ..\screen\screen.h ..\..\misc\bool.h
	@$(compiler) $(options) cmdbtn.c

frame.obj: frame.c  control.h frame.h ..\screen\screen.h ..\..\misc\bool.h
        @$(compiler) $(options) frame.c

hline.obj: hline.c control.h controls.h ..\screen\screen.h \
           ..\..\misc\bool.h
        @$(compiler) $(options) hline.c 

label.obj: label.c control.h ..\screen\screen.h ..\..\misc\bool.h
        @$(compiler) $(options) label.c 

timlabel.obj: timlabel.c control.h timlabel.h ..\screen\screen.h \
              ..\..\misc\bool.h ..\ovlhost\lowtime.h
        @$(compiler) $(options) timlabel.c

winman.obj: winman.c control.h window.h ..\event\event.h winman.h \
	    ..\dialog\dialog.h ..\screen\screen.h ..\..\misc\bool.h
        @$(compiler) $(options) winman.c

ictrlvls.obj: ictrlvls.c control.h ..\..\misc\bool.h
        @$(compiler) $(options) ictrlvls.c

extevent.obj: extevent.c ..\event\event.h ..\dialog\msgbxs.h \
              ..\mouse\mouse.h ..\screen\screen.h ..\..\misc\bool.h \
              ..\logman\logman.h ..\dialog\hlpdialg.h
        @$(compiler) $(options) extevent.c

cmnctrls.obj: cmnctrls.c ..\..\misc\bool.h ..\dialog\dialog.h cmdbtn.h
        @$(compiler) $(options) cmnctrls.c

slctbtn.obj: slctbtn.c ..\event\event.h ..\mouse\mouse.h \
             ..\screen\screen.h ..\..\misc\bool.h control.h slctbtn.h
        @$(compiler) $(options) slctbtn.c

vscrctrl.obj: vscrctrl.c control.h vscrctrl.h  ..\..\misc\bool.h \
	      ..\event\event.h ..\mouse\mouse.h ..\screen\screen.h
	@$(compiler) $(options) vscrctrl.c

lolstbx.obj: lolstbx.c control.h controls.h vscrctrl.h lolstbx.h \
             ..\event\event.h ..\mouse\mouse.h ..\..\misc\bool.h 
        @$(compiler) $(options) lolstbx.c

drvsltbx.obj: drvsltbx.c control.h controls.h vscrctrl.h lolstbx.h \
	      drvsltbx.h ..\event\event.h ..\screen\screen.h \
	      ..\mouse\mouse.h ..\..\misc\bool.h \
              ..\..\misc\misc.h
        @$(compiler) $(options) drvsltbx.c

eventbtn.obj: eventbtn.c control.h controls.h cmdbtn.h eventbtn.h
        @$(compiler) $(options) eventbtn.c

wndwclsr.obj: wndwclsr.c control.h controls.h
   @$(compiler) $(options) wndwclsr.c

loview.obj: loview.c control.h controls.h vscrctrl.h loview.h \
            ..\..\misc\bool.h
   @$(compiler) $(options) loview.c

helpview.obj: helpview.c control.h controls.h vscrctrl.h loview.h \
              ..\..\misc\bool.h
   @$(compiler) $(options) helpview.c

clean:
        @-del cmdbtn.obj     
        @-del frame.obj 
        @-del hline.obj 
        @-del label.obj  
        @-del timlabel.obj  
        @-del winman.obj  
        @-del ictrlvls.obj  
        @-del extevent.obj  
        @-del cmnctrls.obj  
        @-del slctbtn.obj  
        @-del vscrctrl.obj  
        @-del lolstbx.obj
        @-del drvsltbx.obj
        @-del checkbox.obj
        @-del eventbtn.obj
        @-del wndwclsr.obj
        @-del loview.obj
        @-del helpview.obj
        @-del winman.lib