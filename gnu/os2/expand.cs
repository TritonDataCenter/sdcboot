(-W1 -Za -Ios2 -Ilib -DSTDC_HEADERS -DUSG -DOS2
src\expand.c
)
setargv.obj
os2\textutil.def
out\textutil.lib
out\expand.exe
-AS -LB -S0x8000
