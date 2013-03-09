/*    
   RelocClt.c - Function to relocate a cluster in a volume.

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
#include <stdio.h>

#include "fte.h"
void Log(char* s);
#define log(s, x)                   \
    {                               \
        char buffer[1000];          \
        sprintf(buffer, s"\n", x);  \
        Log(buffer);                \
    }


/* This function returns FALSE if the destination is not free. */
BOOL RelocateCluster(RDWRHandle handle, CLUSTER source, CLUSTER destination)
{   int labelsize, value;
   CLUSTER fatpos=0, freecluster, dircluster, label;
   struct DirectoryPosition dirpos;
   struct DirectoryEntry entry;
   BOOL IsInFAT = FALSE;
   SECTOR srcsector, destsector;
   unsigned long sectorspercluster;
   CLUSTER clustervalue;
   BOOL found, DOTSprocessed=FALSE;
   struct FSInfoStruct FSInfo;

log("%s", "starting relocation");

   /* See wether the destination is actually free. */
   if (!GetNthCluster(handle, destination, &label))
      RETURN_FTEERROR(FALSE);
log("destination label is %lu", label);
   if (!FAT_FREE(label)) 
      RETURN_FTEERROR(FALSE);
log("%s", "which means free");   

   /* Do some preliminary calculations. */
   srcsector = ConvertToDataSector(handle, source);
   if (!srcsector)
      RETURN_FTEERROR(FALSE);
   destsector = ConvertToDataSector(handle, destination);
   if (!destsector)
      RETURN_FTEERROR(FALSE);
   sectorspercluster = GetSectorsPerCluster(handle);
   if (!sectorspercluster)
      RETURN_FTEERROR(FALSE);
         
log("srcsector = %lu", srcsector);
log("destsector = %lu", destsector);
log("sectorspercluster = %lu", sectorspercluster);

   /* Get the value that is stored at the source position in the FAT */
   if (!ReadFatLabel(handle, source, &clustervalue))
      RETURN_FTEERROR(FALSE);

log("source clustervalue = %lu", clustervalue);

  if (!FindClusterInFAT1(handle, source, &fatpos))
      RETURN_FTEERROR(FALSE);

   if (!fatpos)
   {
log("%s", "cluster not refered to by fat");
       if (!FindClusterInDirectories(handle, source, &dirpos, &found))
         RETURN_FTEERROR(FALSE);
      if (!found)
      {
          /* Note: on FAT32 this cluster may be pointing to the root directory.
                   We do not support relocating the root cluster at this time.          
          */
          
log("%s", "not refered to in any directory");          
          RETURN_FTEERROR(FALSE);                /* Non valid cluster! */
      }
      else
      {
         /*
            This is the first cluster of some file. See if it is a directory
            and if it is, adjust the '.' entry of this directory and all
            of the '..' entries of all the (direct) subdirectories to point
            to the new cluster.
         */
log("%s", "directory found");

         if (!GetDirectory(handle, &dirpos, &entry))
            RETURN_FTEERROR(FALSE);
            
log("dirpos.sector = %lu", (unsigned long)dirpos.sector);
log("dirpos.offset = %lu", (unsigned long)dirpos.offset);

         if (entry.attribute & FA_DIREC)
         {
log("%s", "this is a directory");
log("%s", "adapting current and previous dirs");
            dircluster = GetFirstCluster(&entry);
            if (!AdaptCurrentAndPreviousDirs(handle, dircluster, destination))
            {
               RETURN_FTEERROR(FALSE);
            }
            DOTSprocessed = TRUE;
         }
      }
   }
   else
   {
log("cluster refered to by cluster %lu", fatpos);
       IsInFAT = TRUE;
   }

   /* Copy all sectors in this cluster to the new position */
log("%s", "copying sectors");
   if (!CopySectors(handle, srcsector, destsector, sectorspercluster))
   {
      if (DOTSprocessed)
         AdaptCurrentAndPreviousDirs(handle, dircluster, source);
      RETURN_FTEERROR(FALSE);
   }
   
log("writing cluster = %lu to ", clustervalue);
log("destination = %lu\n", destination);
   if (!WriteFatLabel(handle, destination, clustervalue))
   {
      if (DOTSprocessed)
         AdaptCurrentAndPreviousDirs(handle, dircluster, source);
      RETURN_FTEERROR(FALSE);
   }

   /* Adjust the pointer to the relocated cluster */
   if (IsInFAT)
   {
log("%s", "was in fat");

log("Writing dest label %lu", destination);
log("to %lu", fatpos);

      if (!WriteFatLabel(handle, fatpos, destination))
      {
         RETURN_FTEERROR(FALSE);
      }
   }
   else
   {
      CLUSTER label;

log("%s", "adjusting destination in directory");

      if (!GetDirectory(handle, &dirpos, &entry))
      {
         if (DOTSprocessed)
            AdaptCurrentAndPreviousDirs(handle, dircluster, source);
      
         RETURN_FTEERROR(FALSE);
      }
      
log("setting destination = %lu", destination);
log("file = %8c", entry.filename);
log("extension=%3c", entry.extension);

      SetFirstCluster(destination, &entry);
      if (!WriteDirectory(handle, &dirpos, &entry))
      {
         if (DOTSprocessed)
            AdaptCurrentAndPreviousDirs(handle, dircluster, source);
      
         RETURN_FTEERROR(FALSE);
      }
   }

log("writing empty label at %lu", source);

   if (!WriteFatLabel(handle, source, FAT_FREE_LABEL))
   {
      if (DOTSprocessed)
         AdaptCurrentAndPreviousDirs(handle, dircluster, source);
   
      RETURN_FTEERROR(FALSE);
   }
   
   return TRUE;
}

FILE* fptr;

BOOL OpenLogFile(char* logfile)
{
    fptr = fopen(logfile, "wt");
}

void Log(char* s)
{
   printf("%s", s);
   fprintf(fptr, "%s", s);
}

void CloseLogFile()
{
   fclose(fptr);
}
