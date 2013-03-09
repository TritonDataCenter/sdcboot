/*    
   ScnSbDrs.c - perform checks on sub directory entries.

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

#ifdef ENABLE_LOGGING  
#include <stdio.h>         
#endif     

#include "fte.h"
#include "..\misc\useful.h"
#include "..\chkdrvr.h"
#include "dirschk.h"
#include "scnsbdrs.h"
#include "sbdrfncs.h"

static int AmofSucceededChecks;                               
                               
BOOL InitClusterEntryChecking(RDWRHandle handle, CLUSTER cluster, char* filename)
{
    int i;
    
    for (i = 0, AmofSucceededChecks=0; i < AMOFNESTEDDIRCHECKS; i++, AmofSucceededChecks++)
    {
#ifdef ENABLE_LOGGING  
        printf("Initialising %s, on subdir %s\n", 
               DirectoryChecks[i].TestName, 
               filename); 
#endif            
        if (!DirectoryChecks[i].Init(handle, cluster, filename))
        {
           return FALSE;
        }
    }        
    return TRUE;
}

BOOL PreProcessClusterEntryChecking(RDWRHandle handle, 
                                    CLUSTER cluster,
                                    char* filename,
                                    BOOL fixit)
{
    int i;
    BOOL retVal = TRUE;
    
    for (i = 0; i < AmofSucceededChecks; i++)
    {
#ifdef ENABLE_LOGGING  
        printf("Preparing %s on subdir %s\n", 
               DirectoryChecks[i].TestName,
               filename); 
#endif                
            
        switch (DirectoryChecks[i].PreProcess(handle, cluster, filename, fixit))
        {
           case FALSE:      
                retVal = FALSE;
                break;
                
           case FAIL:
                return FAIL;
        }
    }        
    return retVal;
}

BOOL CheckEntriesInCluster(RDWRHandle handle, CLUSTER cluster, char* filename, BOOL fixit)
{
    int i;
    unsigned char sectorspercluster, j;
    struct DirectoryPosition dirpos;
    struct DirectoryEntry direct;
    BOOL retVal = TRUE;
    
    dirpos.sector = ConvertToDataSector(handle, cluster);
    if (!dirpos.sector) return FAIL;

    sectorspercluster = GetSectorsPerCluster(handle);
    if (!sectorspercluster) return FAIL;

    for (j = 0; j < sectorspercluster; j++)
    {
	for (i = 0; i < ENTRIESPERSECTOR; i++)
	{
	    dirpos.offset = i;

            if (!GetDirectory(handle, &dirpos, &direct))
            {
	       return FAIL;
	    }

	    if (direct.filename[0] == LASTLABEL)
	    {
	       return TRUE;
	    }
            
            switch (PerformEntryChecks(handle, &dirpos, &direct, filename, fixit))
            {
               case FALSE:
                    retVal = FALSE;
                    break;
               case FAIL:
                    return FAIL;      
            }
	}

	dirpos.sector++;
    }

    return retVal;
}

BOOL PostProcessClusterEntryChecking(RDWRHandle handle, CLUSTER cluster, 
                                     char* filename, BOOL fixit)
{
    int i;
    BOOL retVal = TRUE;

    for (i = 0; i < AmofSucceededChecks; i++)
    {
#ifdef ENABLE_LOGGING  
    printf("Finishing %s on subdir %s\n", 
           DirectoryChecks[i].TestName, 
           filename); 
#endif  
            
        switch (DirectoryChecks[i].PostProcess(handle, cluster, filename, fixit))
        {
           case FALSE:      
                retVal = FALSE;
                break;
                
           case FAIL:
                return FAIL;
        }
    }        
    return retVal;
}

void DestroyClusterEntryChecking(RDWRHandle handle, CLUSTER cluster, char* filename)
{
    int i;
    
    for (i = 0; i < AmofSucceededChecks; i++)
    {
#ifdef ENABLE_LOGGING  
    printf("Destroying %s on subdir %s\n", 
           DirectoryChecks[i].TestName,
           filename); 
#endif  
            
        DirectoryChecks[i].Destroy(handle, cluster, filename);    
    }
}

static char filebuf[255];
BOOL PerformEntryChecks(RDWRHandle handle, 
                        struct DirectoryPosition* dirpos, 
                        struct DirectoryEntry* direct,
                        char* filename,
                        BOOL fixit)
{
    int i, len;
    BOOL retVal = TRUE;
    char name[8], ext[3];
    
    strcpy(filebuf, filename);
    strcat(filebuf, "\\");
    len = strlen(filebuf);
    
    /* Make sure the file name is printable */
    memcpy(name, direct->filename, 8);
    memcpy(ext, direct->extension, 3);
    
    for (i = 0; i < 8; i++)
        if (name[i] < 32)
           name[i] = '?'; 
           
    for (i = 0; i < 3; i++)
        if (ext[i] < 32)
           ext[i] = '?';         
    
    ConvertEntryPath(&filebuf[len], name, ext);
    
    for (i = 0; i < AmofSucceededChecks; i++)
    {
#ifdef ENABLE_LOGGING  
        printf("Performing %s on subdir %s\n", 
               DirectoryChecks[i].TestName,
               filename); 
#endif     
            
        switch (DirectoryChecks[i].Check(handle, 
                                         dirpos,
                                         direct,
                                         filebuf,
                                         fixit))
        {
           case FALSE:      
                retVal = FALSE;           
                break;
                
           case FAIL:
                return FAIL;
        }
    }   

    return retVal;
}  

                 
                        