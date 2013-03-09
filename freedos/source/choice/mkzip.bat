@if \%1 == \ echo you must give a version number like MKZIP 010
@if \%1 == \ goto end

set _srcfiles=src\choice.c src\kitten.c src\kitten.h src\makefile
set _libfiles=src\prf.c src\tcdummy.c src\talloc.c
set _exefiles=bin\choice.exe bin\_choice.exe
set _flisttest=src\*.bat
set _dirlist=HELP\*.* DOC\*.* NLS\*.*


for %%i in (%_srcfiles%) do if not exist %%i goto error_missing_file
for %%i in (%_libfiles%) do if not exist %%i goto error_missing_file
for %%i in (%_exefiles%) do if not exist %%i goto error_missing_file

                    pkzip CHOICE%1 -P %_srcfiles%
if not errorlevel 1 pkzip CHOICE%1 -P %_libfiles%
if not errorlevel 1 pkzip CHOICE%1 -P %_exefiles%
if not errorlevel 1 pkzip CHOICE%1 -P %_flisttest%
if not errorlevel 1 pkzip CHOICE%1 -P %_dirlist%
if not errorlevel 1 ren CHOICE%1.ZIP choice%1.zip
if not errorlevel 1 goto end


@echo error: while zipping
@goto end

:error_missing_file
@echo error: at least one file was missing
:end
@set _srcfiles=
@set _libfiles=
@set _exefiles=
@set _flisttest=
@set _dirlist=
