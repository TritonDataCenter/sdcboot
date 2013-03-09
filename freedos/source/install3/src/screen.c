#include "mydef.h"
#include <ctype.h>
#include <dos.h>
#include <time.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#if defined(TURBOC)
#include <alloc.h>
#include <string.h>
#include <mem.h>
#elif defined(POWERC)
#include <math.h>
#else
#include <malloc.h>
#include <string.h>
#include <memory.h>
#endif
#include "screen.h"
#include "keys.h"

#if defined(__TURBOC__)
#define _bios_keybrd bioskey
#endif

#if defined(MICROSOFT) || defined(WATCOM)
extern int inp( unsigned );
extern int outp( unsigned, int );
#endif

#define NORMAL_TYPE 0
#define INTENSE 0x08

//int directvideo = SA_DIRECT;
//int _wscroll    = TRUE;

#define NORMAL_TYPE 0

#define INTENSE 0x08

static int           startx, starty, endx, endy;
static int           screenwidth, screenheight;
static int           curx = 0, cury = 0;
static int           cur_attr     = 7;
static int           old_mode     = 0;
static int           oldcolor     = 7;
static int           cur_mode     = 7;
static BYTE          cur_page     = 0;
static int           VideoType    = UNKNOWN;
static int           VideoMode    = SM_UNKNOWN;
static int           OriginalMode = SM_UNKNOWN;
static unsigned int  Width        = 0;
static unsigned int  Length       = 0;
#ifdef __DJGPP__
static unsigned long BaseAdr      = 0;
#else
static unsigned int  BaseAdr      = 0;
#endif
static int           chbuf        = -1;
static int           kb_flag      = 0;
static BYTE          buff[4096];
static int           sort_order;
static int           reverse_order;
static unsigned char Box_Chars[101];


#define isleapyear( year )      ( (! (year % 4) && (year % 100)) || ! (year % 400) )

#define V_SCROLLDN 0x07
#define V_SCROLLUP 0x06
#define MAXCHAR     512
#define TABS          8

#define CGA_MODE_SEL 0x3D8
#define CGA_ENABLE   0x29
#define CGA_DISABLE  0x21

static int   get_adapter(void);
static int   cputn( const char*, int );
static void  ScrEnableVideoCGA( int );
static void  ScrDispElement(int, int, int, int, char*, char* );
static int   islegal( int, int, int );
#if defined(MICROSOFT) || defined(POWERC) || defined(DJGPP)
static int   vsscanf( const char *buf, const char *format, va_list argp );
#endif

#define ScrScroll(dir, lines) ScrollBox( startx, starty, endx, endy, cur_attr, (dir), (lines))

#define CGA_MODE_SEL 0x3D8
#define CGA_ENABLE   0x29
#define CGA_DISABLE  0x21

#if 0
char*  ScrGetPass( const char *prompt )
{      int ch, len, maxlen=9;
       static char s[10]; char *p;

       len = 0;
       ScrCputs((char*)prompt);
       p = s;

       while ((ch = ScrGetch()) != RETURN && ch != NEWLINE && ch != ESC && len <= maxlen)
       {    if (ch == BS)
            {  if ( len > 0 )
               {  cputn("\b \b", 3);
                  --len;
                  --p;
               }
            }
            else if (ch < 32 || ch > 127);
            else
            {   ScrPutch( '*' );
                *p++ = (char) ch;
                ++len;
            }
       }

       *p = NULLCHAR;
       return (char*) ( s );
}

void
ScrWindow( int left, int top, int right, int bottom )
{
       if (left < 1 || right > (Width+1) || top  < 1  || bottom > (Length+1) ||
          ((right - left) < 0) || ((bottom - top) < 0))
          return;

       startx       = top - 1;
       starty       = left - 1;
       endx         = bottom - 1;
       endy         = right - 1;
       screenwidth  = endy - starty;
       screenheight = endx - startx;
       curx         = cury = 0;
}
#endif

void
ScrGotoxy( int c, int r )
{      r--; c--;
       ScrCursorSetPos( r + startx, c + starty );
       curx = r; cury = c;
}

#if 0
int ScrWidth()
{      return (screenwidth+1);
}

int ScrHeight()
{      return (screenheight+1);
}
#endif

int ScrWherey()
{
       int row, col;
       ScrCursorGetPos( &row, &col);
       row -= startx;
       return (row+1);
}

int ScrWherex()
{
       int row, col;
       ScrCursorGetPos( &row, &col);
       col -= starty;
       return (col+1);
}

void
ScrGetTextInfo( struct text_info *info )
{
       info->winleft      = (unsigned char) (starty+1);
       info->wintop       = (unsigned char) (startx+1);
       info->winright     = (unsigned char) (endy+1);
       info->winbottom    = (unsigned char) (endx+1);
       info->attribute    = (unsigned char) oldcolor;
       info->normattr     = (unsigned char) cur_attr;
       info->currmode     = (unsigned char) VideoMode;
       info->screenheight = (unsigned char) (screenheight+1);
       info->screenwidth  = (unsigned char) (screenwidth+1);
       info->curx         = (unsigned char) (curx+1);
       info->cury         = (unsigned char) (cury+1);
}

#if 0
void
ScrNormVideo( void )
{
       cur_attr = oldcolor;
}

void
ScrLowVideo( void )
{
       cur_attr &= ~INTENSE;
}

void
ScrHighVideo( void )
{
       cur_attr |= INTENSE;
}
#endif

void
ScrTextAttr( int newcolor )
{
       oldcolor = cur_attr;
       cur_attr = newcolor;
}

void
ScrTextColor( int newcolor )
{
       cur_attr = (cur_attr & 0x70) | (newcolor & 0x8F);
}

void
ScrTextBackground( int newcolor )
{
       cur_attr = (cur_attr & 0x8F) | ((newcolor<<4) & 0x7f);
}

#if 0
int
ScrCscanf( char *format, ... )
{      va_list argv;
       int     cnt;
       char    buf[100];

       memset(buf, 0, 100);
       buf[0]=100;
       ScrCgets(buf);
       va_start(argv,format);
       cnt = vsscanf(&buf[2],format,argv);
       va_end(argv);
       return cnt;
}
#endif

void ourSIGINThandler(int sig);

char*
ScrCgets( char *s )
{      int ch, len, maxlen;
       char *p = s + 2;

       len = 0;
       maxlen = s[ 0 ] & 0xff;
       if (!maxlen) maxlen = screenwidth - cury;

       while ((ch = ScrGetch()) != RETURN && ch != NEWLINE && ch != ESC && len < maxlen)
       {    if (ch == BS)
            {  if ( len > 0 )
               {  cputn("\b \b", 3);
                  --len;
                  --p;
               }
            }
            else if (ch == ('\x03' & 0xFF)) {
                unsigned x = wherex(), y = wherey();
                int oldattr = cur_attr;
                ourSIGINThandler(0);
                cur_attr = oldattr;
                gotoxy(x, y);
            }/* Nab ctrl-C */
            else if (ch < 32 || ch > 127);
            else
            {   ScrPutch(ch);
                *p++ = (char) ch;
                ++len;
            }
       }

       *p = NULLCHAR;
       s[ 1 ] = len;
       return( char* ) ( s + 2 );
}

static int
cputn( const char *s, int len )
{
       int ch;
       while ( len-- )
       {     ch = *s++;
             switch (ch) {
                 case '\0':
                    return (0);
#if 0
                 case '\a':
                    BEEP();
                    break;
#endif
                 case '\b':
                    if (cury > 0)
                       cury--;
                    else
                       curx--;
                    break;
                 case '\t':
                    do
                    {  ScrPutChar( startx + curx, starty + cury, cur_attr, SPACE );
                       cury++;
                    }  while ((cury % TABS) && (cury < screenwidth));
                    break;
                 case '\r':
                    cury = 0;
                    break;
                 case '\n':
                    curx++; cury = 0;
                    break;
                 default:
                    ScrPutChar( startx + curx, starty + cury, cur_attr, (int)ch );
                    cury++;
             }
             if (cury > screenwidth) {
                cury = 0;
                curx++;
             }
             if (curx > screenheight) {
                /*if (_wscroll)*/  ScrScroll( UP, 1 );
                curx--;
             }
             ScrGotoxy( cury+1, curx+1 );
       }
       return 1;
}

void
ScrClreol( void )
{     int row, col;
      ScrCursorGetPos( &row, &col );
      ScrollBox(row, col, row, endy, cur_attr, UP, 0);
}

#if 0
void
ScrInsline(void)
{      int line, col;
       ScrCursorGetPos( &line, &col );
       ScrollBox( line, starty, endx, endy, cur_attr, DN, (line==endx)?0:1);
}      /* insline */

void
ScrDelline(void)
{
       int line, col;
       ScrCursorGetPos( &line, &col );
       ScrollBox( line, starty, endx, endy, cur_attr, UP, (line==endx)?0:1);
}    /* delline */
#endif

void
ScrCprintf( char *format, ... )
{
       va_list args;
       char buffer[512];
       int len;

       va_start(args, format);
       len = vsprintf(buffer,format,args);
       va_end(args);

       cputn(buffer, len);
}

int
ScrCputs( const char *s )
{
       return cputn(s, strlen(s));
}

int
ScrPutch( int ch )
{
       return cputn((char *)&ch, 1);
}

static void ScrEnableVideoCGA( int nStatus )
{
    if ( VideoType == CGA ) /* CGA */
#if defined(__TURBOC__) || defined(__DJGPP__) || defined(__BORLANDC__)
       outportb(CGA_MODE_SEL, (unsigned char) nStatus);
#else
       outp(CGA_MODE_SEL, nStatus);
#endif
    return;
}

#if 0
void
ScrMoveText( int sx1, int sy1, int sx2, int sy2, int dx1, int dy1)
{
       char workbuffer[MAXCHAR];
       int first, last, direction, y;

       sx1--; sy1--; sx2--; sy2--; dx1--; dy1--;
       if ( sy1 < 0 || sy2 > Width  || sy1 > sy2 )  return;
       if ( sx1 < 0 || sx2 > Length || sx1 > sx2 )  return;
       if ( dy1 < 0 || (dy1+sy2-sy1) > Width )      return;
       if ( dx1 < 0 || (dx1+sx2-sx1) > Length )     return;

       first     = sx1;
       last      = sx2;
       direction = 1;

       if (sx1 < dx1)
       {  first     = sx2;
          last      = sx1;
          direction = -1;
       }

       for (y = first; y != last + direction; y += direction)
       {    vid_memread ( (void *) workbuffer, y,            sy1,  sy2-sy1+1);
            vid_memwrite( (void *) workbuffer, dx1+(y-sx1),  dy1,  sy2-sy1+1);
       }
       return;
}
#endif

int  ScrGetText( int cs, int rs, int ce, int re, void *ptr )
{
       int width = ce - cs + 1, size = width * 2;
       BYTE *sv = (BYTE *) ptr;
       register int i;

       cs--; rs--; ce--; re--;
       for (i = rs; i <= re; i++ )
       {   vid_memread((void *) sv, i, cs, width);
           sv += size;
       }
       return 1;
}

int  ScrPutText( int cs, int rs, int ce, int re, void *ptr )
{
       int width = ce - cs + 1, size = width * 2;
       BYTE *sv = (BYTE *) ptr;
       register int i;

       cs--; rs--; ce--; re--;
       for (i = rs; i <= re; i++ )
       {   vid_memwrite((void *) sv, i, cs, width);
           sv += size;
       }
       return 1;
}

static int vid_memread( void *sv, int row, int col, int cnt )
{      unsigned short *bufptr = (unsigned short *) sv;
       unsigned short offset;
       union REGS r;

//       if (directvideo == SA_DIRECT) {
          offset = (2 * Width * row) + (col * 2);
          ScrEnableVideoCGA(CGA_DISABLE);
          while ( cnt-- ) { *bufptr++ = PEEK( BaseAdr, offset); offset += 2; }
          ScrEnableVideoCGA(CGA_ENABLE);
//       }
#if 0
       else
       {  while (cnt--)
          {  ScrCursorSetPos(row, col); col++;
             if (col >= Width) { col = 0; row++; }
             r.h.ah = 0x08;
             r.h.bh = (unsigned char) cur_page;
             *bufptr++ = int86(VIDEO, &r, &r);
          }
       }
#endif
       return 1;
}

static int vid_memwrite( void *sv, int row, int col, int cnt )
{      unsigned short *bufptr = (unsigned short *) sv;
       unsigned char  *xbufptr = (unsigned char *) sv;
       unsigned short offset;
       union REGS r;

//       if (directvideo == SA_DIRECT) {
          offset = (2 * Width * row) + (col * 2);
          ScrEnableVideoCGA(CGA_DISABLE);
          while ( cnt-- ) { POKE( BaseAdr, offset, *bufptr++); offset += 2; }
          ScrEnableVideoCGA(CGA_ENABLE);
//       }
#if 0
       else
       {  while (cnt--)
          {  ScrCursorSetPos(row, col); col++;
             if (col >= Width) { col = 0; row++; }
             r.h.ah = 0x09;
             r.h.bh = (unsigned char) cur_page;
             r.x.cx = 1;
             r.x.dx = 0;
             r.h.al = *xbufptr++;
             r.h.bl = *xbufptr++;
             int86(VIDEO, &r, &r);
          }
       }
#endif
       return 1;
}

int ScrOpen(void)
{
       union REGS regs;

       VideoType = get_adapter();
       if (VideoType == UNKNOWN)
       {  //fputs("\aVideo was identified as UNKNOWN\n", stderr);
          fputs("Program has normally terminated\n", stderr);
          exit(1);
       }

       /* initialize box characters for display */
       memset( Box_Chars, 0x00, 100 );
//       ScrSetBox( 0,  32,  32,  32,  32,  32,  32,  32,  32 );
//       ScrSetBox( 1, 218, 196, 191, 179, 217, 196, 192, 179 );
//       ScrSetBox( 2, 201, 205, 187, 186, 188, 205, 200, 186 );
//       ScrSetBox( 3, 213, 205, 184, 179, 190, 205, 212, 179 );
//       ScrSetBox( 4, 214, 196, 183, 186, 189, 196, 211, 186 );
//       ScrSetBox( 5, 176, 176, 176, 176, 176, 176, 176, 176 );
//       ScrSetBox( 6, 177, 177, 177, 177, 177, 177, 177, 177 );
//       ScrSetBox( 7, 178, 178, 178, 178, 178, 178, 178, 178 );
//       ScrSetBox( 8, 0xDB, 0xDB, 0xDB, 0xDB, 0xDB, 0xDB, 0xDB, 0xDB );
//       ScrSetBox( 9, 43, 45, 43, 124, 43, 45, 43, 124 );

       /* find width of the orginal screen */
       regs.h.ah = 0x0F;
       int86(VIDEO,&regs,&regs);

       switch ((int) regs.h.al)
         {
         case 0: case 1:
         case 2: case 3: case 7:
              break;
         default:
              regs.h.ah = 0x00;  /* try setting mode 2 -- BW80 */
              regs.h.al = 0x02;
              int86(VIDEO, &regs, &regs);
              regs.h.ah = 0x0F;
              int86(VIDEO, &regs, &regs);
              break;
         }

       cur_mode = (int) regs.h.al;
#ifdef __DJGPP__
       BaseAdr  = ( cur_mode == 7 ) ? 0xB0000 : 0xB8000;
#else
       BaseAdr  = ( cur_mode == 7 ) ? 0xB000 : 0xB800;
#endif
       Width    = (int) regs.h.ah;
       cur_page = regs.h.bh;

       /* save the original cursor shape */
       /* and the original cursor position */

       regs.h.ah = 3;
       regs.h.bh = cur_page;
       int86(VIDEO,&regs,&regs);
       curx = (int) regs.h.dh;
       cury = (int) regs.h.dl;

       /* turn off blinking */
       regs.h.ah = 0x10;
       regs.h.al = 0x03;
       regs.h.bl = 0x00;
       int86(VIDEO, &regs, &regs);

       /* find length of the original screen */

       regs.x.ax = 0x1130;
       regs.h.bh = 0;
       regs.x.dx = 0;
       int86(VIDEO,&regs,&regs);

       Length = regs.x.dx + 1;

       if (Length == 1) Length = 25;

       OriginalMode = Length;
       VideoMode    = OriginalMode;

       startx = starty = 0;
       endx = Length - 1;
       endy = Width - 1;
       screenwidth = Width - 1;
       screenheight = Length - 1;

       /* get outa here */
       return cur_mode;
}

static int get_adapter( void )
{
       union REGS inregs,outregs;
       inregs.x.ax=0x1200;
       inregs.h.bl=0x32;
       int86(0x10,&inregs,&outregs);
       if (outregs.h.al==0x12) return VGA;
       inregs.h.ah=0x12;
       inregs.h.bl=0x10;
       int86(0x10,&inregs,&outregs);
       if (outregs.h.bl < 4) return EGA;
       inregs.h.ah=0x0F;
       int86(0x10,&inregs,&outregs);
       if (outregs.h.al == 7) return MDA;
       return CGA;
}

void ScrClose(void)
{
       union REGS regs;

       /* turn on blinking */
       regs.h.ah = 0x10;
       regs.h.al = 0x03;
       regs.h.bl = 0x01;
       int86(VIDEO, &regs, &regs);
}

#if 0
int ScrTextMode(int mode)
{
       int mode_to_set = mode;
       struct SREGS sregs;
       union REGS regs;

       if (mode == LASTMODE)
          mode_to_set = OriginalMode;

       if (mode == C4350) mode_to_set = 0x03;
       regs.h.ah = 0x00;               /* set mode */
       regs.h.al = mode_to_set;
       int86(0x10, &regs, &regs);

       if (mode == CO80 || mode == BW80 || mode == C4350)
       {  if (VideoType>=EGA)
          {  regs.h.ah = 0x12;
             regs.h.bl = 0x34;
             regs.h.al = 0x00;         /* 0: enable (1: disable) */
             int86(0x10, &regs, &regs);
          }
       }

       if (mode == C4350)
       {  if (VideoType>=EGA)
          {  regs.x.ax = 0x1112;
             regs.x.bx = 0;
             int86(0x10, &regs, &regs);
          }
       }

       /* check to see if Base Address changed */
       regs.h.ah = 0x0F;
       int86(VIDEO, &regs, &regs);

       cur_mode  = (int) regs.h.al;
#ifdef __DJGPP__
       BaseAdr  = ( cur_mode == 7 ) ? 0xB0000 : 0xB8000;
#else
       BaseAdr  = ( cur_mode == 7 ) ? 0xB000 : 0xB800;
#endif
       cur_page  = (int) regs.h.bh;
       VideoMode = mode;
       Width     = (int) regs.h.ah;

       /* find length of the original screen */
       regs.x.ax = 0x1130;
       regs.h.bh = 0;
       regs.x.dx = 0;
       int86(VIDEO,&regs,&regs);

       Length = regs.x.dx + 1;
       if (Length == 1) Length = 25;

       startx       = starty = 0;
       endx         = Length - 1;
       endy         = Width - 1;
       screenwidth  = Width - 1;
       screenheight = Length - 1;

       ScrClrscr();
       return 1;
}
#endif

void ScrCursorSetPos( int line, int col)
{
    union REGS regs;

    regs.h.ah = 0x02;
    regs.h.bh = (unsigned char) cur_page;
    regs.h.dh = (unsigned char) line;
    regs.h.dl = (unsigned char) col;
    int86(VIDEO,&regs,&regs);
}

void ScrCursorGetPos( int *line,  int *col )
{
    union REGS regs;
    regs.h.ah = 0x03;
    regs.h.bh = (unsigned char) cur_page;
    int86(VIDEO,&regs,&regs);

    *line = (int) regs.h.dh;
    *col  = (int) regs.h.dl;
}

void ScrPutChar( int line,  int col,  int attr, int ch)
{
    union REGS regs;
    USHORT scrnofs;
//    if (directvideo == SA_DIRECT)
//        {
        scrnofs = (line * Width * 2) + (col * 2);
        ScrEnableVideoCGA(CGA_DISABLE);
        POKEB( BaseAdr, scrnofs, ch );
        POKEB( BaseAdr, scrnofs+1, attr );
        ScrEnableVideoCGA(CGA_ENABLE);
//        }
#if 0
    else
        {
        regs.h.ah = 2;
        regs.h.bh = (unsigned char) cur_page;
        regs.h.dh = (unsigned char) line;
        regs.h.dl = (unsigned char) col;
        int86(VIDEO,&regs,&regs);

        regs.h.ah = 9;
        regs.h.al = (unsigned char) ch;
        regs.h.bh = (unsigned char) cur_page;
        regs.h.bl = (unsigned char) attr;
        regs.x.cx = 1;
        int86(VIDEO,&regs,&regs);
        }
#endif
}

void ScrollBox( int topLine,  int leftCol, int btmLine,  int rightCol, int attr,  int dir, int noOfLines)
{
    union REGS regs;

    regs.h.ah = (dir == UP) ? V_SCROLLUP : V_SCROLLDN;
    regs.h.al = (unsigned char) noOfLines;
    regs.h.bh = (unsigned char) attr;
    regs.h.ch = (unsigned char) topLine;
    regs.h.cl = (unsigned char) leftCol;
    regs.h.dh = (unsigned char) btmLine;
    regs.h.dl = (unsigned char) rightCol;
    int86(VIDEO,&regs,&regs);
}

void ScrClrscr()
{
    ScrollBox(startx,starty,endx,endy,cur_attr, UP, 0);
    ScrCursorSetPos(startx,starty);
    curx=cury=0;
}

void
ScrSetCursorType( USHORT state )
{
   union REGS regs;

   regs.h.ah = 0x01;
   /* ch == start line. cl == end line */

   switch (state) {
//   {      case _SOLIDCURSOR :  regs.x.cx = (cur_mode == 7) ? 0x010C : 0x0107; break;
          case _NORMALCURSOR:  regs.x.cx = (cur_mode == 7) ? 0x0B0C : 0x0607; break;
          case _NOCURSOR    :  regs.x.cx = 0x2020; break;
//          case _HALFCURSOR  :  regs.x.cx = (cur_mode == 7) ? 0x070C :  0x0407; break;
//          default           :  regs.x.cx = state;  break;
   }
   int86(VIDEO,&regs,&regs);
}

#if 0
int
ScrGetche( void )
{
       int ch = ScrGetch();
       if (ch == '\b' || ch == '\t' || ch == '\n')
          cputn( (char *)&ch, 1 );
       if (ch >= 32 && ch <= 127)
          cputn( (char *)&ch, 1 );
       return ch;
}
#endif

int
ScrUngetch( int ascii )
{
       union REGS rg;
       int key = 0;
       if (ascii >= 256) { key = ascii - 256; ascii = 0xE0; }

       rg.x.cx = (key << 8) + ascii;
       rg.h.ah = 0x05;
       int86(KEYBOARD, &rg, &rg);
       return (!(int) rg.h.al);
}

#if 0
int
ScrKbhit( void )
{
       union REGS regs;
       regs.h.ah = 0x0B;
       int86(0x21, &regs, &regs);
       return (regs.h.al)?-1:0;
}
#endif

int
ScrGetch( void )
{
     union REGS regs;
     regs.x.ax = 0x0700;
     int86(0x21, &regs, &regs);
     return (regs.h.al)&0xFF;
}

#if 0
void
ScrFlush( void )
{
       while (ScrKbhit()) ScrGetch();
}
#endif

#define TIMERMODE 182           /* code to put timer in right mode */
#define FREQSCALE 1193280L      /* basic time frequency in hertz   */
#define TIMESCALE 1230L         /* number of counts in 0.1 second  */
#define T_MODEPORT 67           /* port controls timer mode        */
#define FREQPORT   66           /* port controls tone frequency    */
#define BEEPPORT   97           /* port controls speaker           */
#define BEEP_ON    79           /* signal to turn speaker on       */

#if 0
void   honk( USHORT freq, USHORT duration )
{      int  hibyte, lowbyte;
       long divisor;

       divisor = FREQSCALE / (freq+1);       /* scale freq to timer units  */
       lowbyte = divisor % 256;          /* break integer into         */
       hibyte  = divisor >> 8;           /*  two bytes                 */

       outp (T_MODEPORT, TIMERMODE);   /* prepare timer for input    */
       outp (FREQPORT, lowbyte);       /* set low byte of timer reg  */
       outp (FREQPORT, hibyte);        /* set high byte of timer reg */

       outp (BEEPPORT, BEEP_ON);       /* turn speaker on            */

       delay( duration * 10 );

       outp (BEEPPORT, inp(BEEPPORT) & 0xFC);          /* turn speaker off, restore  */
}
#endif

#ifndef HAVE_DELAY
void  delay( WORD msec )
{     clock_t t0;
      unsigned long diff = 0L;

      for (t0 = clock(); diff < (unsigned long)msec; )
      {     diff = (unsigned long)(clock() - t0);
            diff *= CLK_TCK;
            diff /= CLK_TCK;
      }
}
#endif

#if defined(MICROSOFT) || defined(POWERC) || defined(DJGPP)
#define STD_SIZE        0
#define SHORT_SIZE      1
#define LONG_SIZE       2
#define SPACE_PAD       0x10

static char* _strscn(const char *s, const char *pattern);

static int vsscanf(const char *buf, const char *fmt, va_list parms)
{
         int scanned = 0, size = 0, suppress = 0;
         long int w = 0, flag = 0, l = 0;
         char c, *c_ptr;
         long int n1, *n1l;
         int *n1b;
         short int *n1s;
         unsigned long n2, *n2l, parsing = 0;
         unsigned *n2b;
         unsigned short *n2s;
         double n3, *n3l;
         float *n3s;
         char *base = (char*) buf;
         while (*fmt != 0) {
                 if (*fmt != '%' && !parsing) {
                         /* No token detected */
                         fmt++;
                 } else {
                         /* We need to make a conversion */
                         if (*fmt == '%') {
                                 fmt++;
                                 parsing = 1;
                                 size = STD_SIZE;
                                 suppress = 0;
                                 w = 0;
                                 flag = 0;
                                 l = 0;
                         }
                         /* Parse token */
                         switch (*fmt) {
                         case '1':
                         case '2':
                         case '3':
                         case '4':
                         case '5':
                         case '6':
                         case '7':
                         case '8':
                         case '9':
                         case '0':
                                 if (parsing == 1) {
                                         w = strtol(fmt, &base, 10);
                                         /* We use SPACE_PAD to parse %10s
                                          * commands where the number is the
                                          * maximum number of char to store!
                                          */
                                         flag |= SPACE_PAD;
                                         fmt = base - 1;
                                 }
                                 break;
                         case 'c':
                                 c = *buf++;
                                 c_ptr = va_arg(parms, char *);
                                 *c_ptr = c;
                                 scanned++;
                                 parsing = 0;
                                 break;
                         case 's':
                                 c_ptr = va_arg(parms, char *);
                                 while (*buf != 0 && isspace(*buf))
                                         buf++;
                                 l = 0;
                                 while (*buf != 0 && !isspace(*buf)) {
                                         if (!(flag & SPACE_PAD))
                                                 *c_ptr++ = *buf;
                                         else if (l < w) {
                                                 *c_ptr++ = *buf;
                                                 l++;
                                         }
                                         buf++;
                                 }
                                 *c_ptr = 0;
                                 scanned++;
                                 parsing = 0;
                                 break;
                         case 'i':
                         case 'd':
                                 buf = _strscn(buf, "1234567890-+");
                                 n1 = strtol(buf, &base, 10);
                                 buf = base;
                                 if (!suppress) {
                                         switch (size) {
                                         case STD_SIZE:
                                                 n1b = va_arg(parms, int *);
                                                 *n1b = (int) n1;
                                                 break;
                                         case LONG_SIZE:
                                                 n1l = va_arg(parms,
                                                              long int *);
                                                 *n1l = n1;
                                                 break;
                                         case SHORT_SIZE:
                                                 n1s = va_arg(parms,
                                                              short int *);
                                                 *n1s = (short) (n1);
                                                 break;
                                         }
                                         scanned++;
                                 }
                                 parsing = 0;
                                 break;
                         case 'u':
                                 buf = _strscn(buf, "1234567890");
                                 n2 = strtol(buf, &base, 10);
                                 buf = base;
                                 if (!suppress) {
                                         switch (size) {
                                         case STD_SIZE:
                                                 n2b = va_arg(parms,
                                                              unsigned *);
                                                 *n2b = (unsigned) n2;
                                                 break;
                                         case LONG_SIZE:
                                                 n2l = va_arg(parms,
                                                              unsigned long *);
                                                 *n2l = n2;
                                                 break;
                                         case SHORT_SIZE:
                                                 n2s = va_arg(parms, unsigned short *);
                                                 *n2s = (short) (n2);
                                                 break;
                                         }
                                         scanned++;
                                 }
                                 parsing = 0;
                                 break;
                         case 'x':
                                 buf = _strscn(buf, "1234567890xabcdefABCDEF");
                                 n2 = strtol(buf, &base, 16);
                                 buf = base;
                                 if (!suppress) {
                                         switch (size) {
                                         case STD_SIZE:
                                                 n2b = va_arg(parms, unsigned *);
                                                 *n2b = (unsigned) n2;
                                                 break;
                                         case LONG_SIZE:
                                                 n2l = va_arg(parms, unsigned long *);
                                                 *n2l = n2;
                                                 break;
                                         case SHORT_SIZE:
                                                 n2s = va_arg(parms, unsigned short *);
                                                 *n2s = (short) (n2);
                                                 break;
                                         }
                                         scanned++;
                                 }
                                 parsing = 0;
                                 break;
                         case 'f':
                         case 'g':
                         case 'e':
                                 buf = _strscn(buf, "1234567890.e+-");
                                 n3 = strtod(buf, &base);
                                 buf = base;
                                 if (!suppress) {
                                         switch (size) {
                                         case STD_SIZE:
                                                 n3l = va_arg(parms, double *);
                                                 *n3l = n3;
                                                 break;
                                         case LONG_SIZE:
                                                 n3l = va_arg(parms, double *);
                                                 *n3l = n3;
                                                 break;
                                         case SHORT_SIZE:
                                                 n3s = va_arg(parms, float *);
                                                 *n3s = (float) (n3);
                                                 break;
                                         }
                                         scanned++;
                                 }
                                 parsing = 0;
                                 break;
                         case 'l':
                                 size = LONG_SIZE;
                                 break;
                         case 'h':
                         case 'n':
                                 size = SHORT_SIZE;
                                 break;
                         case '*':
                                 suppress = 1;
                                 break;
                         default:
                                 parsing = 0;
                                 break;
                         }
                         fmt++;
                 }
         }
         return (scanned);
}

static  char* _strscn(const char *s, const char *pattern)
{
        const char *scan;
        while (*s != 0) {
              scan = pattern;
              while (*scan != 0) {
                    if (*s == *scan)
                                return (char*)(s);
                    else
                                scan++;
              }
              s++;
        }
        return (NULL);
}
#endif

#if 0
void ScrHaveSnow( int tf )
{
     VideoType = (tf) ? CGA : get_adapter();
}

ScrMode ScrGetMode(void)
{
    return VideoMode;
}

void ScrSetAccess(ScrAccess access)
{
    directvideo = access;
}

ScrAccess ScrGetAccess(void)
{
    return directvideo;
}

void ScrDimensions( int *wid, int *len )
{
    *wid = Width;
    *len = Length;
}
#endif

#if 0
void ScrGetChar( int line, int col, int *attr, int *ch)
{
    union REGS regs;
    USHORT scrnofs;

    if (directvideo == SA_DIRECT)
        {
        ScrEnableVideoCGA(CGA_DISABLE);
        scrnofs = (line * Width << 1) + (col << 1) + (4096 * cur_page);
        *ch   = PEEKB( BaseAdr, scrnofs );;
        *attr = PEEKB( BaseAdr, scrnofs+1 );
        ScrEnableVideoCGA(CGA_ENABLE);
        }
    else
        {
        regs.h.ah = 2;
        regs.h.bh = (unsigned char) cur_page;
        regs.h.dh = (unsigned char) line;
        regs.h.dl = (unsigned char) col;
        int86(VIDEO,&regs,&regs);

        regs.h.ah = 8;
        regs.h.bh = (unsigned char) cur_page;
        int86(VIDEO,&regs,&regs);

        *attr = (int) regs.h.ah;
        *ch   = (int) regs.h.al;
        }
}
#endif

#if 0
void ScrPrintf( int line,  int col,  int attr, char *format, ...)
{
    va_list args;
    char buffer[MAXCHAR];

    va_start(args, format);
    vsprintf(buffer,format,args);
    va_end(args);

    ScrPutStr( line, col, attr, buffer );
}

/***************************************************************************/
int  ScrGets(char *Buffer, int Row, int Col, int Attr, int Length, int Mode)
{
     register int Pos = 0;                         /* current column/loop index */
     int Key, InsertFlag=0, Len=strlen(Buffer); /* stores input key value    */

     ScrSetCursorType(_NORMALCURSOR);
     ScrFlush();
     if (Len>Length) Len=Length;

     for (;;)/* Keyboard read loop */
     {   ScrWritef( Row, Col, Attr, Length, "%s", Buffer );
         ScrCursorSetPos( Row, Col+Pos );

         switch (Key = getkey())
         {   case 27:      /* Escape          */ return (27);
             case HOME:  Pos=0; break;
             case END:   Pos=Len; break;
             case LEFT:  if (Pos > 0) Pos--;
                           break;
             case RIGHT: if (Pos < Len) Pos++;
                           break;
             case TAB:                         return (TAB);
             case BACKTAB:                     return (BACKTAB);
             case UP:    /* Up-Arrow        */ return (UP);
             case DN:  /* Down-Arrow      */ return (DN);
             case 13:      /* Return          */ return (13);
             case INS:   InsertFlag = !InsertFlag;
                           ScrSetCursorType( (InsertFlag)?_SOLIDCURSOR:_NORMALCURSOR);
                           break;
             case DEL:   if (Pos < Len)
                           {  memmove(&Buffer[Pos], &Buffer[Pos+1], Len - Pos);
                              Len--;
                           }
                           break;
             case BS:    /* Backspace       */
                           if (Pos>0)
                           {  memmove(&Buffer[Pos-1], &Buffer[Pos], Len - Pos + 1);
                              Pos--;
                              Len--;
                           }
                           break;
             default:      if (islegal(Mode, Key, Pos) && Pos < Length)
                           {  if (InsertFlag)
                              {  memmove(&Buffer[Pos + 1], &Buffer[Pos], Len - Pos + 1);
                                 Buffer[Pos]=' '; Buffer[Len+1]=0;
                                 Len++;
                              }
                              if (Mode & UPPER)       /* Uppercase conversion     */
                                 Key = toupper (Key);
                              if (Mode & LOWER)
                                 Key = tolower (Key);
                              /* Place key in video memory & buffer   */
                              Buffer[Pos++] = Key;
                              if (Pos > Len) Len++;
                              if (Len > Length) Len=Length;
                              if ((Pos == Length-1) && (Mode & AUTOEXIT))
                                 return (13);     /* Code for autoexit       */
                           }
                           break;
         }
         Buffer[Len]=0;
     }
}
#endif

static int endstroke( int c )
{
     if (c == TAB || c == BACKTAB || c == RETURN || c == ESC) return 1;
     return (int) (c >= 256);
}

#if 0
static int islegal( int type, int ch, int pos )
{
    if (ch < 32 || ch > 126) return FALSE;
    if (type & ALPHA)
    {
       if (isspace(ch)) return TRUE;
       if (isalpha(ch)) return TRUE;
       return FALSE;
    }
    if ((pos==0)&&(ch=='+'||ch=='-'))return TRUE;
    if (ch=='.') return TRUE;
    return isdigit(ch);
}
#endif

#if 0
void ScrWritef( int line, int col, int attr, int width, char *format, ...)
{
    va_list args;
    char    buffer[MAXCHAR];
    int     len;

    va_start(args, format);
    vsprintf(buffer,format,args);
    va_end(args);

    buffer[width] = '\0';
    if ((len = strlen(buffer)) < width)
       memset(&buffer[len], 32, width - len);

    ScrPutStr(line, col, attr, buffer);
}
#endif

#if 0
void ScrPutf( char *str, int len )
{
    int line, col, ch, attr;
    ScrCursorGetPos(&line, &col);
    ScrGetChar(line, col, &attr, &ch);
    ScrWritef(line, col, attr, len, "%s\0", str);
}
#endif

#if 0
void ScrPutStr( int line, int col, int attr, char *str)
{
    register int i;

    for (i = 0; (str[i] != '\0') && (col < Width); i++, col++)
        ScrPutChar( line, col, attr, str[i] );
}
#endif

#if 0
int ScrSetBox( int set, int topleft, int top, int topright, int right, int lowright,
               int low, int lowleft, int left )
{
    unsigned char *ch = &Box_Chars[10 * set];

    *(ch + 0) = (unsigned char) top;
    *(ch + 1) = (unsigned char) low;
    *(ch + 2) = (unsigned char) left;
    *(ch + 3) = (unsigned char) right;
    *(ch + 4) = (unsigned char) topleft;
    *(ch + 5) = (unsigned char) topright;
    *(ch + 6) = (unsigned char) lowleft;
    *(ch + 7) = (unsigned char) lowright;
    *(ch + 8) = 0x01;

    return (1);
}
#endif

#if 0
void    hline( int row, int y1, int y2, int ch, int attr )
{       register int i;
        for (i = y1; i <= y2; i++) ScrPutChar( row, i, attr, ch );
}

void    vline( int col, int x1, int x2, int ch, int attr )
{       register int i;
        for (i = x1; i <= x2; i++) ScrPutChar( i, col, attr, ch );
}
#endif

#if 0
int ScrDrawBox( int sx, int sy, int ex, int ey, int attr, BoxType type, int shadow_type  )
{
 register int i;
 unsigned char *ch = &Box_Chars[type * 10];

 if (!*(ch + 8))
    return (0);

 hline(sx, (sy + 1), (ey - 1), (int) *(ch + 0), attr);
 vline(sy, (sx + 1), (ex - 1), (int) *(ch + 2), attr);
 vline(ey, (sx + 1), (ex - 1), (int) *(ch + 3), attr);
 hline(ex, (sy + 1), (ey - 1), (int) *(ch + 1), attr);

 ScrPutChar( sx, sy, attr, *(ch + 4));
 ScrPutChar( sx, ey, attr, *(ch + 5));
 ScrPutChar( ex, sy, attr, *(ch + 6));
 ScrPutChar( ex, ey, attr, *(ch + 7));

 if ( type > 0 && shadow_type > 0 )
 {  if (shadow_type == 3)
    {   vline( sy-1, sx+1, ex+1, 176, RVS(attr));
        hline( ex+1, sy, ey-2, 176, RVS(attr));
    }
    else if (shadow_type == 1)
    {   vline( ey+1, sx+1, ex+1, 176, RVS(attr));
        hline( ex+1, sy+2, ey, 176, RVS(attr));
    }
    else if (shadow_type == 2)
    {   vline( sy-1, sx-1, ex-1, 176, RVS(attr));
        hline( sx-1, sy-1, ey-2, 176, RVS(attr));
    }
    else
    {   vline( ey+1, sx-1, ex-1, 176, RVS(attr));
        hline( sx-1, sy+2, ey+1, 176, RVS(attr));
    }
 }
 return (TRUE);
}

void ScrClearLine( int line, int col)
{
     register int c;

     for (c = col; c < Width; ++c)
         ScrPutChar(line,c,7,' ');
}

void change_attr( int row, int col, int length, int attr )
{
     WORD         a, offset;
     register int k;
     union REGS rg;

     if (directvideo == SA_DIRECT)
     {
       a      = attr << 8;
       offset = (Width * 2 * row) + (2 * col) + (4096 * cur_page);

       ScrEnableVideoCGA(CGA_DISABLE);
       for ( k = 0; k < length; k++ )
           {
            POKE( BaseAdr, offset, a | (PEEK(BaseAdr, offset) & 255) );
            offset += 2;
           }
       ScrEnableVideoCGA(CGA_ENABLE);
     }
     else {
       for ( k = 0; k < length; k++, col++)
           {
            rg.h.ah = 0x02;
            rg.h.bh = (unsigned char) cur_page;
            rg.h.dh = (unsigned char) row;
            rg.h.dl = (unsigned char) col;
            int86(0x10, &rg, &rg);
            rg.h.ah = 0x08;
            rg.h.bh = (unsigned char) cur_page;
            int86(0x10, &rg, &rg);
            rg.h.ah = 0x09;
            rg.x.cx = 1;
            rg.h.bh = (unsigned char) cur_page;
            rg.h.bl = (unsigned char) attr;
            int86(0x10, &rg, &rg);
           }
     }
}
#endif

#if 0
static int  getkey( void )
{    int c = ScrGetch();
     if (c == 0 || c == 0xE0) c = ScrGetch() + 256;
     return c;
}
#endif

#if 0
int
iscolor( void )
{
      WORD monitor;

      monitor = (WORD) PEEK (0x0000, 0x0463);
      if (monitor != 0x3B4)
         return (1);
      return (0);
}
#endif


