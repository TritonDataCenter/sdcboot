/* vmmvs.h:  include file for both VM/CMS and MVS ports of UnZip */
#ifndef __vmmvs_h               /* prevent multiple inclusions */
#define __vmmvs_h
#ifndef NULL
#  define NULL (zvoid *)0
#endif

#include <time.h>               /* the usual non-BSD time functions */
#include "vmstat.h"

#define PASSWD_FROM_STDIN
                  /* Kludge until we know how to open a non-echo tty channel */

#define EBCDIC
#define __EBCDIC 2              /* treat EBCDIC as binary! */
/* In the context of Info-ZIP, a portable "text" mode file implies the use of
   an ASCII-compatible (ISO 8859-1, or other extended ASCII) code page. */


/* Workarounds for missing RTL functionality */
#define isatty(t) 1

#ifdef UNZIP                    /* definitions for UNZIP */

#define INBUFSIZ 8192

#define USE_STRM_INPUT
#define USE_FWRITE

#define REALLY_SHORT_SYMS
#define PATH_MAX 128

#define DATE_FORMAT   DF_MDY
#define lenEOL        1
/* The use of "ebcdic[LF]" is not reliable; VM/CMS C/370 uses the
 * EBCDIC specific "NL" (`NewLine') control character (and not the EBCDIC
 * equivalent of the ASCII "LF" (`LineFeed')) as line terminator!
 * To work around this problem, we explicitely emit the C compiler's native
 * '\n' line terminator.
 */
#if 0
#define PutNativeEOL  *q++ = native(LF);
#else
#define PutNativeEOL  *q++ = '\n';
#endif

#endif /* UNZIP */

#endif /* !__vmmvs_h */
