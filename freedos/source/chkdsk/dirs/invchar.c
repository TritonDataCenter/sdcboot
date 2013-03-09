/*
   InvChar.c - Checks for invalid characters in directory entries.
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
#include <ctype.h>

#include "fte.h"
#include "..\chkdrvr.h"
#include "..\struct\FstTrMap.h"
#include "..\errmsgs\errmsgs.h"
#include "..\errmsgs\PrintBuf.h"
#include "..\misc\useful.h"
#include "dirschk.h"

static BOOL RenameFile(RDWRHandle handle,
                       struct DirectoryPosition* pos,
                       struct DirectoryEntry* entry,
                       CLUSTER firstcluster);

#define INVALIDSFNCHARS "\x22\x2a\x2b\x2c\x2e\x3a\x3b\x3c\x3d" \
                        "\x3e\x3f\x5b\x5c\x5d\x7c"

static CLUSTER DirCluster;

BOOL InitInvalidCharChecking(RDWRHandle handle, CLUSTER cluster, char* filename)
{
     UNUSED(handle);
     UNUSED(cluster);
     UNUSED(filename);
    
     return TRUE;
}

BOOL PreProcesInvalidCharChecking(RDWRHandle handle, CLUSTER cluster, char* filename, BOOL fixit)
{
    UNUSED(handle);
    UNUSED(fixit);
    UNUSED(filename);
    
    DirCluster = cluster;
    
    return TRUE;
}

BOOL CheckEntryForInvalidChars(RDWRHandle handle,                   
                               struct DirectoryPosition* pos, 
                               struct DirectoryEntry* direct, 
                               char* filename,
                               BOOL fixit)
{
    int i;
    BOOL invalidname = FALSE;
    BOOL InRoot;
    struct DirectoryEntry entry;
    
    memcpy(&entry, direct, sizeof(struct DirectoryEntry));
    
    if (((entry.attribute & FA_LABEL) == 0) && /* Not checking volume labels */
        !IsDeletedLabel(entry))   
    {        
        if (IsPreviousDir(entry) || /* .. */
            IsCurrentDir(entry))    /* .  */              
        {
           /* See wether the given directory entry is in the root directory */
           InRoot = IsRootDirPosition(handle, pos);
           if (InRoot == FAIL) return FAIL;   
           
           /* All the '.' and '..' entries must be directories */
           if (((entry.attribute & FA_DIREC) == 0) || (InRoot))
           {
              if (IsCurrentDir(entry))
              {  
                 if (InRoot)
                 {
                    ShowFileNameViolation(handle, "",  
                                          "The root directory contains an '.' entry");   
                 }
                 else
                 {
                    ShowParentViolation(handle, filename,  
                                        "%s contains an '.' entry that is not a directory");
                 }
                 memcpy(entry.filename, "DOT     ", 8); 
              }
              else
              {
                 if (InRoot)
                 {
                    ShowFileNameViolation(handle, "",  
                                          "The root directory contains an '..' entry");                     
                 }   
                 else 
                 {
                    ShowParentViolation(handle, filename,  
                                        "%s contains an '..' entry that is not a directory");
                 }
                  
                 memcpy(entry.filename, "DOTDOT     ", 8);    
              }   
             
              if (fixit)
              {         
                 memcpy(entry.extension, "   ", 3);
                 if (!RenameFile(handle, pos, &entry, DirCluster)) /* Write the changes to disk */
                    return FAIL;          
              }
          
              return FALSE;          
           }

           /* Check wether you realy have '.' and '..' */
           if (IsCurrentDir(entry))
           {                             
              if ((memcmp(entry.filename, ".       ", 8) != 0) ||
                  (memcmp(entry.extension, "   ", 3) != 0))
              {
                 ShowFileNameViolation(handle, filename,  
                                       "%s contains invalid char(s)");  
                 if (fixit)
                 {                          
                    memcpy(entry.filename, ".       ", 8);
                    memcpy(entry.extension, "   ", 3);
                    if (!RenameFile(handle, pos, &entry, DirCluster)) /* Write the changes to disk */
                       return FAIL;                        
                 }
              }
           }           
           
           if (IsPreviousDir(entry))
           {                             
              if ((memcmp(entry.filename, "..      ", 8) != 0) ||
                  (memcmp(entry.extension, "   ", 3) != 0))
              {
                 ShowFileNameViolation(handle, filename,  
                                       "%s contains invalid char(s)");     
                 if (fixit)
                 {
                    memcpy(entry.filename, "..      ", 8);
                    memcpy(entry.extension, "   ", 3);
                    if (!RenameFile(handle, pos, &entry, DirCluster)) /* Write the changes to disk */
                       return FAIL;                    
                 }
              }
           }
        }
        else
        {
          if (entry.filename[0] == ' ')
          {
             invalidname = TRUE;
             entry.filename[0] = 'A';
          }

          /* file name */
          for (i = 0; i < 8; i++)
          {
              if ((strchr(INVALIDSFNCHARS, entry.filename[i]))                ||
                  (((unsigned char)(entry.filename[i]) < 0x20) && (entry.filename[i] != 0x05)) ||
                  (islower(entry.filename[i])))
              {
                 entry.filename[i] = 'A';
                 invalidname = TRUE;
              }
          }

          /* extension */
          for (i = 0; i < 3; i++)
          {
              if ((strchr(INVALIDSFNCHARS, entry.extension[i]))               ||
                  (((unsigned char)(entry.extension[i]) < 0x20) &&
                                                (entry.extension[i] != 0x05)) ||
                 (islower(entry.extension[i])))           
              {
                 entry.extension[i] = 'A';
                 invalidname = TRUE;
              }
          }

          if (invalidname)
          {
             ShowFileNameViolation(handle, filename,  
                                   "%s contains invalid char(s)");
      
             if (fixit)
             {         
                if (!RenameFile(handle, pos, &entry, DirCluster)) /* Write the changes to disk */
                   return FAIL;          
             }
          
             return FALSE;
          }
        }
    }
       
    return TRUE;
}

BOOL PostProcesInvalidCharChecking(RDWRHandle handle, CLUSTER cluster, char* filename, BOOL fixit)
{
    UNUSED(handle);
    UNUSED(fixit);
    UNUSED(filename);
    UNUSED(cluster);
   
    return TRUE;
}

void DestroyInvalidCharChecking(RDWRHandle handle, CLUSTER cluster, char* filename)
{
     UNUSED(filename);   
     UNUSED(handle);
     UNUSED(cluster);
}

/*************************************************************************
**                           RenameDoubleFile
**************************************************************************
** Renames a file with an invalid name.
**************************************************************************/

static BOOL RenameFile(RDWRHandle handle,
                       struct DirectoryPosition* pos,
                       struct DirectoryEntry* entry,
                       CLUSTER firstcluster)
{
    BOOL retval;
    int counter=0;
   
    for (;;)
    {
        retval = LoFileNameExists(handle, firstcluster,
                                  entry->filename, entry->extension);

        if (retval == FALSE) break;
        if (retval == FAIL)  return FALSE;

        counter++;
        sprintf(entry->extension, "%03d", counter);
        
        if (IsCurrentDir(*entry))
        {
           memcpy(entry->filename, "DOT     ", 8);
        }        
        if (IsPreviousDir(*entry))
        {
           memcpy(entry->filename, "DOTDOT  ", 8);
        }
    }

    return WriteDirectory(handle, pos, entry);
}