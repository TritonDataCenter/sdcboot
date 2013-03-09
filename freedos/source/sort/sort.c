/*
*    SORT - reads line of a file and sorts them in order
*    Copyright  1995  Jim Lynch
*    Updated 2003-2004 by Eric Auer: NLS for sort order, smaller binary.
*    Updated 2007 by Imre Leber: long file name support
*
*    This program is free software; you can redistribute it and/or modify
*    it under the terms of the GNU General Public License as published by
*    the Free Software Foundation; either version 2 of the License, or
*    (at your option) any later version.
*
*    This program is distributed in the hope that it will be useful,
*    but WITHOUT ANY WARRANTY; without even the implied warranty of
*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*    GNU General Public License for more details.
*
*    You should have received a copy of the GNU General Public License
*    along with this program; if not, write to the Free Software
*    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110, USA
*/
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "lfnapi.h"
#include "kitten.h"
#include <fcntl.h> /* O_RDONLY */
#define StdIN  0
#define StdOUT 1
#define StdERR 2

#ifndef __MSDOS__
#include <malloc.h>
#endif

#ifdef __MSDOS__
#include <dos.h> /* intdosx */

/*
NLS is int 21.65 -> get tables... country=-1 is current... The data:
02 - upcase table: 128 bytes of "what is that 0x80..0xff char in uppercase?"
03 - downcase table: 256 bytes of "what is that 0x00..0xff char in lowercase?"
  (only DOS 6.2+ with country.sys loaded, probably only codepage 866)
04 - filename uppercase table
05 - filename terminator table
06 - collating: 256 "values used to sort char 0x00..0xff" (I think you can
  use those byte values to "XLAT" (ASM) / look up (C) a number for each
  char, and then use that number for sorting comparisons)
07 - DBCS is somewhat different, see RBIL (double byte chars, DOS 4.0+)

The returned buffer contents are NOT the tables but a byte followed by
a far POINTER to the actual tables!
All tables are prefixed by a word indicating the table size (128, 256...)!
Minimum DOS version for NLS stuff is 3.3 ... For table request, give BX
and DX as codepage / country, or -1 to use current. You must provide a
buffer at ES:DI and tell its length in CX. On return, CY / buffer / CX set.
Only if NLSFUNC loaded, other codepages / countries than current are avail.

From DOS 4.0 on, you also have:
Upcase char:   21.6520.dl -> dl
Upcase array:  21.6521.ds:dx.cx (string at ds:dx, length cx)
Upcase string: 21.6522.ds:dx (string terminated by 0 byte)
*/

/* actually TINY was worst, because of non-malloc list[] array! */
#ifndef __TINY__     /* tiny model (.com) has no stack setting */
unsigned _stklen = 16383;  /* use bigger stack, for stdlib qsort...  */
#endif            /* default stack size is only 4096 bytes  */
/* for NORMAL (avg. 40 char/line) 10k line text, 6-8k of stack is ok */
/* on the other hand, 1400 "nnnn\tx" lines sorted /+5 take 32k stack */
/* (with 16k stack, you can "sort" up to 700 identical lines, etc.)  */

unsigned char far * collate;
#endif



#ifndef MAXPATH
#define MAXPATH 80
#endif

#ifndef MAXPATH95
#define MAXPATH95 270
#endif

/* Optimizations 7/2004: sort bigger files both with COM and EXE version  -ea */
#ifdef __TINY__   /* .com: 8k+1k kitten, 7k*2 list array, ca. 28k max file size */
#define MAXRECORDS  7000   /* maximum number of records that can be
             * sorted: we use an array of N pointers! */
#define MAXLEN 1023     /* maximum record length */
#else          /* if far pointers */
#define MAXRECORDS 12000   /* malloc limited to 64k -> max 16k items */
#define MAXLEN 8191     /* maximum record length */
#endif            /* !12k items means 48k for list[] alone! */
/* first 10k lines of Ralf Browns Interrupt List 61 are 382701 bytes, we */
/* can SORT them with "largest executable program size 626k (641,424)".  */
/* Limit for 626k seems to be - with 12k items and 12k stack - 400000 bytes */


int             rev;    /* reverse flag */
#ifdef __MSDOS__
int      nls;     /* NLS use flag */
#endif
int             help;      /* help flag */
int             sortcol;   /* sort column */
int             err = 0;   /* error counter */

void
WriteString(char *s, int handle) /* much smaller than fputs */
{
  write(handle, s, strlen(s));
}

int
cmpr(const void *a, const void *b)
{
    unsigned char *A, *B, *C;

    A = *(unsigned char **) a;
    B = *(unsigned char **) b;

    if (sortcol > 0) { /* "sort from column... " */
   if (strlen(A) > sortcol)
       A += sortcol;
   else
       A = "";
   if (strlen(B) > sortcol)
       B += sortcol;
   else
       B = "";
    }
    
    if (rev) { /* reverse sort: swap strings */
        /* (or swap sign of result, of course) */
        C = A;
        A = B;
        B = C;
    }

#ifdef __MSDOS__
    if (nls) {
        while (collate[A[0]] == collate[B[0]]) {
            /* we use collate in the while as well: two different *
             * bytes may have the same collate position...        */
            if (A[0] == '\0') return 0; /* both at end */
            A++;
            B++;
        }
        if (collate[A[0]] < collate[B[0]]) return -1;
        return 1;
    } else {
#endif
        return strcmp(A, B);
#ifdef __MSDOS__
    }
#endif
}

void
usage(nl_catd cat)
{
    if (cat != cat) {}; /* avoid unused argument error in kitten */
  
    WriteString("FreeDOS SORT v1.5\r\n", StdERR);  /* *** VERSION *** */

    if (err)
        WriteString(catgets(cat, 2, 0, "Invalid parameter\r\n"), StdERR);
    WriteString(catgets(cat, 0, 0, "    SORT [/R] [/+num] [/A] [/?] [file]\r\n"), StdERR);
    WriteString(catgets(cat, 0, 1, "    /R    Reverse order\r\n"), StdERR);
#ifdef __MSDOS__
#if 0
    WriteString(catgets(cat, 0, 2, "    /N    Enable NLS support\r\n"), StdERR);
#endif
    WriteString(catgets(cat, 0, 2, "    /A    Sort by ASCII, ignore COUNTRY\r\n"), StdERR);
#endif
    WriteString(catgets(cat, 0, 3, 
        "    /+num start sorting with column num, 1 based\r\n"), StdERR);
    WriteString(catgets(cat, 0, 4, "    /?    help\r\n"), StdERR);

}

int main(int argc, char **argv)
{
    char filename[MAXPATH95];
    char        origfilename[MAXPATH95];
#ifdef __TINY__
    char temp[MAXLEN + 1];
    char *list[MAXRECORDS]; /* eats up lots of stack! */
#else
    char *temp;
    char        **list;
#endif
    char *cp;  /* option character pointer */
    int     nr;
    int     i;
    /* FILE *fi; */  /* file descriptor */
    int     fi;   /* file HANDLE (fopen/... are big) */
    nl_catd cat;  /* handle for localized messages (catalog) */

#ifdef __MSDOS__
    /* MAKE SURE THAT YOU USE BYTE ALIGNMENT HERE! */
    struct NLSBUF {
      char id;
      unsigned char far * content;
    } collbuf;
    union REGS     dosr;
    struct SREGS   doss;
#endif

    cat = catopen ("sort", 0);
  
    sortcol = 0;
    strcpy(filename, "");
    rev = 0;
#ifdef __MSDOS__
    nls = 1;   /* *** changed to default ON 7/2004 *** */
#endif

    while (--argc) {
        if (*(cp = *++argv) == '/') {
            switch (cp[1]) {
       case 'R':
       case 'r':
           rev = 1;
           break;
#ifdef __MSDOS__
       case 'N':
       case 'n':
           nls = 1; /* is the new default anyway ... */
           break;
       case 'A':     /* use strict  ASCII  sort order */
       case 'a':
           nls = 0; /* new option 7/2004: suppress NLS collate */
           break;
#endif
       case '?':
       case 'h':
       case 'H':
           help = 1;
           break;
       case '+':
           sortcol = atoi(cp + 1);
           if (sortcol)
               sortcol--;
           break;
       default:
           err++;
            }
        } else {    /* must be a file name */
            strcpy(filename, *argv);
        }
    }

    if (err || help) {
        usage(cat);
        catclose(cat);
        exit(1);
    }

    /* *** following was switch case "N" (in above parse) before 7/2004 *** */
#ifdef __MSDOS__
    if (nls) {
        if ( ((_osmajor >= 3) && (_osminor >= 3)) || (_osmajor > 3) ) {
            dosr.x.ax = 0x6506; /* get collate table */
            dosr.x.bx = 0xffff; /* default codepage  */
            dosr.x.dx = 0xffff; /* default country   */
            dosr.x.cx = 5;      /* buffer size   */
            doss.es = FP_SEG(&collbuf);
            dosr.x.di = FP_OFF(&collbuf);
            intdosx(&dosr,&dosr,&doss);
            if ((dosr.x.flags & 1) == 1) {
                WriteString(catgets(cat, 2, 1,
                    /* catgets ... */           "Error reading NLS collate table\r\n"),
                    StdERR);
                nls = 0;
            } else {
                /* ... CX is returned as table length ... */
                collate = collbuf.content; /* table pointer */
                collate++; /* skip leading word, which is */
                collate++; /* not part of the table */
                /* "nls on" accepted now */
                /* printf("NLS SORT ENABLED %d\r\n",dosr.x.cx); */
            }
        } else {
            WriteString(catgets(cat, 2, 2,
                /* catgets ... */       "Only DOS 3.3 or newer supports NLS!\r\n"),
                StdERR);
            nls = 0;  /* *** disable NLS sort order *** */
        }
    } else {
        /* accepting "nls off" is easy ;-) */
        /* printf("NLS SORT DISABLED AT USER REQUEST\r\n"); */
    }
#endif
    /* *** end of former switch case "N" *** */

    fi = StdIN;         /* just in case */
    if (strlen(filename)) {

        strcpy(origfilename, filename);

        if (!LFNConvertToSFN(filename) ||
            (fi = open(filename, O_RDONLY)) == -1) {
                /* was: ... fopen(...,"r") ... == NULL ... */
                WriteString(catgets(cat, 2, 3, "SORT: Can't open "), StdERR);
                WriteString(origfilename,StdERR);
                WriteString(catgets(cat, 2, 4, " for read\r\n"), StdERR);
                /* avoided 1.5k-2k overhead of *printf()... */
                catclose(cat);
                exit(2);
            }
    }

    (void)get_line(fi, NULL, 0); /* initialize buffers */

#ifndef __TINY__
    nr = -1;
    list = (char **) malloc(MAXRECORDS * sizeof(char *));
    if (list == NULL) goto out_of_mem;
    temp = (char *) malloc(MAXLEN + 1);
    if (temp == NULL) goto out_of_mem;
#endif

    for (nr = 0; nr < MAXRECORDS; nr++) {
        if (!get_line(fi, temp, MAXLEN))
            /* was: fgets(temp, MAXLEN, fi) == NULL */
            break;
        list[nr] = (char *) malloc(strlen(temp) + 1);
        /* malloc might have big overhead, but we cannot avoid it */
        if (list[nr] == NULL) {
out_of_mem:
            WriteString(catgets(cat, 2, 5, "SORT: Insufficient memory\r\n"),
                StdERR);
#if 1
            itoa(nr, temp, 10);
            WriteString(temp, StdERR); /* print "offending" line number */
#endif
            catclose(cat);
            exit(3);
        }
        strcpy(list[nr], temp);
    }
#ifndef __TINY__
    /* free(temp); */   /* doesn't really help, qsort doesn't malloc anything */
    /* not using free() at all saves 0.7k (UPXed 0.3k) binary size... */
#endif

    if (nr == MAXRECORDS) {
        WriteString(catgets(cat, 2, 6,
            "SORT: number of records exceeds maximum\r\n"), /* catgets ... */
            StdERR);
        catclose(cat);
        exit(4);
    }

    qsort((void *) list, nr, sizeof(char *), cmpr);   /* *main action* */

    for (i = 0; i < nr; i++) {
        WriteString(list[i], StdOUT);
        WriteString("\r\n",StdOUT);
    }
    catclose(cat);
    return 0;
}

