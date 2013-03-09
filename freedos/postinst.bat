@echo off
if [%errorlevel%]==[] goto error
if "%_CWD%"=="" goto end
if "%cdrom%"=="C:" goto error2
if "%cdrom%"=="c:" goto error2
if "%step6%"=="û" goto end
if "%dosdir%"=="" set dosdir=%_CWD%
if "%fdosdir%"=="" set fdosdir=%_CWD%
if "%rd%"=="" goto detect
if exist %rd%:\NUL set ramdisk=%ramdisk%:
if exist %rd%\NUL goto begin
goto detect

:detect
if not exist %dosdir%\bin\tdsk.* goto error2
%dosdir%\bin\tdsk 100 /C
cls
if not "%errorlevel%"=="255" goto begin
%dosdir%\bin\devload /Q /DD %dosdir%\bin\tdsk.exe
if errorlevel 27 goto error
for %%x in ( D E F G H I J K L M N O P Q R S T U V W X Y Z ) do if errorlevel H%%x set rd=%%x:
goto detect

:begin
echo Entering the real stuff, using RAMDISK %rd% from [%_CWD%]
if "%_CWD%"=="%rd%\" goto skipcopy
if exist %rd%\postinst.bat goto ramstart
for %%x in ( %0 %0.bat %dosdir%\%0 %dosdir%\%0.bat ) do if exist %%x copy %%x %rd%\postinst.bat
goto ramstart

:ramstart
CDD %rd%\
if exist %rd%\postinst.bat %rd%\postinst.bat
goto end

:error
echo Unable to determine driveletter, return status code %errorlevel%
goto end

:error2
echo Something went very very wrong..oops!
goto end

:skipcopy
echo We're running from RAMDISK %rd% right now
CDD %dosdir%
CD \
%dosdir%\bin\move /y %dosdir% %dosdir%.tmp
echo yes > %dosdir%
for %%x in ( * ) do if exist %%x.tmp\NUL set /U MYDIR=%%x
if exist %dosdir% del %dosdir%
ren %dosdir%.tmp %mydir%
echo CWD = %_CWD%
for %%x in ( C D E F G H I J K L M N O P Q R S T U V W X Y Z ) do if "%_CWD%"=="%%x:\" set destdrv=%%x:
echo MYDIR: [%mydir%] at drive [%destdrv%]
CDD %rd%\
%dosdir%\bin\xmssize 5>NUL
if errorlevel 15 set XMS=15
if not errorlevel 15 set XMS=%errorlevel%
for %%x in ( 1 2 ) do if "%cputype%"=="80%%x86" goto init
set cputype=80386
goto init

rem #1: Copy kernel
rem #2: Search bootup files
rem  -2a: comfile (comspec/shell/command interpreter, command.com)
rem  -2b: autofile (bootup automation script, autoexec.bat)
rem  -2c: cfgfile (device driver loading script, [fd]config.sys)
rem #3: Create bootup files
rem #4: Translate programs and install them
rem #5: Clean up

:init
set /U path=%fdosdir%;%fdosdir%\bin
for %%x in ( %dosdir% %fdosdir% ) do set /U nlspath=%%x
if "%bootsrc%"=="" set /U path=%fdosdir%;%fdosdir%\bin
if "%bootsrc%"=="diskette" set /U path=%fdosdir%;%fdosdir%\bin
if not "%BOOT_IMAGE%"=="" set toolpath=a:\freedos
rem Whoever allowed environment variables to end with a space..
for %%x in ( EN %lang% ) do set /U lang=%%x
rem ctty nul
set nlspath=%ramdisk%
if not exist %fdosdir%\localize.%lang% copy %fdosdir%\localize.txt %fdosdir%\localize.%lang%
if exist localize.%lang% goto infomenu
copy /y %fdosdir%\localize.*
for %%x in ( localize.en localize.txt ) do if exist %fdosdir%\%%x copy /y %fdosdir%\%%x %ramdisk%\localize.%lang%
goto infomenu




:infomenu
for %%x in ( %dosdir% %fdosdir% ) do if not exist %nlspath%\localize.%lang% set nlspath=%%x
ctty con
cls
for %%x in ( 0 1 2 ) do localize 3.%%x
for %%x in ( 1 2 3 4 5 6 ) do if "%step%%x%"=="û" localize 4.%%x @@@ [+] Menu step %%x [LANG=%lang%],[NLSPATH=%nlspath%],[PATH=%path%]
for %%x in ( 1 2 3 4 5 6 ) do if "%step%%x%"=="" localize 5.%%x @@@ [-] Menu step %%x [LANG=%lang%],[NLSPATH=%nlspath%],[PATH=%path%]
REM for %%x in ( %dosdir% %fdosdir% ) do set nlspath=%%x
REM for %%x in ( set pause ) do if errorlevel 1 %%x
%fdosdir%\bin\choice /c:.2 /n /t:.,1 > nul
for %%x in ( 1 2 3 4 5 6 7 ) do if "%step%%x%"=="" goto step%%x
goto end

:step1
set step1=û
set copycmd=/y
copy %fdosdir%\bin\kernel.sys %destdrv%\kernel.sys
goto infomenu

:step2
set /U autofile=%fdosdir%\fdauto.bat
set /U cfgfile=%destdrv%\fdconfig.sys
if not exist %cfgfile% goto endloop
goto endloop
set testn=0
:loop
if not exist %destdrv%\fdconfig.%testn% goto endloop
add %testn% 1>NUL
set testn=%errorlevel%
goto loop
:endloop
if not "%testn%"=="" copy %destdrv%\fdconfig.sys %destdrv%\fdconfig.%testn%
set testn=
set /U comfile=%fdosdir%\bin\command.com
goto step2b

:step2b
echo @echo off > %autofile%
echo SET LANG=%LANG% >> %autofile%
if exist %fdosdir%\MTCP.CFG echo SET MTCPCFG=%fdosdir%\MTCP.CFG >> %autofile%
if exist %fdosdir%\WATTCP.CFG echo SET WATTCP.CFG=%fdosdir% >> %autofile%
if "%cputype%"=="" set cputype=80386
REM if "%cputype%"=="80386" if exist %fdosdir%\bin\banner2.com echo REM %fdosdir%\BIN\BANNER2 >> %autofile%
for %%x in ( 0 1 2 3 4 5 6 7 8 9 16 17 18 ) do localize 2.%%x >> %autofile%
rem LFN support in FreeCOM isn't 100% stable yet
if exist %fdosdir%\bin\DOSLFN.* echo LH DOSLFN >> %autofile%
rem if not !%cp%==! localize 2.13 >> %autofile%
rem NLS stuff below, won't work before kernel 2041 likely, also waste of memory on US-systems
rem due to NLSFUNC, DISPLAY and KEYB consuming memory
if "%cp%"=="" set cp=858
if "%cpxfile%"=="" set cpxfile=EGA
echo REM NLSFUNC %fdosdir%\BIN\COUNTRY.SYS>>%autofile%
echo REM DISPLAY CON=(EGA),%cp%,2)>>%autofile%
echo REM MODE CON CP PREP=((%cp%) %fdosdir%\CPI\%cpxfile%.CPX)>>%autofile%
echo REM KEYB US,%cp%,%fdosdir%\bin\keyboard.sys>>%autofile%
echo REM CHCP %cp%>>%autofile%
echo REM LH PCNTPK INT=0x60>>%autofile%
echo REM DHCP>>%autofile%
set cpxfile=
for %%x in ( 19 20 21 22 23 24 25 26 ) do localize 2.%%x >> %autofile%
ECHO SET CFGFILE=%cfgfile%>> %autofile%
set step2=û
if exist %destdrv%\metakern.sys goto infomenu
if exist %destdrv%\autoexec.bat goto step2c
rem Moving file and adjusting %autofile%
rem Only use \AUTOEXEC.BAT if it doesn't exist already and no other DOS-flavour is detected.
copy /y %autofile% %destdrv%\autoexec.bat
if exist %destdrv%\autoexec.bat del %fdosdir%\fdauto.bat
if exist %destdrv%\autoexec.bat set /U autofile=%destdrv%\autoexec.bat
goto step2c


:step2c
rem if not exist %destdrv%\config.sys ren %destdrv%\fdconfig.sys config.sys
rem if exist %destdrv%\config.sys set /U cfgfile=%destdrv%\config.sys
rem if exist %destdrv%\fdconfig.sys set /U cfgfile=%destdrv%\fdconfig.sys
goto infomenu


:step3
pushd
CDD %fdosdir%\NLS
rem MakeCMD creates command.com by binary copying *.cln and strings.dat
if exist makecmd.bat call makecmd.bat
if exist command.com copy /y command.com %comfile%
if not exist \command.com copy %comfile% \
if exist %fdosdir%\nls\command.com del %fdosdir%\nls\command.com
if not exist %fdosdir%\bin\command.com copy /y c:\command.com %fdosdir%\bin\command.com
set copycmd=/y
popd
set step3=û
goto infomenu

:step4
cls
echo !COUNTRY=001,%cp%,%fdosdir%\BIN\COUNTRY.SYS > %cfgfile%
echo !SET DOSDIR=%fdosdir%>> %cfgfile%
for %%x in ( 3 4 5 6 7 ) DO LOCALIZE 0.%%x >> %cfgfile%
if "%cputype%"=="" set cputype=80386
set /U cmd_dir=C:\
if not "%comfile%"=="C:\COMMAND.COM" set /U cmd_dir=%fdosdir%\bin
goto s4_%cputype%

:s4_80186
for %%x in ( 1 2 3 ) DO LOCALIZE 0.1%%x >> %cfgfile%
rem swap FreeCOM (non-XMS version!) to harddisk when using CALL /S prog.exe
ECHO 1?SHELL=%fdosdir%\bin\KSSF.COM %comfile% /K %autofile% >> %cfgfile%
set step4=û
goto infomenu

:s4_80286
copy %fdosdir%\NLS\command.com %comfile%
for %%x in ( 1 2 ) DO LOCALIZE 0.2%%x >> %cfgfile%
ECHO 1?DEVICE=%fdosdir%\BIN\FDXMS286.SYS >> %cfgfile%
for %%x in ( 5 ) DO LOCALIZE 0.2%%x >> %cfgfile%
ECHO 12?SHELL=%comfile% %cmd_dir% /P=%autofile% >> %cfgfile%
goto sub4keyb

:s4_80386
if exist %fdosdir%\bin\kernel32.sys copy /y %fdosdir%\bin\kernel32.sys %destdrv%\kernel.sys
for %%x in ( 0 1 2 3 ) DO LOCALIZE 0.3%%x >> %cfgfile%
if exist %fdosdir%\packages\grubx\nul localize 0.34 >> %cfgfile%
if exist %fdosdir%\packages\grubx\nul echo 5?DEVICE=%fdosdir%\bin\grub.exe >> %cfgfile%
for %%x in ( 5 6 7 ) DO LOCALIZE 0.3%%x >> %cfgfile%
ECHO 1?DEVICE=%fdosdir%\BIN\JEMMEX.EXE NOEMS X=TEST I=TEST NOVME NOINVLPG>> %cfgfile%
ECHO 2?DEVICE=%fdosdir%\BIN\HIMEMX.EXE >> %cfgfile%
ECHO 2?DEVICE=%fdosdir%\BIN\JEMM386.EXE X=TEST I=TEST I=B000-B7FF NOVME NOINVLPG>> %cfgfile%
ECHO 3?DEVICE=%fdosdir%\BIN\XMGR.SYS >> %cfgfile%
REM IF EXIST %fdosdir%\BIN\BLACKOUT.EXE ECHO REM 123?INSTALL=%fdosdir%\BIN\BLACKOUT.EXE >> %cfgfile%
REM IF EXIST %fdosdir%\BIN\BANNER1.COM ECHO REM 123?INSTALL=%fdosdir%\BIN\BANNER1.COM >> %cfgfile%
IF EXIST %fdosdir%\PACKAGES\MORESYSX\NUL ECHO 123?DEVICEHIGH=%fdosdir%\BIN\MORESYS.SYS >> %cfgfile%
if NOT exist %fdosdir%\BIN\4DOS.COM echo ECHO 34?SHELL=%comfile% %cmd_dir% /E:1024 /P=%autofile% >> %cfgfile%
if exist %fdosdir%\BIN\4DOS.COM echo 3?SHELL=%fdosdir%\bin\4dos.com %fdosdir%\bin /E:1024 /P:%autofile%  >> %cfgfile%
if exist %fdosdir%\BIN\4DOS.COM echo 4?SHELL=%comfile% %cmd_dir% /E:1024 /P=%autofile% >> %cfgfile%
ECHO 12?SHELLHIGH=%comfile% %cmd_dir% /E:1024 /P=%autofile% >> %cfgfile%
goto sub4keyb
 
:sub4keyb
set step4=û
if "%keybfile%"=="" goto infomenu
if not exist %fdosdir%\bin\KEYB.* goto infomenu
echo LH KEYB %kblayout%,,%keybfile% %keybrest% >> %autofile%
set keybrest=
set kblayout=
set keybfile=
cdd -
goto infomenu

:step5
set /U nlspath=%fdosdir%
REM localize 7.4 %cfgfile%, %autofile%, %comfile% >> %autofile%
for %%x in ( 0 1 2 3 4 5 6 7 8 9 ) do localize 7.%%x >> %autofile%
for %%x in ( cmd_dir cp comfile ) do set %%x=
goto step5a

:step5a
set step5=û
if not exist %fdosdir%\bin\ctmouse.exe goto infomenu
localize 3.3
REM ctty nul
%fdosdir%\bin\ctmouse.exe
if errorlevel 5 goto infomenu
copy /y %fdosdir%\bin\ctmouse.exe %fdosdir%\bin\mouse.exe 
if exist %fdosdir%\bin\ctmouse\ctm-%lang%.exe copy /y %fdosdir%\bin\ctmouse\ctm-%lang%.exe %fdosdir%\bin\mouse.exe
REM if exist %fdosdir%\bin\mouse.exe ECHO if not "%%config%%"=="4" mouse >> %autofile%
set dosdir=%fdosdir%
ctty con
set /U path=%fdosdir%\bin;%path%
REM call postset.bat
goto infomenu

:step6
set step6=û
cd \
cdd %fdosdir%
if not exist %fdosdir%\temp\nul md %fdosdir%\temp
if not exist %fdosdir%\SOURCE\*.zip goto infomenu
CDD %fdosdir%\SOURCE
REM for %%x in ( *.ZIP ) DO %fdosdir%\bin\TUNZ %%x
CDD %fdosdir%
goto infomenu

:step7
if not exist %fdosdir%\bootsect.bss SYS C: /BOOTONLY /DUMPBS %fdosdir%\bootsect.bss
if not "%ramdrive%"=="C:" SYS C: C: %fdosdir%\freedos.bss /BOOTONLY
if "%ramdrive%"=="C:" SYS C: C: %fdosdir%\freedos.bss /BOOTONLY /BOTH
for %%x in ( com cfg ) do if not exist %fdosdir%\bin\syslinux.%%x goto step8
REM if not exist %fdosdir%\syslinux.bss syslinux -d /%mydir% C: %fdosdir%\syslinux.bss
copy /y %fdosdir%\bin\syslinux.cfg %fdosdir%\syslinux.cfg
goto step8

:step8
ctty con
if not exist %fdosdir%\bin\choice.* goto reboot
REM for %%x in ( %dosdir% %fdosdir% ) do set nlspath=%%x
REM for %%x in ( 1 2 3 4 ) do localize 6.%%x
REM %fdosdir%\bin\choice /N /C:NYJI /T:Y,8 .F.R.E.E.D.O.S.:
REM set reboot=%errorlevel%
REM for %%x in ( 1 ) do if "%reboot%"=="%%x" goto clean_up
REM goto reboot
localize 8.10
echo (restore by "SYS C: /BOOTONLY /RESTORBS %fdosdir%\BOOTSECT.BSS").
CDD %ramdisk%\
OSCHECK C: > NUL
for %%x in ( %errorlevel% 12 25 12 26 27 28 29 30 ) do localize 8.%%x
REM echo 4) Skip updating bootsector area and reboot system to finalise installation
REM echo 5) Drop to commandline (thus skipping bootsector update) and complete install
REM pause
%fdosdir%\bin\choice /N /C:12345 /T:1,99 Please select now:
if "%errorlevel%"=="5" goto clean_up
if "%errorlevel%"=="4" goto reboot
if "%errorlevel%"=="3" set bs=%fdosdir%\bootsect.bss
if "%errorlevel%"=="2" set bs=%fdosdir%\syslinux.bss
if "%errorlevel%"=="1" set bs=%fdosdir%\freedos.bss
REM echo Backend not implemented yet for user selection %errorlevel%
if "%bs%"=="" goto clean_up
if not exist %bs% goto clean_up
if exist %bs% SYS C: /RESTORBS %bs%
echo Saved bootsector %bs% was succesfully written to drive C:
set bs=
goto reboot

:reboot
rem This case handles a minimalistic installation where CHOICE is not installed.
if not exist %fdosdir%\bin\choice.* pause FreeDOS Installation has finished successfully, press a key to restart
%fdosdir%\bin\FDISK /REBOOT
%fdosdir%\bin\FDAPM /COLDBOOT
cls
echo Automatic reboot failed: please restart computer manually by pressing 
echo RESET button on your computer, or by pressing CTRL-ALT-DEL on your keyboard
echo.
goto clean_up

:clean_up
for %%x in ( 1 2 3 4 5 6 7 ) do set step%%x=
for %%x in ( reboot ) do set %%x=
set path=%fdosdir%\bin;%fdosdir%
set nlspath=%fdosdir%\NLS
set tmp=%fdosdir%\temp
set temp=%tmp%
goto end

:end
