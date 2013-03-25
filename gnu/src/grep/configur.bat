@echo off
echo configuring GNU grep for compiling on MS-DOS
echo This requires the patch program -- hit ^C if you don't have this utility.
pause
ren make.com vmsmake.com
copy msdos\config.h . >nul
copy msdos\makefile . >nul
patch <msdos\patches.dos
echo To build GNU grep, type "nmake" for Microsoft C (7.0+) or "make 386" for
echo GNU C (DJGPP 1.12+).  Both versions can be built by "make all 386" or
echo "nmake all 386".  The Microsoft C version is named GREP.EXE, and the
echo DJGPP version is named GREP386.EXE.  Testing the compiled programs can
echo be done by "nmake check" or "make check386".
