@echo off

rem
rem This Source Code Form is subject to the terms of the Mozilla Public
rem License, v. 2.0. If a copy of the MPL was not distributed with this
rem file, You can obtain one at http://mozilla.org/MPL/2.0/.
rem

rem
rem Copyright (c) 2014, Joyent, Inc.
rem

c:\dos\freedos\getargs > c:\dos\tmp.bat
call c:\dos\tmp.bat > nul
del c:\dos\tmp.bat > nul

if "%console%"=="" goto end
if "%console%"=="text" goto end
if "%console%"=="graphics" goto end

if "%console%"=="ttya" set condev=com1
if "%console%"=="ttyb" set condev=com2
if "%console%"=="ttyc" set condev=com3
if "%console%"=="ttyd" set condev=com4

echo Redirecting console to %condev%
ctty %condev%
:end

SET DIRCMD=/P /OGN /4 
SET LANG=EN
SET PATH=C:\DOS\FREEDOS;C:\DOS\GNU;A:\

:initfdum
cls
echo Joyent Firmware Diagnostics and Upgrade Mode [FreeDOS]
echo\
if not exist c:\firmware\main.bat goto nofdum
call c:\firmware\main.bat
goto initfdum
:nofdum

echo No firmware bundle found; dropping to FreeDOS shell.
echo A: is a nonpersistent boot ramdisk
echo C: is /mnt/usbkey
echo Useful commands may be found in C:\DOS.  To reboot, utter 'fdapm warmboot'.
echo\
