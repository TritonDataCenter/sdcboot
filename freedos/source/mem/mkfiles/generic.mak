# Perl is needed if you want to rebuild the mem_<lang>.exe binaries
PERL      = perl

UPX       = upx --8086 --best

COMPILER = watcom
#COMPILER  = turbocpp

!include "mkfiles\$(COMPILER).mak"
