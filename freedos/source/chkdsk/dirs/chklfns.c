/*
   ChkLFNs.c - LFN checking.
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
#include "..\errmsgs\PrintBuf.h"
#include "..\misc\useful.h"
#include "dirschk.h"

/*
** We use a state machine. Every directory entry is scanned and the state
** machine is updated accordingly.
*/

/* The state can be: */
#define SCANNING_SFN_STATE 1    /* A short file name is just proccessed. */
#define SCANNING_LFN_STATE 2    /* A long file name is just processed. */

static int LFNState;            /* The state of the state machine. */
static int LFNIndex;            /* The index in the series of LFN entries,
                                   belonging to an SFN. */
static unsigned char LFNChkSum; /* The checksum of the SFN (repeated in every
                                   LFN belonging to that SFN). */ 
static struct DirectoryPosition LFNStart; /* The first of a series of LFN
                                             entries found.               */  
static struct DirectoryPosition PrevPos; /* The previous position */
                                             
static CLUSTER LFNCluster;        

struct RemovePipe
{
   struct DirectoryPosition* begin;
   struct DirectoryPosition* end;

   BOOL hasstarted;
};

static BOOL RemoveInvalidLFNs(RDWRHandle handle, CLUSTER firstcluster,
                              struct DirectoryPosition* pos1,
                              struct DirectoryPosition* pos2);
static BOOL LFNRemover(RDWRHandle handle, struct DirectoryPosition* pos,
		       void** structure);


BOOL InitLFNChecking(RDWRHandle handle, CLUSTER cluster, char* filename)
{
     UNUSED(handle);
     UNUSED(cluster);
     UNUSED(filename);
    
     return TRUE;
}

BOOL PreProcesLFNChecking(RDWRHandle handle, CLUSTER cluster, char* filename, BOOL fixit)
{
    UNUSED(handle);
    UNUSED(fixit);
    UNUSED(filename);
    
    LFNCluster = cluster;
    LFNState   = SCANNING_SFN_STATE;
    LFNIndex   = 0;
    
    return TRUE;
}

BOOL CheckLFNInDirectory(RDWRHandle handle,                   
                         struct DirectoryPosition* pos, 
                         struct DirectoryEntry* entry, 
                         char* filename,
                         BOOL fixit)
{
    BOOL retVal = TRUE;
    unsigned char ChkSum;
    struct LongFileNameEntry* lfnentry = (struct LongFileNameEntry*) entry;

    if (IsDeletedLabel(*entry))
    {        
       /* Check for the most common of LFN faults, deleting a file in plain DOS.
          I.e. the SFN is deleted, but the associated LFN entries not.  */       
       if (LFNState == SCANNING_LFN_STATE)
       {     
          ShowParentViolation(handle, filename,  
                              "%s contains a number of invalid long file name entries");            
          if (fixit)
          {
             if (!RemoveInvalidLFNs(handle, LFNCluster, &LFNStart, &PrevPos))
                return FAIL;
          } 
          LFNState = SCANNING_SFN_STATE; /* Reset the state machine */
          return FALSE;
       }
       
       return TRUE;
    }
       
    if ((entry->attribute & FA_LABEL) && 
        (!IsLFNEntry(entry)))
       return TRUE;

    if (!IsLFNEntry(entry)) /* A short filename entry */
    {            
       if (LFNState == SCANNING_LFN_STATE)
       {
          /* Calculate the checksum for this SFN entry */
          ChkSum = CalculateSFNCheckSum(entry);
          
          if ((LFNIndex != 0) ||
              (ChkSum != LFNChkSum))
          {
             ShowParentViolation(handle, filename,  
                                 "%s contains a number of invalid long file name entries");                                             
             if (fixit)
             {
                if (!RemoveInvalidLFNs(handle, LFNCluster, &LFNStart, &PrevPos))
                   return FAIL;
             }
             retVal = FALSE;
          }
          LFNState = SCANNING_SFN_STATE;
       }
    }
    else if (IsFirstLFNEntry(lfnentry)) /* First LFN entry */
    {    
       if (LFNState == SCANNING_LFN_STATE)
       {
          ShowParentViolation(handle, filename,  
                              "%s contains a number of invalid long file name entries");                                         
          if (fixit)
          {
             if (!RemoveInvalidLFNs(handle, LFNCluster, &LFNStart, &PrevPos))
                return FAIL;
          }
          retVal = FALSE;       
       }

       /* Even if the previous entry was wrong, the new one may be correct */
       memcpy(&LFNStart, pos, sizeof(struct DirectoryPosition));
       LFNState  = SCANNING_LFN_STATE;
       LFNIndex  = GetNrOfLFNEntries(lfnentry)-1;
       LFNChkSum = lfnentry->checksum;
    }
    else /* LFN entry in the middle. */
    {                       
       if (LFNState == SCANNING_SFN_STATE)
       {
          memcpy(&LFNStart, pos, sizeof(struct DirectoryPosition));
       }
            
       if ((LFNState == SCANNING_SFN_STATE)  ||
           (lfnentry->NameIndex != LFNIndex) ||
           (LFNIndex == 0)                   ||
           (lfnentry->checksum != LFNChkSum))
       {
          ShowParentViolation(handle, filename,  
                              "%s contains a number of invalid long file name entries");
            
          if (fixit)
          {
             if (!RemoveInvalidLFNs(handle, LFNCluster, &LFNStart, pos))
                return FAIL;
          }
          retVal = FALSE;
       }
       else
       {
	  LFNIndex--;
       }
    }
    
    memcpy(&PrevPos, pos, sizeof(struct DirectoryPosition));
    return retVal;
}

BOOL PostProcesLFNChecking(RDWRHandle handle, CLUSTER cluster, char* filename, BOOL fixit)
{
    /* After cheking the directory the state machine must be in SFN state. */   
    if ((LFNState != SCANNING_SFN_STATE) ||
        (LFNIndex > 0))/* And the right number of LFN entries must be found. */
    {
       ShowFileNameViolation(handle, filename,  
                             "%s contains a number of invalid long file name entries");       
                           
       if (fixit)
       {
          if (!RemoveInvalidLFNs(handle, cluster, &LFNStart, NULL))
             return FAIL;
       }
       
       return FALSE;
    }   
   
    return TRUE;
}

void DestroyLFNChecking(RDWRHandle handle, CLUSTER cluster, char* filename)
{
     UNUSED(filename);   
     UNUSED(handle);
     UNUSED(cluster);
}

/*************************************************************************
**                          RemoveInvalidLFNs
**************************************************************************
**  Removes all the LFN entries between pos1 and pos2 in the directory 
**  pointed to by firstcluster.
**
**  If pos2 == 0, all the long file name entries starting at pos1 are
**  changed.
**************************************************************************/

BOOL RemoveInvalidLFNs(RDWRHandle handle, CLUSTER firstcluster,
                       struct DirectoryPosition* pos1,
                       struct DirectoryPosition* pos2)
{
    struct RemovePipe pipe, *ppipe = &pipe;

    pipe.begin = pos1;
    pipe.end   = pos2;

    return TraverseSubdir(handle, firstcluster, LFNRemover, (void**) &ppipe,
                          TRUE);
}

/*************************************************************************
**                                LFNRemover
**************************************************************************
**  Removes all the LFN entries between pos1 and pos2 in the directory 
**  pointed to by firstcluster.
**
**  If pos2 == 0, all the long file name entries starting at pos1 are
**  changed.
**************************************************************************/

static BOOL LFNRemover(RDWRHandle handle, struct DirectoryPosition* pos,
                       void** structure)
{
    struct RemovePipe* pipe = *((struct RemovePipe**) structure);
    struct DirectoryEntry entry;

    if (!GetDirectory(handle, pos, &entry))
       return FAIL;

    /* Check the range in which we have to delete the long file name entries */
    if (!pipe->hasstarted) /* Outside the interval */
    {  /* Check wether the starting position was found. */
       if (memcmp(pos, pipe->begin, sizeof(struct DirectoryPosition)) == 0)
          pipe->hasstarted = TRUE;
    }
    
    if (!IsLFNEntry(&entry))
       return TRUE;
       
    if (pipe->hasstarted)   /* Inside the interval, first create a dummy
                               file name entry. */
    {
       struct tm* tmp;
       time_t now;
   
       /* file name and extension, 0xe5 indicates file removed.    */
       memcpy(entry.filename, "å" "LFNRMVD", 8);
       memset(entry.extension, ' ', 3);

       /* attribute */
       entry.attribute = 0;

       /* first cluster */
       SetFirstCluster(2, &entry);

       /* file size */
       entry.filesize = 0;

       /* NT reserved field */
       entry.NTReserved = 0;

       /* Mili second stamp */
       entry.MilisecondStamp = 0;

       /* Last access date */
       memset(&entry.LastAccessDate, 0, sizeof(struct PackedDate));
    
       /* Time last modified */
       memset(&entry.timestamp, 0, sizeof(struct PackedTime));

       /* Date last modified */
       memset(&entry.datestamp, 0, sizeof(struct PackedDate));

       /* Get the current date and time and store it in the last write
          time and date. */
       time(&now);
       tmp = localtime(&now);

       entry.LastWriteTime.second = tmp->tm_sec / 2;
       if (entry.LastWriteTime.second == 30) /* DJGPP help says range is [0..60] */
          entry.LastWriteTime.second--;
    
       entry.LastWriteTime.minute = tmp->tm_min;
       entry.LastWriteTime.hours  = tmp->tm_hour;

       entry.LastWriteDate.day   = tmp->tm_mday;
       entry.LastWriteDate.month = tmp->tm_mon + 1;

       if (tmp->tm_year < 80)
          entry.LastWriteDate.year = 0;
       else
          entry.LastWriteDate.year = (tmp->tm_year+1900)-1980;

       if (!WriteDirectory(handle, pos, &entry))
          return FAIL;
          
       /* Check wether the ending position was found. */
       if ((pipe->end != NULL) &&
           (memcmp(pos, pipe->end, sizeof(struct DirectoryPosition)) == 0))
       {
          return FALSE;       
       }           
    }    
    
    return TRUE;
}