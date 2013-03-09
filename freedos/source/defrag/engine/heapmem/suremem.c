/*    
   Suremem.c - heap memory management for memory that HAS to be available
               in order not to trash important drive data.

   Copyright (C) 2002 Imre Leber

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

#include <stdlib.h>
#include <string.h>
#include <limits.h>

#include "fte.h"
#define DEBUG
struct GuaranteedHandle
{
   int      locked:1;
   unsigned offset;
   unsigned size;
};

static void* GMemory = NULL;
static unsigned GMemoryLength = 0;
static struct GuaranteedHandle* GHandles = NULL;

static unsigned char NumHandles = 0;

static void* FreeStart = NULL;

static int FindFreeHandle(void)
{
   unsigned char i;

   for (i = 0; i < NumHandles; i++)
   {
       if (GHandles[i].size == 0)
          return i;
   }
   
   return -1;
}

/* Return the handle of which the offset is the first in the memory */
static int GetFirstHandle(void)
{
   unsigned char i;
   int pos = -1;
   unsigned smallest = UINT_MAX;

   for (i = 0; i < NumHandles; i++)
   {
       if ((GHandles[i].size) && (GHandles[i].offset < smallest))
       {
          smallest = GHandles[i].offset;
          pos = i;
       }
   }

   return pos;
}

static int GetNextHandle(int handle)
{
   int pos=-1;
   unsigned char i;
   unsigned offset = GHandles[handle].offset;
   unsigned smallest = UINT_MAX;

   for (i = 0; i < NumHandles; i++)
   {
       if (GHandles[i].size                &&
           (GHandles[i].offset >= offset)  &&
           (GHandles[i].offset < smallest) &&
           (i != handle))
       {
          smallest = GHandles[i].offset;
          pos = i;
       }
   }

   return pos;
}

static void CrunchMemory(void)
{
   int handle;
   unsigned offset;
   char* head = (char*)GMemory;

   handle = GetFirstHandle();
   while (handle != -1)
   {
       if (GHandles[handle].locked == 0)
       {
          offset = GHandles[handle].offset;
	  if (((char*)GMemory)+offset != head)
          {
             memmove(head, (char*)GMemory+offset, GHandles[handle].size);
             GHandles[handle].offset = (unsigned)(head - (char*) GMemory);
          }
          head += GHandles[handle].size;
       }
       else
       {
          head = ((char*) GMemory) +
                          (GHandles[handle].offset + GHandles[handle].size);
       }
       handle = GetNextHandle(handle);
   }

   FreeStart = (char*)head;
}

BOOL AllocateGuaranteedMemory(unsigned guaranteed,
                              unsigned char guaranteedblocks)
{
   GMemory = malloc(guaranteed);
   if (!GMemory) return FALSE;

   GHandles = (struct GuaranteedHandle*) malloc(guaranteedblocks *
                                            sizeof(struct GuaranteedHandle));
   if (!GHandles)
   {
      free(GMemory);
      return FALSE;
   }

   memset(GHandles, 0, guaranteedblocks * sizeof(struct GuaranteedHandle));
   FreeStart     = GMemory;
   GMemoryLength = guaranteed;
   NumHandles    = guaranteedblocks;

   return TRUE;
}

void FreeGuaranteedMemory(void)
{
   if (GMemory)  free(GMemory);
   if (GHandles) free(GHandles);
}

int AllocateGuaranteedBlock(unsigned length)
{
   unsigned taken;
   int handle = FindFreeHandle();

   if (handle != -1)
   {
      taken = (unsigned)(((char*)FreeStart) - ((char*)GMemory));
      if (GMemoryLength - taken < length)
      {
         CrunchMemory();
         taken = (unsigned)(((char*)FreeStart) - ((char*)GMemory));
         if (GMemoryLength - taken < length)
         {
            SetFTEerror(FTE_MEM_INSUFFICIENT);
            return -1;
         }
      }

      GHandles[handle].offset = (unsigned)((char*)FreeStart - (char*)GMemory);
      GHandles[handle].size   = length;
      FreeStart = (void*) (((char*) FreeStart) + length);
      return handle;
   }
   else
   {
      SetFTEerror(FTE_INSUFFICIENT_HANDLES);
      return -1;
   }
}

void FreeGuaranteedBlock(int handle)
{
   if (GHandles[handle].locked == 0)
      GHandles[handle].size = 0;
}

void* LockGuaranteedBlock(int handle)
{
   GHandles[handle].locked = 1;
   return (void*) (((char*) GMemory) + (GHandles[handle].offset));
}

void UnlockGuaranteedBlock(void* memory)
{
   unsigned offset = (unsigned)(((char*) memory)  - ((char*) GMemory));
   unsigned char handle;

   for (handle = 0; handle < NumHandles; handle++)
   {
       if (GHandles[handle].size && (GHandles[handle].offset == offset))
          GHandles[handle].locked = 0;
   }
}

unsigned char CountGuaranteedBlocks(void)
{
   return NumHandles;
}

unsigned char CountFreeGuaranteedBlocks(void)
{
   unsigned char i, count = 0;

   for (i = 0; i < NumHandles; i++)
       if (GHandles[i].size == 0)
          count++;

   return count;
}

#ifdef DEBUG1

static void PrintMemoryBlocks(void)
{
   int handle = GetFirstHandle();

   while (handle != -1)
   {
      printf("offset: %u\n", GHandles[handle].offset);
      printf("size:   %u\n", GHandles[handle].size);
   
      if (GHandles[handle].locked)
	 printf("Locked\n\n");
      else
	 printf("Not locked\n\n");

      handle = GetNextHandle(handle);
   }
}

int main(int argc, char *argv[])
{
  int h1, h2, h3, h4, h5, h6, h7;

  clrscr();

  AllocateGuaranteedMemory(4096, 16);

  h1 = AllocateGuaranteedBlock(512);
  h2 = AllocateGuaranteedBlock(512);
  h3 = AllocateGuaranteedBlock(512);
  h4 = AllocateGuaranteedBlock(512);
  h5 = AllocateGuaranteedBlock(512);

  FreeGuaranteedBlock(h2);
  FreeGuaranteedBlock(h4);

  h2 = AllocateGuaranteedBlock(512);
  h4 = AllocateGuaranteedBlock(512);

  h6 = AllocateGuaranteedBlock(512);
  h7 = AllocateGuaranteedBlock(512);

  PrintMemoryBlocks();

  FreeGuaranteedMemory();

  return 0;
}


#endif