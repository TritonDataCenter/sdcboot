#ifndef __ONLYONCE__
#define __ONLYONCE__ 1

#if defined(__TURBOC__) || defined(__BORLANDC__)
#   define TURBOC         2
#   define HAVE_MOVMEM    2
#   define HAVE_COUNTRY   1
#   define HAVE_SOUND     8
#   define HAVE_DELAY     7
#if (0x300 > __TURBOC__)
#   define __TC__         1
#endif
#elif defined(__ZTC__) || defined(__SC__)
#   define HAVE_MOVMEM     2
#   define ZORTECH         2
#   define HAVE_COUNTRY    1
#elif defined(__PACIFIC__)
#   define PACIFIC         1
#   define HAVE_SOUND      1
#elif defined(__DJGPP__)
#   define HAVE_SOUND      3
#   define HAVE_TEXTINFO   1
#   define HAVE_DELAY      2
#elif defined(__WATCOMC__)
#   define HAVE_DELAY      1
#   define HAVE_SOUND      2
#   define HAVE_DATE       3
#   define WATCOM          2
#elif defined(_QC) || defined(_MSC)
#   define MICROSOFT       2
#   define HAVE_DATE       1
#else
#   define POWERC
#endif

#define isleapyear( year ) ( (! (year % 4) && (year % 100)) || ! (year % 400) )

#define NORMAL_VIDEO   7
#define REVERSE_VIDEO  0x70
#define MAX_CHAR       133

#define V_SETMODE       0x00
#define V_GETMODE       0x0F
#define V_SETXY         0x02
#define V_GETXY         0x03
#define V_SETPAGE       0x05
#define V_SETCURSOR     0x01
#define V_SCROLLUP      0x06
#define V_SCROLLDN      0x07
#define V_READCHAR      0x08
#define V_WRITECHAR     0x09
#define V_WRITECHARTTY  0x0E

#define XNUMERIC  0x0004
#define XFLOAT    0x0008
#define XALPHANUM 0x0010
#define XDATE     0x0020
#define XSIGN     0x0040
#define XLOGICAL  0x0080
#define XALPHA    0x0100

#ifndef TRUE
#define TRUE       1
#endif
#ifndef FALSE
#define FALSE      0
#endif
#define DEFAULT    (-1)

#define EOS       '\0'

#define MAXLINE        512

#ifndef HAVE_MOVMEM
#define movmem(s,  t, n) (void) memmove((t), (s), (n))
#define setmem(to, n, c) (void) memset((to), (c), (n))
#endif

#include <dos.h>

#define BYTE    unsigned char
#define USHORT  unsigned short
#define DWORD   unsigned long
#define WORD    unsigned int
#define QWORD   double
#define LOCAL   static
#define u_char  unsigned char
#define u_long  unsigned long
#define u_short unsigned short
#define u_int   unsigned int
#define Bool    short

#define MAX(a,b)  (((a) > (b)) ? (a) : (b))
#define MIN(a,b)  (((a) < (b)) ? (a) : (b))

#endif
