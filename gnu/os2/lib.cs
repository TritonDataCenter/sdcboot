(-W1 -Za -DSTDC_HEADERS -DUSG -DOS2
-DSTPCPY_MISSING
lib\bcopy.c
lib\error.c
lib\getopt.c
lib\getopt1.c
lib\linebuffer.c
lib\stpcpy.c
lib\xmalloc.c
lib\xwrite.c
src\version.c
)

(-W1
os2\regex.c
os2\sleep.c
)

out\textutil.lib
-AS -LL
