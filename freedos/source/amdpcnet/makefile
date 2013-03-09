#
# makefile for Packet Drivers for Turbo Make.
#
# MAKE  Version 3.6
# TASM  Version 3.1
# TLINK Version 5.1
#
# The multiple language support is implemented
# To select the specific language use the /dXXXXX parameter
#

# if you use Borland products
ASM	=C:\TASM\tasm /dENGLISH
LINK	=C:\TASM\tlink

# If you use Microsoft products (some lossage will ensue).  Great
# lossage will ensue if you use masm 4.0, but most things will
# assemble.  For those that do not, simply create the .obj file by
# hand, then create the .com file.  That will enable make to skip
# over them.  Please send the editor email
# <nelson@crynwr.com>, or a FAX <315-268-1925> if Microsoft
# support is useful to you.
#ASM	=	masm
#LINK	=	link

# These are split into ASM and C files because you may not have Turbo C.
# If you don't, then remove the "c" dependency from the following:


.asm.obj:
	$(ASM) $*;


pcntpk.com: head.obj pcntpk.obj tail.obj pninit.obj
	$(LINK) head pcntpk tail pninit,pcntpk/m;
	exe2com pcntpk
	del pcntpk.exe


head.obj: head.asm defs.asm

tail.obj: tail.asm defs.asm printnum.asm decout.asm digout.asm chrout.asm \
	  timeout.asm verifypi.asm getnum.asm getdig.asm skipblk.asm \
	  printea.asm

pcntpk.obj: pcntpk.asm defs.asm timeout.asm msg.asm


pninit.obj: pninit.asm defs.asm

clean: nul
	del *.obj
	del pcntpk.com
	del pcntpk.map

