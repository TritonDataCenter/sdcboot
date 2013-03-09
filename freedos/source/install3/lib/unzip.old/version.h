/*
   version.h (for UnZip) by Info-ZIP.

   This header file is not copyrighted and may be distributed without
   restriction.  (That's a little geek humor, heh heh.)
 */

#ifndef __version_h   /* don't include more than once */
#define __version_h

/* #define BETA */

#ifdef BETA
#  define BETALEVEL         "x BETA"
#  define VERSION_DATE      "27 Apr 96"         /* internal beta version */
#  define WIN_VERSION_DATE  "27 Apr 96"
#else
#  define BETALEVEL         ""
#  define VERSION_DATE      "30 April 1996"     /* official release version */
#  define WIN_VERSION_DATE  VERSION_DATE
#  define RELEASE
#endif

#define UZ_MAJORVER  5   /* UnZip */
#define UZ_MINORVER  2

#define ZI_MAJORVER  2   /* ZipInfo */
#define ZI_MINORVER  1

#define D2_MAJORVER  1   /* DLL for OS/2 */  /* prob. should link to UZ ver */
#define D2_MINORVER  0

#define DW_MAJORVER  0   /* old DLL For Win16 */
#define DW_MINORVER  1

#define WIZUZ_MAJORVER 3 /* WizUnZip */
#define WIZUZ_MINORVER 0

#define PATCHLEVEL   0

#endif /* !__version_h */
