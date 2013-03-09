@echo off
rem Need to use a separate batch file to call MEMTEST.EXE instead of
rem calling it directly from AUTOEXEC.BAT given that we modify AUTOEXEC.BAT
rem inside MEMTEST.EXE
..\memtest %1
