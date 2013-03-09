/*    
   Backmem.c - heap memory manager for a pre-reserved amount of memory.

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

#ifndef __GNUC__
#include <dos.h>
#endif

#ifdef DEBUG
#include <stdio.h>
#endif

#include "bool.h"
#include "backmem.h"
#include "fteerr.h"

struct FreeListElement
{
    unsigned size;

    void* positionprev;
    void* positionnext;

    void* sizenext;
};

struct ReservedListElement
{
    unsigned size;
    void* next;
};

#define SMALLESTFREEBLOCK sizeof(struct FreeListElement)

static void*    BackupMemory     = NULL;
static void*    FreePositionHead = NULL;
static void*    FreeSizeHead     = NULL;
static void*    ReservedHead     = NULL;
static unsigned BackupSize       = 0;

static void* GetFirstBlock(void)
{
   return FreePositionHead;
}

static void* GetNextBlock(void* block)
{
   return ((struct FreeListElement*) block)->positionnext;
}

static void* GetSmallestBlock(void)
{
   return FreeSizeHead;
}

static void* GetLargerBlock(void* block)
{
   return ((struct FreeListElement*) block)->sizenext;
}

static unsigned GetBlockSize(void* block)
{
   return ((struct FreeListElement*) block)->size;
}

static void* GetFittingBlock(unsigned size)
{
   unsigned currentsize;
   void* current;

   current = GetSmallestBlock();
   while (current)
   {
      currentsize = GetBlockSize(current);
      if (size <= currentsize)
         return current;

      current = GetLargerBlock(current);
   }

   return NULL;
}

static BOOL IsFirstFreeBlock(void* block)
{
   return (char*)FreePositionHead == (char*)block;
}

static void RemoveBlockFromPositionList(void* block)
{
   void* previous;
   void* next;

   previous = ((struct FreeListElement*) block)->positionprev;
   next = ((struct FreeListElement*) block)->positionnext;

   if (previous)
   {
      ((struct FreeListElement*)previous)->positionnext = next;
   }

   if (next)
   {
      ((struct FreeListElement*)next)->positionprev = previous;
   }
}

static void DecreaseFreeBlock(void* block, unsigned size)
{
   void* previous;
   void* next;
   
   previous = ((struct FreeListElement*) block)->positionprev;
   next = ((struct FreeListElement*) block)->positionnext;

   if (previous)
   {
      char* temp = (char*)(((struct FreeListElement*)previous)->positionnext);
      temp += size;
      ((struct FreeListElement*)previous)->positionnext=(void*)temp;
   }

   if (next)
   {
      char* temp = (char*)(((struct FreeListElement*)next)->positionprev);
      temp += size;
      ((struct FreeListElement*)next)->positionprev=(void*)temp;
   }

   ((struct FreeListElement*) block)->size -= size;
   memcpy(((char*)block)+size, block, sizeof(struct FreeListElement));
}

static void InsertBlockIntoPositionList(void* block)
{
   void* current, *previous;

   current = GetFirstBlock();
   if ((char*)block < (char*)current) /* Make this the head of the list */
   {
      ((struct FreeListElement*) current)->positionprev = block;
      ((struct FreeListElement*) block)->positionnext = current;
      ((struct FreeListElement*) block)->positionprev = 0;
      FreePositionHead = block;
   }
   else                               /* Insert somewhere in the middle */
   {
      do {
          previous = current;         /* We need to keep a chaising pointer
                                         to be able to add to the end
                                         of the list.                      */
          current  = GetNextBlock(current);
      } while (current && ((char*)block > (char*)current));

      if (current) /* Add in the middle of the list. */
      {
         ((struct FreeListElement*) current)->positionprev = block;                                             
         ((struct FreeListElement*) previous)->positionnext = block;
         ((struct FreeListElement*) block)->positionprev = previous;
         ((struct FreeListElement*) block)->positionnext = current;
      }
      else         /* Add to the end of the list */
      {
         ((struct FreeListElement*) previous)->positionnext = block;
         ((struct FreeListElement*) block)->positionprev = previous;
         ((struct FreeListElement*) block)->positionnext = 0;
      }
   }
}

static void RemoveFromSizeList(void* block)
{
   void* chaising=NULL, *current;

   current = GetSmallestBlock();
   if (!current) return;                /* Size list empty */
  
   while (current)
   {
      if (current == block)
      {
         if (chaising)
         {
            ((struct FreeListElement*) chaising)->sizenext =
                           ((struct FreeListElement*) current)->sizenext;
         }
         else
         {
            FreeSizeHead = ((struct FreeListElement*) current)->sizenext;
         }
         return;
      }

      chaising = current;
      current  = GetLargerBlock(current);
   }
}

static void InsertIntoFreeList(void* block)
{
   unsigned size = ((struct FreeListElement*)block)->size;
   void* chaising = NULL, *current;

   current = GetSmallestBlock();
   if (!current)          
   {
      FreeSizeHead = block;
      ((struct FreeListElement*) block)->sizenext = 0;
      return;
   }

   while (current)
   {
      if (GetBlockSize(current) >= size)
      {
	 if (chaising)
	 {
	    ((struct FreeListElement*) chaising)->sizenext = block;
	    ((struct FreeListElement*) block)->sizenext = current;
	 }
	 else /* Make this the head of the list */
	 {
	    FreeSizeHead = block;
	    ((struct FreeListElement*) block)->sizenext = current;
	 }

	 return;
      }

      chaising = current;
      current  = GetLargerBlock(current);
   }

   /* Add the block at the end of the size list */
   ((struct FreeListElement*) chaising)->sizenext = block;                                  
   ((struct FreeListElement*) block)->sizenext = 0;
}

static void MergeFreeBlock(void* block)
{
   void* previous, *current = block;
   void* next;
   unsigned size;

   previous = ((struct FreeListElement*) block)->positionprev;
   next = ((struct FreeListElement*) block)->positionnext;

   if (previous)
   { 
      size = ((struct FreeListElement*) previous)->size;

      if (((char*) previous+size) == block)
      {
         ((struct FreeListElement*) previous)->size += GetBlockSize(block);
	 RemoveFromSizeList(previous);
	 RemoveFromSizeList(block);
	 InsertIntoFreeList(previous);
	 RemoveBlockFromPositionList(block);
         current = previous;
      }
   }

   if (next)
   {
      if (((char*)current+GetBlockSize(current)) == next)
      {
         ((struct FreeListElement*) current)->size += GetBlockSize(next);
	 RemoveFromSizeList(current);
	 RemoveFromSizeList(next);
	 InsertIntoFreeList(current);
	 RemoveBlockFromPositionList(next);
      }
   }
}

static void MarkAsReserved(void* block)
{
   if (ReservedHead)
   {
      ((struct ReservedListElement*) block)->next = ReservedHead;
   }
   else /* No blocks reserved yet */
   {
      ((struct ReservedListElement*) block)->next = 0;  
   }
   ReservedHead = block;
}

static void UnmarkAsReserved(void* block)
{
   void* chaising=NULL, *current = ReservedHead;

   if (block == ReservedHead)
   {
      ReservedHead = ((struct ReservedListElement*) block)->next;
      return;
   }

   while (current)
   {
       if (current == block)
       {
	  ((struct ReservedListElement*) chaising)->next =
				((struct ReservedListElement*) block)->next;
	  return;
       }

       chaising = current;
       current  = ((struct ReservedListElement*) current)->next;
   }
}

static BOOL IsReserved(void* block)
{
   void* current = ReservedHead;

   while (current)
   {
      if (current == block)
         return TRUE;

      current = ((struct ReservedListElement*) current)->next;
   }
   return FALSE;
}

BOOL AllocateBackupMemory(unsigned size)
{
   if (size < SMALLESTFREEBLOCK)
      size = SMALLESTFREEBLOCK;

   BackupMemory = malloc(size);
   if (BackupMemory)
   {
      ((struct FreeListElement*)BackupMemory)->size = size;
      ((struct FreeListElement*)BackupMemory)->positionprev = 0;
      ((struct FreeListElement*)BackupMemory)->positionnext = 0;
      ((struct FreeListElement*)BackupMemory)->sizenext = 0;

      FreeSizeHead     = BackupMemory;
      FreePositionHead = BackupMemory;
      BackupSize       = size;
   }
   
   return BackupMemory != 0;
}

void FreeBackupMemory(void)
{
   if (BackupMemory) free(BackupMemory);
}

void* BackupAlloc(unsigned size)
{
   void* newblock;

   size += sizeof(struct ReservedListElement);
   if (size < SMALLESTFREEBLOCK)
      size = SMALLESTFREEBLOCK;

   newblock = GetFittingBlock(size);
   if (newblock)
   {
      if (GetBlockSize(newblock) == size)
      {
         RemoveBlockFromPositionList(newblock);
         RemoveFromSizeList(newblock);
      }
      else
      {
	 DecreaseFreeBlock(newblock, size);

	 if (IsFirstFreeBlock(newblock))
	 {
	    FreePositionHead = ((char*)newblock+size);
	 }

         RemoveFromSizeList(newblock);
         InsertIntoFreeList((void*)((char*)newblock+size));
      }

      MarkAsReserved(newblock);
      ((struct ReservedListElement*) newblock)->size = size;

      return (void*)(((struct ReservedListElement*)newblock)+1);
   }
   return NULL;
}

void BackupFree(void* block)
{
   /* Go to the real begining of the block. */
   block = (void*) (((struct ReservedListElement*)block)-1);

   if (IsReserved(block))
   {
      UnmarkAsReserved(block);
      InsertBlockIntoPositionList(block);
      InsertIntoFreeList(block);
      MergeFreeBlock(block);
   }
   else
   {
      SetFTEerror(FTE_NOT_RESERVED);
   }
}

BOOL InBackupMemoryRange(void* block)
{
   unsigned long difference;
   unsigned long pos1 = (FP_SEG(BackupMemory) << 4) + FP_OFF(BackupMemory);
   unsigned long pos2 = (FP_SEG(block) << 4) + FP_OFF(block);

   if (pos1 < pos2)
      difference = pos2 - pos1;
   else
      difference = pos1 - pos2;

   return difference < BackupSize;
}

#ifdef DEBUG1

static unsigned CalculateBlockOffset(void* block)
{
   if (block)
      return (unsigned) (((char*) block) - ((char*) BackupMemory));
   else
      return 0;
} 

void PrintFreeList(void)
{
   void* current = GetFirstBlock();

   printf("Free: \n");

   while (current)
   {
       printf("offset: %u\n", CalculateBlockOffset(current));
       printf("size:   %u\n", ((struct FreeListElement*) current)->size);
      
       if (((struct FreeListElement*) current)->positionprev)
	  printf("previous: %u\n", CalculateBlockOffset(
			  ((struct FreeListElement*) current)->positionprev));
       else
	  printf("previous: NULL\n");


       if (((struct FreeListElement*) current)->positionnext)
	  printf("next: %u\n\n", CalculateBlockOffset(
			  ((struct FreeListElement*) current)->positionnext));
       else
	  printf("next: NULL\n\n");

       current = GetNextBlock(current);
   }
}

void PrintSizeList(void)
{
    void* current = GetSmallestBlock();

    printf("Size: \n");

    while (current)
    {
        printf("offset: %u\n", CalculateBlockOffset(current));
        printf("size:   %u\n\n", ((struct FreeListElement*) current)->size);  
    
        current = GetLargerBlock(current);
    }
}

void PrintReservedList(void)
{
   void* current = ReservedHead;

   printf("Reserved: \n");

   while (current)
   {
      printf("offset: %u\n", CalculateBlockOffset(current));
      printf("size:   %u\n\n", ((struct ReservedListElement*) current)->size);

      current = ((struct ReservedListElement*) current)->next;
   }
}

int main()
{
    void* block1, *block2, *block3, *block4, *block5;

    clrscr();

    AllocateBackupMemory(16384);

    block1 = BackupAlloc(94);
    block2 = BackupAlloc(94);
    block3 = BackupAlloc(94);
    block4 = BackupAlloc(94);
    block5 = BackupAlloc(94);

    BackupFree(block2);
    BackupFree(block4);

    block2 = BackupAlloc(94);
    block4 = BackupAlloc(20);

    BackupFree(block2);
    BackupFree(block3);

    PrintFreeList();
    getch();
    PrintSizeList();
    PrintReservedList();
}

#endif
