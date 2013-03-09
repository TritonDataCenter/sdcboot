/*---------------------------------------------------------------------------
    Win32 specific configuration section:
  ---------------------------------------------------------------------------*/

#ifndef __w32cfg_h
#define __w32cfg_h

#include <sys/types.h>        /* off_t, time_t, dev_t, ... */
#include <sys/stat.h>
#include <io.h>               /* read(), open(), etc. */
#include <time.h>
#include <memory.h>
#include <direct.h>           /* mkdir() */
#include <fcntl.h>
#if (defined(MSC) || defined(__WATCOMC__))
#  include <sys/utime.h>
#else
#  include <utime.h>
#endif
#if defined(FILEIO_C)
#  include <conio.h>
#  include <windows.h>
#endif

#define DATE_FORMAT   DF_MDY
#define lenEOL        2
#define PutNativeEOL  {*q++ = native(CR); *q++ = native(LF);}

#define USE_EF_UX_TIME

/* Static variables that we have to add to struct Globals: */
#define SYSTEM_SPECIFIC_GLOBALS \
    int created_dir, renamed_fullpath, fnlen;\
    unsigned nLabelDrive;\
    char *rootpath, *buildpathHPFS, *buildpathFAT, *endHPFS, *endFAT;\
    char *dirname, *wildname, matchname[FILNAMSIZ];\
    int rootlen, have_dirname, dirnamelen, notfirstcall;\
    zvoid *wild_dir;

/* created_dir, renamed_fullpath, fnlen, and nLabelDrive are used by   */
/*    both mapname() and checkdir().                                   */
/* rootlen, rootpath, buildpathHPFS, buildpathFAT, endHPFS, and endFAT */
/*    are used by checkdir().                                          */
/* wild_dir, dirname, wildname, matchname[], dirnamelen, have_dirname, */
/*    and notfirstcall are used by do_wild().                          */

#if (defined(_MSC_VER) && !defined(MSC))
#  define MSC
#endif

#ifdef __WATCOMC__
#  ifdef __386__
#    ifndef WATCOMC_386
#      define WATCOMC_386
#    endif
#    define __32BIT__
#    undef far
#    define far
#    undef near
#    define near

/* Get asm routines to link properly without using "__cdecl": */
#    ifndef USE_ZLIB
#      pragma aux crc32         "_*" parm caller [] value [eax] modify [eax]
#      pragma aux get_crc_table "_*" parm caller [] value [eax] \
                                      modify [eax ecx edx]
#    endif /* !USE_ZLIB */
#  endif /* __386__ */

#  ifndef EPIPE
#    define EPIPE -1
#  endif
#  define PIPE_ERROR (errno == EPIPE)
#endif /* __WATCOMC__ */

#endif /* !__w32cfg_h */
