/* Convert graphical HP-PCL print data into raw pixel byte array  */
/* Public Domain by Eric Auer, May 2003, for Epson printer tests  */

/* recognized (only ESC*b123W... really processed):
 * ESC "E" - reset
 * CR      - CR
 * LF      - LF
 * FF      - FF
 * ESC "*t" "123" "R" - set to 123 dpi
 * ESC "*r" "1" "A"   - start at current column
 * ESC "*r" "1" "U"   - use 1 plane (b/w)
 * ESC "*r"     "B"   - end graphics data
 * ESC "*b"  "0" "M"  - no compression
 * ESC "*b" "123" "V" - 123 bytes of graphics data, not last plane
 * ESC "*b" "123" "W" - 123 bytes of graphics data coming, last plane
 * (after that, go to next line)
 * NOT recognized: ESC typestring number lowercase-letter, which would
 * mean that another number and letter follow, as continuation of the
 * current ESC typestring ... sequence: An uppercase-letter marks the
 * end of an ESC typestring number letter sequence. See HP manuals.
 */

/* compile: gcc -Wall -s -o hp-read hp-read.c               */
/* use: ./hp-read <hppcldata >bitmapdata                    */
/* then: convert -size 1600x2000 gray:bitmapdata result.png */
/* (adjust the 1600x2000 value to what hp-read reports)     */

#include <stdio.h>
/* #include <stdlib.h> *//* rand, random, srand, ... */
#include <unistd.h> /* write */

#define byte unsigned short
#define XPAPER 2300
#define YPAPER 3000

int seed = 0;

byte pixels[XPAPER][YPAPER];

int Parser(int * value) {
  int chi;
  *value = 0;
  chi = 0;
  while (chi!=EOF) {
    chi = getc(stdin);
    if (chi<32)
      fprintf(stderr,"%2.2x",chi);
    else
      fprintf(stderr,"%c",chi);
    if ((chi>='0') && (chi<='9')) {
      *value *= 10;
      *value += (chi-'0');
      continue;
    } else
    if ((chi>='A') && (chi<='Z')) {
      return chi;
    } else
    return EOF; /* unexpected condition! */
  } /* while */
  return EOF;
} /* Parser */

int main () {
  byte by;
  int chi;
  int x;
  int xcnt;
  int xused = -1;
  int y;
  int yused = -1;
  int cols;

  for (x=0;x<XPAPER;x++)
    for (y=0;y<YPAPER;y++)
      pixels[x][y] = 0;

  x = 0;
  y = 0;
  while (1) {
    chi = getc(stdin);
    if (chi == EOF)
      break;
    by = (byte)chi;
    if (by<32) {
      switch (by) {
        case 13: fprintf(stderr,"<CR>");
          break;
        case 10: fprintf(stderr,"<LF>\n");
          break;
        case 12: fprintf(stderr,"<FF>\n");
          break;
        case 27: {
          chi = getc(stdin);
          if (chi==EOF)
            goto done;
          switch (chi) {
            case 'E': fprintf(stderr,"<Reset>\n");
              break;
            case '*': {
              chi = getc(stdin);
              if (chi==EOF)
                goto done;
              if (chi<32)
                fprintf(stderr,"<ESC>*<%2.2x>",chi);
              else
                fprintf(stderr,"<ESC>*%c",chi);
              switch (chi) {
                case EOF: goto done;
                case 't': chi = Parser(&cols);
                  if (chi=='R')
                    fprintf(stderr," resolution set to %d\n",cols);
                  else
                    fprintf(stderr," ??? (%c) set to %d\n", chi, cols);
                  break;
                case 'r': chi = Parser(&cols);
                  if (chi=='A')
                    fprintf(stderr," starting at %s position (choice %d)\n",
                      (cols>0) ? "CURRENT" : "left", cols);
                  else {
                    if (chi=='U') {
                      fprintf(stderr," using %d planes in %s space\n",
                        (cols>0) ? cols : -cols,
                        (cols>0) ? ((cols==1) ? "s/w" : "RGB(K)")
                                 : "CMY(K)" );
                    } else {
                      if (chi=='B')
                        fprintf(stderr,"\r<ESC>*b...."
                          "\n<ESC>*rB end of graphics mode (%d)\n",cols);
                      else
                        fprintf(stderr," ??? (%c) set to %d\n", chi, cols);
                    }
                  } /* ESC*r..., not A */
                  break;
                case 'b': chi = Parser(&cols);
                  if (chi=='M')
                    fprintf(stderr," compression type %d\n", cols);
                  else {
                    if ((chi=='V') || (chi=='W')) {
#if 0
                      fprintf(stderr," graphics data, %s plane, %d bytes\n",
                      (chi=='V') ? "normal" : "last/only", cols);
#endif
#if 0
                      fprintf(stderr,"%d%c...", cols, chi);
#endif
#if 1
                      fprintf(stderr," graphics data, %s plane, %d bytes%s\r",
                      (chi=='V') ? "normal" : "last/only", cols, "          ");
#endif
                      by=chi;
                      while (cols>0) {
                        chi = getc(stdin);
                        if (chi==EOF)
                          goto done;
                        for (xcnt=0x80; xcnt!=0; xcnt>>=1) {
                          if ((chi & xcnt) != 0)
                            pixels[x][y] = 0; /* pixel set: black */
                          else
                            pixels[x][y] = 255; /* no pixel: white */
                          x++;
                        } /* for bits in byte */
                        cols--;
                      } /* while graphics data */
                      if (x>xused)
                        xused=x;
                      x = 0; /* go back to left - other not yet supported */
                      if (by=='W') {
                        y++;
                        yused = y;
                      }
                    } else
                      fprintf(stderr," ??? (%c) set to %d\n", chi, cols);
                  } /* ESC*b..., not M */
                  break;
                default: /* (not parsed) */
              } /* switch */
              break;
            } /* ESC*... */
            default: if (chi<32)
              fprintf(stderr,"<ESC><%2.2x>",chi);
            else
              fprintf(stderr,"<ESC>%c",chi);
          } /* switch */
          break;
        } /* ESC */
        default: fprintf(stderr,"<%2.2x>",by); break;
      } /* switch */
    } else {
      fprintf(stderr,"%c",by);
    }
  } /* while */

done:
  fprintf(stderr,"\n");
  for (y=0;y<yused;y++) {
    for (x=0;x<xused;x++)
      fwrite(&(pixels[x][y]), 1, 1, stdout);
    fprintf(stderr,"\rWriting row: %5d / %5d", y+1, yused);
  }

  fprintf(stderr,"\nPaper size: %d x %d\n", xused, yused);
  return 0;
}

