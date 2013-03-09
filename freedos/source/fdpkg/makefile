!include config.mak

LIBS=misclib/misc.lib
LIBDIR=misclib
OBJS=fdpkg.obj func.obj
AOBJ=tools/autoadd.obj
POBJ=tools/packages.obj
FOBJ=tools/fdpkgvar.obj
SOBJ=tools/sfn2lfn.obj
LOBJ=tools/lfn2sfn.obj
MOBJ=tools/mset.obj
LIBOBJS=$(LIBDIR)/getenv.obj $(LIBDIR)/access.obj $(LIBDIR)/fnsplit.obj &
	$(LIBDIR)/fnmerge.obj $(LIBDIR)/splitpth.obj $(LIBDIR)/setdta.obj &
	$(LIBDIR)/strrev.obj $(LIBDIR)/memicmp.obj $(LIBDIR)/fmemcpy.obj &
	$(LIBDIR)/findfile.obj $(LIBDIR)/getcwd.obj $(LIBDIR)/getdisk.obj &
	$(LIBDIR)/prf.obj $(LIBDIR)/kitten.obj $(LIBDIR)/strupr.obj $(LIBDIR)/strlwr.obj

all : fdpkg.exe autoadd.exe packages.exe fdpkgvar.exe sfn2lfn.exe &
	  lfn2sfn.exe mset.com .SYMBOLIC

fdpkg.exe: $(OBJS) $(LIBOBJS) $(LIBS)
	$(CC) $(LFLAGS) $(OBJS) $(LIBDIR)$(PATHSEP)misc.lib
autoadd.exe: $(AOBJ) $(LIBS)
	$(CC) $(LFLAGS) tools$(PATHSEP)autoadd.obj $(LIBDIR)$(PATHSEP)misc.lib
	@-mv tools$(PATHSEP)autoadd.exe .
packages.exe: $(POBJ) $(LIBS)
	$(CC) $(LFLAGS) tools$(PATHSEP)packages.obj $(LIBDIR)$(PATHSEP)misc.lib
	@-mv tools$(PATHSEP)packages.exe .
fdpkgvar.exe: $(FOBJ) $(LIBS)
	$(CC) $(LFLAGS) tools$(PATHSEP)fdpkgvar.obj $(LIBDIR)$(PATHSEP)misc.lib
	@-mv tools$(PATHSEP)fdpkgvar.exe .
sfn2lfn.exe: $(SOBJ)
	$(CC) $(LFLAGS) tools$(PATHSEP)sfn2lfn.obj $(%WATCOM)/lib286/dos/lfns.lib
	@-mv tools$(PATHSEP)sfn2lfn.exe .
lfn2sfn.exe: $(LOBJ)
	$(CC) $(LFLAGS) tools$(PATHSEP)lfn2sfn.obj $(%WATCOM)/lib286/dos/lfns.lib
	@-mv tools$(PATHSEP)lfn2sfn.exe .
mset.com: $(MOBJ)
	$(CC) $(LFLAGS) $(LINKTINY) tools$(PATHSEP)mset.obj
	@-mv tools$(PATHSEP)mset.com .

.c.obj: .autodepend
	$(CC) $(FLAGS) -c $(OUTOBJ)$*.obj $*.c

fdpkg.obj: fdpkg.c func.h
func.obj: func.c func.h
$(AOBJ): tools/autoadd.c $(LIBDIR)/misc.h func.h
	$(CC) $(FLAGS) -c $(OUTOBJ)$*.obj $*.c
	@-mv autoadd.obj tools
$(POBJ): tools/packages.c $(LIBDIR)/misc.h func.h
	$(CC) $(FLAGS) -c $(OUTOBJ)$*.obj $*.c
	@-mv packages.obj tools
$(FOBJ): tools/fdpkgvar.c $(LIBDIR)/misc.h func.h
	$(CC) $(FLAGS) -c $(OUTOBJ)$*.obj $*.c
	@-mv fdpkgvar.obj tools
$(SOBJ): tools/sfn2lfn.c
	$(CC) $(FLAGS) -c $(OUTOBJ)$*.obj $*.c
	@-mv sfn2lfn.obj tools
$(LOBJ): tools/lfn2sfn.c
	$(CC) $(FLAGS) -c $(OUTOBJ)$*.obj $*.c
	@-mv lfn2sfn.obj tools
$(MOBJ): tools/mset.c
	$(CC) $(FLAGS) -c $(OUTOBJ)$*.obj $*.c
	@-mv mset.obj tools
$(LIBDIR)/getenv.obj: $(LIBDIR)/getenv.c $(LIBDIR)/misc.h
	$(CC) $(FLAGS) -c $(OUTOBJ)$*.obj $*.c
	@-mv getenv.obj $(LIBDIR)
$(LIBDIR)/access.obj: $(LIBDIR)/access.c $(LIBDIR)/misc.h
	$(CC) $(FLAGS) -c $(OUTOBJ)$*.obj $*.c
	@-mv access.obj $(LIBDIR)
$(LIBDIR)/fnsplit.obj: $(LIBDIR)/fnsplit.c $(LIBDIR)/misc.h
	$(CC) $(FLAGS) -c $(OUTOBJ)$*.obj $*.c
	@-mv fnsplit.obj $(LIBDIR)
$(LIBDIR)/fnmerge.obj: $(LIBDIR)/fnmerge.c $(LIBDIR)/misc.h
	$(CC) $(FLAGS) -c $(OUTOBJ)$*.obj $*.c
	@-mv fnmerge.obj $(LIBDIR)
$(LIBDIR)/splitpth.obj: $(LIBDIR)/splitpth.c $(LIBDIR)/misc.h
	$(CC) $(FLAGS) -c $(OUTOBJ)$*.obj $*.c
	@-mv splitpth.obj $(LIBDIR)
$(LIBDIR)/setdta.obj: $(LIBDIR)/setdta.c $(LIBDIR)/misc.h
	$(CC) $(FLAGS) -c $(OUTOBJ)$*.obj $*.c
	@-mv setdta.obj $(LIBDIR)
$(LIBDIR)/strrev.obj: $(LIBDIR)/strrev.c $(LIBDIR)/misc.h
	$(CC) $(FLAGS) -c $(OUTOBJ)$*.obj $*.c
	@-mv strrev.obj $(LIBDIR)
$(LIBDIR)/memicmp.obj: $(LIBDIR)/memicmp.c $(LIBDIR)/misc.h
	$(CC) $(FLAGS) -c $(OUTOBJ)$*.obj $*.c
	@-mv memicmp.obj $(LIBDIR)
$(LIBDIR)/fmemcpy.obj: $(LIBDIR)/fmemcpy.c $(LIBDIR)/misc.h
	$(CC) $(FLAGS) -c $(OUTOBJ)$*.obj $*.c
	@-mv fmemcpy.obj $(LIBDIR)
$(LIBDIR)/findfile.obj: $(LIBDIR)/findfile.c $(LIBDIR)/misc.h
	$(CC) $(FLAGS) -c $(OUTOBJ)$*.obj $*.c
	@-mv findfile.obj $(LIBDIR)
$(LIBDIR)/getcwd.obj: $(LIBDIR)/getcwd.c $(LIBDIR)/misc.h
	$(CC) $(FLAGS) -c $(OUTOBJ)$*.obj $*.c
	@-mv getcwd.obj $(LIBDIR)
$(LIBDIR)/getdisk.obj: $(LIBDIR)/getdisk.c $(LIBDIR)/misc.h
	$(CC) $(FLAGS) -c $(OUTOBJ)$*.obj $*.c
	@-mv getdisk.obj $(LIBDIR)
$(LIBDIR)/kitten.obj: $(LIBDIR)/kitten.c $(LIBDIR)/kitten.h
	$(CC) $(FLAGS) -c $(OUTOBJ)$*.obj $*.c
	@-mv kitten.obj $(LIBDIR)
$(LIBDIR)/prf.obj: $(LIBDIR)/prf.c
	$(CC) $(FLAGS) -c $(OUTOBJ)$*.obj $*.c
	@-mv prf.obj $(LIBDIR)
$(LIBDIR)/strupr.obj: $(LIBDIR)/strupr.c
	$(CC) $(FLAGS) -c $(OUTOBJ)$*.obj $*.c
	@-mv strupr.obj $(LIBDIR)
$(LIBDIR)/strlwr.obj: $(LIBDIR)/strlwr.c
	$(CC) $(FLAGS) -c $(OUTOBJ)$*.obj $*.c
	@-mv strlwr.obj $(LIBDIR)

misclib/misc.lib: $(LIBOBJS)
	@cd misclib
	@-rm misc.lib
	$(LIB) $(LIBFLAGS) misc.lib $(LIBADD)getenv.obj
	$(LIB) $(LIBFLAGS) misc.lib $(LIBADD)access.obj
	$(LIB) $(LIBFLAGS) misc.lib $(LIBADD)fnsplit.obj
	$(LIB) $(LIBFLAGS) misc.lib $(LIBADD)fnmerge.obj
	$(LIB) $(LIBFLAGS) misc.lib $(LIBADD)splitpth.obj
	$(LIB) $(LIBFLAGS) misc.lib $(LIBADD)setdta.obj
	$(LIB) $(LIBFLAGS) misc.lib $(LIBADD)strrev.obj
	$(LIB) $(LIBFLAGS) misc.lib $(LIBADD)memicmp.obj
	$(LIB) $(LIBFLAGS) misc.lib $(LIBADD)strupr.obj
	$(LIB) $(LIBFLAGS) misc.lib $(LIBADD)strlwr.obj
	$(LIB) $(LIBFLAGS) misc.lib $(LIBADD)fmemcpy.obj
	$(LIB) $(LIBFLAGS) misc.lib $(LIBADD)findfile.obj
	$(LIB) $(LIBFLAGS) misc.lib $(LIBADD)getcwd.obj
	$(LIB) $(LIBFLAGS) misc.lib $(LIBADD)getdisk.obj
	$(LIB) $(LIBFLAGS) misc.lib $(LIBADD)prf.obj
	$(LIB) $(LIBFLAGS) misc.lib $(LIBADD)kitten.obj
	@cd ..

release: all .SYMBOLIC
	@-upx --best --8086 *.exe *.com
	@-rm *.obj

clean: fdpkg.c func.c func.h .SYMBOLIC
	@-rm *.obj
	@-rm *.bak
	@-rm *.err
	@cd misclib
	@-rm *.obj
	@-rm *.bak
	@-rm *.err
	@cd ..
	@cd tools
	@-rm *.obj
	@-rm *.bak
	@-rm *.err
	@cd ..

distclean: clean .SYMBOLIC
	@-rm fdpkg.exe
	@-rm autoadd.exe
	@-rm packages.exe
	@-rm fdpkgvar.exe
	@cd misclib
	@-rm *.lib
	@cd ..

