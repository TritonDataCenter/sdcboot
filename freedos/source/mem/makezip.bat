@echo off
REM This should be called via 'make mem.zip' to ensure that all the files
REM that need to go into the zip file are compiled.
cd ..\..
zip source\mem\mem.zip @source\mem\filelist.txt -x */cvs/ *~
REM Must return to original directory to avoid confusing OpenWatcom WMAKE
cd source\mem
