/*
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 *  If you have any questions, comments, suggestions, or fixes please
 *  email me at:  imre.leber@worldonline.be   
 */

#include <stdio.h>
#include <string.h>
#include <dir.h>

#include "fte.h"
#include "path.h"
#include "truname.h"
#include "recover.h"

static BOOL FileRecoverer(RDWRHandle handle, struct DirectoryPosition* pos, void** structure);
static BOOL ReadWriteCheckCluster(RDWRHandle handle, CLUSTER cluster);
static BOOL ClusterChecker(RDWRHandle handle, CLUSTER label, SECTOR datasector,
                           char* LoopCheckField, unsigned long* filesize, CLUSTER* prevcluster);


void RecoverFile(char* file)
{
    char drive[3];
    RDWRHandle handle;
    char temppath[MAXPATH], fullpath[MAXPATH];
    char* pos;
    
    strcpy(temppath, file);
    if (!GetPreviousDir(temppath))
	strcpy(temppath, ".");
    
    if (!Truename(fullpath, temppath) || (!(*fullpath)) || (fullpath[1] != ':'))
    {
	printf("Cannot access %s\n", file);
	return;
    }    
    
    strcpy(drive, "?:");
    drive[0] = fullpath[0];
    
    if (!InitReadWriteSectors(drive, &handle))
    {
	printf("Cannot access %s\n", drive);
	return;	
    }
    
    addsep(fullpath);
    pos = strrchr(file, '\\');
    if (pos)
    {
	strcat(fullpath, pos+1);
    }	
    else
    {
	strcat(fullpath, file); 	
    }
    
    if (!WalkWildcardPath(handle, &fullpath[2], 0, FileRecoverer, (void**) NULL))
    {
	printf("Problem recovering files\n");
	
	SynchronizeFATs(handle);
	CloseReadWriteSectors(&handle);
	return;
    }
    
    if (!ConvertLostClustersToFiles(handle))
    {
	printf("Problem reclaiming lost clusters\n");
	
	SynchronizeFATs(handle);
	CloseReadWriteSectors(&handle);
	return;
    }
    
    if (!TruncateCrossLinkedFiles(handle))
    {
	printf("Problem resolving cross linked clusters\n");
	
	SynchronizeFATs(handle);
	CloseReadWriteSectors(&handle);	
	return;    
    }
    
    SynchronizeFATs(handle);
    CloseReadWriteSectors(&handle);    
}

static BOOL FileRecoverer(RDWRHandle handle, struct DirectoryPosition* pos, void** structure)
{
    CLUSTER cluster;
    struct DirectoryEntry entry;
    unsigned long newsize;
    char filename[8+1+3+1];

    if (!GetDirectory(handle, pos, &entry))
	return FAIL;    

    if (!IsDeletedLabel(entry) &&
	!IsCurrentDir(entry)   &&
        !IsPreviousDir(entry)  &&
        !(entry.attribute & FA_LABEL))
    {    
       cluster = GetFirstCluster(&entry);
    
       ConvertEntryPath(filename, entry.filename, entry.extension);
       printf("Recovering %s\n", filename);
    
       if (cluster)
       {	
	   if (!RecoverFileChain(handle, cluster, &newsize))
	       return FAIL;
    
	   if (((entry.attribute & FA_DIREC) == 0) && (newsize != entry.filesize))
	   {
	       entry.filesize = newsize;
	
   	       if (newsize == 0)
	       {	
	           MarkEntryAsDeleted(entry); 
	       }
	
	       if (!WriteDirectory(handle, pos, &entry))
		   return FAIL;
	   }
       }
    }
    
    return TRUE;
}

static BOOL ReadWriteCheckCluster(RDWRHandle handle, CLUSTER cluster)
{
    SECTOR datasector;
    char sectordata[BYTESPERSECTOR];
    unsigned char SectorsPerCluster, i;
    
    SectorsPerCluster = GetSectorsPerCluster(handle);
    if (!SectorsPerCluster) return FAIL;
	
    datasector = ConvertToDataSector(handle, cluster);
    if (!datasector) return FAIL;
	
    for (i = 0; i < SectorsPerCluster; i++)
    {
	if (!ReadDataSectors(handle, 1, datasector+i, sectordata))
	    return FAIL;
	
	if (!WriteDataSectors(handle, 1, datasector+i, sectordata))
	    return FALSE;	
    }
    
    return TRUE;
}

static BOOL ClusterChecker(RDWRHandle handle, CLUSTER label, SECTOR datasector,
                           char* LoopCheckField, unsigned long* filesize, CLUSTER* prevcluster)
{
    CLUSTER cluster;
    
    cluster = DataSectorToCluster(handle, datasector);
    if (!cluster) return FAIL;
	
    /* Check for loops */
    if (GetBitfieldBit(LoopCheckField, cluster))
    {
	/* Break the loop */
	if (!WriteFatLabel(handle, *prevcluster, FAT_LAST_LABEL))
	    return FAIL;
	
	return FALSE;	 /* And then stop */
    }
    SetBitfieldBit(LoopCheckField, cluster);
    
    /* Check the validity of the label */
    switch (IsLabelValidInFile(handle, label))
    {
	case FALSE:
	     if ((*filesize) && !FAT_FREE(label))
	     {
                if (!WriteFatLabel(handle, *prevcluster, FAT_LAST_LABEL))
	           return FAIL;	     
	     }
	     else if (FAT_FREE(label))
             {
		if (!WriteFatLabel(handle, cluster, FAT_LAST_LABEL)) 
		   return FAIL;
             }		 
	     return FALSE;
	
	case FAIL:
	     return FAIL;	
    }
    
    /* Perform a read/write check on the cluster */
    switch (ReadWriteCheckCluster(handle, cluster))
    {	
	case FALSE:
	     if (filesize)
             {  		 
		 if (FAT_NORMAL(label) && (IsLabelValidInFile(handle, label) == TRUE))    
		 {
		    if (!WriteFatLabel(handle, *prevcluster, label))
		       return FAIL;	     
		 }
		 else
		 {
		    if (!WriteFatLabel(handle, *prevcluster, FAT_LAST_LABEL))
		       return FAIL;	     
                    if (!WriteFatLabel(handle, cluster, FAT_BAD_LABEL))
 		       return FAIL;
		    return FALSE;
		 }		 
	     }
	     
	     if (!WriteFatLabel(handle, cluster, FAT_BAD_LABEL))
		 return FAIL;
	     
	     if (filesize == 0)
		 return FALSE;           /* PROBLEM: when recovering a file and the first cluster of that file is bad,
                                                     the rest of the data in the file is discarded. */
 
	     break;
	     
	case TRUE:
	     (*filesize)++;
	     break;
	    
	case FAIL:
	     return FAIL; 
    }
        
    if (FAT_LAST(label))
	return FALSE;
    
    *prevcluster = cluster;
    return TRUE;    
}

BOOL RecoverFileChain(RDWRHandle handle, CLUSTER firstclust, unsigned long* newsize)
{
    BOOL goon = TRUE;
    char* LoopCheckField;
    SECTOR sector;
    CLUSTER prevcluster=0, cluster;
    unsigned long filesize;    
    unsigned long LabelsInFat;
    unsigned long sectorspercluster;
    
    LabelsInFat = GetLabelsInFat(handle);
    if (!LabelsInFat) return FALSE;	
    
    prevcluster = 0;
    LoopCheckField = CreateBitField(LabelsInFat);
    if (!LoopCheckField) return FALSE;
    filesize = 0;    
	    
    cluster = firstclust;
    
    do {
	sector = ConvertToDataSector(handle, cluster);
	if (!sector) return FALSE;
	
	if (!GetNthCluster(handle, cluster, &cluster))
	    return FALSE;	
	
        switch (ClusterChecker(handle, cluster, sector, LoopCheckField, &filesize, &prevcluster))
	{
	   case FALSE:
	        goon = FALSE;
	        break;
	   case FAIL:
	        DestroyBitfield(LoopCheckField);  
	        return FALSE;	    
	}
	   
    } while (goon); 
    
        
    DestroyBitfield(LoopCheckField); 

    sectorspercluster = GetSectorsPerCluster(handle); 
    if (!sectorspercluster) return FALSE;

    *newsize = filesize * sectorspercluster * BYTESPERSECTOR;

    return TRUE;
}
