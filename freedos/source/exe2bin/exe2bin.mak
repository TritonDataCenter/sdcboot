#
# Borland C++ IDE generated makefile
#
.AUTODEPEND


#
# Borland C++ tools
#
IMPLIB  = Implib
BCCDOS  = Bcc +BccDos.cfg 
TLINK   = TLink
TLIB    = TLib
TASM    = Tasm
#
# IDE macros
#


#
# Options
#
IDE_LFLAGSDOS =  -LC:\BC45\LIB
IDE_BFLAGS = 
LLATDOS_exe2bindexe =  -c -Tde -A=2
RLATDOS_exe2bindexe = 
BLATDOS_exe2bindexe = 
CNIEAT_exe2bindexe = -IC:\BC45\INCLUDE -D
LNIEAT_exe2bindexe = -x
LEAT_exe2bindexe = $(LLATDOS_exe2bindexe)
REAT_exe2bindexe = $(RLATDOS_exe2bindexe)
BEAT_exe2bindexe = $(BLATDOS_exe2bindexe)

#
# Dependency List
#
Dep_exe2bin = \
   exe2bin.exe

exe2bin : BccDos.cfg $(Dep_exe2bin)
  echo MakeNode 

Dep_exe2bindexe = \
   exe2bin.obj

exe2bin.exe : $(Dep_exe2bindexe)
  $(TLINK)   @&&|
 /v $(IDE_LFLAGSDOS) $(LEAT_exe2bindexe) $(LNIEAT_exe2bindexe) +
C:\BC45\LIB\c0t.obj+
exe2bin.obj
$<,$*
C:\BC45\LIB\ct.lib

|

exe2bin.obj :  exe2bin.c
  $(BCCDOS) -P- -c @&&|
 $(CEAT_exe2bindexe) $(CNIEAT_exe2bindexe) -o$@ exe2bin.c
|

# Compiler configuration file
BccDos.cfg : 
   Copy &&|
-W-
-R
-v
-vi
-H
-H=exe2bin.csm
-mt
-f-
-ff-
-1-
-Z
-O
-Oe
-Ol
-Ob
-Og
-Oa
-K
-d
-v-
-R-
| $@


