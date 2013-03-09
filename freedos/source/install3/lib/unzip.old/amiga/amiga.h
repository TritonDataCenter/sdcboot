/* amiga.h
 *
 * Globular definitions that affect all of AmigaDom.
 *
 * Originally included in unzip.h, extracted for simplicity and eeze of
 * maintenance by John Bush.
 *
 * THIS FILE IS #INCLUDE'd by unzip.h 
 *
 */

#include <time.h>
#include <fcntl.h>

#ifdef AZTEC_C                    /* Manx Aztec C, 5.0 or newer only */
#  include <clib/dos_protos.h>
#  include <pragmas/dos_lib.h>        /* do inline dos.library calls */
#  define O_BINARY 0
#  include "amiga/z-stat.h"               /* substitute for <stat.h> */
#  define direct dirent

#  define DECLARE_TIMEZONE
#  define MALLOC_WORK
#  define ASM_INFLATECODES
#  define ASM_CRC

/* Note that defining REENTRANT will not eliminate all global/static */
/* variables.  The functions we use from c.lib, including stdio, are */
/* not reentrant.  Neither are the stuff in amiga/stat.c or the time */
/* functions in amiga/filedate.c, because they just augment c.lib.   */
/* If you want a fully reentrant and reexecutable "pure" UnZip with  */
/* Aztec C, assemble and link in the startup module purify.a by Paul */
/* Kienitz.  REENTRANT should be used to reduce memory waste, though */
/* it is not strictly necessary, since G is now a static struct.     */
#endif /* AZTEC_C */


#if defined(LATTICE) || defined(__SASC) || defined(__SASC_60)
#  include <sys/types.h>         
#  include <sys/stat.h>
#  include <sys/dir.h>
#  include <dos.h>
#  include <proto/dos.h>  /* needed? */
#  if ( (!defined(O_BINARY)) && defined(O_RAW))
#    define O_BINARY O_RAW
#  endif
#endif /* LATTICE */

#define USE_EF_UX_TIME
#define AMIGA_FILENOTELEN 80
#define DATE_FORMAT       DF_MDY
#define lenEOL            1
#define PutNativeEOL      *q++ = native(LF);
#define PIPE_ERROR        0
/* #define USE_FWRITE if write() returns 16-bit int */

#ifdef GLOBAL         /* crypt.c usage conflicts with AmigaDOS headers */
#  undef GLOBAL
#endif


/* Funkshine Prough Toe Taipes */

LONG FileDate (char *, time_t[]);
int windowheight (BPTR fh);

#define SCREENLINES windowheight(Output())


/* Static variables that we have to add to struct Globals: */
#define SYSTEM_SPECIFIC_GLOBALS \
    int N_flag;\
    int filenote_slot;\
    char *(filenotes[DIR_BLKSIZ]);\
    int created_dir, renamed_fullpath, rootlen;\
    char *rootpath, *buildpath, *build_end;\
    DIR *wild_dir;\
    char *dirname, *wildname, matchname[FILNAMSIZ];\
    int dirnamelen, notfirstcall;

/* N_flag, filenotes[], and filenote_slot are for the -N option that      */
/*    restores zipfile comments as AmigaDOS filenotes.  The others        */
/*    are used by functions in amiga/amiga.c only.                        */
/* created_dir and renamed_fullpath are used by mapname() and checkdir(). */
/* rootlen, rootpath, buildpath, and build_end are used by checkdir().    */
/* wild_dir, dirname, wildname, matchname[], dirnamelen and notfirstcall  */
/*    are used by do_wild().                                              */
