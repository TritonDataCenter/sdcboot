/*    
   Idxstack.c - stack for implementing help context switching.

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
   email me at:  imre.leber@worldonline.be
*/

/*
   This stack contains the indexes that are going to be used
   when the user asks for help. This helps in implementing
   context sensitive help.
*/

#define DEBUGGING

#include "idxstack.h"

#ifdef DEBUGGING
#include <stdio.h>

#include "..\logman\logman.h"
#endif

static int stack[STACK_DEPTH];
static int StackPointer;

void PushHelpIndex(int index)
{
#ifdef DEBUGGING
   if (StackPointer == STACK_DEPTH)
   {
      printf("\a");
      LogPrint("Stack depth exceeded!");
   }
   else
#endif

   stack[StackPointer++] = index;
}

int PopHelpIndex(void)
{
   return stack[--StackPointer];
}

int TopHelpIndex(void)
{
   return stack[StackPointer-1];
}
