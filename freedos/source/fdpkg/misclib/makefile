include ..\config.mak

LIBOBJS=getenv.obj access.obj fnsplit.obj fnmerge.obj splitpth.obj setdta.obj strrev.obj \
	memicmp.obj findfile.obj getcwd.obj getdisk.obj prf.obj

getenv.obj: getenv.c misc.h
access.obj: access.c misc.h
fnsplit.obj: fnsplit.c misc.h
fnmerge.obj: fnmerge.c misc.h
splitpth.obj: splitpth.c misc.h
setdta.obj: setdta.c misc.h
strrev.obj: strrev.c misc.h
memicmp.obj: memicmp.c misc.h
findfile.obj: findfile.c misc.h
getcwd.obj: getcwd.c misc.h
getdisk.obj: getdisk.c misc.h
prf.obj: prf.c

NAME=misc.lib

all: $(NAME)

.c.obj:
	$(CC) -c $(FLAGS) $<

$(NAME): $(OBJS)
	@if exist $(NAME) del $(NAME)
	$(LIB) $(LIBFLAGS) $(NAME) $(LIBADD) getenv.obj
	$(LIB) $(LIBFLAGS) $(NAME) $(LIBADD) access.obj
	$(LIB) $(LIBFLAGS) $(NAME) $(LIBADD) fnsplit.obj
	$(LIB) $(LIBFLAGS) $(NAME) $(LIBADD) fnmerge.obj
	$(LIB) $(LIBFLAGS) $(NAME) $(LIBADD) splitpth.obj
	$(LIB) $(LIBFLAGS) $(NAME) $(LIBADD) setdta.obj
	$(LIB) $(LIBFLAGS) $(NAME) $(LIBADD) strrev.obj
	$(LIB) $(LIBFLAGS) $(NAME) $(LIBADD) memicmp.obj
	$(LIB) $(LIBFLAGS) $(NAME) $(LIBADD) findfile.obj
	$(LIB) $(LIBFLAGS) $(NAME) $(LIBADD) getcwd.obj
	$(LIB) $(LIBFLAGS) $(NAME) $(LIBADD) getdisk.obj

clean:
	@if exist *.obj del *.obj
	@if exist *.bak del *.bak
	@if exist *.err del *.err

distclean:
	@if exist *.lib del *.lib

