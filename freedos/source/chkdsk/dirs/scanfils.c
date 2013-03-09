/*
   ScanFils.c - intelligent scanning of directory entries.
   Copyright (C) 2003 Imre Leber

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

#include "fte.h"
#include "..\errmsgs\PrintBuf.h"
#include "scnsbdrs.h"

BOOL ScanFileChain(RDWRHandle handle, CLUSTER firstcluster,
                   struct DirectoryPosition* pos, 
                   struct DirectoryEntry* entry,
                   char* filename,
                   BOOL fixit)
{
    char* bitfield;
    unsigned long labelsinfat;    
    BOOL breakoff = FALSE;
    CLUSTER current = firstcluster, label, breakingcluster, lengthBreakingCluster=0;  
    unsigned long length = 0, calculatedsize, bytespercluster;
    static char tempbuf[255];   
    BOOL retVal = TRUE;

    /* If it is a root directory that we have to scan see wether it is
       a FAT32 volume and if it is get the root cluster */ 
    if (!firstcluster)
    {
       switch (GetFatLabelSize(handle))
       {
          case FAT12:
          case FAT16:
               return TRUE;
               
          case FAT32:
               current = GetFAT32RootCluster(handle);
               break; 
               
          default:
               return FAIL;
       }
    }
    else
    {
       bytespercluster = GetSectorsPerCluster(handle) * BYTESPERSECTOR;
       if (!bytespercluster) return FAIL;   
       
       calculatedsize = (entry->filesize / bytespercluster) +
                                   ((entry->filesize % bytespercluster) > 0);       
    }
      
    labelsinfat = GetLabelsInFat(handle);
    if (!labelsinfat) return FAIL;
  
    bitfield = CreateBitField(labelsinfat);
    if (!bitfield)
    {        
       DestroyBitfield(bitfield);
       return FAIL;  
    }
    
    SetBitfieldBit(bitfield, current);  
    
    /* Initialise the checks on the directory entries */
    if (entry->attribute & FA_DIREC)
    {
       InitClusterEntryChecking(handle, current, filename);
    
       switch (PreProcessClusterEntryChecking(handle, current, filename, fixit))
       {
          case FALSE:
               retVal = FALSE;              
               break;
               
          case FAIL:
               DestroyBitfield(bitfield);
               DestroyClusterEntryChecking(handle, current, filename);
               return FAIL;
       }
    }
    
    while (TRUE)
    {   
        length++;
  

        if (!GetNthCluster(handle, current, &label))
        {
           DestroyBitfield(bitfield);
           if (entry->attribute & FA_DIREC)
              DestroyClusterEntryChecking(handle, current, filename);           
           return FAIL;
        }
         
        /* Check the read cluster: */
        /* the last cluster */
        if (FAT_LAST(label))
        {
           if (entry->attribute & FA_DIREC) /* Here the current cluster is valid */
           {            
              switch (CheckEntriesInCluster(handle, current, filename, fixit))
              {
                  case FALSE:
                       retVal = FALSE;                                
                       break;
                   
                  case FAIL:
                       DestroyBitfield(bitfield);     
                       if (entry->attribute & FA_DIREC)
                          DestroyClusterEntryChecking(handle, current, filename);                 
                       return FAIL;                   
              }
           }                        
           break;                 
        }
        
        /* Check wether it is in range */
        if (FAT_NORMAL(label) && !IsLabelValid(handle, label))
        {
           if (firstcluster)
           {
              ShowFileNameViolation(handle, filename,
                                    "%s contains an invalid cluster");
           }     
           else
           {
              ShowFileNameViolation(handle, filename,
                                     "The root directory contains an invalid cluster");           
           }   
               
           breakoff = TRUE;
           breakingcluster = current;
           retVal = FALSE;                         
        }
        
        /* bad cluster */                                         
        if (FAT_BAD(label))
        {
           if (firstcluster)
           {
              ShowFileNameViolation(handle, filename,
                                    "%s contains a bad cluster");
           }     
           else
           {
              ShowFileNameViolation(handle, filename,
                                    "The root directory contains a bad cluster");           
           }     
           
           breakingcluster = current;
           breakoff = TRUE;
           retVal = FALSE;                         
        }       
                       
        if (FAT_FREE(label))
        {
           if (firstcluster)
           {
              ShowFileNameViolation(handle, filename,
                                     "%s contains a free cluster");
           }     
           else
           {
              ShowFileNameViolation(handle, filename,
                                     "The root directory contains a free cluster");           
           }
             
           breakoff = TRUE;      
           breakingcluster = current;     
           retVal = FALSE;                         
        }
                 
        /* Check wether there is a loop */
        if (!breakoff && GetBitfieldBit(bitfield, label))
        {
           if (firstcluster)
           {
              ShowFileNameViolation(handle, filename,
                                    "%s contains a loop");
           }     
           else
           {
              ShowFileNameViolation(handle, filename,
                                    "The root directory contains a loop");           
           }
                
           breakoff = TRUE;      
           breakingcluster = current;              
           retVal = FALSE;                     
        }
/*
        if ((firstcluster && ((entry->attribute & FA_DIREC) == 0)) &&
            (length > calculatedsize)                              &&
            (lengthBreakingCluster == 0))
        {
           lengthBreakingCluster = current;
        }
*/        
        if (breakoff)
        {
           if (fixit)
           {        
              if (!WriteFatLabel(handle, breakingcluster, FAT_LAST_LABEL))
              {
                 DestroyBitfield(bitfield);     
                 if (entry->attribute & FA_DIREC)
                    DestroyClusterEntryChecking(handle, current, filename);                 
                 return FAIL;
              }
           }
                
           break;
        }
        SetBitfieldBit(bitfield, label);  
      
        if (entry->attribute & FA_DIREC) /* Here the current cluster is valid */
        {            
           switch (CheckEntriesInCluster(handle, current, filename, fixit))
           {
              case FALSE:
                   retVal = FALSE;                                
                   break;
                   
              case FAIL:
                   DestroyBitfield(bitfield);     
                   if (entry->attribute & FA_DIREC)
                      DestroyClusterEntryChecking(handle, current, filename);                 
                   return FAIL;                   
           }
        }
        
        current = label;  
    }
    
    /* Check the length of the file */
    if (firstcluster)
    {
/*            
       if (((entry->attribute & FA_DIREC) == 0) && (length > calculatedsize))
       {
          sprintf(tempbuf, 
                  "%%s has an invalid size, the size should be about %lu, but the entry says it's %lu",
                  length * bytespercluster, entry->filesize);     

          ShowFileNameViolation(handle, filename, tempbuf);
          
          if (fixit &&
              !WriteFatLabel(handle, lengthBreakingCluster, FAT_LAST_LABEL))
          {
             DestroyBitfield(bitfield);     
             if (entry->attribute & FA_DIREC)
                DestroyClusterEntryChecking(handle, current, filename);             
             return FAIL;
          }    
 
          retVal = FALSE;                                   
       }                       
*/     

       if (((entry->attribute & FA_DIREC) == 0) && (length != calculatedsize))
       {            
          sprintf(tempbuf, 
                  "%%s has an invalid size, the size should be about %lu, but the entry says it's %lu",
                  length * bytespercluster, entry->filesize);     


          ShowFileNameViolation(handle, filename, tempbuf); 
       
          if (fixit)
          {
             /* Notice that we are modifying the filesize of the same entry in chkentrs.c,
                the entry shall be written to disk there. */     
             entry->filesize = length * bytespercluster;          
          }
          
          retVal = FALSE;                       
       }
    }
    
    /* Destroy the checks on the directory entries */
    if (entry->attribute & FA_DIREC)
    {
       switch (PostProcessClusterEntryChecking(handle, current, filename, fixit))
       {
          case FALSE:
               retVal = FALSE;                           
               break;
               
          case FAIL:
               retVal = FAIL;
       }
       
       DestroyClusterEntryChecking(handle, current, filename);     
    }
   
    DestroyBitfield(bitfield);
    return retVal;
}
