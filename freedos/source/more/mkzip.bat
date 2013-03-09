@if \%1 == \ echo you must give a version number like MKZIP 010
@if \%1 == \ goto end

set _srcfiles=more.c kitten.c kitten.h makefile
set _libfiles=prf.c tcdummy.c talloc.c
set _exefiles=more.exe _more.exe
set _flisttest=*.bat
set _dirlist=HELP\*.* DOC\*.* NLS\*.*


for %%i in (%_srcfiles%) do if not exist %%i goto error_missing_file
for %%i in (%_libfiles%) do if not exist %%i goto error_missing_file
for %%i in (%_exefiles%) do if not exist %%i goto error_missing_file

                    pkzip more%1 %_srcfiles%
if not errorlevel 1 pkzip more%1 %_libfiles%
if not errorlevel 1 pkzip more%1 %_exefiles%
if not errorlevel 1 pkzip more%1 %_flisttest%
if not errorlevel 1 pkzip more%1 -P %_dirlist%
if not errorlevel 1 ren more%1.ZIP more%1.zip
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
