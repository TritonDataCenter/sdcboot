/* A function to draw a listbox on the screen. */

/*
   Copyright (C) 2000 Jim Hall <jhall@freedos.org>

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/


/* No screen bounds checking is done here.  It is the responsibility
 of the programmer to ensure that the box does not exceed the screen
 boundaries. */

#ifdef __WATCOMC__
#include <screen.h>
#include <stdlib.h>
#include <direct.h>
#define fnmerge _makepath
#define MAXPATH _MAX_PATH
#else
#include <conio.h>
#include <dir.h>
#endif
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "dat.h"
#include "box.h"
#include "getkey.h"
#include "cat.h"
#include "lsm.h"
#include "isfile.h"

#define KEY_ENTER 13

dat_t *listbox(dat_t *dat_ary, int dat_count, char *fromdir)
{
    int boxlength = dat_count / 5, centre = 12 - boxlength, i, j;
    key_t ch;
    char *thecwd = getcwd(NULL, 0);

    centre += 4;

    textbackground(BLUE);
    textcolor(WHITE);

    /* OK, lets draw all of the selections using some simple calculations */
    for(i = 0, j = 0; i < dat_count; i++, j++) {
        gotoxy(8 + (j * 13), centre + (i / 5));
        if(toupper(dat_ary[i].rank) == 'N') cputs("[ ] ");
        else {
            cputs("[X] ");
            dat_ary[i].rank = 'Y';
        }
        /* Don't bother printing the pathname (if it exists) */
        cputs(strrchr(dat_ary[i].name, '\\') != NULL ?
              &strrchr(dat_ary[i].name, '\\')[1] : dat_ary[i].name);
        if(j == 4) j = -1;
    }
    /* Draw the DONE button */
    gotoxy(37, centre + (dat_count / 5) + 2);
    cputs("[DONE]");
    gotoxy(8, centre);
    cputs(toupper(dat_ary[i].rank) == 'N' ? "< >" : "<X>");
    gotoxy(9, centre);
    i = j = 0;

    /* Begin the selection loop; runs forever until ENTER is pressed */
    for(;;) {
        unsigned x = wherex(), y = wherey(), k, l;
        for(k = 19; k < 22; k++) for(l = 2; l < 80; l++) {
            gotoxy(l, k);
            putch(' ');
        }
        gotoxy(2, 20);
        if(i != dat_count) {
            char lsmfile[MAXPATH];
            unsigned total;

            chdir(fromdir);
            _dos_setdrive(toupper(fromdir[0]) - 'A' + 1, &total);
            fnmerge(lsmfile, "", fromdir, dat_ary[i].name, ".LSM");
            if(isfile(lsmfile)) lsm_description(20, 2, 3, lsmfile);
            else cat_file(dat_ary[i].name, 3);
            chdir(thecwd);
            _dos_setdrive(toupper(thecwd[0]) - 'A' + 1, &total);
        }
        gotoxy(x, y);
        ch = getkey();
        switch(ch.extended) {
            /* Moves the current selection to the next row */
            case KEY_DOWN:
                if(i < (dat_count - 4)) i += 5;
                else {
                    j = (dat_count - ((dat_count / 5) * 5) + 5) - 5;
                    i = dat_count;
                }
                break;
            /* 
             * Moves the current selection to the next column (or the first
             * column of the next row)
             */
            case KEY_RIGHT:
                if(i < dat_count) {
                    i++;
                    j++;
                }
                break;
            /* 
             * Moves the current selection to the previous row or to the
             * first column of the current row
             */
            case KEY_UP:
                if(i > 4) i -= 5;
                else j = i = 0;
                break;
            /*
             * Moves the current selection to the previous column (or the
             * last column of the previous row
             */
            case KEY_LEFT:
                if(i != 0) {
                    i--;
                    j--;
                }
                break;
        }
        switch(ch.key) {
            /* Complete the selection, return the results */
            case KEY_ENTER:
                free(thecwd);
                return(dat_ary);
            /* 
             * If pressed on the DONE button, return the results, else
             * toggle the current selection yes or no
             */
            case ' ':
                if(i == dat_count) {
                    free(thecwd);
                    return(dat_ary);
                }
                dat_ary[i].rank = (toupper(dat_ary[i].rank) == 'Y') ? 'N' : 'Y';
                break;
        }
        if(j > 4) j = 0;
        if(j < 0) j = 4;
        /* 
         * Change the previous selection borders to [], move the current to
         * <>
         */
        if (wherex() != 38) {
            gotoxy(wherex() - 1, wherey());
            putch('[');
            gotoxy(wherex() + 1, wherey());
            putch(']');
        }
        if(i == dat_count) gotoxy(38, centre + (dat_count / 5) + 2);
        else {
            gotoxy(8 + (j * 13), centre + (i / 5));
            cputs(toupper(dat_ary[i].rank) == 'Y' ? "<X>" : "< >");
            gotoxy(9 + (j * 13), centre + (i / 5));
        }
    }
}
