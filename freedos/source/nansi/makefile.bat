@echo off
rem Batch file version based on the NANSI makefile.
rem Useful if you have no MAKE tool. Otherwise, better use that.
rem Using the Arrowsoft Assembler here - Freeware and MASM compatible.
rem Using the Turbo C 2.x linker TLINK as LINK,
rem because VAL does not accept files without stack segment.
echo Replace ASM by MASM and VAL by LINK if you want.
echo If you have MAKE, use that, not this batch script.
echo.
echo NANSI: nansi.asm
asm nansi;
echo NANSI_P: nansi_p.asm
asm nansi_p;
echo NANSI_F: nansi_f.asm
asm nansi_f;
echo NANSI_I: nansi_i.asm
asm nansi_i;
echo nansi.sys:	nansi.obj nansi_p.obj nansi_f.obj nansi_i.obj
tlink /m nansi nansi_p nansi_f nansi_i;
exe2bin nansi nansi.sys
del nansi.exe
del *.obj
