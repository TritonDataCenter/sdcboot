#
# Makefile for files in this directory.
#

!include "..\..\config.mak"

incdir = -I..\header;..\..\misc

all: high.lib
        @copy high.lib ..\lib 
        @echo high FTE routines compiled.
     
high.lib: walktree.obj cpysct.obj Dircnt.obj FndCiDir.obj \
          FndCiFAT.obj FndFFSpc.obj FndLstCt.obj GtNthCst.obj \
          NthEntry.obj NthFlClt.obj RelocClt.obj SwpClst.obj  \
          AdCPDirs.obj FileChn.obj fexist.obj locpath.obj \
          walkwpth.obj TracePth.obj ClcFSize.obj freespac.obj
        @-del high.lib
        @-del high.bak
        @$(libman) high + walktree.obj
        @$(libman) high + cpysct.obj
        @$(libman) high + Dircnt.obj
        @$(libman) high + FndCiDir.obj
        @$(libman) high + FndCiFAT.obj
        @$(libman) high + FndFFSpc.obj
        @$(libman) high + FndLstCt.obj
        @$(libman) high + GtNthCst.obj
        @$(libman) high + NthEntry.obj
        @$(libman) high + NthFlClt.obj
        @$(libman) high + RelocClt.obj
        @$(libman) high + SwpClst.obj
        @$(libman) high + AdCPDirs.obj
        @$(libman) high + FileChn.obj
        @$(libman) high + fexist.obj
        @$(libman) high + locpath.obj
        @$(libman) high + walkwpth.obj
        @$(libman) high + TracePth.obj
        @$(libman) high + ClcFSize.obj
        @$(libman) high + FreeSpac.obj
        @-del high.bak
                
walktree.obj: walktree.c ..\header\fte.h ..\..\misc\bool.h
        @$(compiler) $(options) $(incdir) walktree.c

cpysct.obj: cpysct.c ..\header\fte.h ..\..\misc\bool.h
        @$(compiler) $(options) $(incdir) cpysct.c
        
Dircnt.obj: Dircnt.c ..\header\fte.h ..\..\misc\bool.h
        @$(compiler) $(options) $(incdir) Dircnt.c

FndCiDir.obj: FndCiDir.c ..\header\fte.h ..\..\misc\bool.h
        @$(compiler) $(options) $(incdir) FndCiDir.c
        
FndCiFAT.obj: walktree.c ..\header\fte.h ..\..\misc\bool.h
        @$(compiler) $(options) $(incdir) FndCiFAT.c
        
FndFFSpc.obj: FndFFSpc.c ..\header\fte.h ..\..\misc\bool.h
        @$(compiler) $(options) $(incdir) FndFFSpc.c
        
FndLstCt.obj: walktree.c ..\header\fte.h ..\..\misc\bool.h
        @$(compiler) $(options) $(incdir) FndLstCt.c
        
GtNthCst.obj: GtNthCst.c ..\header\fte.h ..\..\misc\bool.h
        @$(compiler) $(options) $(incdir) GtNthCst.c
        
NthEntry.obj: NthEntry.c ..\header\fte.h ..\..\misc\bool.h
        @$(compiler) $(options) $(incdir) NthEntry.c
        
NthFlClt.obj: NthFlClt.c ..\header\fte.h ..\..\misc\bool.h
        @$(compiler) $(options) $(incdir) NthFlClt.c
        
RelocClt.obj: RelocClt.c ..\header\fte.h ..\..\misc\bool.h
        @$(compiler) $(options) $(incdir) RelocClt.c
        
SwpClst.obj: SwpClst.c ..\header\fte.h ..\..\misc\bool.h
        @$(compiler) $(options) $(incdir) SwpClst.c

AdCPDirs.obj: AdCPDirs.c ..\header\fte.h ..\..\misc\bool.h
        @$(compiler) $(options) $(incdir) AdCPDirs.c

FileChn.obj: FileChn.c ..\header\fte.h ..\..\misc\bool.h
        @$(compiler) $(options) $(incdir) FileChn.c

fexist.obj: fexist.c ..\header\fte.h ..\..\misc\bool.h
        @$(compiler) $(options) $(incdir) fexist.c

locpath.obj: locpath.c ..\header\fte.h ..\..\misc\bool.h 
        @$(compiler) $(options) $(incdir) locpath.c

walkwpth.obj: walkwpth.c ..\header\fte.h ..\..\misc\bool.h 
        @$(compiler) $(options) $(incdir) walkwpth.c
        
TracePth.obj: TracePth.c ..\header\fte.h ..\..\misc\bool.h 
        @$(compiler) $(options) $(incdir) TracePth.c
        
ClcFSize.obj: ClcFSize.c ..\header\fte.h ..\..\misc\bool.h 
        @$(compiler) $(options) $(incdir) ClcFSize.c

FreeSpac.obj: FreeSpac.c ..\header\fte.h ..\..\misc\bool.h 
        @$(compiler) $(options) $(incdir) FreeSpac.c
                
clean:
        @-del walktree.obj
        @-del cpysct.obj
        @-del Dircnt.obj
        @-del FndCiDir.obj
        @-del FndCiFAT.obj
        @-del FndFFSpc.obj
        @-del FndLstCt.obj
        @-del GtNthCst.obj
        @-del NthEntry.obj
        @-del NthFlClt.obj
        @-del RelocClt.obj
        @-del SwpClst.obj
        @-del AdCPDirs.obj
        @-del FileChn.obj
        @-del fexist.obj
        @-del locpath.obj
        @-del TracePth.obj
        @-del ClcFSize.obj
        @-del FreeSpac.obj
        @-del WalkWPth.obj
        @-del high.lib
        @-del high.bak
        @-del *.bak
