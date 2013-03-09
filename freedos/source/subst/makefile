##!dmake -f

# main goal of this Makefile:
# SWSUBST, the SUBST & JOIN replacement

# source in the Free-Dos initialization 
.INCLUDE .FIRST .IGNORE : fd_init.mk ../fd_init.mk ../../fd_init.mk

#
#Project related settings
#
PRG = swsubst.exe
SRC = cds.c drive.c flushdsk.c mkdir.c print.c remove.c set.c subst.c \
	swap.c swsubst.c yerror.c
OBJ = cds.obj drive.obj flushdsk.obj mkdir.obj print.obj remove.obj set.obj \
	subst.obj swap.obj swsubst.obj yerror.obj
HDR = cds.h msgdecl.inc msgfeat.inc swsubst.h yerror.h
MYCFLAGS = # TEST=YES
MSGLIB !:= msg.lib
MSGDCL !:= yerror.h
##LDLIBS = $(MSGLIB) $(FDLIB)\$(_MODEL:u)_$(LNG).lib $(FDLIB)\SUPPL_$(_MODEL).lib
LDLIBS = $(MSGLIB) $(FDLIB)\\$(LNG)$(_MODEL:u).lib $(FDLIB)\SUPPL_$(_MODEL).lib

#
#First target
#
all : $(CFG) $(PRG)	# copy library into Free-Dos library dir

.INIT : $(CFG) tags refs errlist # Will make the utilizing files

#
#C initialization file
#
.IF $(CFG)
# Compiler configuration file, for Borland C only
# options: no Windows, no RTTI, use pre-compiled headers, no floating point

CONFIGURATION = -W-	\
-X-	\
-H	\
-I.;$(INCDIR)	\
-L.;$(LIBDIR)	\
-H=swsubst.csm	\
-f-	\
-ff-	\
-m$(_MODEL)

.IF $(USE31) == $(NULL)
CONFIGURATION += -RT-
.ENDIF

##$(CFG) : $(MAKEFILE:s/-f//)
##	Cat $(mktmp $(CONFIGURATION:t"\n")\n) >$@

.ENDIF


$(PRG) : $(MSGLIB) $(OBJ) 
	$(CC) $(CCLDFLAGS) -e$@ @$(mktmp $(OBJ:t"\n") $(LDLIBS:t"\n")\n)

#MAKEDEP START
mkdir.obj : mkdir.c \
	 cds.h msgdecl.inc msgfeat.inc swsubst.h yerror.h
drive.obj : drive.c \
	 cds.h msgdecl.inc msgfeat.inc swsubst.h yerror.h
yerror.obj : yerror.c \
	 msgdecl.inc msgfeat.inc yerror.h
flushdsk.obj : flushdsk.c \
	 cds.h msgdecl.inc msgfeat.inc swsubst.h yerror.h
set.obj : set.c \
	 cds.h msgdecl.inc msgfeat.inc swsubst.h yerror.h
print.obj : print.c \
	 cds.h msgdecl.inc msgfeat.inc swsubst.h yerror.h
remove.obj : remove.c \
	 cds.h msgdecl.inc msgfeat.inc swsubst.h yerror.h
subst.obj : subst.c \
	 cds.h msgdecl.inc msgfeat.inc swsubst.h yerror.h
cds.obj : cds.c \
	 cds.h
swsubst.obj : swsubst.c \
	 cds.h msgdecl.inc msgfeat.inc swsubst.h yerror.h
swap.obj : swap.c \
	 cds.h msgdecl.inc msgfeat.inc swsubst.h yerror.h
DYNSOURCES =
#MAKEDEP STOP

ci :: $(shell dir /b *.1 *.1g)

# source in the Free-Dos standard targets 
.INCLUDE .FIRST .IGNORE : fd_exit.mk ../fd_exit.mk ../../fd_exit.mk
