/*
   FstTrMap.c - fast tree map.
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

#include "fte.h"

#define NOMOREENTRIES -22

static char* fatmap = NULL;
static char* FastTreeMap = NULL;
unsigned long FastTreeLen = 0;
static BOOL TreeIncomplete = FALSE;

static void ConvertTreeMaps(RDWRHandle handle, char* fatmap,
			    unsigned long elements,
			    char** result, unsigned long* newlen);
static BOOL BackupCaller(RDWRHandle handle, struct DirectoryPosition* pos,
                         void** structure);
static void WriteDigits(char* bitfield, unsigned long index, unsigned long number,
                        int base);
static unsigned long ReadDigits(char* bitfield, unsigned long index, int base);
static BOOL GenerateDirectoryPositions(RDWRHandle handle, CLUSTER cluster,
                                       BOOL (*func)(RDWRHandle handle, 
                                                    struct DirectoryPosition* pos,
                                                    struct DirectoryEntry* entry,
                                                    void** structure),
                                        void** structure);

static BOOL CallWithCheckedEntry(RDWRHandle handle,
                                 struct DirectoryPosition* pos,
                                 BOOL (*func)(RDWRHandle handle,
                                              struct DirectoryPosition* pos,
                                              struct DirectoryEntry* entry,
                                              void** structure),
                                 void** structure);

struct BackupPipe
{
    BOOL (*func)(RDWRHandle handle, 
                 struct DirectoryPosition* pos,
                 struct DirectoryEntry* entry,
                 void** structure);
                 
    void** structure;
};

BOOL CreateFastTreeMap(RDWRHandle handle)
{ 
    unsigned long labelsinfat;  
       
    /* First make a normal bitfield */
    labelsinfat = GetLabelsInFat(handle);
    if (!labelsinfat) return FALSE;
    
    fatmap = CreateBitField(labelsinfat);
    if (!fatmap) return FALSE;
    
    return TRUE;
}    
    
BOOL CompressFastTreeMap(RDWRHandle handle)
{    
    unsigned long labelsinfat;  
       
    labelsinfat = GetLabelsInFat(handle);
    if (!labelsinfat) return FALSE;        
        
    if (fatmap)
    {
       ConvertTreeMaps(handle, fatmap, labelsinfat, &FastTreeMap, &FastTreeLen);        
       DestroyBitfield(fatmap);
       fatmap = NULL;
    }
    
    return TRUE;
}    

BOOL MarkDirectoryEntry(RDWRHandle handle, struct DirectoryPosition* pos)
{
    int FatLabelSize;
    CLUSTER cluster;
    
    if (!fatmap) return TRUE;
    
    FatLabelSize = GetFatLabelSize(handle);
    switch (FatLabelSize)
    {
       case 0:
            return FALSE;
            
       case FAT12:
       case FAT16:
           /* Not taking into accout the root directory */
           switch (IsRootDirPosition(handle, pos))
           {
             case TRUE:
                  return TRUE;
             case -1:                        /* Media error */
                  return FALSE;
           }
           break;
    }       
    cluster = DataSectorToCluster(handle, pos->sector);
    if (!cluster) return FALSE;  
  
    SetBitfieldBit(fatmap, cluster);
    
    return TRUE;
}

void DestroyFastTreeMap(void)
{
   if (FastTreeMap)
   {        
      free(FastTreeMap);
      FastTreeMap = NULL;
   }
   
   if (fatmap)
   {
      DestroyBitfield(fatmap);
      fatmap = NULL;   
   }
}

static void ConvertTreeMaps(RDWRHandle handle, char* fatmap,
			    unsigned long elements,
			    char** result, unsigned long* newlen)
{
   int fatlabelsize;
   unsigned long i, amofnuls=0;
   unsigned long outptr;
    
   fatlabelsize = GetFatLabelSize(handle);
   if (!fatlabelsize) return;
   
   /* First count how much memory we have to allocate */ 
   *newlen = 0;
   for (i= 0; i < elements; i++)
   {
       if (GetBitfieldBit(fatmap, i))
       {
          if (amofnuls == 1)
             *newlen += 2;
          if (amofnuls > 1)
             *newlen += fatlabelsize+2;
          
          (*newlen)++;
          amofnuls = 0;
       }
       else
       {
          amofnuls++;
       }        
   }

   /* Now allocate the necessary space */
   if (!*newlen)
   {
	   *result = 0;
	   return;
   }

   *result = (char*)CreateBitField(*newlen);
   if (!*result) return;
   
   /* Finaly write the fast tree map. */
   outptr = 0;
   amofnuls = 0;
   for (i= 0; i < elements; i++)
   {
       if (GetBitfieldBit(fatmap, i))
       {
          if (amofnuls == 1)
          {
             /* output two 0's */
             outptr += 2; /* Notice that there the bitfield starts out all 0 */
          }             
          if (amofnuls > 1)
          {
	     SetBitfieldBit(*result, outptr+1);
             outptr+=2;
             
             /* Write the number of 0's we encountered as 
                a digit in the right base. */
	     WriteDigits(*result, outptr, amofnuls, fatlabelsize);
             outptr += fatlabelsize; 
          }
	  SetBitfieldBit(*result, outptr);
          outptr++;
          
          amofnuls = 0;
       }
       else
       {
          amofnuls++;
       }        
   }
}

static void WriteDigits(char* bitfield, unsigned long index, unsigned long number,
                        int base)
{
   unsigned long i;
     
   for (i = index; i < index+base; i++)
   {
       if (number & 1)
          SetBitfieldBit(bitfield, i);
          
       number >>= 1;          
   } 
}

static unsigned long ReadDigits(char* bitfield, unsigned long index, int base)
{
   unsigned long i, radix = 1, number=0;
     
   for (i = index; i < index+base; i++)
   {
       if (GetBitfieldBit(bitfield, i))
          number += radix;
          
       radix <<= 1;          
   }
   
   return number;
}

static BOOL BackupCaller(RDWRHandle handle, struct DirectoryPosition* pos,
                         void** structure)
{
   struct BackupPipe *pipe = *((struct BackupPipe**) structure);
   struct DirectoryEntry entry;
   
   if (!GetDirectory(handle, pos, &entry))
      return FAIL;
      
   return pipe->func(handle, pos, &entry, (void**) pipe->structure);
}

BOOL FastWalkDirectoryTree(RDWRHandle handle,
                           BOOL (*func)(RDWRHandle handle, 
                                        struct DirectoryPosition* pos,
                                        struct DirectoryEntry* entry,
                                        void** structure), 
                           void** structure)
{
   int labelsize;
   unsigned long i;
   CLUSTER current=0;
   struct BackupPipe backpipe, *pBackupPipe = &backpipe;
          
   backpipe.func = func;
   backpipe.structure = structure;

   /* See wether the fast tree map exists */ 
   if (!FastTreeMap)
   {
      /* If not, call normal walkdirectoryTree */
      return WalkDirectoryTree(handle, BackupCaller, (void**) &pBackupPipe);
   }

   /* If it is: */
   
   /* First walk the root dir */
   if (!TraverseRootDir(handle, BackupCaller, (void**) &pBackupPipe,
                        TRUE))
      return FALSE;
   
   /* Then walk the map */
   i = 0; 
   labelsize = GetFatLabelSize(handle);
   if (!labelsize) return FALSE;
           
   while (i < FastTreeLen)
   {
       if (GetBitfieldBit(FastTreeMap, i))
       {
          /* Cluster found, generate a call. */ 
	  switch (GenerateDirectoryPositions(handle, current, func, structure))
          {
             case FALSE: 
		  return TRUE;
             case FAIL:
		  return FALSE;
          }
          
	  current++;
          i++;     
       }
       else
       {
          if (GetBitfieldBit(FastTreeMap, i+1))
          {
	     current += ReadDigits(FastTreeMap, i+2, labelsize);
	     i+=labelsize+2;
          } 
          else
          {
	     current++;
             i+=2;
          }
       }
   }
   
   return TRUE;
}

static BOOL GenerateDirectoryPositions(RDWRHandle handle, CLUSTER cluster,
                                       BOOL (*func)(RDWRHandle handle, 
                                                    struct DirectoryPosition* pos,
                                                    struct DirectoryEntry* entry,
                                                    void** structure),
                                        void** structure)
{
   int j;     
   unsigned char sectorspercluster, i;
   struct DirectoryPosition current; 
      
   sectorspercluster = GetSectorsPerCluster(handle);
   if (!sectorspercluster) return FALSE;
   
   current.sector = ConvertToDataSector(handle, cluster);
   
   for (i = 0; i < sectorspercluster; i++)
   {
       for (j = 0; j < ENTRIESPERSECTOR; j++)
       {
           current.offset = j;
	   switch (CallWithCheckedEntry(handle, &current, func, structure))
           {
              case FALSE:
		   return FALSE;
              case FAIL:
                   return FAIL;
              case NOMOREENTRIES:
                   return TRUE;
           }
       }
           
       current.sector++;
   }

   return TRUE;
}

static BOOL CallWithCheckedEntry(RDWRHandle handle,
                                 struct DirectoryPosition* pos,
                                 BOOL (*func)(RDWRHandle handle,
                                              struct DirectoryPosition* pos,
                                              struct DirectoryEntry* entry,
                                              void** structure),
                                 void** structure)
{
   struct DirectoryEntry entry;
   
   if (!GetDirectory(handle, pos, &entry))
      return FAIL;
      
   if (IsLastLabel(entry))
      return NOMOREENTRIES;
            
   return func(handle, pos, &entry, structure);
}
                  
void IndicateTreeIncomplete(void)
{
   TreeIncomplete = TRUE;
}

BOOL IsTreeIncomplete(void)
{
   return TreeIncomplete;
}