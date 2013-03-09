@ECHO OFF
ECHO Tree v3.7 for FreeDOS compile using all tested compilers.
ECHO Expects to be in the directory with tree.c
ECHO Any compilers you lack should be commented (REM) out.
ECHO .
ECHO To compile tree, setup your compiler environment properly, then
ECHO compile specifying tree.c as the source file.
ECHO .
ECHO Type ctrl-C if you do not wish to continue or you have not edited this file.
ECHO .
ECHO If you are not compiling with CATS support then ignore the
ECHO message about catgets.obj db.obj get_line.obj not found
ECHO or comment out the appropriate del lines below.
pause

ECHO Saving PATH environement variable to be restored later.
ECHO @ECHO OFF > svpath.bat
ECHO SET PATH=%PATH% >> svpath.bat

ECHO .

ECHO Borland:

ECHO Turbo C 2.01
set PATH=C:\TC201;
REM tcc -mt -lt tree.c
tcc -mt -lt -w-par -DUSE_CATGETS tree.c catgets.c db.c get_line.c
ren tree.com treetc2.com
move treetc2.com ..\BIN
del tree.obj
del catgets.obj
del db.obj
del get_line.obj
pause

ECHO Turbo C/C++ 1.01
set PATH=C:\TC101\BIN;
REM tcc -mt -lt tree.c
tcc -mt -lt -w-par -DUSE_CATGETS tree.c catgets.c db.c get_line.c
ren tree.com treetcc1.com
move treetcc1.com ..\BIN
del tree.obj
del catgets.obj
del db.obj
del get_line.obj
pause

ECHO Turbo C/C++ 3.0
set PATH=C:\TC30\BIN;
REM tcc -mt -tDc tree.c
tcc -mt -tDc -w-par -DUSE_CATGETS tree.c catgets.c db.c get_line.c
ren tree.com treetcc3.com
move treetcc3.com ..\BIN
del tree.obj
del catgets.obj
del db.obj
del get_line.obj

ECHO Borland C/C++ 3.1
set PATH=C:\BORLANDC\BIN;
REM bcc -mt -tDc tree.c
bcc -mt -tDc -w-par -DUSE_CATGETS tree.c catgets.c db.c get_line.c
ren tree.com treebcc3.com
move treebcc3.com ..\BIN
del tree.obj
del catgets.obj
del db.obj
del get_line.obj

ECHO .
pause

ECHO Borland C/C++ 4.5 DOS
set PATH=C:\BC45\BIN;
REM bcc -mt -tDc tree.c
bcc -mt -tDc -w-par -DUSE_CATGETS tree.c catgets.c db.c get_line.c
ren tree.com treebc45.com
move treebc45.com ..\BIN
del tree.obj
del catgets.obj
del db.obj
del get_line.obj

ECHO Borland C/C++ 4.5 WIN32
set PATH=C:\BC45\BIN;
REM bcc32 -tWC tree.c
bcc32 -tWC -w-par -DUSE_CATGETS tree.c catgets.c db.c get_line.c
ren tree.exe treebc45.exe
move treebc45.exe ..\BIN
del tree.obj
del catgets.obj
del db.obj
del get_line.obj

ECHO .
pause

ECHO Borland C/C++ 5.5
set PATH=c:\Borland\bcc55\bin;
REM bcc32 -Lc:\Borland\bcc55\Lib\PSDK -tWC -etree.exe tree.c user32.lib
bcc32 -Lc:\Borland\bcc55\Lib\PSDK -tWC -w-par -w-aus -etree.exe -DUSE_CATGETS tree.c catgets.c db.c get_line.c user32.lib
ren tree.exe treebc55.exe
move treebc55.exe ..\BIN
del tree.obj
del tree.tds
del catgets.obj
del db.obj
del get_line.obj

ECHO .
pause


ECHO Microsoft:

ECHO Visual C/C++ 5.0
REM Ensure VCVARS32.BAT has been ran to setup paths and environment
CALL c:\Progra~1\Devstu~1\vc\bin\VCVARS32.BAT
REM cl /Fe"tree.exe" /nologo /ML /GX /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" tree.c /link user32.lib
cl /Fe"tree.exe" /nologo /ML /GX /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /D "USE_CATGETS" tree.c catgets.c db.c get_line.c /link user32.lib
ren tree.exe treevc5.exe
move treevc5.exe ..\BIN
del tree.obj
del catgets.obj
del db.obj
del get_line.obj

ECHO MS C 6.0 (DOS)
REM Must also setup its environment
CALL C:\C600\BIN\NEW-VARS.BAT
REM cl tree.c
cl /D "USE_CATGETS" tree.c catgets.c db.c get_line.c
REM c:\winnt\system32\forcedos cl /D "USE_CATGETS" tree.c catgets.c db.c get_line.c
ren tree.exe treemsc6.exe
move treemsc6.exe ..\BIN

ECHO .
pause

ECHO Digital Mars [Symantec]
set PATH=C:\dm\bin

ECHO Win32
REM sc -mn tree.c
sc -mn -DUSE_CATGETS tree.c catgets.c db.c get_line.c
ren tree.exe treedm8w.exe
move treedm8w.exe ..\BIN
del tree.obj
del catgets.obj
del db.obj
del get_line.obj

ECHO DOS (tiny model)
REM sc -mt tree.c
REM ren tree.com treedm8.com
sc -ms -DUSE_CATGETS tree.c catgets.c db.c get_line.c
ren tree.exe treedm8.exe
move treedm8.exe ..\BIN
del tree.obj
del catgets.obj
del db.obj
del get_line.obj

del tree.map

ECHO .
pause


ECHO Dave Dunfield:
ECHO Assumes MCSETUP has already been run to generate CC
ECHO and TASM 1,2,3 [4,5*] and TLINK specified.
ECHO *Tasm 5 includes Tasm 4, which is actually used.


ECHO cats does NOT compile with Micro-C/PC.
REM cc tree.c -M -P MICROC=321 USE_CATGETS=1
REM cc catgets.c -M -P
REM cc db.c -M -P
REM cc get_line.c -M -P
REM LC tree catgets db get_line

ECHO Micro-C/PC 3.14
set MCDIR=C:\MC314
set PATH=C:\MC314;C:\TASM\BIN
REM set TEMP=C:\TEMP
cc tree.c -P MICROC=314
ren tree.com treem314.com
move treem314.com ..\BIN

ECHO Micro-C/PC 3.15
set MCDIR=C:\MC315
set PATH=C:\MC315;C:\TASM\BIN
REM set TEMP=C:\TEMP
cc tree.c -P MICROC=315
ren tree.com treem315.com
move treem315.com ..\BIN

ECHO Micro-C/PC 3.21
set MCDIR=C:\MC321
set PATH=C:\MC321;C:\TASM\BIN
REM set TEMP=C:\TEMP
cc tree.c -P MICROC=321
ren tree.com treem321.com
move treem321.com ..\BIN

ECHO .
pause

ECHO HiTech Pacific-C
ECHO cats does NOT compile with Pacific-C.
set PATH=C:\PACIFIC\BIN
pacc tree.c
REM pacc -DUSE_CATGETS tree.c catgets.c db.c get_line.c
ren tree.exe treepac7.exe
move treepac7.exe ..\BIN

ECHO .
pause

ECHO DJGPP:
ECHO TODO

ECHO .
REM pause

ECHO Compilers with POSIX support:
ECHO TODO

ECHO .
REM pause

ECHO Compilers with generic Win32 (C api) support:
ECHO Supported without cats but not used, requires USE_WIN32 defined to enable it.

ECHO .

ECHO Now cleaning Up
CALL svpath.bat
del svpath.bat
ECHO Done.
pause
