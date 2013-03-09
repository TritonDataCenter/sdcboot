/*    
   drvmap.c - routines to draw on the drive map.
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

#include <assert.h>
#include <conio.h>

#include "FTE.h"
#include "screen.h"
#include "scrmask.h"
#include "useful.h"
#include "drvmap.h"

#define MAP_X 5
#define MAP_Y 3

#if 0
int  hidden = 0;

int  hiddenX1, hiddenY1, hiddenX2, hiddenY2;
char hiddenblockchars[BLOCKSPERLINE][MAXLINE];
char hiddenblockcolors[BLOCKSPERLINE][MAXLINE];
#endif

CLUSTER ClustersPerBlock;

/*
    Draws the drive map on the screen.
*/
void DrawDrvMap(CLUSTER maxcluster)
{
     int i, amofblocks, amoflines;

     /* Calculate how many clusters there have to be per block. */
     ClustersPerBlock = maxcluster / MAXBLOCK + (maxcluster % MAXBLOCK > 0);

     /* Calculate how many blocks there will be. */
     amofblocks = (int)((maxcluster / ClustersPerBlock) 
                + (maxcluster % ClustersPerBlock > 0));
     
     /* Clear the area. */
     for (i = 0; i < MAXLINE; i++)
         DrawSequence(MAP_X, MAP_Y+i, BLOCKSPERLINE, ' ', WHITE, BLUE);

     /* Draw the blocks on the screen. */
     amoflines = amofblocks / BLOCKSPERLINE;
     for (i = 0; i < amoflines; i++)
         DrawSequence(MAP_X, MAP_Y+i, BLOCKSPERLINE, '±', WHITE, BLUE);
     DrawSequence(MAP_X, MAP_Y+i, amofblocks % BLOCKSPERLINE, '±', WHITE, BLUE);

     DrawBlockSize(ClustersPerBlock);
}

/*
    Draw a certain symbol on the screen, on the place corresponding to 
    cluster.
*/

STATIC void DrawTheBlock(int blocknumber, char* symbol, int forcolor, int backcolor)
{
    int posx, posy, c;
    
    posx = (blocknumber % BLOCKSPERLINE) + MAP_X;
    posy = (blocknumber / BLOCKSPERLINE) + MAP_Y;

#if 0
    if ((posx < hiddenX1) || (posx > hiddenX2) ||
        (posy < hiddenY1) || (posy > hiddenY2) || (!hidden))
    {
#endif
       ChangeCursorPos(posx, posy);

       if (((c = ReadCursorChar()) != 'X') && (c != 'b'))
          DrawText(posx, posy, symbol, forcolor, backcolor);
        
#if 0     
    }
    else
    {
       hiddenblockchars[psx][psy]  = symbol[0];
       hiddenblockcolors[psx][psy] = forcolor << 4 + backcolor;
    }
#endif    
}

STATIC void DrawBlock(CLUSTER cluster, char* symbol, int forcolor, int backcolor)
{
    assert(ClustersPerBlock);

    /* First cluster is 2 */
    cluster -= 2;
    
    DrawTheBlock((int)(cluster / ClustersPerBlock), symbol, forcolor, backcolor);
}

STATIC void DrawBlocks(CLUSTER cluster, char* symbol, int forcolor, int backcolor, unsigned long n)
{
    int blocknumber, nrofblocks=1, i;
    unsigned long rest;

    /* First cluster is 2 */ 
    cluster -= 2;   

    if (!n) return;

    assert(ClustersPerBlock);
    blocknumber  = (int)(cluster / ClustersPerBlock);

    rest = cluster % ClustersPerBlock;
    
    if (n > ClustersPerBlock - rest)
    {
	n -= ClustersPerBlock - rest;
 
	nrofblocks = n / ClustersPerBlock;
	
	if (rest) nrofblocks++;
	
	if (n % ClustersPerBlock) nrofblocks++;
    }
    else
    {
	nrofblocks = 1;
    }

    for (i=0; i<nrofblocks; i++)
    {
	DrawTheBlock(blocknumber + i, symbol, forcolor, backcolor);	
    }    
}

#if 0
void SetHiddenMapArea(int x1, int y1, int x2, int y2)
{
     hiddenX1 = x1;
     hiddenY1 = y1;
     hiddenX2 = x2;
     hiddenY2 = y2;
     hidden   =  1;
}

void SetMapHidingOff ()
{
     hidden = 0;
}

void UnhideMapArea()
{
     char buf[2];
     int x, y, forcolor, backcolor;

     buf[1] = 0;

     for (x = hiddenX1; x <= hiddenX2; x++)
         for (y = hiddenY1; y <= hiddenY2; y++)
         {
             buf[0]    = hiddenblockchars[x - MAP_X][y - MAP_Y];
             forcolor  = hiddenblockcolors[x - MAP_X][y - MAP_Y] >> 4;
             backcolor = hiddenblockcolors[x - MAP_X][y - MAP_Y];
             
             DrawText(x, y, buf, forcolor, backcolor);
         }
}
#endif

void DrawUsedBlock(CLUSTER cluster)
{
     DrawBlock(cluster, "\t", BLUE+BLINK, WHITE);
}

void DrawOptimizedBlock(CLUSTER cluster)
{
     DrawBlock(cluster, "\t", BLUE+BLINK, YELLOW);
}

void DrawUnusedBlock(CLUSTER cluster)
{
     DrawBlock(cluster, "±", WHITE, BLUE);
}

void DrawUnmovableBlock(CLUSTER cluster)
{
     DrawBlock(cluster, "X", WHITE, BLUE);
}

void DrawBadBlock(CLUSTER cluster)
{
     DrawBlock(cluster, "b", WHITE, BLUE);
}

void DrawReadBlock(CLUSTER cluster)
{
     DrawBlock(cluster, "r", WHITE, BLUE);
}

void DrawWriteBlock(CLUSTER cluster)
{
     DrawBlock(cluster, "W", WHITE, BLUE);
}

/* Drawing more clusters */

void DrawUsedBlocks(CLUSTER cluster, unsigned long n)
{
     DrawBlocks(cluster, "\t", BLUE+BLINK, WHITE, n);
}

void DrawOptimizedBlocks(CLUSTER cluster, unsigned long n)
{
     DrawBlocks(cluster, "\t", BLUE+BLINK, YELLOW, n);
}

void DrawUnusedBlocks(CLUSTER cluster, unsigned long n)
{
     DrawBlocks(cluster, "±", WHITE, BLUE, n);
}

void DrawUnmovableBlocks(CLUSTER cluster, unsigned long n)
{
     DrawBlocks(cluster, "X", WHITE, BLUE, n);
}

void DrawBadBlocks(CLUSTER cluster, unsigned long n)
{
     DrawBlocks(cluster, "b", WHITE, BLUE, n);
}

void DrawReadBlocks(CLUSTER cluster, unsigned long n)
{
     DrawBlocks(cluster, "r", WHITE, BLUE, n);
}

void DrawWriteBlocks(CLUSTER cluster, unsigned long n)
{
     DrawBlocks(cluster, "W", WHITE, BLUE, n);
}
