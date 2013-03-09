@ECHO OFF
ECHO Building SYS
MKDIR BIN 2> NUL
PUSHD .
IF "%COMPILER%"=="" GOTO SET_VARS

:BOOTSECT
ECHO building boot sectors %NASM%
CD BOOT
wmake -h -ms %1
IF NOT %ERRORLEVEL%==0 GOTO ERROR_BS
ECHO boot sectors successfully built
CD ..

:SYS
ECHO building sys program
CD SYS
wmake -h -ms %1
IF NOT %ERRORLEVEL%==0 GOTO ERROR_SYS
ECHO sys successfully built
CD ..

GOTO DONE


:SET_VARS
SET COMPILER=WATCOM
SET NASM=C:\NASM\NASM.EXE
GOTO BOOTSECT

:ERROR_BS
ECHO Failure assembling boot sectors
GOTO DONE

:ERROR_SYS
ECHO Failure compiling sys program
GOTO DONE

:DONE
POPD
ECHO done.

:CLEAR_VARS
SET COMPILER=
SET NASM=
