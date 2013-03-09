(-W1 -Za -Ios2 -Ilib -DSTDC_HEADERS -DUSG -DOS2
src\join.c
)
setargv.obj
os2\textutil.def
out\textutil.lib
out\join.exe
-AS -LB -S0x4000
