/*    
   FTEMem.c - heap memory management for the FAT transformation engine.

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

#include <assert.h>
#include <stdlib.h>

#include "fte.h"
#include "ftemem.h"
#include "backmem.h"
#include "suremem.h"

/*#define DEBUG_FTEMEM */
#ifdef DEBUG_FTEMEM
 long FTEMEM_Debug_Counter=0;
#endif

#ifdef MANAGE_OWN_MEMORY

ALLOCRES AllocateFTEMemory(unsigned guaranteed, unsigned char guaranteedblocks,
                           unsigned backupbytes)
{
   if (guaranteed && guaranteedblocks)
   {
      if (!AllocateGuaranteedMemory(guaranteed, guaranteedblocks))
      {
         RETURN_FTEERROR(TOTAL_FAIL);	
      }
   }

   if (backupbytes)
   {
      if (!AllocateBackupMemory(backupbytes))
      {
         RETURN_FTEERROR(BACKUP_FAIL);	
      }
   }

   return ALLOC_SUCCESS;
}

void DeallocateFTEMemory(void)
{
   FreeGuaranteedMemory();
   FreeBackupMemory();
}

#endif

/* Generic allocation */
void* FTEAlloc(size_t bytes)
{
   void* retval;

#ifdef DEBUG_FTEMEM
   gotoxy(40,1);
   printf("Alloc: %ld\n", FTEMEM_Debug_Counter++);
#endif

   retval = malloc(bytes);
#ifdef MANAGE_OWN_MEMORY
   if (!retval) retval = BackupAlloc(bytes);
#endif

#ifdef DEBUG_FTEMEM
   gotoxy(40,2);
   printf("Allocated at: %lu\n", (unsigned long) retval);
#endif

   if (!retval)
   {
      SetFTEerror(FTE_MEM_INSUFFICIENT);
      RETURN_FTEERROR(NULL);
   }
      
   return retval;
}

void  FTEFree(void* tofree)
{
   assert(tofree);
    
#ifdef DEBUG_FTEMEM
   gotoxy(40,3);
   printf("Free: %ld\n", --FTEMEM_Debug_Counter);
   gotoxy(40,4);
   printf("Freeing: %lu\n", (unsigned long) tofree);
#endif

#ifdef MANAGE_OWN_MEMORY
   if (InBackupMemoryRange(tofree))
   {
      BackupFree(tofree);
   }
   else

#endif   
      free(tofree);
}

/* Sectors -- Assumes that bytespersector field is filled in at handle
              creation (InitReadWriteSectors,...)                     */
SECTOR* AllocateSector(RDWRHandle handle)
{
   return (SECTOR*) FTEAlloc(handle->BytesPerSector);
}

SECTOR* AllocateSectors(RDWRHandle handle, int count)
{
   return (SECTOR*) FTEAlloc(handle->BytesPerSector * count);
}

void FreeSectors(SECTOR* sector)
{
   FTEFree((void*)sector);
}

/* Boot */
struct BootSectorStruct* AllocateBootSector(void)
{
   return (struct BootSectorStruct*)
          FTEAlloc(sizeof(struct BootSectorStruct));
}

void FreeBootSector(struct BootSectorStruct* boot)
{
   FTEFree((void*) boot);
}

/* Directories */
struct DirectoryEntry* AllocateDirectoryEntry(void)
{
   return (struct DirectoryEntry*) FTEAlloc(sizeof(struct DirectoryEntry));
}

void FreeDirectoryEntry(struct DirectoryEntry* entry)
{
   FTEFree(entry);
}

struct FSInfoStruct* AllocateFSInfo(void)
{
   return (struct FSInfoStruct*) FTEAlloc(sizeof(struct FSInfoStruct));
}

void FreeFSInfo(struct FSInfoStruct* info)
{
   FTEFree(info);
}
