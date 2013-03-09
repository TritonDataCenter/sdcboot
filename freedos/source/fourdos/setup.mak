#
# NMAKE include file to define macros for 4xxx make files
#
!IFNDEF _LANGUAGE
_LANGUAGE=ENGLISH
!ENDIF

!IF "$(_LANGUAGE)" == "ENGLISH"
!IFNDEF OBJ
OBJ=$(ENV)
!ENDIF
TXT=txt
TXT_ASM=asm
MHDR=h
!ENDIF

!IF "$(_LANGUAGE)" == "GERMAN"
!IFNDEF OBJ
OBJ=$(ENV_D)
!ENDIF
TXT=txd
TXT_ASM=asd
MHDR=h_d
!ENDIF


#
# Define link target
#
#!IF "$(_LANGUAGE)" == "ENGLISH"
#LINKTARG=$(EXENAME)
#!ELSE
LINKTARG=$(OBJ)\$(EXENAME)
#!ENDIF


#
# Define compiler and linker commands depending on compiler and platform
#

# Base switches used in all cases
CBASE=/c /nologo /D$(_LANGUAGE) /D$(PRODUCT) 
ABASE=/nologo /D$(_LANGUAGE) /D$(PRODUCT)
!IF "$(_BETA)" == "Y"
CBASE=$(CBASE) /D__BETA
ABASE=$(ABASE) /D__BETA
!ENDIF
CFLAGS_COMP=/Fo$@


# Assembler flags vary for MASM (DOS and OS/2) vs. MASM386 (NT)
!IF ("$(ENV)" == "NT") || ("$(ENV)" == "W32")
AFLAGS_ENV=$(ABASE) /DENVDEF=4096 /V /Mx
ANAME=MASM386
!ELSE
AFLAGS_ENV=$(ABASE) /c /Zm
AFLAGS_COMP=/Fo$@
!IFDEF _MASM
ANAME=$(_MASM)
!ELSE
ANAME=ML
!ENDIF
!ENDIF


# C flags vary for each compiler.

!IF "$(COMPNAME)" == "MSC7"
!IF ("$(ENV)" == "NT") || ("$(ENV)" == "W32")
!IF "$(DLL)" == "Y"
CFLAGS_ENV=$(CBASE) /Di386=1 /D_X86_ /DSTRICT /G3 /J /LD /W3 /Ot /Og /Gz /MT
!ELSE
CFLAGS_ENV=$(CBASE) /Di386=1 /D_X86_ /DSTRICT /G3 /J /W3 /Ot /Og /Gz /MT
!ENDIF
CNAME=CL
!ELSE
CNAME=CL
!IF "$(ENV)" == "W16"
!IF "$(DLL)" == "Y"
CFLAGS_ENV=$(CBASE) /AMw /J /Ot /Gs /GEa /G2 /GD /W3 /Zp1
!ELSE
CFLAGS_ENV=$(CBASE) /AM /J /Ot /Gs /GA /GEae /G2 /W3 /Zp1
!ENDIF
!ELSE
CFLAGS_ENV=$(CBASE) /AM /J /Ocs /G2 /Gs /W3 /Zp1
!ENDIF
!ENDIF
!ELSE
!IF "$(COMPNAME)" == "MSC7S"
CNAME=CL
CFLAGS_ENV=$(CBASE) /AS /J /Ocs /Gs /W3 /Zp1
!ENDIF
!ENDIF

# Link flags vary for each target environment
!IF "$(ENV)" == "DOS"
CLFLAGS=/NOD /NOE /ST:8192 /F
ALFLAGS=/NOD /NOE
LNAME=LINK
!ENDIF

!IF "$(ENV)" == "NT"
CLFLAGS = -base:0x10000 -debug:none -subsystem:console -entry:mainCRTStartup -map:$(MAPNAME) -out:$(EXENAME) -def:..\$(DEFNAME) $(LOCLIBS) $(LIB)\libc.lib $(LIB)\*.lib
LNAME=LINK
!ENDIF

!IF "$(ENV)" == "W32"
!IF "$(DLL)" == "Y"
# CLFLAGS = kernel32.lib user32.lib /debug:none /subsystem:windows /dll /map:$(MAPNAME) /out:$(EXENAME) /def:..\$(DEFNAME) $(LOCLIBS) /entry:DLLMain
CLFLAGS = kernel32.lib user32.lib /debug:none /subsystem:windows /dll /map:$(MAPNAME) /out:$(EXENAME) /def:..\$(DEFNAME) $(LOCLIBS)
!ELSE
!IF "$(CMODE)" == "Y"
CLFLAGS = -base:0x10000 -debug:none -subsystem:console -entry:mainCRTStartup -map:$(MAPNAME) -out:$(EXENAME) -def:..\$(DEFNAME) $(LOCLIBS) $(LIB)\libc.lib $(LIB)\*.lib
!ELSE
CLFLAGS = /base:0x10000 /debug:none /subsystem:windows,4.0 /map:$(MAPNAME) /out:$(EXENAME) /def:..\$(DEFNAME) $(LOCLIBS) $(LIB)\libc.lib $(LIB)\*.lib
!ENDIF
!ENDIF
LNAME=LINK
!ENDIF


# Define ASM command, and C commands with and without file names
ASM=$(ANAME) $(AFLAGS_COMP)
COMP=$(CNAME) $(CFLAGS_COMP)
CCF=$(COMP) %s

# Define link commands
LINK=$(LNAME) $(CLFLAGS)
!IFDEF ALFLAGS
ALINK=$(LNAME) $(ALFLAGS)
!ENDIF

