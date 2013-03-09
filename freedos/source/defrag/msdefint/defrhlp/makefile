#
# Makefile for files in this directory
#

helppages  = index.0 overview.1 keyboard.3 checking.4 sorting.5 defrag.6 seldrv.7 selmethd.8 filesort.9 legend.10
helppages  = $(helppages) about.11 begin.12 drive.13 method.14 sort.15 maplegnd.16 aboutdlg.17 exit.18 stop.19 speed.20

helpkit = ..\helpkit\mkhelp.exe

all: defrag.hlp

defrag.hlp: $(helppages) $(helpkit) defrag.lst
        @$(helpkit) defrag.lst defrag.hlp
        @copy defrag.hlp ..\..\..\bin
        @copy defrag.hlp ..\..
        
clean:
        @-del defrag.hlp        
