CC = tcc -mt -f- -ff- -w+ -O -Z -k- -d -b- -vi- -tDc -IC:\tc\include /Lc:\tc\lib

OBJ = whatis.obj

en: $(OBJ)
        $(CC) -ewhatis *.obj

.c.obj:
	$(CC) -c $<
