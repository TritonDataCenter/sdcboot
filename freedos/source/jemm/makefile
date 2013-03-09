#
# To build Jemm386 and JemmEx, you will need:
#
# Tool        Default (recommended) Alternatives
#-----------------------------------------------------------------
# Assembler   JWasm                 Masm v6.1 or better
# OMF Linker  Open Watcom Wlink     MS Link (link16.exe)
# COFF Linker Open Watcom Wlink     MS Link (link.exe), PoLink
# Make        Open Watcom Wmake     MS Nmake
#
# note: Wmake must be used with the -ms option!
#
# note: WLink < v1.8 shouldn't be used as COFF linker. It contains a bug 
#       which might cause unexpected results in Jemm.
#
# Jemm consists of 2 parts. which are created separately: the 32-bit
# part is the true Jemm application ( the "v86-monitor" program ) -
# its sources are assembled and linked to Jemm32.bin. The 16-bit part
# is (mostly) used during the initialization phase and - except for a 
# small stub - not necessary to be kept in memory. As a result, the
# make process consists of:
#
#  1. assemble the 32-bit assembly sources
#  2. link 32-bit modules to Jemm32.bin (format is a PE binary).
#  3. use EXTRSECT to extract the section contents of this binary to file
#     $(OUTDx)\_Jemm32.inc.
#  4. assemble the 16-bit assembly sources (file Jemm16.asm will include
#     $(OUTDx)\_Jemm32.inc).
#  5. link 16-bit modules (Jemm.obj, Jemm16.obj) to Jemm386/JemmEx

NAME1=JEMM386
NAME2=JEMMEX

!ifndef DEBUG
DEBUG=0
!endif

# select assembler, JWasm or Masm, default is JWasm

!ifndef MASM
MASM=0
!endif

!if $(MASM)
ASM=ml.exe  
!else
ASM=jwasm.exe
!endif

# select 32-bit COFF linker, default WLink

!ifndef WLINK32
WLINK32=0
!endif
!ifndef MSLINK32
MSLINK32=0
!endif
!if $(WLINK32)+$(MSLINK32)==0
WLINK32=1
!endif

# select 16-bit OMF linker, default WLink

!ifndef WLINK
WLINK=0
!endif
!ifndef MSLINK
MSLINK=0
!endif
!if $(WLINK)+$(MSLINK)==0
WLINK=1
!endif

!if $(DEBUG)
AOPTD=-D_DEBUG
!else
AOPTD=
!endif

# list of 32bit modules
COFFMODS=.\jemm32.obj .\ems.obj .\vcpi.obj .\dev.obj .\xms.obj .\dma.obj .\i15.obj .\emu.obj .\vds.obj .\pool.obj .\init.obj .\debug.obj

COFFDEP1=$(COFFMODS:.\=REL386\)
COFFDEP2=$(COFFMODS:.\=RELEX\)

OUTD1=REL386
OUTD2=RELEX

!if $(WLINK32)
# wlink
LINK321=wlink format windows nt file {$(COFFDEP1)} name $(OUTD1)\jemm32.bin option offs=0x100000, objalign=0x10000, start=_start, map=$(OUTD1)\jemm32.map, quiet, norelocs
LINK322=wlink format windows nt file {$(COFFDEP2)} name $(OUTD2)\jemm32.bin option offs=0x100000, objalign=0x10000, start=_start, map=$(OUTD2)\jemm32.map, quiet, norelocs
!else
COFFOPT=/fixed /driver /subsystem:native /entry:start /base:0x100000 /align:0x10000 /MAP /nologo
# MS link (newer versions won't accept option FILEALIGN anymore)
LINK321=link.exe /FileAlign:0x200 $(COFFOPT) $(COFFDEP1) /OUT:$(OUTD1)\jemm32.bin 
LINK322=link.exe /FileAlign:0x200 $(COFFOPT) $(COFFDEP2) /OUT:$(OUTD2)\jemm32.bin 
# PoLink
#LINK321=polink.exe $(COFFOPT) $(COFFDEP1) /OUT:$(OUTD1)\jemm32.bin 
#LINK322=polink.exe $(COFFOPT) $(COFFDEP2) /OUT:$(OUTD2)\jemm32.bin 
!endif

!if $(WLINK)
LINK161=wlink.exe format dos file $(OUTD1)\jemm16.obj,$(OUTD1)\jemm.obj name $(OUTD1)\$(NAME1).EXE option map=$(OUTD1)\$(NAME1).MAP, quiet
LINK162=wlink.exe format dos file $(OUTD2)\jemm16.obj,$(OUTD2)\jemm.obj name $(OUTD2)\$(NAME2).EXE option map=$(OUTD2)\$(NAME2).MAP, quiet
!endif
!if $(MSLINK)
LINK161=link16.exe /NOLOGO/MAP:FULL/NOD /NOI $(OUTD1)\jemm16.obj $(OUTD1)\jemm.obj,$(OUTD1)\$(NAME1).EXE,$(OUTD1)\$(NAME1).MAP;
LINK162=link16.exe /NOLOGO/MAP:FULL/NOD /NOI $(OUTD2)\jemm16.obj $(OUTD2)\jemm.obj,$(OUTD2)\$(NAME2).EXE,$(OUTD2)\$(NAME2).MAP;
!endif

32BITDEPS=jemm32.inc jemm.inc external.inc debug.inc Makefile

.asm{$(OUTD1)}.obj:
	@$(ASM) -c -nologo -coff -Cp -D?INTEGRATED=0 $(AOPTD) -Fl$(OUTD1)\ -Fo$(OUTD1)\ $<

.asm{$(OUTD2)}.obj:
	@$(ASM) -c -nologo -coff -Cp -D?INTEGRATED=1 $(AOPTD) -Fl$(OUTD2)\ -Fo$(OUTD2)\ $<

ALL: $(OUTD1) $(OUTD2) $(OUTD1)\$(NAME1).EXE $(OUTD2)\$(NAME2).EXE

$(OUTD1):
	@mkdir $(OUTD1)

$(OUTD2):
	@mkdir $(OUTD2)

$(OUTD1)\$(NAME1).EXE: $(OUTD1)\jemm16.obj $(OUTD1)\jemm.obj
	@$(LINK161)

$(OUTD2)\$(NAME2).EXE: $(OUTD2)\jemm16.obj $(OUTD2)\jemm.obj
	@$(LINK162)

$(OUTD1)\jemm.obj: jemm.asm jemm16.inc jemm.inc Makefile
	@$(ASM) -c -nologo -D?INTEGRATED=0 $(AOPTD) -Fl$(OUTD1)\ -Fo$(OUTD1)\ jemm.asm

$(OUTD2)\jemm.obj: jemm.asm jemm16.inc jemm.inc Makefile
	@$(ASM) -c -nologo -D?INTEGRATED=1 $(AOPTD) -Fl$(OUTD2)\ -Fo$(OUTD2)\ jemm.asm

$(OUTD1)\jemm16.obj: jemm16.asm $(OUTD1)\_jemm32.inc jemm.inc jemm16.inc debug.inc Makefile
	@$(ASM) -c -nologo -D?INTEGRATED=0 $(AOPTD) -Fl$(OUTD1)\ -Fo$(OUTD1)\ -I$(OUTD1) jemm16.asm

$(OUTD2)\jemm16.obj: jemm16.asm $(OUTD2)\_jemm32.inc jemm.inc jemm16.inc debug.inc Makefile
	@$(ASM) -c -nologo -D?INTEGRATED=1 $(AOPTD) -Fl$(OUTD2)\ -Fo$(OUTD2)\ -I$(OUTD2) jemm16.asm

$(OUTD1)\_jemm32.inc: $(OUTD1)\jemm32.bin
	@extrsect.exe -i -q -x 0 $(OUTD1)\jemm32.bin $(OUTD1)\_jemm32.inc

$(OUTD2)\_jemm32.inc: $(OUTD2)\jemm32.bin
	@extrsect.exe -i -q -x 0 $(OUTD2)\jemm32.bin $(OUTD2)\_jemm32.inc

$(OUTD1)\jemm32.bin: $(COFFDEP1)
	@$(LINK321)

$(OUTD2)\jemm32.bin: $(COFFDEP2)
	@$(LINK322)

$(COFFDEP1): $(32BITDEPS)

$(COFFDEP2): $(32BITDEPS)

clean:
	@if exist $(OUTD1)\*.obj erase $(OUTD1)\*.obj
	@if exist $(OUTD1)\*.lst erase $(OUTD1)\*.lst
	@if exist $(OUTD1)\*.map erase $(OUTD1)\*.map
	@if exist $(OUTD1)\*.exe erase $(OUTD1)\*.exe
	@if exist $(OUTD1)\*.bin erase $(OUTD1)\*.bin
	@if exist $(OUTD1)\_jemm32.inc erase $(OUTD1)\_jemm32.inc
	@if exist $(OUTD2)\*.obj erase $(OUTD2)\*.obj
	@if exist $(OUTD2)\*.lst erase $(OUTD2)\*.lst
	@if exist $(OUTD2)\*.map erase $(OUTD2)\*.map
	@if exist $(OUTD2)\*.exe erase $(OUTD2)\*.exe
	@if exist $(OUTD2)\*.bin erase $(OUTD2)\*.bin
	@if exist $(OUTD2)\_jemm32.inc erase $(OUTD2)\_jemm32.inc

