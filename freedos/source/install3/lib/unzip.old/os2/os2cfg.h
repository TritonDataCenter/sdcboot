/*---------------------------------------------------------------------------
    OS/2 specific configuration section:
  ---------------------------------------------------------------------------*/

#ifndef __os2cfg_h
#define __os2cfg_h

#ifdef MSDOS
#  include <dos.h>           /* for REGS macro (TC) or _dos_setftime (MSC) */
#  ifdef __TURBOC__          /* includes Power C */
#    include <sys/timeb.h>   /* for structure ftime */
#    ifndef __BORLANDC__     /* there appears to be a bug (?) in Borland's */
#      include <mem.h>       /*  MEM.H related to __STDC__ and far poin-   */
#    endif                   /*  ters. (dpk)  [mem.h included for memcpy]  */
#  endif
#endif /* MSDOS */

#ifdef __IBMC__
#  define S_IFMT 0xF000
#  define timezone _timezone
#  define PIPE_ERROR (errno == EERRSET || errno == EOS2ERR)
#endif /* __IBMC__ */

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

#ifndef __32BIT__
#  define __16BIT__
#endif

#ifdef MSDOS
#  undef MSDOS
#endif

#if defined(M_I86CM) || defined(M_I86LM)
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
# if (defined(USE_ZLIB) && !defined(USE_OWN_CRCTAB))
#   define USE_OWN_CRCTAB
# endif
#endif

#ifdef isupper
#  undef isupper
#endif
#ifdef tolower
#  undef tolower
#endif
#ifndef OS2_EAS
#  define OS2_EAS    /* for -l and -v listings (list.c) */
#endif
#define isupper(x)   IsUpperNLS((unsigned char)(x))
#define tolower(x)   ToLowerNLS((unsigned char)(x))
#define USETHREADID

#endif /* !__os2cfg_h */
