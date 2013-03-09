(-W1 -Za -Ios2 -Ilib -DSTDC_HEADERS -DUSG -DOS2
src\sort.c
src\version.c
lib\bcopy.c
lib\error.c
)
setargv.obj
os2\textutil.def
out\sort.exe
-AC -LB -S0x8000
