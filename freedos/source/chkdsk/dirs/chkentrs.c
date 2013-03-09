/*
   ChkEntrs.c - directory entry checking.
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
#include <ctype.h>

#include "fte.h"
#include "..\chkdrvr.h"
#include "..\struct\FstTrMap.h"
#include "..\struct\intelwlk.h"
#include "..\errmsgs\errmsgs.h"
#include "..\errmsgs\PrintBuf.h"
#include "..\chkargs.h"
#include "scnsbdrs.h"
#include "dirschk.h"

static BOOL EntryChecker(RDWRHandle handle, struct DirectoryPosition* pos,
			 struct DirectoryEntry* entry, char* filename,
                         void** structure);
static BOOL EntryFixer(RDWRHandle handle, struct DirectoryPosition* pos,
		       struct DirectoryEntry* entry, char* filename,
                       void** structure);
static BOOL LookAtEntry(RDWRHandle handle, 
                        struct DirectoryPosition* pos,
                        struct DirectoryEntry* entry,
                        char* filename,
			BOOL fixit);
static BOOL CheckDots(RDWRHandle handle, CLUSTER firstcluster, BOOL fixit,
                      char* filename); 

static char tempbuf[255];
                        
/*================================= Checking ==============================*/

/*******************************************************************
**                        CheckDirectoryEntries
********************************************************************
** Checks all the directory entries in the volume on validity.
********************************************************************/

struct Pipe
{
	BOOL invalid;
	BOOL been;
};

RETVAL CheckDirectoryEntries(RDWRHandle handle)
{
	struct Pipe pipe, *ppipe = &pipe;

	pipe.invalid = FALSE;
	pipe.been    = FALSE;

    switch (PerformRootDirChecks(handle, FALSE))
    {
       case FALSE:
            pipe.invalid = TRUE;
            break;
            
       case FAIL:
            return ERROR;
    }
    
    if (!IntelligentWalkDirectoryTree(handle, EntryChecker, (void**) &ppipe))
       return ERROR;
        
    /* Compress the fast tree map */
	if (pipe.been)
	{
		if (!CompressFastTreeMap(handle))
		   return ERROR;
	}
       
    return (pipe.invalid) ? FAILED : SUCCESS;
}

/*******************************************************************
**                          EntryChecker
********************************************************************
** For every directory entry the entry is checked for invalid data.
********************************************************************/

static BOOL EntryChecker(RDWRHandle handle, struct DirectoryPosition* pos,
			 struct DirectoryEntry* entry, char* filename,
                         void** structure)
{
    struct Pipe* pipe = *((struct Pipe**) structure);

	pipe->been = TRUE;
    entry = entry;

    switch (LookAtEntry(handle, pos, entry, filename, FALSE))
    {
      case FALSE:
           pipe->invalid = TRUE;
      case TRUE:
           return TRUE;
      case FAIL:
           return ERROR;
    }

    return FAIL;                         /* Should never be executed. */
}

/*================================= Fixing ==============================*/

/*******************************************************************
**                        CheckDirectoryEntries
********************************************************************
** Checks all the directory entries in the volume on validity. And if 
** they contain invalid data, fills them with default values.
********************************************************************/
RETVAL FixDirectoryEntries(RDWRHandle handle)
{
    switch (PerformRootDirChecks(handle, TRUE))
    {
       case FAIL:
            return ERROR;
    }        
    
    /* Compress the fast tree map */
    if (!CompressFastTreeMap(handle))
       return ERROR;    
        
    return (IntelligentWalkDirectoryTree(handle, EntryFixer, NULL)) ?
           SUCCESS : ERROR;
}

/*******************************************************************
**                          EntryFixer
********************************************************************
** For every directory entry the entry is checked for invalid data. 
** If the entry contains invalid data it is replaced with default data.
********************************************************************/

static BOOL EntryFixer(RDWRHandle handle, struct DirectoryPosition* pos,
		       struct DirectoryEntry* entry, char* filename,
                       void** structure)
{
    structure = structure;

    if (LookAtEntry(handle, pos, entry, filename, TRUE) == FAIL)
       return FAIL;

    return TRUE;
}

/*================================= Common ==============================*/

/*******************************************************************
**                        LookAtEntry
********************************************************************
** Checks a certain entry for validity. The following checks are performed:
**
** - All the '.' and '..' must be directories.
** - Check the file chain for free or bad clusters.
** - The first cluster of an LFN entry must be 0.
** - The characters of a filename must be valid.
** - The attribute must be valid.
** - The time and date must be valid.
** - A directory may not have a length.
** - If this is a directory then check the '.' entry in it.
** - The size of a file must be correctly noted in the entry.
** - The first cluster must be valid
**
** The values in the directory entries are adjusted regardless of the 
** value of fixit. However the entry is only written if fixit is true.
**
** Returns: FALSE - an invalid entry was found
**          TRUE  - an invalid entry was not found
**          FAIL  - media error
********************************************************************/

static BOOL LookAtEntry(RDWRHandle handle, 
                         struct DirectoryPosition* pos,
                         struct DirectoryEntry* entry,
                         char* filename,
                         BOOL fixit)
{
    BOOL invalid = FALSE;
    CLUSTER firstcluster;
    unsigned long labelsinfat;   
    unsigned long bytespercluster;  
        
    labelsinfat = GetLabelsInFat(handle);            
    if (!labelsinfat) return FAIL;
    
    bytespercluster = GetSectorsPerCluster(handle) * BYTESPERSECTOR;
    if (!bytespercluster) return FAIL;    
    
    /* Do not check deleted entries */
    if (IsDeletedLabel(*entry))
       return TRUE;

    /* Initialise some values we will need */
    firstcluster = GetFirstCluster(entry);       
       
    if (IsLFNEntry(entry)) /* Checks on LFN entries */
    {
       if (firstcluster)
       { 
          SetFirstCluster(0, entry);
          invalid = TRUE;
       }
           
       if (invalid)
       {
          ShowParentViolation(handle, filename,      
                              "%s contains an invalid long file name entry");       
       }
    }
    else /* Checks on SFN entries */
    {
       /* Show the file name if we have to */     
       if (MustListAllFiles())
       {
          ReportFileName(filename);
       }            
            
       /* Check the attribute */
       if (entry->attribute & 0xC0)
       {
          ShowFileNameViolation(handle, filename,  
                                "%s has an invalid attribute");

          if (entry->attribute & FA_DIREC)
          {
             entry->attribute = FA_DIREC;
          }
          else
          {
             entry->attribute = 0;
          }
          
          invalid = TRUE;
       }

       /*
          Check the time stamps:
             Notice that year is valid over it's entire range.
       */

       /* Creation time */
       if ((memcmp(&entry->timestamp, "\0\0", 2) != 0 &&  /* Optional field */
            memcmp(&entry->timestamp, "\xff\xff", 2) != 0) &&  /* Optional field */
           ((entry->timestamp.second > 29)  ||
            (entry->timestamp.minute > 59)  ||
            (entry->timestamp.hours  > 23)))
       {
          sprintf(tempbuf, "%%s has an invalid creation time (%02u:%02u:%02u)",
                  entry->timestamp.hours, 
                  entry->timestamp.minute, 
                  entry->timestamp.second*2);
                                               
          ShowFileNameViolation(handle, filename, tempbuf);                                               
          memset(&entry->timestamp, 0, sizeof(struct PackedTime));
          
          invalid = TRUE;
       }

       /* Creation date */
       if ((memcmp(&entry->datestamp, "\0\0", 2) != 0 &&  /* Optional field */
            memcmp(&entry->datestamp, "\xff\xff", 2) != 0) &&  /* Optional field */
           (((entry->datestamp.day   < 1) || (entry->datestamp.day > 31))  ||
            ((entry->datestamp.month < 1) || (entry->datestamp.month > 12))))
       {
          sprintf(tempbuf, "%%s has an invalid creation date (%02u/%02u)",
                  entry->datestamp.month, entry->datestamp.day);
                                               
          ShowFileNameViolation(handle, filename, tempbuf); 
          memset(&entry->datestamp, 0, sizeof(struct PackedDate));
          
          invalid = TRUE;
       }

       /* Last access date */
       if ((memcmp(&entry->LastAccessDate, "\0\0", 2) != 0) &&
           (((entry->LastAccessDate.day   < 1) ||
                                         (entry->LastAccessDate.day > 31))  ||
            ((entry->LastAccessDate.month < 1) ||
                                         (entry->LastAccessDate.month > 12))))
       {
          sprintf(tempbuf, "%%s has an invalid access date (%02u/%02u)",
                  entry->LastAccessDate.month, entry->LastAccessDate.day);
                                               
          ShowFileNameViolation(handle, filename, tempbuf); 
          memset(&entry->LastAccessDate, 0, sizeof(struct PackedDate));
          
          invalid = TRUE;
       }

       /* Last write time (mandatory) */
       if ((entry->LastWriteTime.second > 29)  ||
           (entry->LastWriteTime.minute > 59)  ||
           (entry->LastWriteTime.hours  > 23))
       {
          sprintf(tempbuf, "%%s has an invalid last write time (%02u:%02u:%02u)",
                  entry->LastWriteTime.hours, 
                  entry->LastWriteTime.minute, 
                  entry->LastWriteTime.second*2);               
               
          ShowFileNameViolation(handle, filename, tempbuf); 
          memset(&entry->LastWriteTime, 0, sizeof(struct PackedTime));
          
          invalid = TRUE;
       }

       /* Last write date (mandatory) */
       if (((entry->LastWriteDate.day   < 1) || (entry->LastWriteDate.day > 31))  ||
           ((entry->LastWriteDate.month < 1) || (entry->LastWriteDate.month > 12)))
       {
	  sprintf(tempbuf, "%%s has an invalid last write date (%02u/%02u)",
                  entry->LastWriteDate.month, entry->LastWriteDate.day);
          ShowFileNameViolation(handle, filename, tempbuf); 
          
          entry->LastWriteDate.day = 1;
          entry->LastWriteDate.month = 1;
          
          invalid = TRUE;
       }

       /* A directory may not have a length */
       if ((entry->attribute & FA_DIREC) && (entry->filesize != 0))
       {             
          ShowFileNameViolation(handle, filename,  
                                 "%s is a directory with an invalid length (should be 0)");
          entry->filesize = 0;
          invalid = TRUE;
       }   

       if (!(((firstcluster < 2)                    ||
             (firstcluster >= labelsinfat)) &&
            (!((firstcluster == 0) &&
	     (IsPreviousDir(*entry) || entry->filesize == 0)))))
       {

          /* If this is a directory then check the '.' entry in it. */   
          if ((!IsPreviousDir(*entry)) &&
              (!IsCurrentDir(*entry)))
          {             
             if (entry->attribute & FA_DIREC)    /* Only if the first cluster is valid */
             {
                if (firstcluster)
                {
	           if (CheckDots(handle, firstcluster, fixit,
                                 filename) == FAIL)
                   {
                      return FAIL;
                   }
                }
             }

             switch (ScanFileChain(handle, firstcluster, 
                                   pos, entry, 
                                   filename, fixit))
             {
                case TRUE:
                     break;
                case FALSE:                      
                     invalid = TRUE;  
                     break;
                case FAIL:
                     return FAIL;
             }
          }
       }
    }

    if (invalid && fixit)       /* If we found an error and we have to fix it */
    {
       if (!WriteDirectory(handle, pos, entry)) /* Write the changes to disk */
          return FAIL;
    }

    
    /* indicate this directory entry */
    if (!invalid && !MarkDirectoryEntry(handle, pos))
    {
       return FAIL;
    }

    return !invalid;
}

/*******************************************************************
**                        CheckDots
********************************************************************
** Takes the first cluster of a directory and sees wether the directory
** contains '.' as first entry and if it does checks wether it points
** to the given cluster.
**
** Returns: FALSE - an invalid entry was found
**          TRUE  - an invalid entry was not found
**          FAIL  - media error
********************************************************************/

static BOOL CheckDots(RDWRHandle handle, CLUSTER firstcluster, BOOL fixit,
                      char* filename)
{
   struct DirectoryPosition pos = {0, 0};
   struct DirectoryEntry entry;

   if (!GetNthDirectoryPosition(handle, firstcluster, 0, &pos))
      return FAIL;

   if ((pos.sector == 0) && (pos.offset == 0))
   {
      ShowFileNameViolation(handle, filename, "%s is an empty directory");    
      return FALSE;
   }

   if (!GetDirectory(handle, &pos, &entry))
      return FAIL;

   if (!IsCurrentDir(entry))
   {
      ShowFileNameViolation(handle, filename, 
                            "%s doesn't contain an '.' as first entry");      
      return FALSE;
   }

   if (GetFirstCluster(&entry) != firstcluster)
   { 
      ShowFileNameViolation(handle, filename, "%s has a wrong '.' entry");  
                                      
      if (fixit)
      {
         SetFirstCluster(firstcluster, &entry);
         return (WriteDirectory(handle, &pos, &entry) ? FALSE : FAIL);
      }

      return FALSE;
   }

   return TRUE;
}