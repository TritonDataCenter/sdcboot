@echo off
rem
rem This batch file creates the different modules.
rem
cd chkfat
make clean
cd ..\infofat
make clean
cd ..\sortfat
make clean
cd ..\dfragfat
make clean
cd ..\modbins
make clean
cd ..\dtstruct
make clean
cd ..
