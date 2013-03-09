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

#include <stdio.h>
#include <stdlib.h>

#include "fte.h"
#include "..\chkdrvr.h"
#include "..\struct\FstTrMap.h"
#include "..\struct\intelwlk.h"
#include "..\errmsgs\errmsgs.h"
#include "..\misc\useful.h"

struct Pipe
{
   char* bitfield;
   BOOL  invalid;
   BOOL  fixit;
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
                            
static BOOL FirstClusterChecker(RDWRHandle handle,
                                struct DirectoryPosition* pos,
                                struct DirectoryEntry* entry,
                                void** structure);

static BOOL CopyCompleteFileChain(RDWRHandle handle,
                                  struct DirectoryPosition* pos);

static BOOL CloneFileChain(RDWRHandle handle,
                           CLUSTER clusterbeforecross);

static BOOL CopyFileChain(RDWRHandle handle, CLUSTER source, CLUSTER target);

static BOOL SearchCrossLinkedFiles(RDWRHandle handle, BOOL fixit);

static BOOL ExploreClusterReferences(RDWRHandle handle, 
                                     CLUSTER cluster,
                                     struct FileTraceInformation** References);
                                     
static BOOL PrintCrossLinkedFileNames(RDWRHandle handle, CLUSTER cluster);  

static BOOL ClusterExplorer(RDWRHandle handle, CLUSTER label,
                            SECTOR datasector, void** structure);    

void DestroyClusterReferenceList(struct FileTraceInformation* References);   

static BOOL FirstClustersPrintDirectories(RDWRHandle handle,
                                          struct FileTraceInformation* References,
                                          int listcount);     
                                          
static BOOL FirstClusterPrinter(RDWRHandle handle, 
                                struct DirectoryPosition* pos,
                                struct DirectoryEntry* entry,
                                char* filename,
                                void** structure);                                          

/*============================== Checking ==============================*/

/*************************************************************************
**                           FindCrossLinkedFiles
**************************************************************************
** Searches trough the FAT and finds out wether there are any cross
** linked clusters.
**************************************************************************/                                 
RETVAL FindCrossLinkedFiles(RDWRHandle handle)
{
   return SearchCrossLinkedFiles(handle, FALSE);
}

/*============================== Fixing ==============================*/

/*************************************************************************
**                           SplitCrossLinkedFiles
**************************************************************************
** Searches trough the FAT and finds out wether there are any cross
** linked clusters. If there are any they are split.
**************************************************************************/  
RETVAL SplitCrossLinkedFiles(RDWRHandle handle)
{
   return (SearchCrossLinkedFiles(handle, TRUE) != ERROR) ?
          SUCCESS : ERROR; 
}

/*************************************************************************
**                           CopyCompleteFileChain
**************************************************************************
** Say the first cluster of two files point to the same physical file on 
** disk, then this function will create a new file and copy all the data
** from the common file chain.
**************************************************************************/ 

static BOOL CopyCompleteFileChain(RDWRHandle handle,
                                  struct DirectoryPosition* pos)
{
   CLUSTER newcluster, firstcluster;
   struct DirectoryEntry entry;

   if (!GetDirectory(handle, pos, &entry))
      return FALSE;

   firstcluster = GetFirstCluster(&entry);
   if (!firstcluster) return TRUE;

   switch (CreateFileChain(handle, pos, &newcluster))
   {
      case FALSE:
           printf("Insufficient disk space to undouble cross linked clusters\n");
           return TRUE;
           
      case FAIL:
           return FAIL;
   }

   return CopyFileChain(handle, firstcluster, newcluster);
}

/*************************************************************************
**                           CopyCompleteFileChain
**************************************************************************
** Say two files have an intersection at a certain point. Then this function
** creates a new file chain by copying all the data from the common file
** chain to the new chain.
**************************************************************************/ 

static BOOL CloneFileChain(RDWRHandle handle,
                           CLUSTER clusterbeforecross)
{
   CLUSTER current;

   /* Get the cluster that contains the cross. */
   if (!GetNthCluster(handle, clusterbeforecross, &current))
      return FALSE;

   /* Make a new chain, by first breaking the file right before the
      intersection point. */
   if (!WriteFatLabel(handle, clusterbeforecross, FAT_LAST_LABEL))
      return FALSE;

   return CopyFileChain(handle, current, clusterbeforecross);
}

/*************************************************************************
**                               CopyFileChain
**************************************************************************
** Copies the remaining of the file chain addressed by source, extending
** the target before copying each cluster.
**************************************************************************/ 

static BOOL CopyFileChain(RDWRHandle handle, CLUSTER source, CLUSTER target)
{
   CLUSTER current = source, newcluster;
   SECTOR srcsect, dstsect;
   unsigned char sectorspercluster;

   sectorspercluster = GetSectorsPerCluster(handle);
   if (!sectorspercluster) return FALSE;

   while (FAT_NORMAL(current))
   {              
       /* Reserve a new cluster */
       switch (ExtendFileChain(handle, target, &newcluster))
       {
         case FAIL:
              return FALSE;
         case FALSE:
              ReportGeneralRemark("Insufficient disk space to undouble cross linked clusters\n");
              return TRUE;
       }

       /* Copy the contents of the source cluster */
       srcsect = ConvertToDataSector(handle, current);
       dstsect = ConvertToDataSector(handle, newcluster);

       if (!CopySectors(handle, srcsect, dstsect, sectorspercluster))
          return FAIL;

       /* Get the next cluster in the original file chain */
       if (!GetNthCluster(handle, current, &current))
       {           
          return FALSE;
       }
   }

   return TRUE;
}

/*============================== Common ==============================*/

/*************************************************************************
**                           SearchCrossLinkedFiles
**************************************************************************
** Searches trough the FAT and finds out wether there are any cross
** linked clusters. If there are any they are optionally split.
**************************************************************************/  
static BOOL SearchCrossLinkedFiles(RDWRHandle handle, BOOL fixit)
{
   unsigned long clustersindataarea;
   struct Pipe pipe, *ppipe = &pipe;

   /* Create a bitfield large enough to mark all the clusters in the FAT. */
   clustersindataarea = GetClustersInDataArea(handle);
   if (!clustersindataarea) return ERROR;

   pipe.bitfield = CreateBitField(clustersindataarea);
   if (!pipe.bitfield) return ERROR;

   pipe.invalid = FALSE;
   pipe.fixit   = fixit;

   /* Go through the FAT and mark all the clusters that are refered to. */
   if (!LinearTraverseFat(handle, CrossLinkFinder, (void**) &ppipe))
   {
      DestroyBitfield(pipe.bitfield);
      return ERROR;
   }

   /* Go through the directory entries and mark all the clusters that are 
      refered to. */
   if (!FastWalkDirectoryTree(handle, FirstClusterChecker, (void**) &ppipe))
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
         if (!PrintCrossLinkedFileNames(handle, label))
            return FAIL;
            
         pipe->invalid = TRUE;
         
         if (pipe->fixit)
         {
            cluster = DataSectorToCluster(handle, datasector);
            if (!CloneFileChain(handle, cluster))                
               return FAIL;              
         }
      }

      SetBitfieldBit(pipe->bitfield, bit); /* Mark it as being refered to. */
   }

   return TRUE;
}

/*************************************************************************
**                             FirstClusterChecker
**************************************************************************
** Takes a certain directory entry and tries to mark the cluster it points
** to.
**************************************************************************/

static BOOL FirstClusterChecker(RDWRHandle handle,
                                struct DirectoryPosition* pos,
                                struct DirectoryEntry* entry,
                                void** structure)
{
   CLUSTER firstcluster;
   unsigned long bit;
   struct Pipe* pipe = *((struct Pipe**) structure);

   pos = pos, handle = handle;

   if ((entry->attribute & FA_LABEL) ||
       IsLFNEntry(entry)             ||
       IsPreviousDir(*entry)         ||
       IsCurrentDir(*entry)          ||
       IsDeletedLabel(*entry))
   {
      return TRUE;
   }

   firstcluster = GetFirstCluster(entry);
   if ((firstcluster) && FAT_NORMAL(firstcluster) && 
       IsLabelValid(handle, firstcluster))
   {
      bit = firstcluster-2;
      if (GetBitfieldBit(pipe->bitfield, bit))/* Cluster already refered
                                                 somewhere else.         */
      {
         if (!PrintCrossLinkedFileNames(handle, firstcluster))
            return FAIL;
                 
         pipe->invalid = TRUE;
         
         if (pipe->fixit)
         {
            if (CopyCompleteFileChain(handle, pos) == FAIL)
               return FAIL;
         }
      }

      SetBitfieldBit(pipe->bitfield, bit); /* Mark it as being refered to. */
   }

   return TRUE;
}

/*************************************************************************
**                           PrintCrossLinkedFileNames
**************************************************************************
** This function prints a list of file names that are cross linked to a
** certain cluster
**************************************************************************/
static BOOL PrintCrossLinkedFileNames(RDWRHandle handle, CLUSTER cluster)
{
    BOOL result;
    int listcount=0;
    CLUSTER temp;
    struct FileTraceInformation* References = NULL, *current;

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
    
    /* Print out the file names */
    result = FirstClustersPrintDirectories(handle, 
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
**                          FirstClustersPrintDirectories
**************************************************************************
** Goes through all directories and if a file is found for which the 
** first cluster is in the list, then the corresponding file name is 
** printed out.
**************************************************************************/
static BOOL FirstClustersPrintDirectories(RDWRHandle handle,
                                          struct FileTraceInformation* References,
                                          int listcount)
{
   BOOL result;
   struct FilePrintPipe pipe, *ppipe = &pipe;
   
   pipe.list = References;
   pipe.listcount = listcount;
   
   result = IntelligentWalkDirectoryTree(handle, FirstClusterPrinter, (void**) &ppipe);
    
   return result;
}

static BOOL FirstClusterPrinter(RDWRHandle handle, 
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
#if 0              
         if (pipe->listcount > 2)
         {
            printf("%s, ", filename);
         }
         else if (pipe->listcount == 2)
         {
            printf("%s and ", filename);
         }
         else
         {
            pipe->listcount--;
            printf("%s", filename);  
            return FALSE;
         }
#endif         
         break;
      }
      
      pipe->listcount--;   
      ref = ref->next;
   }
   
   return TRUE;
}
                                           