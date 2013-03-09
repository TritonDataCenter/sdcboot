
# Makefile for NMAKE
#  tools                      Alternatives
#----------------------------------------------------------
#  JWasm                      Masm v6.x
#  WLink                      MS Link, PoLink
#  PatchPE 
# 
# PatchPE: HX tool to patch a PE binary to PX

NAME = KEYBGR
AOPTM=-c -coff -Fl$*.lst -Fo$*.obj -I..\..\Include
AOPTW=-Fl$*.lst -Fo$*.obj -I..\..\Include
OUTDIR=Release

$(OUTDIR)\$(NAME).DLL: $(OUTDIR)\$(NAME).obj Makefile
#    wlink system nt_dll ru console file $*.obj name $*.DLL op MAP=$*.MAP 
#    polink /SUBSYSTEM:CONSOLE /DLL $*.obj /OUT:$*.DLL /MAP:$*.MAP 
    link /SUBSYSTEM:CONSOLE /DLL $*.obj /OUT:$*.DLL /MAP:$*.MAP /Entry:DllMain /OPT:NOWIN98 
    ..\patchpe $*.DLL

$(OUTDIR)\$(NAME).obj: $(NAME).asm
#	ml $(AOPTM) $(NAME).asm
	jwasm $(AOPTW) $(NAME).asm
