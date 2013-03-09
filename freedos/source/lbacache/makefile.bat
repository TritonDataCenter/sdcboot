@echo off

rem if you do not have UPX ( http://upx.sf.net ), just ignore
rem the UPX error messages. Binaries will be bigger but working.

rem uncomment the following line to get the debug version
rem goto debugv
goto normalv

rem Options mean:
rem DEBUGNB to see when cache becomes full
rem REDIRBUG if redirection of messages does not work on old FreeDOS kernels
rem FLOPCHGMSG to enable floppy "disk changed" message
rem SANE_A20 to suppress waiting for keyboard controller ready before XMS
rem   copy - the waiting works around a Bochs 1.x BIOS bug.
rem FORCEFDD to allow caching even floppy drives without change line, like
rem   in dosemu. You MUST do LBAcache flush after removing a floppy and before
rem   inserting the next floppy manually in that case.
rem MUTEFDWRERR to suppress the message about cache flush after write error
rem   on floppy disks - partial flush would be better, please contribute it...
rem BIGGERBINS... to select other bin sizes than 4096 bytes, see binsel.asm

rem ***** ***** please do not edit below this line ***** *****

:debugv
echo Now creating debug version lbacachD.com...
nasm -DDEBUGNB=1 -DSANE_A20=1 -DFORCEFDD=1 -DFLOPCHGMSG=1 -o lbacachd.com lbacache.asm
upx --8086 lbacachd.com
echo Debug version lbacachD.com has been updated.
goto made

:normalv
echo Now creating lbacache.com...
nasm -DBIGGERBINS -DSANE_A20=1 -DMUTEFDWRERR=1 -o lbacache.com lbacache.asm
upx --8086 lbacache.com
echo lbacache.com has been updated.

:made

rem uncache.com is deprecated and now part of lbacache.com ...

if not x%1==xinstall goto done

if exist lbacache.com copy lbacache.com c:\freedos\bin

if exist lbacachd.com copy lbacachd.com c:\freedos\bin

:done

echo To get a debug version with some extra messages and workarounds
echo please edit the top of this makefile.bat file as explained there.
echo VERSION is:
find "VERSION " lbacache.asm
