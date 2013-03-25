/* config.h	Generated manually for MS-DOS: Microsoft C and GNU C (DJGPP)
 *	_MSC_VER indicates Microsoft C
 *	__GNUC__ indicates GNU C (presumably the DJGPP port)
 */
#undef MSDOS		/* just in case... */
#define MSDOS 1

/* Define if you have alloca, as a function or macro.  */
#define HAVE_ALLOCA 1

/* Define if the `long double' type works.  */
#ifdef __GNUC__
#define HAVE_LONG_DOUBLE 1
#endif

/* Define if your struct stat has st_blksize.  */
#ifdef __GNUC__
#define HAVE_ST_BLKSIZE 1
#endif

/* Define if you have the vprintf function.  */
#define HAVE_VPRINTF 1

/* Define as the return type of signal handlers (int or void).  */
#define RETSIGTYPE void

/* Define if the `S_IS*' macros in <sys/stat.h> do not work properly.  */
#ifdef _MSC_VER
#define STAT_MACROS_BROKEN 1
#endif

/* Define if you have the ANSI C header files.  */
#define STDC_HEADERS 1

/* The number of bytes in a long.  */
#define SIZEOF_LONG 4

/* Define if you have the strerror function.  */
#define HAVE_STRERROR 1

/* Define if you have the <dirent.h> header file.  */
#ifdef __GNUC__
#define HAVE_DIRENT_H 1
#endif

/* Define if you have the <fcntl.h> header file.  */
#define HAVE_FCNTL_H 1

/* Define if you have the <limits.h> header file.  */
#define HAVE_LIMITS_H 1

/* Define if you have the <memory.h> header file.  */
#define HAVE_MEMORY_H 1

/* Define if you have the <ndir.h> header file.  */
#ifdef _MSC_VER
#define HAVE_NDIR_H 1
#define NDIR 1		/* for msdos\glob.c (from glibc-1.09) */
#endif

/* Define if you have the <string.h> header file.  */
#define HAVE_STRING_H 1

/* Define if you have the <sys/dir.h> header file.  */
#ifdef __GNUC__
#define HAVE_SYS_DIR_H 1
#endif

/* Define if you have the <unistd.h> header file.  */
#ifdef __GNUC__
#define HAVE_UNISTD_H 1
#endif

/* Define if you have the <sys/param.h> header file.  */
#ifdef __GNUC__
#define HAVE_SYS_PARAM_H 1
#endif

/* Define if you have the getpagesize() function */
#ifdef __GNUC__
#define HAVE_GETPAGESIZE 1
#endif

/* Define if you have the memchr() function */
#define HAVE_MEMCHR 1

#ifdef _MSC_VER
#define REGEX_MALLOC 1
#endif

#if _MSC_VER >= 700
#define isascii __isascii
#define alloca _alloca
#define open _open
#define read _read
#define close _close
#define stat _stat
#endif
