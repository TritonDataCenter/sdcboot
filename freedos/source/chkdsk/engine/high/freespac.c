/*    
   FndFFSpc.c - functions to return the free spaces on a volume.

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


#include "fte.h"

static CLUSTER CurrentFreeSpace = 0;

static BOOL ActualyFindSpace(RDWRHandle handle, CLUSTER startfrom, 
			     CLUSTER *space, unsigned long *length);

                                 
/**************************************************************
**                    FindFirstFreeSpace
***************************************************************
** Searches for the first free cluster in the volume and returns 
** the length. 
***************************************************************/
                                 
BOOL FindFirstFreeSpace(RDWRHandle handle, CLUSTER* space,
                        unsigned long* length)
{ 
    int labelsize;
    struct FSInfoStruct* FSInfo;
    CLUSTER startfrom;
    
    *space = 0;
    *length = 0;
       
    /*
    ** Get the first cluster where to search from the FSInfo
    ** structure.
    **
    ** Search starts at the cluster returned in pipe.startfrom.
    */
    if (!GetFreeClusterSearchStart(handle, &startfrom))
	RETURN_FTEERROR(FALSE);              
      
    /*
	Find the free space    
    */    
    if (!ActualyFindSpace(handle, startfrom, space, length))
	RETURN_FTEERROR(FALSE);		
   
    /* 
    ** If this is FAT32 and the volume is not read only, update the
    ** FSInfo structure  
    */
    if (*space)
    {
       labelsize = GetFatLabelSize(handle);
       if ((labelsize == FAT32) && 
           (GetReadWriteModus(handle) == READINGANDWRITING))
       {
          FSInfo = AllocateFSInfo();
          if (!FSInfo) RETURN_FTEERROR(FALSE);
         
          if (!ReadFSInfo(handle, FSInfo))
          {
             FreeFSInfo(FSInfo);
             RETURN_FTEERROR(FALSE);
          }
    
          WriteFreeClusterStart(FSInfo, CurrentFreeSpace);
    
          if (!WriteFSInfo(handle, FSInfo))
          {
             FreeFSInfo(FSInfo);
             RETURN_FTEERROR(FALSE);
          }  
          FreeFSInfo(FSInfo);
       }
    }
     
    CurrentFreeSpace = *space + *length;  
    return TRUE;
}

/**************************************************************
**                    FindNextFreeSpace
***************************************************************
** Searches for the first free cluster in the volume and returns 
** the length. 
***************************************************************/

BOOL FindNextFreeSpace(RDWRHandle handle, CLUSTER* space,
                       unsigned long* length)
{
    BOOL retVal;
    
    unsigned long labelsinfat;
    
    labelsinfat = GetLabelsInFat(handle);   
    if (!labelsinfat) RETURN_FTEERROR(FALSE);
	
    if ((CurrentFreeSpace == 0) ||
	(CurrentFreeSpace > labelsinfat))
    {
       if (!GetFreeClusterSearchStart(handle, &CurrentFreeSpace))
          RETURN_FTEERROR(FALSE);
    }
       
    retVal = ActualyFindSpace(handle, CurrentFreeSpace, space, length);
       
    CurrentFreeSpace = *space + *length;
       
    
    RETURN_FTEERR(retVal);
}

/**************************************************************
**                    ActualyFindSpace
***************************************************************
** Searches for the first free cluster in the volume and returns 
** the length. 
***************************************************************/

static BOOL ActualyFindSpace(RDWRHandle handle, CLUSTER startfrom, 
			     CLUSTER *space, unsigned long *length) 	
{
    CLUSTER cluster, label;
    unsigned long labelsinfat = GetLabelsInFat(handle);

    if (!labelsinfat) RETURN_FTEERROR(FALSE);
    
    *space = 0;
    *length = 0;
    
    /*
    ** Look through the FAT to find the first free space.
    */
    for (cluster = startfrom; cluster < labelsinfat; cluster++) /* Get the free cluster */
    {
	if (!GetNthCluster(handle, cluster, &label))
	    RETURN_FTEERROR(FALSE);
	
	if (FAT_FREE(label))
	{
	    *space = cluster;
	    break;
	}	
    }        
    
    if (*space) /* Get the free space */
    {
	for (; cluster < labelsinfat; cluster++)
	{
	    if (!GetNthCluster(handle, cluster, &label))
		RETURN_FTEERROR(FALSE);

	    if (FAT_FREE(label))
	       (*length)++;
	    else
	       break;
	}
    } 
    
    return TRUE;
}    

/**************************************************************
**                    FindFirstFittingFreeSpace
***************************************************************
** Searches for the first free space large enough to contain 
** data of a certain length.
***************************************************************/ 

BOOL FindFirstFittingFreeSpace(RDWRHandle handle, 
                               unsigned long size,
                               CLUSTER* freespace)
{
    CLUSTER space;
    unsigned long length=0;
        
    CurrentFreeSpace = 0;
    
    while (length < size)
    {
       if (!FindNextFreeSpace(handle, &space, &length))
          RETURN_FTEERROR(FAIL);
             
       if (!space) return FALSE; /* No free space large enough */
    }
   
    *freespace = space;
    return TRUE;                          
}

/**************************************************************
**                    FindLastFreeCluster
***************************************************************
** Function that returns the last free cluster in the volume.
**
** This doesn't return the length.
***************************************************************/                             
/*                              
BOOL FindLastFreeCluster(RDWRHandle handle, CLUSTER* result)
{
    CLUSTER space=0;
    unsigned long length=1;
        
    do
    {
	*result = space+length-1;
	
       if (!FindNextFreeSpace(handle, &space, &length))
          RETURN_FTEERROR(FAIL);
	
    } while (space);
        
    
    return TRUE;
}
*/

BOOL FindLastFreeCluster(RDWRHandle handle, CLUSTER* result)
{ 
    CLUSTER i, label;
    unsigned long labelsinfat; 

    labelsinfat = GetLabelsInFat(handle); 
    if (!labelsinfat) return FALSE; 

    for (i = labelsinfat-1; i >= 2; i--) 
    { 
	if (!GetNthCluster(handle, i, &label)) 
	   return FALSE; 

	if (FAT_FREE(label)) 
	{ 
	    *result = i; 
	    return TRUE; 
	} 
    } 

    *result = 0; 
    return TRUE; 
} 


/**************************************************************
**                    GetPreviousFreeCluster
***************************************************************
** Function that returns the last free cluster in the volume.
**
** This doesn't return the length.
***************************************************************/                             

BOOL GetPreviousFreeCluster(RDWRHandle handle, CLUSTER cluster, CLUSTER* previous) 
{ 
    CLUSTER i, label;

    for (i = cluster-1; i >= 2; i--) 
    { 
	if (!GetNthCluster(handle, i, &label)) 
	    return FALSE; 

	if (FAT_FREE(label)) 
	{ 
	    *previous = i; 
	    return TRUE; 
	} 
    } 

    *previous = 0; 
    return TRUE; 
} 

/**************************************************************
**                    GetFreeDiskSpace
***************************************************************
** Returns total free disk space (in clusters).
***************************************************************/                             

BOOL GetFreeDiskSpace(RDWRHandle handle, unsigned long* length)
{
    CLUSTER label;
    CLUSTER current;
    unsigned long labelsinfat;
    
    *length = 0;    
    
    labelsinfat = GetLabelsInFat(handle);
    if (!labelsinfat) return FALSE;
    
    for (current = 2; current < labelsinfat; current++)
    {
        if (!GetNthCluster(handle, current, &label))
           return FALSE;

	if (FAT_FREE(label)) *length = *length + 1;
    }

    return TRUE;
}

/**************************************************************
**                    GetFreeDiskSpace
***************************************************************
** Returns the number of free clusters starting at start.
***************************************************************/   

BOOL CalculateFreeSpace(RDWRHandle handle, CLUSTER start, unsigned long* length)
{
    CLUSTER current, label;
    unsigned long labelsinfat;

    *length = 0;

    labelsinfat = GetLabelsInFat(handle);
    if (!labelsinfat) return FALSE;
    
    for (current = start; current < labelsinfat; current++)
    {
        if (!GetNthCluster(handle, current, &label))
           return FALSE;

	if (FAT_FREE(label)) 
	    *length = *length + 1;
	else
	    break;
    }

    return TRUE;    
}

