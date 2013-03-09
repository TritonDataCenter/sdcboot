/*
   Subdir.c - sub directory manipulation code.
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


#include <string.h>
#include <stdlib.h>

#include "../../misc/bool.h"
#include "../header/rdwrsect.h"
#include "../header/direct.h"
#include "../header/boot.h"
#include "../header/fat.h"
#include "../header/subdir.h"
#include "../header/fatconst.h"
#include "../header/fsinfo.h"
#include "../header/ftemem.h"
#include "../header/traversl.h"

struct PipeStruct
{
       BOOL   (*func) (RDWRHandle handle, struct DirectoryPosition* direct,
		       void** buffer);
       void** buffer;
       int    exact;
};

/*static*/ int SubdirTraverser(RDWRHandle handle,
			   CLUSTER label,
			   SECTOR datasector,
			   void** structure)
{
    int i;
    unsigned char sectorspercluster, j;
    struct DirectoryPosition dirpos;
    
    struct PipeStruct** pipe = (struct PipeStruct**) structure;

    label = label;

    dirpos.sector = (SECTOR) datasector;

    sectorspercluster = GetSectorsPerCluster(handle);
    if (!sectorspercluster) return FAIL;

    for (j = 0; j < sectorspercluster; j++)
    {
	for (i = 0; i < ENTRIESPERSECTOR; i++)
	{
	    dirpos.offset = i;

	    if ((*pipe)->exact)
	    {
	       struct DirectoryEntry   direct;

               if (!GetDirectory(handle, &dirpos, &direct))
	       {
		  return FAIL;
	       }

	       if (direct.filename[0] == LASTLABEL)
	       {
		  return FALSE;
	       }
	    }

	    switch ((*pipe)->func(handle, &dirpos, (*pipe)->buffer))
	    {
	       case FAIL:
		    return FAIL;
	       case FALSE:
		    return FALSE;
	    }
	}

	dirpos.sector++;
    }

    return TRUE;
}

/*
   This traversal is in this file and not in directory.c, because
   it also has to work for FAT32. If the volume contains FAT32
   this function just calls TraverseSubDir with the cluster of the
   root directory as indicated by the BPB.
*/
int TraverseRootDir(RDWRHandle handle,
		    int (*func) (RDWRHandle handle,
				 struct DirectoryPosition* pos,
				 void** buffer),
		    void** buffer, int exact)
{
     int            i, j=0;
     unsigned short entries;

     struct DirectoryPosition dirpos;

     int fatlabelsize = GetFatLabelSize(handle);
     
     if (fatlabelsize == FAT32)
     {
	CLUSTER RootCluster = GetFAT32RootCluster(handle);
	if (RootCluster == 0) return FALSE;

	return TraverseSubdir(handle, RootCluster, func, buffer, exact);
     }

     if ((fatlabelsize != FAT12) && (fatlabelsize != FAT16))
        return FALSE;
	       
     entries = GetNumberOfRootEntries(handle);
     if (entries == 0) return FALSE;

     if (GetRootDirPosition(handle, 0, &dirpos) == FALSE) return FALSE;

     for (i = 0; i < entries; i++, j++)
     {
	 if (j == 16)
	 {
	    dirpos.sector++;
	    j = 0;
	 }
	 dirpos.offset = j;

	 if (exact)
	 {
	    struct DirectoryEntry*   direct; 
           
            direct = AllocateDirectoryEntry();
            if (!direct) return FALSE;
	                
            if (!GetDirectory(handle, &dirpos, direct))
            {
               FreeDirectoryEntry(direct);
               return FALSE;
            }
            
	    if (direct->filename[0] == LASTLABEL)
            {        
               FreeDirectoryEntry(direct);
               return TRUE;
            }
            FreeDirectoryEntry(direct);
	 }

	 switch (func(handle, &dirpos, buffer))
	 {
	    case FALSE:
		 return TRUE;
	    case FAIL:
		 return FALSE;
	 }
     }

     return TRUE;
}

int TraverseSubdir(RDWRHandle handle, CLUSTER fatcluster,
		   int (*func) (RDWRHandle handle,
				struct DirectoryPosition* pos,
				void** buffer),
		   void** buffer, int exact)
{
    int                 result;
    struct PipeStruct*  pipe;
    struct PipeStruct  myPipe;

    if (fatcluster == 0)
       return TraverseRootDir(handle, func, buffer, exact);

    pipe = &myPipe;

    pipe->func   = func;
    pipe->buffer = buffer;
    pipe->exact  = exact;

    result = FileTraverseFat(handle, fatcluster, SubdirTraverser,
			     (void**) &pipe);

    return result;
}

#ifdef __BORLANDC__ /* 16 bit FreeDOS */

int GetDirectory(RDWRHandle handle,
		 struct DirectoryPosition* position,
		 struct DirectoryEntry* direct)
{
    char *sectbuf;
	
    sectbuf = ReadSectorsAddress(handle, position->sector);	
    if (sectbuf == NULL)
       return FALSE;
		
    memcpy(direct, &sectbuf[DIRLEN2BYTES(position->offset)],
	   sizeof(struct DirectoryEntry));
        
    return TRUE;
}

#else /* linux */

int GetDirectory(RDWRHandle handle,
		 struct DirectoryPosition* position,
		 struct DirectoryEntry* direct)
{
	char buffer[BYTESPERSECTOR];
	
    if (ReadSectors(handle, 1, position->sector, buffer) == -1)
    {
		return FALSE;
    }

    memcpy(direct, &buffer[DIRLEN2BYTES(position->offset)],
		   sizeof(struct DirectoryEntry));
        
    return TRUE;
}

#endif

int DirectoryEquals(struct DirectoryPosition* pos1,
		    struct DirectoryPosition* pos2)
{
   return memcmp(pos1, pos2, sizeof(struct DirectoryPosition)) == 0;
}

int WriteDirectory(RDWRHandle handle, struct DirectoryPosition* pos,
		   struct DirectoryEntry* direct)
{
    char* buffer;
    
    buffer = (char*)AllocateSector(handle);
    if (!buffer) return FALSE;

    if (ReadSectors(handle, 1, pos->sector, buffer) == -1)
    {
	FreeSectors((SECTOR*) buffer);
        return FALSE;
    }

    memcpy(&buffer[pos->offset << 5], direct,
	   sizeof(struct DirectoryEntry));

    if (WriteSectors(handle, 1, pos->sector, buffer, WR_DIRECT) == -1)
    {
	FreeSectors((SECTOR*) buffer);
        return FALSE;
    }

    FreeSectors((SECTOR*) buffer);
    return TRUE;
}
