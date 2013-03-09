/*
   LostClts.c - lost cluster checking.
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

#include "fte.h"
#include "..\chkdrvr.h"
#include "..\struct\FstTrMap.h"
#include "..\errmsgs\errmsgs.h"

static BOOL SalvageClusters(RDWRHandle handle, char* bitfield,
                            unsigned long clustersindataarea);

static BOOL FillNewEntry(RDWRHandle handle,
                         struct DirectoryEntry* entry,
                         CLUSTER firstcluster);

static BOOL CheckFileChain(RDWRHandle handle, CLUSTER firstcluster,
                           unsigned long* filesize);

static BOOL MakeUpFileName(RDWRHandle handle,
                           char* filename, char* extension);
                           
static BOOL MarkClusters(RDWRHandle handle, char* fatmap);

static BOOL UsedMarker(RDWRHandle handle, CLUSTER label,
                       SECTOR datasector, void** structure);

static BOOL FirstClusterMarker(RDWRHandle handle,
                               struct DirectoryPosition* pos,
                               struct DirectoryEntry* entry,
                               void** structure);

static BOOL IsStartOfChain(RDWRHandle handle, CLUSTER cluster);

static BOOL ReferedMarker(RDWRHandle handle, CLUSTER label,
                          SECTOR datasector, void** structure);
                          
static BOOL MarkAllClusters(RDWRHandle handle, 
                            char* fatmap, unsigned long clustersindataarea);                          

static BOOL FinaliseFatMap(RDWRHandle handle, char* fatmap, CLUSTER cluster);                            
                            
/*============================== Checking ==============================*/

/*************************************************************************
**                           FindLostClusters
**************************************************************************
** Searches trough the FAT and finds out wether there are any lost clusters.
**************************************************************************/
RETVAL FindLostClusters(RDWRHandle handle)
{
   char* fatmap;
   unsigned long clustersindataarea;
   RETVAL retval=0;
   
   clustersindataarea = GetClustersInDataArea(handle);
   if (!clustersindataarea) return ERROR;

   /* Create the bit field. */
   fatmap = CreateBitField(clustersindataarea);
   if (!fatmap) return ERROR;
   
   /* Look for lost clusters. */
   switch (MarkAllClusters(handle, fatmap, clustersindataarea))
   {
       case TRUE:
            retval = FAILED;
            break;
               
       case FALSE:
            retval = SUCCESS;
            break;
               
       case FAIL:
            retval = ERROR;
            break;
   }
   
   DestroyBitfield(fatmap);
   return retval;
}

/*============================== Fixing ================================*/

/*************************************************************************
**                           ConvertLostClustersToFiles
**************************************************************************
** Searches trough the FAT and finds out wether there are any lost clusters,
** if there are the clusters are converted to files.
**************************************************************************/

RETVAL ConvertLostClustersToFiles(RDWRHandle handle)
{
   char* fatmap;
   unsigned long clustersindataarea;
   RETVAL retval=SUCCESS;
   
   clustersindataarea = GetClustersInDataArea(handle);
   if (!clustersindataarea) return ERROR;

   /* Create the bit field. */
   fatmap = CreateBitField(clustersindataarea);
   if (!fatmap) return ERROR;
   
   /* Look for lost clusters. */
   switch (MarkAllClusters(handle, fatmap, clustersindataarea))
   {
       case TRUE:
            if (!SalvageClusters(handle, fatmap, clustersindataarea))
               retval = ERROR;
            break;
             
       case FAIL:
            retval = ERROR;
            break;
   }
   
   DestroyBitfield(fatmap);
   return retval;
}

/*************************************************************************
**                           SalvageClusters
**************************************************************************
** Goes through the bitfield,
** if there are the clusters are converted to files.
**************************************************************************/

static BOOL SalvageClusters(RDWRHandle handle, char* bitfield,
                            unsigned long clustersindataarea)
{
    unsigned long i;
    struct DirectoryPosition newpos;
    struct DirectoryEntry entry;

    for (i = 0; i < clustersindataarea; i++)
    {
        if (GetBitfieldBit(bitfield, i))
        {
           switch (IsStartOfChain(handle, i+2))
           {
              case TRUE:        /* This is the first cluster in the chain */
              {
                 /* We have found a lost cluster chain, convert it to a file. */
                 /* Allocate a new directory entry in the root directory. */
                 switch (AddDirectory(handle, 0, &newpos))
                 {
                    case FALSE:
                         ReportGeneralRemark("Insufficient space in the root directory to save lost clusters\n");
                         return TRUE;
            
                    case TRUE:
                         if (!FillNewEntry(handle, &entry, i+2))
                            return FALSE;
                         if (!WriteDirectory(handle, &newpos, &entry))
                            return FALSE;
                         break;
       
                    case FAIL:
                         return FALSE;
                 }
              }
              break;
              
              case FAIL:
                   return FALSE;
           }
        }
    }

    return TRUE;
}

/*************************************************************************
**                           FillNewEntry
**************************************************************************
**  Fills the given entry with newly generated data.
**************************************************************************/

static BOOL FillNewEntry(RDWRHandle handle,
                         struct DirectoryEntry* entry,
                         CLUSTER firstcluster)
{
    struct tm* tmp;
    time_t now;
    
    unsigned long filesize;
    char filename[8], extension[3];

    /* Check the file chain */
    if (!CheckFileChain(handle, firstcluster, &filesize))
       return FALSE;
    
    /* file name and extension */
    if (!MakeUpFileName(handle, filename, extension))
       return FALSE;

    memcpy(entry->filename, filename, 8);
    memcpy(entry->extension, extension, 3);

    /* attribute */
    entry->attribute = 0;

    /* first cluster */
    SetFirstCluster(firstcluster, entry);

    /* file size */
    entry->filesize = filesize;

    /* NT reserved field */
    entry->NTReserved = 0;

    /* Mili second stamp */
    entry->MilisecondStamp = 0;

    /* Last access date */
    memset(&entry->LastAccessDate, 0, sizeof(struct PackedDate));
    
    /* Time last modified */
    memset(&entry->timestamp, 0, sizeof(struct PackedTime));

    /* Date last modified */
    memset(&entry->datestamp, 0, sizeof(struct PackedDate));

    /* Get the current date and time and store it in the last write
       time and date. */
    time(&now);
    tmp = localtime(&now);

    entry->LastWriteTime.second = tmp->tm_sec / 2;
    if (entry->LastWriteTime.second == 30) /* DJGPP help says range is [0..60] */
       entry->LastWriteTime.second--;
    
    entry->LastWriteTime.minute = tmp->tm_min;
    entry->LastWriteTime.hours  = tmp->tm_hour;

    entry->LastWriteDate.day   = tmp->tm_mday;
    entry->LastWriteDate.month = tmp->tm_mon + 1;

    if (tmp->tm_year < 80)
       entry->LastWriteDate.year = 0;
    else
       entry->LastWriteDate.year  = (tmp->tm_year+1900)-1980;

    return TRUE;
}

/*************************************************************************
**                           CheckFileChain
**************************************************************************
** Goes through a chain of lost clusters and checks wether all cluster
** values are valid. If they are not, the file chain is adjusted.
**************************************************************************/
    
static BOOL CheckFileChain(RDWRHandle handle, CLUSTER firstcluster,
                           unsigned long* filesize)
{
    CLUSTER current, previous1 = firstcluster, previous2;
    unsigned long numclusters = 1, labelsinfat;
    unsigned char sectorspercluster;

    labelsinfat = GetLabelsInFat(handle);
    if (!labelsinfat) return FALSE;

    if (!GetNthCluster(handle, firstcluster, &current))
       return FALSE;

    while (FAT_NORMAL(current) && IsLabelValid(handle, current))
    {
       previous2 = previous1;     
       previous1 = current;
       
       if (!GetNthCluster(handle, current, &current))
          return FALSE;

       numclusters++;
    }

    if (FAT_BAD(current) || FAT_FREE(current) || 
        !IsLabelValid(handle, current))
    {           
       if (!WriteFatLabel(handle, previous2, FAT_LAST_LABEL))
          return FALSE;
                        
       numclusters --;  
    }

    /* Return the size of the file */
    sectorspercluster = GetSectorsPerCluster(handle);
    if (!sectorspercluster) return FALSE;
    
    *filesize = numclusters * sectorspercluster * BYTESPERSECTOR;

    return TRUE;
}

/*************************************************************************
**                           MakeUpFileName
**************************************************************************
** Makes up a file name for a lost cluster chain.
**************************************************************************/

static BOOL MakeUpFileName(RDWRHandle handle,
                           char* filename, char* extension)
{
    int counter=0;
    char buffer[9];
    BOOL retval;

    do {
        sprintf(buffer, "FILE%04d", counter++);
        memcpy(filename, buffer, 8);
        memcpy(extension, "CHK", 3);
        
        retval = LoFileNameExists(handle, 0, filename, extension);
        
        if (retval == FAIL)
           return FALSE;
        
    } while (retval);

    return TRUE;
}

/*================================== Common ============================*/

/*************************************************************************
**                                MarkAllClusters
**************************************************************************
** Searches trough the FAT and finds out wether there are any lost clusters.
**
** It does this by taking a bit field long enough to contain all the clusters
** in the volume. Then it goes through the FAT twice:
**
** - the first time all clusters that are marked as used are marked (put to 1).
** - then all the clusters that are refered are marked (put to 0).
**
** After this if any of the clusters are marked as used then it is a lost 
** cluster. 
**
** Returns:
**      TRUE  : lost clusters found
**      FALSE : no lost clusters found
**      FAIL  : media error
**************************************************************************/
static BOOL MarkAllClusters(RDWRHandle handle, 
                            char* fatmap, unsigned long clustersindataarea)
{
   char* p;
   char message[80];
   unsigned long i, clustercount = 0, chains=0;
   
   /* Do the marking. */
   if (!MarkClusters(handle, fatmap))
   {
      DestroyBitfield(fatmap);
      return FAIL;
   }

   /* Now all the clusters are marked, see wether there are still 
      clusters set to 1. */
   /* Count the number of chains */
   for (i = 0; i < clustersindataarea; i++)
   {
       if (GetBitfieldBit(fatmap, i)) /* Lost cluster found */
       {              
          chains++;            
       }
   }
   
   /* Count the number of clusters in the chains */
   for (i = 0; i < clustersindataarea; i++)
   {
       if (GetBitfieldBit(fatmap, i)) /* Lost cluster found */
       {          
          if (!FinaliseFatMap(handle, fatmap, i))
          {
             return FAIL;
          }
       }
   }
   
   for (i = 0; i < clustersindataarea; i++)
   {
       if (GetBitfieldBit(fatmap, i)) /* Lost cluster found */
       {
          clustercount++;            
       }
   }   
   
   /* If lost clusters found give a message. */
   if (clustercount)
   {
     sprintf(message,
             "Found %lu lost clusters in %lu chains\n", 
             clustercount, chains);
              
      ReportFileSysError(message, 0, &p, 0, FALSE);
   }
   
   return clustercount > 0;
}

/*************************************************************************
**                                MarkClusters
**************************************************************************
** Searches trough the FAT and marks all the clusters in the bitfield.
**
** - the first time all clusters that are marked as used are marked (put to 1).
** - then all the clusters that are refered are marked (put to 0).
**************************************************************************/

static BOOL MarkClusters(RDWRHandle handle, char* fatmap)
{
   /* Mark all the clusters that are used */
   if (!LinearTraverseFat(handle, UsedMarker, (void**) &fatmap))
      return FALSE;
      
   /* Mark all the clusters that are refered */
   /* All the references in the FAT */
   if (!LinearTraverseFat(handle, ReferedMarker, (void**) &fatmap))
      return FALSE;

   /* and all the references in the directory entries */   
   if (!FastWalkDirectoryTree(handle, FirstClusterMarker, (void**) &fatmap))
      return FALSE;
      
   return TRUE;
}

/*************************************************************************
**                                UsedMarker
**************************************************************************
** If the given the cluster is used the given cluster is marked as used in
** the bit field.
**************************************************************************/

static BOOL UsedMarker(RDWRHandle handle, CLUSTER label,
                       SECTOR datasector, void** structure)
{
   char* fatmap = *((char**) structure);
   CLUSTER cluster;

   if (FAT_NORMAL(label) || FAT_LAST(label))
   {
      cluster = DataSectorToCluster(handle, datasector);
      if (!cluster) return FALSE;
   
      SetBitfieldBit(fatmap, cluster-2);
   }
   
   return TRUE;
}

/*************************************************************************
**                             ReferedMarker
**************************************************************************
** If the given the cluster contains a reference to an other cluster then 
** that the refered cluster is marked as refered to.
**************************************************************************/

static BOOL ReferedMarker(RDWRHandle handle, CLUSTER label,
                          SECTOR datasector, void** structure)
{
   char* fatmap = *((char**) structure);
   CLUSTER cluster;

   if (FAT_NORMAL(label) && IsLabelValid(handle, label))
   {
      cluster = DataSectorToCluster(handle, datasector);
      if (!cluster) return FALSE;
   
      ClearBitfieldBit(fatmap, label-2);
   }
   
   return TRUE;
}

/*************************************************************************
**                             FirstClusterMarker
**************************************************************************
** If the given the directory entry contains a reference to a cluster then 
** the refered cluster is marked as refered to.
**************************************************************************/

static BOOL FirstClusterMarker(RDWRHandle handle,
                               struct DirectoryPosition* pos,
                               struct DirectoryEntry* entry,
                               void** structure)
{
   CLUSTER firstcluster;
   char* fatmap = *((char**) structure);

   pos = pos, handle = handle;

   if ((entry->attribute & FA_LABEL) ||
       (IsLFNEntry(entry))           ||
       (IsCurrentDir(*entry))        ||
       (IsPreviousDir(*entry))       ||
       (IsDeletedLabel(*entry)))
   {
      return TRUE;
   }

   firstcluster = GetFirstCluster(entry);
   if (firstcluster && FAT_NORMAL(firstcluster) && 
       IsLabelValid(handle, firstcluster))
   {
      ClearBitfieldBit(fatmap, firstcluster-2);
   }

   return TRUE;
}

/*************************************************************************
**                             IsStartOfChain
**************************************************************************
** Looks wether the given cluster is the first one in a lost cluster chain.
**************************************************************************/

static BOOL IsStartOfChain(RDWRHandle handle, CLUSTER cluster)
{
    CLUSTER found;
    
    /* See wether this cluster is refered to in the FAT */
    if (!FindClusterInFAT(handle, cluster, &found))
       return FAIL;

    /* If it is refered to in the FAT, it is not the start of a lost cluster chain. */
    return (found) ? FALSE : TRUE;
}

/*************************************************************************
**                             FinaliseFatMap
**************************************************************************
** Looks wether the given cluster is the first one in a lost cluster chain.
**************************************************************************/
static BOOL FinaliseFatMap(RDWRHandle handle, char* fatmap, 
                           unsigned long bit)
{
    CLUSTER cluster = bit+2;
        
    do {
         if (!GetNthCluster(handle, cluster, &cluster))
            return FALSE;     
            
         if (FAT_BAD(cluster))   
            ClearBitfieldBit(fatmap, bit);   
         else   
            SetBitfieldBit(fatmap, bit);   
            
         bit = cluster - 2;   
            
    } while (FAT_NORMAL(cluster) && IsLabelValid(handle, cluster) &&
             !GetBitfieldBit(fatmap, bit));
    
    return TRUE;
}