# Makefile for Tree v3.7 for FreeDOS - compile using all tested compilers.
# Expects to be in the directory with tree.c
#
# To compile tree, setup your compiler environment properly, then
# compile specifying tree.c as the source file.
#
# Compile this using:
#
#  make -ftree.mak

# If you are not compiling with CATS support then ignore the
# message about catgets.obj db.obj get_line.obj not found

SRC= tree.c getopt.c catgets.c db.c get_line.c


# Any compilers you lack should be commented ('#') out below:


## Borland:
#
## Turbo C 2.01
## (set PATH=C:\TC201)
#CC=tcc
#CFLAGS= -mt -lt -w-par -DUSE_CATGETS
#
## Turbo C/C++ 1.01
## (set PATH=C:\TC101\BIN)
#CC=tcc
#CFLAGS= -mt -lt -w-par -DUSE_CATGETS
#
## Turbo C/C++ 3.0
## (set PATH=C:\TC30\BIN)
#CC=tcc
#CFLAGS= -mt -tDc -w-par -DUSE_CATGETS
#
# Borland C/C++ 3.1
# (set PATH=C:\BORLANDC\BIN)
CC=bcc
CFLAGS= -mt -tDc -w-par -DUSE_CATGETS
#
## Borland C/C++ 4.5 DOS
## (set PATH=C:\BC45\BIN)
#CC=bcc
#CFLAGS= -mt -tDc -w-par -DUSE_CATGETS
#
## Borland C/C++ 4.5 WIN32
## (set PATH=C:\BC45\BIN)
#CC=bcc32
#CFLAGS= -tWC -w-par -DUSE_CATGETS
#
## Borland C/C++ 5.5
## (set PATH=c:\Borland\bcc55\bin)
#CC=bcc32
#CFLAGS= -Lc:\Borland\bcc55\Lib\PSDK -tWC -w-par -w-aus -etree.exe -DUSE_CATGETS
#LDLIBS= user32.lib
#
## Microsoft:
#
## Visual C/C++ 5.0
## Ensure VCVARS32.BAT has been ran to setup paths and environment
## (for example: CALL c:\Progra~1\Devstu~1\vc\bin\VCVARS32.BAT)
#CC=cl
#CFLAGS= /Fe"tree.exe" /nologo /ML /GX /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /D "USE_CATGETS"
#LDFLAGS= /link user32.lib
#
## MS C 6.0 (DOS)
## Must also setup its environment
## (for example: CALL C:\C600\BIN\NEW-VARS.BAT)
#CC=cl
#CFLAGS= /D "USE_CATGETS"
#
## Digital Mars [Symantec]
## (set PATH=C:\dm\bin)
#
## Win32
#CC=sc
#CFLAGS= -mn -DUSE_CATGETS
#
## DOS (tiny model)
#CC=sc
#CFLAGS= -ms -DUSE_CATGETS


## Dave Dunfield:
#
## cats does NOT compile with Micro-C/PC.
## therefore, these lines from cmplall.bat have been commented out
#
## Assumes MCSETUP has already been run to generate CC
## and TASM 1,2,3 [4,5*] and TLINK specified.
## *Tasm 5 includes Tasm 4, which is actually used.
#
## REM cc tree.c -M -P MICROC=321 USE_CATGETS=1
## REM cc catgets.c -M -P
## REM cc db.c -M -P
## REM cc get_line.c -M -P
## REM LC tree catgets db get_line
#
## # Micro-C/PC 3.14
## set MCDIR=C:\MC314
## set PATH=C:\MC314;C:\TASM\BIN
## REM set TEMP=C:\TEMP
## cc tree.c -P MICROC=314
## ren tree.com treem314.com
## move treem314.com ..\BIN
#
## # Micro-C/PC 3.15
## set MCDIR=C:\MC315
## set PATH=C:\MC315;C:\TASM\BIN
## REM set TEMP=C:\TEMP
## cc tree.c -P MICROC=315
## ren tree.com treem315.com
## move treem315.com ..\BIN
#
## # Micro-C/PC 3.21
## set MCDIR=C:\MC321
## set PATH=C:\MC321;C:\TASM\BIN
## REM set TEMP=C:\TEMP
## cc tree.c -P MICROC=321
## ren tree.com treem321.com
## move treem321.com ..\BIN
#
## HiTech Pacific-C
#
## cats does NOT compile with Pacific-C.
## therefore, these lines from cmplall.bat have been commented out
#
## set PATH=C:\PACIFIC\BIN
## pacc tree.c
## REM pacc -DUSE_CATGETS tree.c catgets.c db.c get_line.c
## ren tree.exe treepac7.exe
## move treepac7.exe ..\BIN


RM=del


all: 
	$(CC) $(CFLAGS) $(SRC) $(LDLIBS)

clean:
	-$(RM) *.map
	-$(RM) *.tmp

realclean: clean
	-$(RM) *.obj

distclean: realclean
	-$(RM) *.exe
