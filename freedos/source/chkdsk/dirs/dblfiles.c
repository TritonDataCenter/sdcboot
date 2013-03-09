/*
   dblfiles.c - double files checking.
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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "fte.h"
#include "..\chkdrvr.h"
#include "..\struct\FstTrMap.h"
#include "..\errmsgs\errmsgs.h"
#include "..\errmsgs\PrintBuf.h"
#include "..\misc\useful.h"
#include "dirschk.h"

/*
** We use a hash table (26*26 +1 in size) and an array large enough to contain
** all the needed info from a directory.
**
** For every directory entry we hash the file name to a certain slot. If the 
** slot already has entries assigned, then it is looked wether any of the
** elements has the same file name and extension.
*/

#define HASHTABLESIZE ((26 * 26) + 1)

struct ArrayElement
{
    char filename[8];
    char extension[3];
    unsigned next;
};

struct HashSlot
{
    unsigned ptr;
};

struct Pipe
{
    struct HashSlot*     hashtable;
    struct ArrayElement* filenamearray;

    CLUSTER surroundingdir;
    BOOL fixit;
    BOOL doublefound;
};

static int ArrayFreePointer;

static BOOL RenameDoubleFile(RDWRHandle handle,
                             struct DirectoryPosition* pos,
                             struct DirectoryEntry* entry,
                             CLUSTER firstcluster);
                             
static unsigned HashFilename(char* filename);                             

static struct HashSlot* hashtable;
static struct ArrayElement* filenamearray;      

static CLUSTER surroundingdir;                            
                                  
                                  
BOOL InitDblNameChecking(RDWRHandle handle, CLUSTER cluster, char* filename)
{
    long dircount;
    
    UNUSED(filename);
    
    hashtable = (struct HashSlot*)
                malloc(sizeof(struct HashSlot) *  HASHTABLESIZE);
    if (!hashtable) return FALSE;
    
    /* Make sure all next pointers are invalidated */
    memset(hashtable, 0xff, sizeof(struct HashSlot) *  HASHTABLESIZE);    
  
    /* 0xFF means all the files in the directory */
    dircount = low_dircount(handle, cluster, 0xFF); 
    if (dircount == FAIL)                               /* FAIL == -1 */
    {
       free(hashtable);
       return FALSE;
    }

    /* Try to allocate memory for the array. */
    filenamearray = (struct ArrayElement*)
		    malloc(((unsigned)dircount) * sizeof(struct ArrayElement));
    if (!filenamearray)
    {
       free(hashtable);
       return FALSE;
    }
  
    surroundingdir = cluster;
    
    return TRUE;
}                                  
                                  
BOOL PreProcesDblNameChecking(RDWRHandle handle, CLUSTER cluster, char* filename, BOOL fixit)
{
    UNUSED(handle);
    UNUSED(cluster);
    UNUSED(fixit);
    UNUSED(filename);
  
    ArrayFreePointer = 0;
    return TRUE;
}

BOOL CheckDirectoryForDoubles(RDWRHandle handle,                                   
                              struct DirectoryPosition* pos, 
                              struct DirectoryEntry* entry, 
                              char* filename, 
                              BOOL fixit)
{
    unsigned slot, now, previous = 0xffff;
    
    if ((entry->attribute & FA_LABEL) ||
        IsLFNEntry(entry)             ||
        IsDeletedLabel(*entry))
    {
       return TRUE;
    }

    slot = HashFilename(entry->filename);

    /* Now walk the list starting at the head in the hash table. */
    now = hashtable[slot].ptr;
    while (now != 0xFFFF)
    {
       if ((memcmp(entry->filename,
                   filenamearray[now].filename, 8) == 0) &&
           (memcmp(entry->extension,
                   filenamearray[now].extension, 3) == 0))
       {
           /* Print out the message and the file name */
           ShowFileNameViolation(handle, filename,  
                                  "Found double file %s");            
           
           if (fixit)
           {
              if (!RenameDoubleFile(handle, pos, entry, surroundingdir))
	         return FAIL;
           }

           return FALSE;
       }

       previous = now;
       now = filenamearray[now].next;
    }

    /* Not found, add it to the end of the list */
    if (previous == 0xFFFF) /* First one in the slot. */
    {
       hashtable[slot].ptr = ArrayFreePointer;
    }
    else
    {  /* We already have the end of the list */
       filenamearray[previous].next = ArrayFreePointer;
    }

    memcpy(filenamearray[ArrayFreePointer].filename, entry->filename, 8);
    memcpy(filenamearray[ArrayFreePointer].extension, entry->extension, 3);
    filenamearray[ArrayFreePointer].next = 0xFFFF;

    ArrayFreePointer++;
  
    return TRUE;
} 

BOOL PostProcesDblNameChecking(RDWRHandle handle, CLUSTER cluster, char* filename, BOOL fixit)
{
    UNUSED(handle);
    UNUSED(cluster);
    UNUSED(fixit);
    UNUSED(filename);
   
    return TRUE;
}                                 
     
void DestroyDblNameChecking(RDWRHandle handle, CLUSTER cluster, char* filename)
{
     UNUSED(handle);
     UNUSED(cluster);
     UNUSED(filename);
     
     free(hashtable);
     free(filenamearray);
}

/*************************************************************************
**                           HashFilename
**************************************************************************
** The hash function
**************************************************************************/

static unsigned HashFilename(char* filename)
{
#if 0
   char buf[2];
   memcpy(buf, filename, 2);

   if (((buf[0] < 'A') || (buf[0] > 'Z')) ||
       ((buf[1] < 'A') || (buf[1] > 'Z')))
   {
      return 26*26;
   }

   buf[0] -= 'A';
   buf[1] -= 'A';

   return ((unsigned)buf[0]) * 26 + ((unsigned)buf[1]);
#endif
#if 1                           /* Alternative method */
   return (unsigned)
             (
              (*((unsigned long*)  filename) +
               *(((unsigned long*) filename) + 1)) %
          HASHTABLESIZE);

#endif
}

/*************************************************************************
**                           RenameDoubleFile
**************************************************************************
** Renames a double file. It does this by taking a file name and adding a 
** a numeric extension to it.
**
** eg. defrag.lst => defrag.000
**************************************************************************/

static BOOL RenameDoubleFile(RDWRHandle handle,
                             struct DirectoryPosition* pos,
                             struct DirectoryEntry* entry,
                             CLUSTER firstcluster)
{
    BOOL retval;
    int counter=0;
    char newext[3];

    for (;;)
    {
	sprintf(newext, "%03d", counter);
        
        retval = LoFileNameExists(handle, firstcluster,
                                  entry->filename, newext);

        if (retval == FALSE) break;
        if (retval == FAIL)  return FALSE;

        counter++;
    }

    memcpy(entry->extension, newext, 3);

    return WriteDirectory(handle, pos, entry);
}