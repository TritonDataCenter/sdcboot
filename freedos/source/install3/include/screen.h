/*
    Screen        2.50        19-Jun-1990
        ANSI C for MS-DOS w/monochrome or color text display

        Provides a set of functions for manipulating a text display.
        To drastically improve the speed of this module, NO RANGE
        CHECKING is done! Invalid line/column values may cause
        portions of non-video memory to be corrupted!

        Written by Scott Robert Ladd. Released into the public domain.

    ORIGINAL SOURCE CODE: BY AUTHOR.
    MODIFIED SOURCE CODE: MARC SMITH (CRAYZEHORSE@YAHOO.COM)
       -CHANGED THE GETS FUNCTION TO WORK WITH A PICTURE CLAUSE.
       -ADDED TURBO C CONIO PACKAGE.
       -MODIFIED GET_ADAPTER() FUNCTION. (INLINE ASSEMBLY)
       -COMPILED WITH TURBO C++, MICROSOFT C++, AND SYMANTEC C++.
       -ORIGINAL SOURCE WAS PORTABLE WITH ALTERATIONS.
*/

#if !defined(__SCREEN_H)
#define __SCREEN_H 1

#include "mydef.h"
//#include "stddef.h"

#define BIOS_SEGMENT 0x40

#ifdef __DJGPP__
#include <sys/farptr.h>
#include <pc.h>
#include <go32.h>

#define PEEK(baseadd,offset)      _farpeekw(_dos_ds, baseadd+offset)
#define POKE(baseadd,offset,val)  _farpokew(_dos_ds, baseadd+offset, val)
#define PEEKB(baseadd,offset)     _farpeekb(_dos_ds, baseadd+offset)
#define POKEB(baseadd,offset,val) _farpokeb(_dos_ds, baseadd+offset, val)

#else
#ifndef MK_FP
#define MK_FP( seg, ofs ) ((void far *) ((unsigned long) (seg) << 16 | (ofs)))
#endif

#define PEEK( a,b )    ( *( (int far *)  MK_FP ( (a),(b)) ))
#define PEEKB( a,b )   ( *( (char far *) MK_FP ( (a),(b)) ))
#define POKE( a,b,c )  ( *( (int far *)  MK_FP ( (a),(b)) ) =(int)(c))
#define POKEB( a,b,c ) ( *( (char far *) MK_FP ( (a),(b)) ) =(char)(c))
#endif

#define VIDEO    0x10
#define EQUIP    0x11
#define KEYBOARD 0x16

#define SCROLL_LOCK 0x0010
#define NUM_LOCK    0x0020
#define CAPS        0x0040

#define ON 1
#define OFF 0

#define DEFAULT (-1)

#define NULLCHAR '\0'
#define RDONLY   0x01
#define HIDDEN   0x02
#define SYSTEM   0x04
#define VOLUME   0x08
#define SUBDIR   0x10
#define ARCHIV   0x20
#define FILES    0x27
#define ALL      0x3F
#define MOST     0x37

#define _NOCURSOR     3
#define _HALFCURSOR   2
#define _NORMALCURSOR 0
#define _SOLIDCURSOR  1

typedef enum {BT_NONE = 0, BT_SINGLE, BT_DOUBLE, BT_TRIPLE, BT_QUAD, BT_SOLID} BoxType;
typedef enum {SA_DIRECT, SA_BIOS} ScrAccess;
typedef enum {SM_UNKNOWN = -1, LASTMODE = -1, BW40 = 0, CO40 = 1, BW80 = 2, CO80 = 3, MONO = 7, C4350 = 64 } ScrMode;

extern int directvideo;
extern int _wscroll;

#define FIELDCHAR '_'

/*--- Graphic Adapters  ---*/
#define UNKNOWN     0
#define MDA         1
#define HGC         3
#define CGA         2
#define MCGA        4
#define EGA         5
#define VGA         6
#define SVGA        14
#define XGA         15
#define UNDEFINED 255

#if !defined(__COLORS)
#define __COLORS

enum COLORS {
    BLACK,     BLUE,      GREEN,      CYAN,      RED,      MAGENTA,      BROWN,    LIGHTGRAY,
    DARKGRAY,  LIGHTBLUE, LIGHTGREEN, LIGHTCYAN, LIGHTRED, LIGHTMAGENTA, YELLOW,   WHITE
};

#define BLINK 128
#endif

#define BOLD  0x08

#define LOWER        0x01
#define UPPER        0x02
#define NUMERIC      0x08
#define ALPHA        0x20
#define AUTOEXIT     0x40
#define DATE         0x80
#define TIME         0x100

#define LONG         0x400
#define SHORT        0x800

#define NAME         0x01
#define FILESIZE     0x02

#define ASCENDING    0x10
#define DESCENDING   0x20


#define MK_COLOR(fg, bg, blink, bold) (( blink | (bg<<4)) | (fg | bold))
#define RVS(atrib) ((atrib&0x88) | ((atrib>>4)&0x07) | ((atrib<<4)&0x70) )

#ifdef __cplusplus
extern "C" {
#endif

#if defined(MICROSOFT) || defined(ZORTECH)
#pragma pack(1)
#endif

struct text_info {
    unsigned char winleft;
    unsigned char wintop;
    unsigned char winright;
    unsigned char winbottom;
    unsigned char attribute;
    unsigned char normattr;
    unsigned char currmode;
    unsigned char screenheight;
    unsigned char screenwidth;
    unsigned char curx;
    unsigned char cury;
};

/* set structure alignment to default */
#if defined(MICROSOFT) || defined(ZORTECH)
#pragma pack()
#endif

void  honk( USHORT, USHORT );

#ifndef HAVE_DELAY
void  delay( WORD );
#define sleep(n) delay(1000 * n)
#endif

#define BEEP() { honk(1000,75); }

void       hline( int, int, int, int, int );
void       vline( int, int, int, int, int );

int        ScrHeight();
int        ScrWidth();
int        ScrTextMode( int );
int        ScrWherex(void);
int        ScrWherey(void);
int        iscolor(void);
void       ScrGetTextInfo( struct text_info *ti );
void       ScrWindow( int, int, int, int );
int        ScrWherex();
int        ScrWherey();
int        ScrOpen(void);
void       ScrClose(void);
int        ScrSetMode(ScrMode mode);
ScrMode    ScrGetMode(void);
void       ScrSetAccess(ScrAccess access);
void       ScrHaveSnow(int tf);
ScrAccess  ScrGetAccess(void);
void       ScrDimensions( int*,  int*);
void       ScrCursorHide(void);
void       ScrCursorUnhide(void);
void       ScrCursorSetPos( int , int);
void       ScrCursorGetPos( int *,  int*);
void       ScrPutChar( int, int, int, int );
void       ScrGetChar( int, int, int*, int*);
void       ScrReplicate( int, int );
void       ScrPutStr( int, int, int, char*);
void       ScrPutf( char*, int );
void       ScrPrintf( int,  int, int, char*, ...);
void       ScrWritef( int, int, int, int, char*, ...);
int        ScrGets(char *Buffer, int Row, int Col, int Attr, int Length, int Mode);
int        ScrCscanf( char *format, ... );
int        ScrDrawBox( int, int, int, int, int, BoxType, int);
int        ScrSetBox( int, int, int, int, int, int, int, int, int );
void       ScrollBox( int, int, int, int, int, int, int );
void       ScrClrscr(void);
void       ScrClearLine( int, int);
int        ScrGetText( int, int, int, int, void* );
int        ScrPutText( int, int, int, int, void* );
void       ScrMoveText( int, int, int, int, int, int );
int        ScrSelect( int hx, int hy, char *text[], int size );
void       change_attr( int, int, int, int );
void       ScrDelline( void );
void       ScrInsline( void) ;
void       ScrScroll( int, int );
void       ScrClreol( void );
char*      ScrCgets( char *s );
char*      ScrGetPass( const char *s );
int        ScrCputs( const char *s );
int        ScrPutch( int );
void       ScrCprintf( char *s, ... );
void       ScrNormVideo( void );
void       ScrLowVideo( void );
void       ScrHighVideo( void );
void       ScrTextAttr( int newcolor );
void       ScrTextColor( int newcolor );
void       ScrTextBackground( int newcolor );
void       ScrGotoxy( int c, int r );
int        ScrGetche( void );
void       ScrFlush( void );
int        ScrGetch( void );
int        ScrKbhit( void );
int        ScrUngetch( int );
void       ScrSetCursorType( USHORT );

static int vid_memread( void*, int, int, int );
static int vid_memwrite( void*, int, int, int );

#define getpass        ScrGetPass
#define _setcursortype ScrSetCursorType
#define gettextinfo    ScrGetTextInfo
#define window         ScrWindow
#define wherex         ScrWherex
#define wherey         ScrWherey
#define textmode       ScrTextMode
#define clrscr         ScrClrscr
#define gettext        ScrGetText
#define puttext        ScrPutText
#define movetext       ScrMoveText
#define delline        ScrDelline
#define insline        ScrInsline
#define clreol         ScrClreol
#define cgets          ScrCgets
#define cputs          ScrCputs
#define putch          ScrPutch
#define cscanf         ScrCscanf
#define cprintf        ScrCprintf
#define normvideo      ScrNormVideo
#define lowvideo       ScrLowVideo
#define highvideo      ScrHighVideo
#define textattr       ScrTextAttr
#define textcolor      ScrTextColor
#define textbackground ScrTextBackground
#define gotoxy         ScrGotoxy
#define getche         ScrGetche
#define flush          ScrFlush
#define getch          ScrGetch
#define kbhit          ScrKbhit
#define ungetch        ScrUngetch

#ifdef __cplusplus
}
#endif

#endif
