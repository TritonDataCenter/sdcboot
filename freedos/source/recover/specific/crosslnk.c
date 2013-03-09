/*
   Crosslnk.c - cross linked cluster checking.
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
/*
   ONLY SUITABLE FOR RECOVER! 
*/


#include <stdio.h>
#include <stdlib.h>

#include "fte.h"
#include "intelwlk.h"
#include "useful.h"

struct Pipe
{
   char* bitfield;
   BOOL  invalid;
};

struct FileTraceInformation
{
    CLUSTER cluster;
    BOOL    ReferedInFat;    
        
    struct FileTraceInformation* next;
};

struct ExplorerPipe
{ 
    CLUSTER cluster;
    struct FileTraceInformation** list;
    
    BOOL found;
};

struct FilePrintPipe
{
   int listcount;
   struct FileTraceInformation* list;
};


/*
** We use a bitfield long enough to contain all the clusters in the FAT.
** We traverse over the FAT and over all the directory entries while marking
** all the clusters that are refered to. If, while doing this, the cluster
** is already marked, we have a cross linked cluster.
*/

static BOOL CrossLinkFinder(RDWRHandle handle, CLUSTER label,
                            SECTOR datasector, void** structure);
                            
static BOOL SearchCrossLinkedFiles(RDWRHandle handle);

static BOOL ExploreClusterReferences(RDWRHandle handle, 
                                     CLUSTER cluster,
                                     struct FileTraceInformation** References);
                                     
static BOOL AdjustCrossLinkedFiles(RDWRHandle handle, CLUSTER cluster);  

static BOOL ClusterExplorer(RDWRHandle handle, CLUSTER label,
                            SECTOR datasector, void** structure);    

void DestroyClusterReferenceList(struct FileTraceInformation* References);   

static BOOL FirstClustersAdjustDirectories(RDWRHandle handle,
                                           struct FileTraceInformation* References,
                                           int listcount);     
                                          
static BOOL FirstClusterAdjuster(RDWRHandle handle, 
                                 struct DirectoryPosition* pos,
                                 struct DirectoryEntry* entry,
                                 char* filename,
                                 void** structure);                                          


BOOL AdjustFileSize(RDWRHandle handle, struct DirectoryPosition* pos,
                    struct DirectoryEntry* entry);				 
				 
#define SUCCESS 0
#define FAILED  1
#define ERROR   2 
				
				
/*************************************************************************
**                       TruncateCrossLinkedFiles
**************************************************************************
** Searches trough the FAT and finds out wether there are any cross
** linked clusters. If there are any they are split.
**************************************************************************/  
BOOL TruncateCrossLinkedFiles(RDWRHandle handle)
{
   return (SearchCrossLinkedFiles(handle) != ERROR) ?
          TRUE : FALSE; 
}

/*************************************************************************
**                           SearchCrossLinkedFiles
**************************************************************************
** Searches trough the FAT and finds out wether there are any cross
** linked clusters. If there are any they are optionally split.
**************************************************************************/  
static BOOL SearchCrossLinkedFiles(RDWRHandle handle)
{
   unsigned long clustersindataarea;
   struct Pipe pipe, *ppipe = &pipe;

   /* Create a bitfield large enough to mark all the clusters in the FAT. */
   clustersindataarea = GetClustersInDataArea(handle);
   if (!clustersindataarea) return ERROR;

   pipe.bitfield = CreateBitField(clustersindataarea);
   if (!pipe.bitfield) return ERROR;

   pipe.invalid     = FALSE;

   /* Go through the FAT and mark all the clusters that are refered to. */
   if (!LinearTraverseFat(handle, CrossLinkFinder, (void**) &ppipe))
   {
      DestroyBitfield(pipe.bitfield);
      return ERROR;
   }

   DestroyBitfield(pipe.bitfield);
   return (pipe.invalid) ? FAILED : SUCCESS;
}

/*************************************************************************
**                             CrossLinkFinder
**************************************************************************
** Takes a certain FAT label and if it is a normal one it tries to mark the
** cluster it points. 
**************************************************************************/
static BOOL CrossLinkFinder(RDWRHandle handle, CLUSTER label,
                            SECTOR datasector, void** structure)
{
   CLUSTER cluster;
   unsigned long bit;
   struct Pipe* pipe = *((struct Pipe**) structure);

   datasector = datasector, handle = handle;

   if (FAT_NORMAL(label) && IsLabelValid(handle, label))
   {
      bit = label-2;
      if (GetBitfieldBit(pipe->bitfield, bit)) /* Cluster already refered
                                                  somewhere else.         */
      {
         pipe->invalid = TRUE;
         
         cluster = DataSectorToCluster(handle, datasector);

	 /* Truncate file chain */ 
         if (!WriteFatLabel(handle, cluster, FAT_LAST_LABEL))
            return FAIL;	     
	    
         if (!AdjustCrossLinkedFiles(handle, label))
            return FAIL;
	 
         if (!AdjustCrossLinkedFiles(handle, cluster))
            return FAIL;
	 
      }

      SetBitfieldBit(pipe->bitfield, bit); /* Mark it as being refered to. */
   }

   return TRUE;
}

/*************************************************************************
**                           AdjustCrossLinkedFileNames
**************************************************************************
** This function prints a list of file names that are cross linked to a
** certain cluster
**************************************************************************/
static BOOL AdjustCrossLinkedFiles(RDWRHandle handle, CLUSTER cluster)
{
    BOOL result;
    int listcount=0;
    CLUSTER temp;
    struct FileTraceInformation* References = NULL, *current;

    /* First check wether the given cluster is refered to in the FAT, if not 
       assume that this is the only cluster in the file (that just got truncated). */
    if (!FindClusterInFAT(handle, cluster, &temp))
	return FALSE;
   
    if (!temp)
    {
        References = (struct FileTraceInformation*) malloc(sizeof(struct FileTraceInformation));
	
	References->cluster = cluster;
        References->ReferedInFat = FALSE;
	References->next = NULL;
	
	result = FirstClustersAdjustDirectories(handle, 
						References,
						1);
                                           
	DestroyClusterReferenceList(References);		
    
	return result;                                           
    }
        
    /* Construct the list of first to use in printing out the file names */
    do 
    {
       if (!ExploreClusterReferences(handle, cluster, &References))
       {
          DestroyClusterReferenceList(References);
          return FALSE;
       }
    
       /* Try finding a cluster that is marked as Refered in the FAT */
       current = References;
       temp = 0;
       while (current)
       {
          if (current->ReferedInFat)
          {        
             temp = current->cluster;
             break;
          }   
          current = current->next;
       }
    
       cluster = temp;
       
    } while (temp); /* All files traced back to their first cluster */
    
    /* Count the number of elements in the list */
    current = References;
    while (current)
    {  
       listcount++;
       current = current->next;
    }
    
    /* Adjust out the file names */
    result = FirstClustersAdjustDirectories(handle, 
                                            References,
                                            listcount);
                                           
    DestroyClusterReferenceList(References);
    
    return result;                                           
}

/*************************************************************************
**                           ExploreClusterReferences
**************************************************************************
** This function takes a cluster and goes through the FAT to add every
** cluster to the list of references, then it removes the given cluster
** from the list. If the cluster is not refered to then this is indicated
** in the structure for this cluster.
**************************************************************************/
static BOOL ExploreClusterReferences(RDWRHandle handle, 
                                     CLUSTER cluster,
                                     struct FileTraceInformation** References)
{
     struct ExplorerPipe pipe, *ppipe = &pipe;
     struct FileTraceInformation* ref, *prev;   

     /* Add all references to the list */
     pipe.found   = FALSE;
     pipe.list    = References;
     pipe.cluster = cluster;
     if (!LinearTraverseFat(handle, ClusterExplorer, (void**) &ppipe))
        return FALSE;
     
     /* See wether the given cluster is in the list */
     ref = *References;
     prev = NULL;
     while (ref)
     {
        if (ref->cluster == cluster)
        {
           if (pipe.found) /* If this cluster is not the first cluster of a file */
           {
              /* if it is remove it */     
              if (prev)
              {
                 prev->next = ref->next;
              }
              else
              {
                 *References = ref->next;   
              }
              free(ref);
           }
           else            /* Indicate it as a first cluster of some file */
           {
              ref->ReferedInFat = FALSE;
           }
        }
        
        prev = ref;
        ref = ref->next;
     }
        
     return TRUE;
}

static BOOL ClusterExplorer(RDWRHandle handle, CLUSTER label,
                            SECTOR datasector, void** structure)
{
    struct ExplorerPipe* pipe = *((struct ExplorerPipe**) structure);   
    CLUSTER cluster = pipe->cluster, thisCluster;
    struct FileTraceInformation** list = pipe->list;
    struct FileTraceInformation* ref;

    UNUSED(handle);
    UNUSED(datasector);
           
    /* If this label referes to the given cluster */
    if (cluster == label)
    {
       thisCluster = DataSectorToCluster(handle, datasector);
       if (!thisCluster) return FAIL;
      
       /* Create a new element */
       ref = (struct FileTraceInformation*) malloc(sizeof(struct FileTraceInformation));
       if (!ref) return FAIL;
   
       ref->cluster = thisCluster;
       ref->ReferedInFat = TRUE;
       
       /* Add it to the list */
       ref->next = *list;
       *list = ref;
       
       pipe->found = TRUE;
    }

    return TRUE;
}

/*************************************************************************
**                          DestroyClusterReferenceList
**************************************************************************
** Removes all the elements from the list of cluster references
**************************************************************************/

void DestroyClusterReferenceList(struct FileTraceInformation* References)
{
   struct FileTraceInformation* next;
   
   while (References)
   {
      next = References->next;
      free(References);
      References = next;
   }
}

/*************************************************************************
**                          FirstClustersAdjustDirectories
**************************************************************************
** Goes through all directories and if a file is found for which the 
** first cluster is in the list, then the corresponding file name is 
** printed out.
**************************************************************************/
static BOOL FirstClustersAdjustDirectories(RDWRHandle handle,
                                          struct FileTraceInformation* References,
                                          int listcount)
{
   BOOL result;
   struct FilePrintPipe pipe, *ppipe = &pipe;
   
   pipe.list = References;
   pipe.listcount = listcount;
   
   result = IntelligentWalkDirectoryTree(handle, FirstClusterAdjuster, (void**) &ppipe);
    
   return result;
}

static BOOL FirstClusterAdjuster(RDWRHandle handle, 
                                 struct DirectoryPosition* pos,
                                 struct DirectoryEntry* entry,
                                 char* filename,
                                 void** structure)
{
   struct FilePrintPipe *pipe = *((struct FilePrintPipe**) structure);
   struct FileTraceInformation* ref;
   CLUSTER firstcluster;
   
   UNUSED(pos);
   UNUSED(handle);
   
   if ((entry->attribute & FA_LABEL) ||
       IsLFNEntry(entry)             ||
       IsPreviousDir(*entry)         ||
       IsCurrentDir(*entry)          ||
       IsDeletedLabel(*entry))
   {
      return TRUE;
   }
   
   firstcluster = GetFirstCluster(entry);
   
   ref = pipe->list;
   while (ref)
   {
      if (ref->cluster == firstcluster)
      {  
         printf("%s is cross linked\n", filename);      
	 
	 if (!AdjustFileSize(handle, pos, entry))
	     return FAIL;
	  
         break;
      }
      
      pipe->listcount--;   
      ref = ref->next;
   }
   
   return TRUE;
}
                                           
/*************************************************************************
**                          AdjustFileSize
**************************************************************************
** Sets the correct file size in the entry.
**************************************************************************/

BOOL AdjustFileSize(RDWRHandle handle, struct DirectoryPosition* pos,
                    struct DirectoryEntry* entry)
{
    CLUSTER cluster;
    unsigned long length=0, sectorspercluster;
    
    sectorspercluster = GetSectorsPerCluster(handle);
    if (!sectorspercluster) return FALSE;

    cluster = GetFirstCluster(entry);
    while ((cluster != 0) && FAT_NORMAL(cluster))
    {
	if (!GetNthCluster(handle, cluster, &cluster))
	    return FALSE;
	length++;
    }
    
    entry->filesize = sectorspercluster * BYTESPERSECTOR * length;
    
    if (!WriteDirectory(handle, pos, entry))
	return FALSE;
    
    return TRUE;
}			
