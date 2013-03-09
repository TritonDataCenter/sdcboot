/*
   Surface.c - data area scanning.
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

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "fte.h"

static BOOL RewriteCluster(RDWRHandle handle, CLUSTER cluster);
static BOOL LenientRelocateCluster(RDWRHandle handle,
                                   CLUSTER source, CLUSTER destination);
static BOOL TestClusterSequence(RDWRHandle handle,
                                SECTOR start,
                                int length,
                                char* buf1, 
                                char* buf2);                                   

#ifndef SCANBLOCKSIZE
#define SCANBLOCKSIZE 16384
#endif
                                   
BOOL ScanSurface(RDWRHandle handle)
{
   unsigned char sectorspercluster;
   unsigned long clustersindataarea, i, sectorsindataarea, totest, j;
   SECTOR datastart, start;
   CLUSTER cluster, label, prevcluster;
   char buffer1[BYTESPERSECTOR], buffer2[BYTESPERSECTOR];
   char *realbuf1, *realbuf2;
   int sectorsperreadbuf = 1;
   unsigned long prevIndication=0;
   int ii;
   unsigned long indication; 

   printf("Scanning, please wait . . .\n");
   printf("\n|------------------------------------------------------------------------------|");
   printf(" ");
    
   datastart = GetDataAreaStart(handle);
   if (!datastart) return FALSE;

   sectorspercluster = GetSectorsPerCluster(handle);
   if (!sectorspercluster) return FALSE;

   clustersindataarea = GetClustersInDataArea(handle);
   if (!clustersindataarea) return FALSE;

   sectorsindataarea = clustersindataarea * sectorspercluster;
   
   /* Try to allocate memory for the scan */
   realbuf1 = malloc(SCANBLOCKSIZE);
   if (realbuf1)
   {
      realbuf2 = malloc(SCANBLOCKSIZE);
      if (!realbuf2)
      {
         free(realbuf1);
         
         realbuf1 = buffer1;
         realbuf2 = buffer2;
      }
      else
      {
         sectorsperreadbuf = SCANBLOCKSIZE / BYTESPERSECTOR;
      }
   }
   else
   {
      realbuf1 = buffer1;
      realbuf2 = buffer2;           
   }   
   
   for (i = datastart; i < sectorsindataarea+datastart; i+=sectorsperreadbuf)
   {
	indication = (i-datastart) * 78 / sectorsindataarea;	 
	   
        for (ii = 0; ii < indication - prevIndication; ii++)
	{
	   printf("o");
	}
	prevIndication = indication;

       prevcluster = 0;  
       start = i;
       for (j = 0; (j < sectorsperreadbuf) && (i+j < sectorsindataarea+datastart); j++)
       {
           cluster = DataSectorToCluster(handle, i+j);
           if (!cluster)
           {        
              if (realbuf1 != buffer1) free(realbuf1);
              if (realbuf1 != buffer1) free(realbuf2);
              return FALSE;
           }
           
           if (cluster != prevcluster)
           {
              if (!GetNthCluster(handle, cluster, &label))
              {        
                 if (realbuf1 != buffer1) free(realbuf1);
                 if (realbuf1 != buffer1) free(realbuf2);                 
                 return FALSE;
              }
                 
              if (FAT_BAD(label))
              {  
                 if (!TestClusterSequence(handle, start, (int)(i+j-start), realbuf1, realbuf2))
                 {
                    if (realbuf1 != buffer1) free(realbuf1);
                    if (realbuf1 != buffer1) free(realbuf2);                 
                    return FALSE;                 
                 }                 
                 start = i+j+1;
              }
           }
           prevcluster = cluster;
       }

       if (i+sectorsperreadbuf > sectorsindataarea+datastart)
          totest = sectorsindataarea+datastart - start;
       else
          totest = i+sectorsperreadbuf-start;
      
       if (!TestClusterSequence(handle, start, (int)totest, realbuf1, realbuf2))
       {
          if (realbuf1 != buffer1) free(realbuf1);
          if (realbuf1 != buffer1) free(realbuf2);                 
          return FALSE;                 
       }           
   }  
   
   for (ii = 0; ii < 78 - prevIndication; ii++)
   {
       printf("o");
   }
   printf("\n");
   
   if (realbuf1 != buffer1) free(realbuf1);
   if (realbuf1 != buffer1) free(realbuf2);       
       
   if (!SynchronizeFATs(handle))
      return FALSE;
      
   return TRUE;
}

static BOOL TestClusterSequence(RDWRHandle handle,
                                SECTOR start,
                                int length,
                                char* buf1, 
                                char* buf2)
{
   BOOL result = TRUE;
   unsigned long i;
   CLUSTER cluster, prevcluster=0, label;
   
   if (length                                                &&
       ((!ReadDataSectors(handle, length, start, buf1))  ||
        (!WriteDataSectors(handle, length, start, buf1)) ||
	(!ReadDataSectors(handle, length, start, buf2))  ||
	(memcmp(buf1, buf2, length*BYTESPERSECTOR) != 0)))
   {
      printf("\nMedia error at sector %lu, relocating . . .\n", start);
   
      for (i = start; i < start+length; i++)
      {  
         if (!ReadDataSectors(handle, 1, i, buf1)  ||
             !WriteDataSectors(handle, 1, i, buf1) ||    
             !ReadDataSectors(handle, 1, i, buf2)  ||
             memcmp(buf1, buf2, BYTESPERSECTOR) != 0)
         {
            cluster = DataSectorToCluster(handle, i);
            if (!cluster) return FALSE;
       
            if (cluster != prevcluster)
            {
               if (!GetNthCluster(handle, cluster, &label))
                  return FALSE;
             
               if (!FAT_FREE(label))
               {
                  result = RewriteCluster(handle, cluster);       
               }
               result = result && WriteFatLabel(handle, cluster, FAT_BAD_LABEL);

               SynchronizeFATs(handle);

               if (!result)
                  return FALSE;
            }
         }
          
          prevcluster = cluster;
      }
   }
   
   return TRUE;
}
                                   

static BOOL RewriteCluster(RDWRHandle handle, CLUSTER cluster)
{
    CLUSTER freespace;
    unsigned long length;

   /* Search for the first free space. */
   if (!FindFirstFreeSpace(handle, &freespace, &length))
      return FALSE;

   for (;;)
   {
      if (length == 0)
      {
         printf("No free space to relocate cluster!\n");
         return FALSE;
      }

      switch (LenientRelocateCluster(handle, cluster, freespace))
      {
         case TRUE:          /* Recovered as much as possible. */
              return TRUE;
         
         case FALSE:         /* Problem writing to free space. */
              if (!FindNextFreeSpace(handle, &freespace, &length))
                 return FALSE;
              break;
              
         case FAIL:          /* Other media error (like in FAT, dirs, ...) */
              return FALSE;
      }
   }
}

/* This function returns FALSE if the destination is not free. */
static BOOL LenientRelocateCluster(RDWRHandle handle,
                                   CLUSTER source, CLUSTER destination)
{
   int j;
   CLUSTER fatpos, dircluster, label;
   struct DirectoryPosition dirpos;
   struct DirectoryEntry entry;
   BOOL IsInFAT = FALSE;
   SECTOR srcsector, destsector;
   unsigned char sectorspercluster, i;
   CLUSTER clustervalue;
   BOOL found, DOTSprocessed=FALSE, retval = TRUE;
   char* sectbuf;
   
   /* See wether the destination is actually free. */
   if (!GetNthCluster(handle, destination, &label))
      return FAIL;
   if (!FAT_FREE(label)) 
      return FAIL;

   /* See wether we are relocating a filled cluster */
   if (!GetNthCluster(handle, destination, &label))
      return FAIL;
   if (FAT_FREE(label))
      return TRUE;

   /* Do some preliminary calculations. */
   srcsector = ConvertToDataSector(handle, source);
   if (!srcsector)
      return FAIL;
   destsector = ConvertToDataSector(handle, destination);
   if (!destsector)
      return FAIL;
   sectorspercluster = GetSectorsPerCluster(handle);
   if (!sectorspercluster)
      return FAIL;
         
   /* Get the value that is stored at the source position in the FAT */
   if (!ReadFatLabel(handle, source, &clustervalue))
      return FAIL;
   
   /* See where the cluster is refered */
   if (!FindClusterInFAT(handle, source, &fatpos))
      return FAIL;
      
   if (!fatpos)
   {
      if (!FindClusterInDirectories(handle, source, &dirpos, &found))
         return FAIL;
      if (!found)
      {
         return TRUE;                /* Unreferenced cluster, never mind */
      }
      else
      {
         /*
            This is the first cluster of some file. See if it is a directory
            and if it is, adjust the '.' entry of this directory and all
            of the '..' entries of all the (direct) subdirectories to point
            to the new cluster.
         */
         if (!GetDirectory(handle, &dirpos, &entry))
            return FAIL;
            
         if (entry.attribute & FA_DIREC)
         {
            dircluster = GetFirstCluster(&entry);
            if (!AdaptCurrentAndPreviousDirs(handle, dircluster, destination))
            {
               return FAIL;
            }
            DOTSprocessed = TRUE;
         }
      }
   }
   else
   {
      IsInFAT = TRUE;
   }

   /* Copy as much sectors as possible in this cluster to the new position */
   sectbuf = (char*) AllocateSector(handle);
   if (!sectbuf) return FAIL;

   for (i = 0; i < sectorspercluster; i++)
   {
       for (j = 0; j < 3; j++) /* Try 3 times */
           if (ReadDataSectors(handle, 1, srcsector + i, sectbuf))
              break;
              
       if (!WriteDataSectors(handle, 1, destsector + i, sectbuf))
       {
          FreeSectors((SECTOR*) sectbuf);
          return FALSE;
        }
    }
    
    FreeSectors((SECTOR*) sectbuf);
   
   /* Write the entry in the FAT */
   if (!WriteFatLabel(handle, destination, clustervalue))
   {
      if (DOTSprocessed)
         AdaptCurrentAndPreviousDirs(handle, dircluster, source);
      return FAIL;
   }

   /* Adjust the pointer to the relocated cluster */
   if (IsInFAT)
   {
      if (!WriteFatLabel(handle, fatpos, destination))
      {
         return FAIL;
      }
   }
   else
   {
      if (!GetDirectory(handle, &dirpos, &entry))
      {
         if (DOTSprocessed)
            AdaptCurrentAndPreviousDirs(handle, dircluster, source);
      
         return FAIL;
      }
      
      SetFirstCluster(destination, &entry);
      if (!WriteDirectory(handle, &dirpos, &entry))
      {
         if (DOTSprocessed)
            AdaptCurrentAndPreviousDirs(handle, dircluster, source);
      
         return FAIL;
      }
   }

   return retval;
}
