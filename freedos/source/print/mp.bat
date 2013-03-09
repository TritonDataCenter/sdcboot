masm5 print.asm,,,,
link print.obj,,,,,
exe2bin print.exe print.com
del print.exe
del *.map
del *.crf
del *.lst
del *.obj
