/*    
   Lowvideo.c - Low video routines for use in ScanDisk's interface.
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
/*
   Joe Cosentino added a small dialog for command buttons and reversed
   the > and < on command buttons.

   Imre - 28 october 2000

   28 october 2000: fixed the code by Joe Cosentino to allow for clickable 
                    buttons and moved the two functions:
                        
                        - DrawShadow, and
                        - DrawDialogShadow
                    
                    together by adding an extra parameter to DrawShadow.
*/

#include <conio.h>
#include <string.h>
#include <dos.h>

#include "screen.h"
#include "..\mouse\mouse.h"
#include "..\..\misc\bool.h"

void DrawShadow (int x1, int x2, int y1, int y2, int small);

static char DrawBuffer[80];

#define BCHARLEFT  16                /* > */
#define BCHARRIGHT 17                /* < */

/*
   Empties the screen and changes to a certain color.
*/

void ColorScreen (int color)
{
     LockMouse(1,1,MAX_X,MAX_Y);
     textbackground(color);
     clrscr();
     UnLockMouse();
}

/*
     Puts a certain text at the botom of the screen.
*/

void SetStatusBar (int forcolor, int backcolor, char* text)
{
     DrawText(1, MAX_Y, text, forcolor, backcolor);
}

/*
    Sets a '³' in a certain position on the botom line.
*/

void DelimitStatusBar (int forcolor, int backcolor, int x)
{
     ChangeCursorPos(x, MAX_Y);
     SetForColor(forcolor);
     SetBackColor(backcolor);

     LockMouse(x, MAX_Y, x, MAX_Y);

     DrawChar('³', 1);

     UnLockMouse();
}

/*
     Draw a box on the screen, the border characters have to be given.
*/

void DrawBox (int x, int y, int lenx, int leny, int forcolor, int backcolor,
              int tlc, int trc, int blc, int brc, int hc, int vc,
              char* caption)
{
     int  i, pos;
     
     SetForColor(forcolor);
     SetBackColor(backcolor);

     LockMouse(x, y, x+lenx+1, y+leny+1);

     /* Draw top line. */
     ChangeCursorPos(x, y);
     PrintChar(tlc, 1); PrintChar(hc, lenx-1); DrawChar(trc, 1);

     /* Draw botom line. */
     ChangeCursorPos(x, y + leny);
     PrintChar(blc, 1); PrintChar(hc, lenx-1); DrawChar(brc, 1);

     /* Draw vertical lines. */
     for (i = 1; i < leny; i++)
     {
         ChangeCursorPos(x, y+i);
         PrintChar(vc, 1); PrintChar(' ', lenx-1); DrawChar(vc, 1);
     }
     
     pos = x + (lenx / 2) - (strlen(caption) / 2);
     ChangeCursorPos(pos, y);
     PrintString(caption);

     UnLockMouse();
}

/*
     Draw a single line box on the screen.
*/
void DrawSingleBox (int x, int y, int lenx, int leny, int forcolor, 
                    int backcolor, char* caption)
{
     DrawBox (x, y, lenx, leny, forcolor, backcolor, 
              'Ú', '¿', 'À', 'Ù', 'Ä', '³',
              caption);
}

/*
     Draw a double line box on the screen.
*/
void DrawDoubleBox (int x, int y, int lenx, int leny, int forcolor, 
                    int backcolor, char* caption)
{
     DrawBox (x, y, lenx, leny, forcolor, backcolor, 
              'É', '»', 'È', '¼', 'Í', 'º',
              caption);
}

/*
     Draws a certain text on the screen.
*/
void DrawText (int x, int y, char* buf, int forcolor, int backcolor)
{
     ChangeCursorPos(x, y);
     SetForColor(forcolor);
     SetBackColor(backcolor);

     LockMouse(x, y, x+strlen(buf), y);
     PrintString(buf);
     UnLockMouse();
}

/*
     Draws a sequence of '±''s on the screen.
*/
void DrawStatusBar (int x, int y, int len, int forcolor, int backcolor)
{
     ChangeCursorPos(x, y);
     
     SetForColor(forcolor);
     SetBackColor(backcolor);

     DrawChar('±', len);
}

/*
     Removes the shadow from a button.
*/

static void RemoveShadow (int x1, int x2, int y1, int y2)
{
     int i, color;

     ChangeCursorPos(x1-1, y1+1);
     color = ReadCursorAttr();

     SetBackColor((color >> 4) & 7);
     SetForColor(color & 15);                
                                        
     /* Horizontal. */
     for (i = x1; i < x2+2; i++)
     {   
         ChangeCursorPos(i, y2+1);
         PrintChar(' ', 1);
     }

     ChangeCursorPos(x1-1, y1); PrintChar(' ', 1);
}

/*
   Draws a push button on the screen.
*/
void DrawButton (int x, int y, int len, int forcolor, int backcolor, 
                 char* caption, int selected, int shadow)
{
     int pos, attr;
     
     memset(DrawBuffer, ' ', len); 
     DrawBuffer[len] = 0;
     if (selected)
     {
         DrawBuffer[0]     = BCHARLEFT;   /* > or < */
         DrawBuffer[len-1] = BCHARRIGHT;  /* < or > */
     }

     pos = (len / 2) - (strlen(caption) / 2);
     memcpy(&DrawBuffer[pos], caption, strlen(caption));
     LockMouse(x-1, y-1, x+len+1, y+2);
     DrawText(x, y, DrawBuffer, forcolor, backcolor);
     if (shadow) 
     {
        /* Draw horizontal shadow. */
        DrawShadow(x, x+len-1, y, y, TRUE);

        /* Draw vertical shadow. */
        attr = (ReadCursorAttr() >> 4) & 7;
        SetForColor(BLACK);
        SetBackColor(attr);

        ChangeCursorPos(x+len, y);
        PrintChar('Ü', 1);                       
     }
     else
        RemoveShadow(x,x+len-1,y,y);

     UnLockMouse();
}


/*
     Draw a shadow around a certain area.
*/
static void DrawShadow (int x1, int x2, int y1, int y2, int small)
{
     int i, attr;

     LockMouse(x1-1, y1-1, x2+1, y2+1);

     if (small)
     {
        ChangeCursorPos(x1-1,y1);

        attr = (ReadCursorAttr() >> 4) & 7;
        SetForColor(BLACK);
        SetBackColor(attr);
     }
     else
     {
        SetForColor(WHITE);
        SetBackColor(BLACK);
     }
     
     /* Horizontal. */
     for (i = x1+1; i <= x2+1; i++)
     {
         ChangeCursorPos(i, y2+1);
         if (small)
            PrintChar('ß', 1);
         else
            PrintChar(ReadCursorChar(), 1);
     }

     /* Vertical. */
     for (i = y1+1; i <= y2; i++)
     {
         ChangeCursorPos(x2+1, i);
         PrintChar(ReadCursorChar(), 1);
     }

     UnLockMouse();
}

/*
     Draws a selection button on the screen.
        => (*) checked, ( ) unchecked.
*/
void DrawSelectionButton (int x, int y, int forcolor, int backcolor, 
                          int selected, char* caption)
{
     ChangeCursorPos(x, y);
     SetForColor(forcolor);
     SetBackColor(backcolor);

     LockMouse(x, y, x+3, y);

     if (selected)
        PrintString("(*) ");
     else
        PrintString("( ) ");
     PrintString(caption);

     UnLockMouse();
}

/*
     Changes the color of a certain character on the screen. 
     The character is read from the screen.
*/
void ChangeCharColor(int x, int y, int newfor, int newback)
{
     ChangeCursorPos(x, y);
     SetForColor(newfor);
     SetBackColor(newback);

     LockMouse(x, y, x, y);

     DrawChar(ReadCursorChar(), 1);

     UnLockMouse();
}

/*
     Inverts a line of text on the screen.
*/
void InvertLine (int x1, int x2, int y)
{
     int i;

     LockMouse(x1, y, x2, y);

     for (i = x1; i <= x2; i++)
     {
        ChangeCursorPos(i, y);
        InvertChar();
     }

     UnLockMouse();
}

/*
     Copy a screen region to the clipboard and draw a shadowed box
     on the screen.
*/
void ShowDialog (int posx, int posy, int xlen, int ylen, char* caption,
                 int forcolor, int backcolor)
{
     int zoomxlen, zoomylen, i;
     
     CopyScreen (posx, posy, posx+xlen+1, posy+ylen+1);
     
     zoomxlen = xlen >> 3;
     zoomylen = ylen >> 3;


     for (i = 0; i < 3; i++)
     {
         DrawSingleBox(posx + ((xlen - zoomxlen) >> 1), 
                       posy + ((ylen - zoomylen) >> 1), 
                       zoomxlen, zoomylen,
                       forcolor, backcolor, "");

	 delay(55);

         zoomxlen <<= 1;
         zoomylen <<= 1;
     }
     
     DrawDoubleBox(posx, posy, xlen, ylen, forcolor, backcolor, caption);
     DrawShadow(posx, posx+xlen, posy, posy+ylen, FALSE);
}

/*
     Colors a certain area on the screen.
*/
void ColorArea (int x1, int y1, int x2, int y2, int color)
{
     int y;

     SetForColor(color);

     LockMouse(x1, y1, x2, y2);
     for (y = y1; y <= y2; y++)
     {
         ChangeCursorPos(x1, y);
         DrawChar(' ', x2 - x1);
     }

     UnLockMouse();
}

/* 
    Draws a sequence of characters on the screen. 
*/

void DrawSequence (int posx, int posy, int len, char r, int forcolor, 
                   int backcolor)
{
     SetForColor(forcolor);
     SetBackColor(backcolor);
     ChangeCursorPos(posx, posy);
     LockMouse(posx, posy, posx+len, posy);

     DrawChar(r, len);

     UnLockMouse();
}

/*
     Draws a vertical scroll box.
*/

void DrawVScrollBox(int posx, int posy, int xlen, int ylen, int forcolor,
                    int backcolor, int barforc, int barbackc)
{
     int y;
     
     /* Draw border. */
     DrawSingleBox(posx, posy, xlen, ylen-1, forcolor, backcolor, "");

     SetForColor(barforc);
     SetBackColor(barbackc);
     
     LockMouse(posx, posy, posx+xlen, posy+ylen);
     
     /* Draw up arrow. */
     ChangeCursorPos(posx+xlen, posy+1);
     DrawChar(24, 1);                     

     /* Draw down arrow. */
     ChangeCursorPos(posx+xlen, posy+ylen-2);
     DrawChar(25, 1);
     
     /* Draw (vertical) status bar. */
     for (y = posy+2; y < posy+ylen-2; y++)
     {                                                                      
         ChangeCursorPos(posx+xlen, y);                                              
         DrawChar('²', 1);
     }

     UnLockMouse();
}

/*
     Draws the status on the status bar of a vertical scroll box.
*/
void DrawStatusOnBar(int barx, int posy, int barforc, int barbackc, 
                     int amofentries, int nrofentry, int ylen, char piece)
{
     int barlen, persliece, rest, part = 0;
     int temp, y;

     if (nrofentry == 1)
        y = posy + 2;
     else   
     {
        barlen = ylen - 4;

        if (amofentries < barlen)
        {
           temp = nrofentry * 100 / amofentries * barlen;
           y = posy + (temp / 100) + 1;
        }
        else
        {
           persliece = amofentries / barlen;
           rest      = amofentries % barlen;
           
           while (nrofentry > 0)
           {
                 nrofentry -= persliece;
                 if (rest > 0)
                 {
                    rest--;
                    nrofentry--;
                 }
                 part++;
           }

           y = posy + part + 1;
        }
     }
     
     /* Change color. */
     if (piece == 'Û')
     {
        SetForColor(barforc);
        SetBackColor(barbackc);
     }
     else
     {
        SetForColor(barbackc);
        SetBackColor(barforc);
     }

     ChangeCursorPos(barx, y);
     
     LockMouse(barx, posy, barx, posy+ylen);
     DrawChar(piece, 1);
     UnLockMouse();
}
