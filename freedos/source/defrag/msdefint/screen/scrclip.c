/*
   scrclip.c - routines to copy and paste pieces from the screen.
   Copyright (C) 2000 Imre Leber

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
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

   If you have any questions, comments, suggestions, or fixes please
   email me at:  imre.leber@vub.ac.be
*/

#include <conio.h>
#include "..\mouse\mouse.h"

#define PAGES 2

static char ClipBoard[PAGES][80*25*2]; /* for 80*25 screen mode.  */
static int ClipBoardIndex = 0;         /* index in the clipboard. */

static int SavedLeft[PAGES]; 
static int SavedTop[PAGES]; 
static int SavedRight[PAGES]; 
static int SavedBottom[PAGES];

/*
     Copies a region of the screen to the clipboard.
*/
void CopyScreen (int left, int top, int right, int bottom)
{
     LockMouse(left, top, right, bottom);

     SavedLeft[ClipBoardIndex]   = left;
     SavedTop[ClipBoardIndex]    = top;
     SavedRight[ClipBoardIndex]  = right;
     SavedBottom[ClipBoardIndex] = bottom;
     
     gettext(left, top, right, bottom, ClipBoard[ClipBoardIndex]);

     UnLockMouse();
}


/*
     Pastes a region of the screen from the clipboard.
*/
void PasteScreen (int xincr, int yincr)
{
     LockMouse(SavedLeft[ClipBoardIndex]   + xincr, 
               SavedTop[ClipBoardIndex]    + yincr,
               SavedRight[ClipBoardIndex]  + xincr,
               SavedBottom[ClipBoardIndex] + yincr);

     puttext(SavedLeft[ClipBoardIndex]+xincr, 
                SavedTop[ClipBoardIndex]+yincr, 
                SavedRight[ClipBoardIndex]+xincr, 
                SavedBottom[ClipBoardIndex]+yincr, 
                ClipBoard[ClipBoardIndex]);

     UnLockMouse();
}

/*
     Changes the clipboard index.
*/
void SetClipIndex(int index)
{
     ClipBoardIndex = index;
}

