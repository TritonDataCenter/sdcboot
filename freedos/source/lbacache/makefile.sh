#!/bin/bash

# old NASM versions need -d, newer ones -D ...
# Options mean:
# DEBUGNB to see when cache becomes full
# REDIRBUG if redirection of messages does not work on old FreeDOS kernels
# FLOPCHGMSG to enable floppy disk changed message
# SANE_A20 to suppress waiting for keyboard controller ready before XMS
#   copy - the waiting works around a Bochs 1.x BIOS bug.
# FORCEFDD to allow caching even floppy drives without change line, like
#   in dosemu. You MUST do LBAcache flush after removing a floppy and before
#   inserting the next floppy manually in that case.
# MUTEFDWRERR to suppress the message about cache flush after write error
#   on floppy disks - partial flush would be better, please contribute it...
# BIGGERBINS / ... to select other bin sizes than 4096 bytes

echo Now creating lbacache.com...

if true; then
  echo Creating normal LBAcache...
  nasm -DBIGGERBINS -DSANE_A20=1 -DMUTEFDWRERR=1 -o lbacache.com lbacache.asm
else
  echo Creating DEBUG version of LBAcache, including workarounds...
  nasm -DDEBUGNB=1 -DSANE_A20=1 -DFORCEFDD=1 -DFLOPCHGMSG=1 -o lbacachd.com lbacache.asm
fi

upx --8086 lbacache.com
echo lbacache.com has been updated.

ls -l *.com

echo 
echo Version: "$(grep 'define.*VERSION' < lbacache.asm)"
echo Please move lbacach*.com to ../../BIN/
echo if you want to make them the active version now.
echo Remember: If you have a lbacach*.sys, it is a pre-5/2004 version.
