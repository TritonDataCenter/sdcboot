@echo off

REM for OpenWatcom (note the space is needed after the -f):
REM NOTE that freecom removes trailing spaces from SET commands :-p
REM do not forget to set the WATCOM and INCLUDE env variables,
REM like WATCOM=c:\watcom and INCLUDE=c:\watcom\h
set MAKE=wmake -z
set MAKE_F=-f 

REM for Turbo C++ (no space needed after -f):
REM set MAKE=make
REM set MAKE_F=-f

REM First, we need to make sure nls.mak is up-to-date because the main
REM makefile includes it and it's too late to update it once it's read.
echo %0: updating MKFILES\NLS.MAK
%MAKE% %MAKE_F% mkfiles\makenls.mak

REM This fails to catch the error with OpenWatcom 1.3's make as it must
REM not set the errorlevel.
if errorlevel 1 goto error_returned

REM Now build whatever the user wanted to build
echo %0: building requested targets
%MAKE% %1 %2 %3 %4 %5 %6 %7 %8 %9

goto end

:error_returned
echo Failed to update MAKEFILE.NLS, do you have Perl installed?  If so, make
echo sure the PERL variable in MAKEFILE is correct.  If not, then you should
echo only see this message if you have updated MAKENLS.PL.  Your changes
echo cannot take effect until you install Perl.  To continue working without
echo your changes taking effect, try using TOUCH MAKEFILE.NLS to prevent it
echo from being rebuilt.

:end
