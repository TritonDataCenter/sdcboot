/* This is a dummy zip.h to allow crypt.c from Zip to compile for UnZip */

#ifndef __zip_h   /* don't include more than once */
#define __zip_h

#define UNZIP_INTERNAL
#include "unzip.h"

#define decrypt_member decrypt  /* for compatibility with zcrypt20 */
#define local static

#if defined(REENTRANT) && defined(DYNALLOC_CRCTAB)
#  undef DYNALLOC_CRCTAB
#endif
#define ziperr(c, h)   return

#ifdef MSWIN
#  include "wingui\password.h"
#endif

#endif /* !__zip_h */
