@echo off
rem
rem This batch file cleans the current version of defrag.
rem
echo Cleaning fat transformation engine.
cd engine\lib
call cleanfte
cd ..\..
echo Cleaning environment checking routines.
cd environ
make clean
cd ..
echo Cleaning miscellanous routines.
cd misc
make clean
cd ..
echo Cleaning modules
cd modules
call cleanmod
cd ..
echo Cleaning Microsoft defrag look alike interface.
cd msdefint
make clean
cd ..
echo Cleaning command line interface.
cd cmdefint
make clean
cd ..
echo Cleaning defrag start up code.
cd main
make clean
cd ..
echo Cleaning module gate.
cd modlgate
make clean
cd ..
del defrag.lib
del defrag.exe 
