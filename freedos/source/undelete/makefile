CC = tcc -mt -f- -ff- -w+ -O -Z -k- -d -b- -vi- -tDc -IC:\tc\include /Lc:\tc\lib

INC  = cluster.h dir16.h dirfind.h diskio.h dostypes.h drives.h fatio.h

OBJ = cluster.obj dir16.obj dirfind.obj diskio.obj drives.obj fatio.obj undelete.obj

en: $(OBJ)
        $(CC) -eundelete *.obj

all:    se ru de sp

$(OBJ): $(INC)

.c.obj:
	$(CC) -c $<
