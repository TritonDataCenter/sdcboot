@ECHO OFF
rem No longer using TASM/ArrowASM and TLink/WarpLink...
rem tasm devload.asm devload.obj nul.lst nul.crf
rem asm devload.asm devload.obj nul.lst nul.crf
rem warplink /c devload.obj
rem tlink /t devload.obj , , nul.map ,
rem
rem nasm - open source assembler
nasm -O5 -o devload.com devload.asm
rem upx - open source binary packer
upx --8086 --best devload.com
