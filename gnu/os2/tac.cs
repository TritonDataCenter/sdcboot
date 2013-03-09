(-W1 -Za -Ios2 -Ilib -DSTDC_HEADERS -DUSG -DOS2
src\tac.c
)
setargv.obj
os2\textutil.def
out\textutil.lib
out\tac.exe
-AS -LB -S0x8000
