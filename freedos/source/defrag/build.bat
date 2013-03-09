@echo off
rem
rem This batch file compiles the current version of defrag.
rem


echo Compiling fat transformation engine.
cd engine\lib
call makefte
if errorlevel 1 goto exit2  
cd ..\..          
echo Compiling environment checking routines.
cd environ
make
if errorlevel 1 goto exit1  
cd ..
echo Compiling miscellanous routines.
cd misc
make
if errorlevel 1 goto exit1  
cd ..
echo Compiling modules
cd modules
call makemods
if errorlevel 1 goto exit1  
cd ..
echo Compiling Microsoft look alike interface.
cd msdefint
make
if errorlevel 1 goto exit1  
cd ..
echo Compiling command line interface.
cd cmdefint
make
if errorlevel 1 goto exit1  
cd ..
echo Compiling defrag start up code.
cd main
make
if errorlevel 1 goto exit1  
cd ..
echo Compiling module gate.
cd modlgate
make
if errorlevel 1 goto exit  
cd ..
echo Putting everything together
del defrag.lib
tlib defrag.lib + environ\environ.lib
tlib defrag.lib + cmdefint\cmdefint.lib
tlib defrag.lib + msdefint\msdefint.lib
tlib defrag.lib + modules\modbins\modules.lib
tlib defrag.lib + misc\misc.lib
tlib defrag.lib + modlgate\modlgate.lib
tlib defrag.lib + engine\lib\fte.lib
echo Creating defrag
tcc -ml -lm -M -edefrag.exe main\defrag.obj defrag.lib
copy defrag.exe ..\..\bin
del defrag.lib
del defrag.bak
goto exit

:exit2
cd..
:exit1
cd..
:exit

