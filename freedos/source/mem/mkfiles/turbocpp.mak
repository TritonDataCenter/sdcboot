#
# TURBOCPP.MAK - MEM copiler options for TCPP 1.01
#

CC            = tcc
CFLAGS        = -f -c -w -ms -a- -k- -N- -d -O -O2
LFLAGS        = -ms -M -l3
MEMSUPT       = memsupt.obj
OBJOUT        = -o
EXEOUT        = -e

