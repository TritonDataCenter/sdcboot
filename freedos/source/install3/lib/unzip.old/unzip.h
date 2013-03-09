/*---------------------------------------------------------------------------

  unzip.h (new)

  This header file contains the public macros and typedefs required by
  both the UnZip sources and by any application using the UnZip API.  If
  UNZIP_INTERNAL is defined, it includes unzpriv.h (containing includes,
  prototypes and extern variables used by the actual UnZip sources).

  ---------------------------------------------------------------------------*/


#ifndef __unzip_h   /* prevent multiple inclusions */
#define __unzip_h


/*****************************************/
/*  Predefined, Machine-specific Macros  */
/*****************************************/

#ifdef __GO32__                 /* MS-DOS extender:  NOT Unix */
#  ifdef unix
#    undef unix
#  endif
#  ifdef __unix
#    undef __unix
#  endif
#  ifdef __unix__
#    undef __unix__
#  endif
#endif

#if ((defined(__convex__) || defined(__convexc__)) && !defined(CONVEX))
#  define CONVEX
#endif

#if (defined(unix) || defined(__unix) || defined(__unix__))
#  ifndef UNIX
#    define UNIX
#  endif
#endif /* unix || __unix || __unix__ */
#if (defined(M_XENIX) || defined(COHERENT) || defined(__hpux))
#  ifndef UNIX
#    define UNIX
#  endif
#endif /* M_XENIX || COHERENT || __hpux */
#if (defined(CONVEX) || defined(MINIX) || defined(_AIX) || defined(__QNX__))
#  ifndef UNIX
#    define UNIX
#  endif
#endif /* CONVEX || MINIX || _AIX || __QNX__ */

#if (defined(VM_CMS) || defined(MVS))
#  define CMS_MVS
#endif

#if (defined(__OS2__) && !defined(OS2))
#  define OS2
#endif

#if (defined(__VMS) && !defined(VMS))
#  define VMS
#endif

#if (defined(__WIN32__) && !defined(WIN32))
#  define WIN32
#endif

#ifdef __COMPILER_KCC__
#  include <c-env.h>
#  ifdef SYS_T20
#    define TOPS20
#  endif
#endif /* __COMPILER_KCC__ */

/* Borland C does not define __TURBOC__ if compiling for a 32 bit platform */
#ifdef __BORLANDC__
#  ifndef __TURBOC__
#    define __TURBOC__
#  endif
#  if (!defined(__MSDOS__) && !defined(OS2) && !defined(WIN32))
#    define __MSDOS__
#  endif
#endif

/* define MSDOS for Turbo C (unless OS/2) and Power C as well as Microsoft C */
#ifdef __POWERC
#  define __TURBOC__
#  define MSDOS
#endif /* __POWERC */

#if (defined(__MSDOS__) && !defined(MSDOS))   /* just to make sure */
#  define MSDOS
#endif

#if (defined(linux) && !defined(LINUX))
#  define LINUX
#endif

#ifdef __arm
#  define RISCOS
#endif

/* use prototypes and ANSI libraries if __STDC__, or Microsoft or Borland C, or
 * Silicon Graphics, or Convex?, or IBM C Set/2, or GNU gcc/emx, or Watcom C,
 * or Macintosh, or Windows NT, or Sequent, or Atari or IBM RS/6000.
 */
#if (defined(__STDC__) || defined(MSDOS) || defined(sgi) || defined(RISCOS))
#  ifndef PROTO
#    define PROTO
#  endif
#  ifndef MODERN
#    define MODERN
#  endif
#endif
#if (defined(__IBMC__) || defined(__EMX__) || defined(__WATCOMC__))
#  ifndef PROTO
#    define PROTO
#  endif
#  ifndef MODERN
#    define MODERN
#  endif
#endif
#if (defined(THINK_C) || defined(MPW) || defined(WIN32) || defined(_SEQUENT_))
#  ifndef PTX   /* Sequent running Dynix/ptx:  non-modern compiler */
#    ifndef PROTO
#      define PROTO
#    endif
#    ifndef MODERN
#      define MODERN
#    endif
#  endif
#endif
#if (defined(ATARI_ST) || defined(__BORLANDC__))  /* || defined(CONVEX) */
#  ifndef PROTO
#    define PROTO
#  endif
#  ifndef MODERN
#    define MODERN
#  endif
#endif
#if (defined(_AIX) || defined(CMS_MVS))
#  ifndef PROTO
#    define PROTO
#  endif
#  ifndef MODERN
#    define MODERN
#  endif
#endif

/* turn off prototypes if requested */
#if (defined(NOPROTO) && defined(PROTO))
#  undef PROTO
#endif

/* used to remove arguments in function prototypes for non-ANSI C */
#ifdef PROTO
#  define OF(a) a
#else
#  define OF(a) ()
#endif

/* Be cautious and disable the "const" keyword if in doubt. */
#if (!defined(MODERN) && !defined(NO_CONST))
#  define NO_CONST
#endif /* !MODERN && !NO_CONST */

/* Avoid using const if compiler does not support it */
#ifdef NO_CONST
#  ifdef const
#    undef const
#  endif
#  define const
#endif

/* bad or (occasionally?) missing stddef.h: */
#if (defined(M_XENIX) || defined(DNIX))
#  define NO_STDDEF_H
#endif

#if (defined(M_XENIX) && !defined(M_UNIX))   /* SCO Xenix only, not SCO Unix */
#  define NO_LIMITS_H        /* no limits.h, but MODERN defined */
#  define NO_UID_GID         /* no uid_t/gid_t */
#endif

#ifdef realix   /* Modcomp Real/IX, real-time SysV.3 variant */
#  define SYSV
#  define NO_UID_GID         /* no uid_t/gid_t */
#endif

#if (defined(_AIX) && !defined(_ALL_SOURCE))
#  define _ALL_SOURCE
#endif

#if defined(apollo)          /* defines __STDC__ */
#    define NO_STDLIB_H
#endif

#ifdef DNIX
#  define SYSV
#  define SHORT_NAMES         /* 14-char limitation on path components */
/* #  define FILENAME_MAX  14 */
#  define FILENAME_MAX  NAME_MAX    /* GRR:  experiment */
#endif

#if (defined(SYSTEM_FIVE) || defined(__SYSTEM_FIVE))
#  ifndef SYSV
#    define SYSV
#  endif
#endif /* SYSTEM_FIVE || __SYSTEM_FIVE */
#if (defined(M_SYSV) || defined(M_SYS5))
#  ifndef SYSV
#    define SYSV
#  endif
#endif /* M_SYSV || M_SYS5 */
/* __SVR4 and __svr4__ catch Solaris on at least some combos of compiler+OS */
#if (defined(__SVR4) || defined(__svr4__) || defined(sgi) || defined(__hpux))
#  ifndef SYSV
#    define SYSV
#  endif
#endif /* __SVR4 || __svr4__ || sgi || __hpux */
#if (defined(LINUX) || defined(__QNX__))
#  ifndef SYSV
#    define SYSV
#  endif
#endif /* LINUX || __QNX__ */

#if (defined(ultrix) || defined(__ultrix) || defined(bsd4_2))
#  if (!defined(BSD) && !defined(SYSV))
#    define BSD
#  endif
#endif /* ultrix || __ultrix || bsd4_2 */
#if (defined(sun) || defined(pyr) || defined(CONVEX))
#  if (!defined(BSD) && !defined(SYSV))
#    define BSD
#  endif
#endif /* sun || pyr || CONVEX */

#ifdef pyr  /* Pyramid:  has BSD and AT&T "universes" */
#  ifdef BSD
#    define pyr_bsd
#    define USE_STRINGS_H  /* instead of more common string.h */
#    define ZMEM           /* ZMEM now uses bcopy/bzero: not in AT&T universe */
#  endif                   /* (AT&T memcpy claimed to be very slow, though) */
#  define DECLARE_ERRNO
#endif /* pyr */

/* stat() bug for Borland, Watcom, VAX C (also GNU?), and Atari ST MiNT on
 * TOS filesystems:  returns 0 for wildcards!  (returns 0xffffffff on Minix
 * filesystem or `U:' drive under Atari MiNT) */
#if (defined(__TURBOC__) || defined(__WATCOMC__) || defined(VMS))
#  define WILD_STAT_BUG
#endif
#if (defined(__MINT__))
#  define WILD_STAT_BUG
#endif

#ifdef WILD_STAT_BUG
#  define SSTAT(path,pbuf) (iswild(path) || stat(path,pbuf))
#else
#  define SSTAT stat
#endif

#ifdef REGULUS  /* returns the inode number on success(!)...argh argh argh */
#  define stat(p,s) zstat(p,s)
#endif

#define STRNICMP zstrnicmp



/***************************/
/*  OS-Dependent Includes  */
/***************************/

#ifdef EFT
#  define LONGINT off_t  /* Amdahl UTS nonsense ("extended file types") */
#else
#  define LONGINT long
#endif

#ifdef MODERN
#  ifndef NO_STDDEF_H
#    include <stddef.h>
#  endif
#  ifndef NO_STDLIB_H
#    include <stdlib.h>  /* standard library prototypes, malloc(), etc. */
#  endif
   typedef size_t extent;
   typedef void zvoid;
#else /* !MODERN */
#  ifndef AOS_VS         /* mostly modern? */
#    ifndef CMS_MVS
       LONGINT lseek();
#    endif
#    ifdef VAXC          /* not fully modern, but does have stdlib.h and void */
#      include <stdlib.h>
#    else
       char *malloc();
#      define void int
#    endif /* ?VAXC */
#  endif /* !AOS_VS */
   typedef unsigned int extent;
   typedef char zvoid;
#endif /* ?MODERN */



typedef unsigned char   uch;    /* code assumes unsigned bytes; these type-  */
typedef unsigned short  ush;    /*  defs replace byte/UWORD/ULONG (which are */
typedef unsigned long   ulg;    /*  predefined on some systems) & match zip  */

/* InputFn is not yet used and is likely to change: */
#ifdef PROTO
   typedef int   (MsgFn)    (zvoid *pG, uch *buf, ulg size, int flag);
   typedef int   (InputFn)  (zvoid *pG, uch *buf, int *size, int flag);
   typedef void  (PauseFn)  (zvoid *pG, const char *prompt, int flag);
#else
   typedef int   (MsgFn)    ();
   typedef int   (InputFn)  ();
   typedef void  (PauseFn)  ();
#endif

typedef struct _UzpBuffer {   /* rxstr */
    ulg   strlength;          /* length of string  */
    char  *strptr;            /* pointer to string */
} UzpBuffer;

typedef struct _UzpInit {
    ulg structlen;            /* length of the struct being passed */

    /* GRR: can we assume that each of these is a 32-bit pointer?  if not,
     * does it matter? add "far" keyword to make sure? */
    MsgFn *msgfn;
    InputFn *inputfn;
    PauseFn *pausefn;

    void (*userfn)();   /* user init function to be called after globals */
                        /*  constructed and initialized */

    /* pointer to program's environment area or something? */
    /* hooks for performance testing? */
    /* hooks for extra unzip -v output? (detect CPU or other hardware?) */
    /* anything else?  let me (Greg) know... */
} UzpInit;

/* intended to be a private struct: */
typedef struct _ver {
    uch major;              /* e.g., integer 5 */
    uch minor;              /* e.g., 2 */
    uch patchlevel;         /* e.g., 0 */
    uch not_used;
} _version_type;

typedef struct _UzpVer {
    ulg structlen;          /* length of the struct being passed */
    ulg flag;               /* bit 0: is_beta   bit 1: uses_zlib */
    char *betalevel;        /* e.g., "g BETA" or "" */
    char *date;             /* e.g., "4 Sep 95" (beta) or "4 September 1995" */
    char *zlib_version;     /* e.g., "0.95" or NULL */
    _version_type unzip;
    _version_type zipinfo;
    _version_type os2dll;
    _version_type windll;
} UzpVer;

typedef struct central_directory_file_header { /* CENTRAL */
    uch version_made_by[2];
    uch version_needed_to_extract[2];
    ush general_purpose_bit_flag;
    ush compression_method;
    ush last_mod_file_time;
    ush last_mod_file_date;
    ulg crc32;
    ulg csize;
    ulg ucsize;
    ush filename_length;
    ush extra_field_length;
    ush file_comment_length;
    ush disk_number_start;
    ush internal_file_attributes;
    ulg external_file_attributes;
    ulg relative_offset_local_header;
} cdir_file_hdr;


#define UZPINIT_LEN   sizeof(UzpInit)
#define UZPVER_LEN    sizeof(UzpVer)
#define cbList(func)  int (*func)(char *filename, cdir_file_hdr *crec)


/*---------------------------------------------------------------------------
    Prototypes for public UnZip API (DLL) functions.
  ---------------------------------------------------------------------------*/

int      UzpMain            OF((int argc, char **argv));
int      UzpAltMain         OF((int argc, char **argv, UzpInit *init));
UzpVer  *UzpVersion         OF((void));
int      UzpUnzipToMemory   OF((char *zip, char *file, UzpBuffer *retstr));
int      UzpFileTree        OF((char *name, cbList(callBack),
                                char *cpInclude[], char *cpExclude[]));

/* default I/O functions (can be swapped out via UzpAltMain() entry point): */

int      UzpMessagePrnt     OF((zvoid *pG, uch *buf, ulg size, int flag));
int      UzpMessageNull     OF((zvoid *pG, uch *buf, ulg size, int flag));
int      UzpInput           OF((zvoid *pG, uch *buf, int *size, int flag));
void     UzpMorePause       OF((zvoid *pG, const char *prompt, int flag));


/*---------------------------------------------------------------------------
    Remaining private stuff for UnZip compilation.
  ---------------------------------------------------------------------------*/

#ifdef UNZIP_INTERNAL
#  include "unzpriv.h"
#endif


#endif /* !__unzip_h */
