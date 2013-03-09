/* Convert graphical Epson print data into raw pixel byte array  */
/* Public Domain by Eric Auer, May 2003, for Epson printer tests */

/* recognized:
 * ESC "3" n  - updates n
 * CR         - X = 0
 * LF         - Y + n pixels, default 24, print "." message
 * FF         - print a message
 * ESC "0"    - ignored
 * ESC "*" 39 lo hi data - read hi:lo times 3 bytes of data
 */

/* compile: gcc -Wall -s -o read-epson read-epson.c         */
/* use: ./read-epson <epsondata >bitmapdata                 */
/* then: convert -size 1600x2000 gray:bitmapdata result.png */
/* (adjust the 1600x2000 value to what read-epson reports)  */

#include <stdio.h>
/* #include <stdlib.h> *//* rand, random, srand, ... */
#include <unistd.h> /* write */

/* define to make bitmap block boundaries visible: */
/* #define HINT 1 */

#define byte unsigned short
#define XPAPER 1600
#define YPAPER 2000

int seed = 0;

byte pixels[XPAPER][YPAPER];

int main () {
  byte by;
  byte thing = 255; /* color */
  int chi;
  int x;
  int xcnt;
  int xused = -1;
  int y;
  int ycnt;
  int ystep = 24;
  int yused = -1;
  int cols;
  unsigned long map;

  for (x=0;x<XPAPER;x++)
    for (y=0;y<YPAPER;y++)
      pixels[x][y] = 0;

  x = 0;
  y = 0;
  do {
    chi = getc(stdin);
    if (chi == EOF) break;
    by = (byte)chi;
    if (by == 13) { /* CR ? */
      x = 0;
    } else if (by == 10) { /* LF ? */
      y += ystep;
      yused = y + ystep;
      if (yused>YPAPER) {
        fprintf(stderr,"*"); /* too high */
        y = YPAPER - ystep;
      }
      fprintf(stderr,".");
    } else if (by == 12) { /* FF ? */
      fprintf(stderr,"\nFormFeed\n");
    } else if (by == 27) { /* ESC ? */
      chi = getc(stdin);
      if (chi == '0') {
        /* ignore */
        fprintf(stderr,"\nIgnoring: ESC '0' (8 lines per inch)\n");
      } else
      if (chi == '3') {
        chi = getc(stdin);
        ystep = chi;
        fprintf(stderr,"\nSetting LF to %d pixels\n", ystep);
      } else
      if (chi == '*') {
#ifdef HINT
        thing = ((thing--) | 0xf0) & 0xff;
#endif
        chi = getc(stdin);
#ifdef DEBUG
        fprintf(stderr,"Mode: %d ",chi);
#endif
        chi = getc(stdin);
        cols = chi;
        chi = getc(stdin);
        cols |= (chi << 8);
#ifdef DEBUG
        fprintf(stderr,"Columns: %d\n",cols);
#endif
        for (xcnt=0; xcnt<cols; xcnt++) {
          map = 0;
          chi = getc(stdin);
          by = (byte)chi;
          map |= by; /* first byte on top */
          map <<= 8;
          chi = getc(stdin);
          by = (byte)chi;
          map |= by;
          map <<= 8;
          chi = getc(stdin);
          by = (byte)chi;
          map |= by;
          for (ycnt=0; ycnt<24; ycnt++) {
            if (map & 0x800000)
              pixels[x][y+ycnt] = 255-thing; /* 0 is black is set */
            else
              pixels[x][y+ycnt] = thing; /* 255 is white is clear */
            map <<= 1; /* MSB is on top */
          }
          if (x<(XPAPER-1))
            x++;
          else
            fprintf(stderr,">"); /* too wide */
        }
        if (x>xused)
          xused = x;
      }
    } else {
      /* pass on other non-graphic bytes as hex dump */
      fprintf(stderr,"[??? %2.2x]",by);
    }
  } while (chi != EOF);

  fprintf(stderr,"\n");
  for (y=0;y<yused;y++) {
    for (x=0;x<xused;x++)
      fwrite(&(pixels[x][y]), 1, 1, stdout);
    fprintf(stderr,"\rWriting row: %5d / %5d", y+1, yused);
  }

  fprintf(stderr,"\nPaper size: %d x %d\n", xused, yused);
  return 0;
}

