/*---------------------------------------------------------------------------
    MS-DOS specific configuration section:
  ---------------------------------------------------------------------------*/

#ifndef __doscfg_h
#define __doscfg_h

#include <dos.h>           /* for REGS macro (TC) or _dos_setftime (MSC) */

#ifdef __TURBOC__          /* includes Power C */
#  include <sys/timeb.h>   /* for structure ftime */
#  ifndef __BORLANDC__     /* there appears to be a bug (?) in Borland's */
#    include <mem.h>       /*  MEM.H related to __STDC__ and far poin-   */
#  endif                   /*  ters. (dpk)  [mem.h included for memcpy]  */
#endif

#ifdef MSWIN
#  if (defined(MSC) || defined(__WATCOMC__))
#    include <sys/utime.h>
#  else /* !(MSC || __WATCOMC__) ==> may be BORLANDC, or GNU environment */
#    include <utime.h>
#  endif /* ?(MSC || __WATCOMC__) */
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
#  else /* !__386__ */
#    ifndef USE_ZLIB
#      pragma aux crc32         "_*" parm caller [] value [ax dx] \
                                      modify [ax cx dx bx]
#      pragma aux get_crc_table "_*" parm caller [] value [ax] \
                                      modify [ax cx dx bx]
#    endif /* !USE_ZLIB */
#  endif /* ?__386__ */

#  ifndef EPIPE
#    define EPIPE -1
#  endif
#  define PIPE_ERROR (errno == EPIPE)
#endif /* __WATCOMC__ */

#ifdef __EMX__
#  ifndef __32BIT__
#    define __32BIT__
#  endif
#  define far
#endif

#if defined(__GO32__) || defined(__DJGPP__)    /* MS-DOS compiler, not OS/2 */
#  ifndef __32BIT__
#    define __32BIT__
#  endif
#  ifndef __GO32__
#    define __GO32__
#  endif
#  include <sys/timeb.h>      /* for structure ftime */
#  if (defined(__DJGPP__) && (__DJGPP__ > 1))
#    include <unistd.h>       /* for prototypes for read/write etc. */
#    include <dir.h>          /* for FA_LABEL */
#  else
     int setmode(int, int);   /* not in older djgpp's include files */
#  endif
#endif

#ifndef __32BIT__
#  define __16BIT__
#endif

#if (defined(M_I86CM) || defined(M_I86LM)) || defined(MSWIN)
#  define MED_MEM
#endif
#if (defined(__COMPACT__) || defined(__LARGE__) || defined(__HUGE__))
#  define MED_MEM
#endif
#ifdef __16BIT__
#  ifndef MED_MEM
#    define SMALL_MEM
#  endif
#endif

#define EXE_EXTENSION ".exe"  /* OS/2 has GetLoadPath() function instead */

#ifdef __16BIT__
# if defined(MSC) || defined(__WATCOMC__)
#   include <malloc.h>
#   define nearmalloc _nmalloc
#   define nearfree _nfree
# endif
# if defined(__TURBOC__) && defined(DYNALLOC_CRCTAB)
#   if defined(__COMPACT__) || defined(__LARGE__) || defined(__HUGE__)
#     undef DYNALLOC_CRCTAB
#   endif
# endif
# ifndef nearmalloc
#   define nearmalloc malloc
#   define nearfree free
# endif
# if defined(USE_ZLIB) && !defined(USE_OWN_CRCTAB)
#   define USE_OWN_CRCTAB
# endif
#endif

#define NOVELL_BUG_WORKAROUND   /* another stat()/fopen() bug with some 16-bit
                                 *  compilers on Novell drives; very dangerous
                                 *  (silently overwrites executables in other
                                 *  directories)  */
#define NOVELL_BUG_FAILSAFE     /* enables additional test & message code
                                 *  that directs UnZip to fail safely in case
                                 *  the "workaround" enabled above does not
                                 *  work as intended.  */

#endif /* !__doscfg_h */
