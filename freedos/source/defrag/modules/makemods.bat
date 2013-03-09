@echo off
rem
rem This batch file creates the different modules.
rem
cd chkfat
make
if errorlevel 1 goto exit
cd ..\infofat
make
if errorlevel 1 goto exit
cd ..\sortfat
make
if errorlevel 1 goto exit
cd ..\dfragfat
make
if errorlevel 1 goto exit
cd ..\dtstruct
make
if errorlevel 1 goto exit
cd ..\modbins
make
:exit
cd ..