/*
   Fat.c - FAT manipulation code.
   Copyright (C) 2000 Imre Leber

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

#include <assert.h>
#include <string.h>

#include "../../misc/bool.h"
#include "../header/rdwrsect.h"
#include "../header/fat.h"
#include "../header/boot.h"
#include "../header/direct.h"
#include "../header/fatconst.h"
#include "../header/fsinfo.h"
#include "../header/ftemem.h"
#include "../header/traversl.h"
#include "../header/dtstrct.h"
#include "../header/fteerr.h"

/*******************************************************************
**                        GetFatStart
********************************************************************
** returns the starting sector of the first FAT.
********************************************************************/

SECTOR GetFatStart(RDWRHandle handle)
{
    return GetReservedSectors(handle);
}

/*******************************************************************
**                        GetFatLabelSize
********************************************************************
** returns the kind of FAT being used (FAT12, FAT16 or FAT32)
********************************************************************/

int GetFatLabelSize(RDWRHandle handle)
{
    struct BootSectorStruct* boot;
     
    if (handle->FATtype) 
       return handle->FATtype;
    else
    {
       boot = AllocateBootSector();
       if (!boot) RETURN_FTEERR(FALSE);
       
       if (!ReadBootSector(handle, boot)) 
       {
          FreeBootSector(boot);
          RETURN_FTEERR(FALSE);
       }
       handle->FATtype = DetermineFATType(boot);
       
       FreeBootSector(boot);
       return handle->FATtype;
    }
}

/*******************************************************************
**                        GetDataAreaStart
********************************************************************
** Returns the start sector of the data area
********************************************************************/

SECTOR GetDataAreaStart(RDWRHandle handle)
{
    int fatlabelsize;
    unsigned short rootcount;
    SECTOR RootDirSectors;
    unsigned long fatsize;
    unsigned short reservedsectors;
    unsigned char NumberOfFats;
    
    fatlabelsize = GetFatLabelSize(handle);
    if (fatlabelsize == FAT32)
    {
       RootDirSectors = 0;
    }
    else if (fatlabelsize)
    {
       assert((fatlabelsize == FAT12) || (fatlabelsize == FAT16) || (fatlabelsize == FAT32));    	
	
       rootcount = GetNumberOfRootEntries(handle);
       if (!rootcount) RETURN_FTEERR(FALSE);
       
       RootDirSectors = (((SECTOR) rootcount * 32) +
                          (BYTESPERSECTOR - 1)) / BYTESPERSECTOR; 
    }
    else
       return FALSE;
    
    fatsize = GetSectorsPerFat(handle); 
    if (!fatsize) RETURN_FTEERR(FALSE);
    
    NumberOfFats = GetNumberOfFats(handle);
    if (!NumberOfFats) RETURN_FTEERR(FALSE);
    
    reservedsectors = GetReservedSectors(handle);
    if (!reservedsectors) RETURN_FTEERR(FALSE);
    
    return (SECTOR)reservedsectors + 
                   ((SECTOR)fatsize * (SECTOR)NumberOfFats) +
                       (SECTOR) RootDirSectors;
}

/*******************************************************************
**                        DataSectorToCluster
********************************************************************
** Returns the cluster that has the given sector as first sector
** of the cluster.
**
** Notice also that there are many cluster values in FAT32 that map on the
** same data sector. This function returns the one that has the highest 4
** bits equal to 0.
********************************************************************/

CLUSTER DataSectorToCluster(RDWRHandle handle, SECTOR sector)
{
    unsigned char sectorspercluster;
    SECTOR datastart;  
    
    assert(sector);
    
    sectorspercluster = GetSectorsPerCluster(handle);
    if (!sectorspercluster) RETURN_FTEERR(FALSE);
    
    datastart = GetDataAreaStart(handle);
    if (!datastart) RETURN_FTEERR(FALSE);        

    sector = ((sector-datastart) / sectorspercluster) * sectorspercluster;

    return (sector / sectorspercluster + 2) & 0x0fffffffL;
}

/*******************************************************************
**                        ConvertToDataSector
********************************************************************
** Returns the first sector that belongs to the given cluster.
********************************************************************/

SECTOR ConvertToDataSector(RDWRHandle handle,
			   CLUSTER fatcluster)
{
    unsigned char sectorspercluster ;
    SECTOR datastart;
    
    assert(fatcluster);
    
    sectorspercluster = GetSectorsPerCluster(handle);
    if (!sectorspercluster) RETURN_FTEERR(FALSE);
    
    datastart = GetDataAreaStart(handle);
    if (!datastart) RETURN_FTEERR(FALSE);
        
    return ((FAT_CLUSTER(fatcluster)-2) * sectorspercluster) + datastart;
}

/*******************************************************************
**                        GetFatLabel
********************************************************************
** Private function that returns the label from a buffer that contains
** a part of the FAT.
**
** The FAT is divided into blocks of 3 sectors each. This way we are
** sure that each block start with a complete FAT label.
**
** Each of these blocks thus contains a fixed number of labels from the
** FAT. When we need a label, we calculate the block number and the
** offset in the block for this label. Then we load the block, i.e. 3
** sectors in memory and we take the label at the calculated offset.
********************************************************************/

static BOOL GetFatLabel(RDWRHandle handle,
	 	        char* readbuf, CLUSTER labelnr,
		        CLUSTER* label)
{
    int     labelsize;
    CLUSTER cluster;

    assert(readbuf);
    assert(labelnr);
    
    labelsize = GetFatLabelSize(handle);
    if (!labelsize) RETURN_FTEERR(FALSE);

    labelnr  = FAT_CLUSTER(labelnr);
    labelnr %= (FATREADBUFSIZE * 8 / labelsize);

    if (labelsize == FAT12)
    {
       cluster = labelnr + (labelnr >> 1);
       memcpy(label, &readbuf[(unsigned) cluster], sizeof(CLUSTER));

       if ((labelnr & 1) == 0)
	  *label &= 0xfff;
       else
	  *label = (*label >> 4) & 0xfff;
    }
    else if (labelsize == FAT16)
    {
       memcpy(label, &readbuf[(unsigned) labelnr << 1], sizeof(CLUSTER));
       *label &= 0xffff;
    }
    else if (labelsize == FAT32)
    {
       memcpy(label, &readbuf[(unsigned) labelnr << 2], sizeof(CLUSTER));
    }
    else
        RETURN_FTEERR(FALSE);

    return TRUE;
}

/*******************************************************************
**                        LastFatLabel
********************************************************************
** Returns wether this is the last cluster of some file.
********************************************************************/

static BOOL LastFatLabel(RDWRHandle handle, CLUSTER label)
{
    int labelsize;

    labelsize = GetFatLabelSize(handle);

    if (labelsize == FAT12)
       return (FAT12_LAST(label));
    else if (labelsize == FAT16)
       return (FAT16_LAST(label));
    else if (labelsize == FAT32)
       return (FAT32_LAST(label));

     RETURN_FTEERR(FALSE);
}

/*******************************************************************
**                        GeneralizeLabel
********************************************************************
** Converts a file system specific label into a generalized one.
********************************************************************/

static CLUSTER GeneralizeLabel(RDWRHandle handle, CLUSTER label)
{
   int labelsize = GetFatLabelSize(handle);

   if (labelsize == FAT12)
   {
      if (FAT12_FREE(label))     return FAT_FREE_LABEL;
      if (FAT12_BAD(label))      return FAT_BAD_LABEL;
      if (FAT12_LAST(label))     return FAT_LAST_LABEL;
      return label;
   }
   else if (labelsize == FAT16)
   {
      if (FAT16_FREE(label))     return FAT_FREE_LABEL;
      if (FAT16_BAD(label))      return FAT_BAD_LABEL;
      if (FAT16_LAST(label))     return FAT_LAST_LABEL;
      return label;
   }
   else if (labelsize == FAT32)
   {
      if (FAT32_FREE(label))     return FAT_FREE_LABEL;
      if (FAT32_BAD(label))      return FAT_BAD_LABEL;
      if (FAT32_LAST(label))     return FAT_LAST_LABEL;
      return FAT32_CLUSTER(label);
   }
   else
       RETURN_FTEERR(FALSE);
}

/*******************************************************************
**                        LinearTraverseFat
********************************************************************
** Calls a function for every label in the FAT.
********************************************************************/

BOOL LinearTraverseFat(RDWRHandle handle,
		      int (*func) (RDWRHandle handle,
				   CLUSTER label,
				   SECTOR datasector,
				   void** structure),
		      void** structure)
{
    int      i, iterations, rest;
    CLUSTER  j=2;
    SECTOR   fatstart;
    int      toreadsectors;
    unsigned long  toreadlabels, labelsinbuf, dataclusters;
    char     *buffer;
    
    CLUSTER  label;
    SECTOR   datasector;

    unsigned short sectorsperfat;
    unsigned char  sectorspercluster;

    assert(func);

    fatstart = GetFatStart(handle);
    if (!fatstart) RETURN_FTEERR(FALSE);

    sectorsperfat     = GetSectorsPerFat(handle);
    if (!sectorsperfat) RETURN_FTEERR(FALSE);

    sectorspercluster = GetSectorsPerCluster(handle);
    if (!sectorspercluster) RETURN_FTEERR(FALSE);

    dataclusters = GetLabelsInFat(handle);
    if (!dataclusters) RETURN_FTEERR(FALSE);

    iterations = sectorsperfat / SECTORSPERREADBUF;
    rest       = sectorsperfat % SECTORSPERREADBUF;

    /* toreadsectors = SECTORSPERREADBUF; */
    toreadlabels  = labelsinbuf = FATREADBUFSIZE * 8 / GetFatLabelSize(handle);
        
    buffer = (char*) AllocateSectors(handle, SECTORSPERREADBUF);
    if (!buffer) RETURN_FTEERR(FALSE);
    
    for (i = 0; i < iterations + (rest > 0); i++)
    {
	toreadsectors = (i == iterations) ? rest : SECTORSPERREADBUF;
    
        if (ReadSectors(handle, toreadsectors,
			fatstart + (i*SECTORSPERREADBUF), buffer) == -1)
        {
	   FreeSectors((SECTOR*)buffer);
           RETURN_FTEERR(FALSE);
        }
           
        for (; j < toreadlabels; j++)
	{
	    if (j >= dataclusters) break;
 
	    if (!GetFatLabel(handle, buffer, j, &label)) 
            {
               FreeSectors((SECTOR*)buffer);
               RETURN_FTEERR(FALSE);
            }

	    datasector = ConvertToDataSector(handle, j);
	    switch (func (handle, GeneralizeLabel(handle, label), datasector,
			  structure))
	    {
	       case FALSE:
		    FreeSectors((SECTOR*)buffer);
		    return TRUE;
	       case FAIL:
		    FreeSectors((SECTOR*)buffer);
		    RETURN_FTEERR(FALSE);
	    }
	}
	toreadlabels += labelsinbuf;
    }
    
    FreeSectors((SECTOR*)buffer);
    return TRUE;
}

/*******************************************************************
**                        FileTraverseFat
********************************************************************
** Calls a function for every cluster in a file.
********************************************************************/

BOOL FileTraverseFat(RDWRHandle handle, CLUSTER startcluster,
		     int (*func) (RDWRHandle handle,
			 	  CLUSTER label,
				  SECTOR  datasector,
				  void** structure),
		     void** structure)
{
    int fatlabelsize;
    char* buffer;
    CLUSTER cluster, seeking, prevpart = -1, gencluster;
    SECTOR  sector;

    SECTOR  fatstart;
    
    assert(func);

    cluster = FAT_CLUSTER(startcluster);
    fatstart = GetFatStart(handle);
    if (!fatstart) RETURN_FTEERR(FALSE);

    fatlabelsize = GetFatLabelSize(handle);
    if (!fatlabelsize) RETURN_FTEERR(FALSE);
    
    buffer = (char*) AllocateSectors(handle, SECTORSPERREADBUF);
    if (!buffer) RETURN_FTEERR(FALSE);

    while (!LastFatLabel(handle, cluster))
    {
	seeking = cluster / (FATREADBUFSIZE * 8 / fatlabelsize);

	if (prevpart != seeking)
	{
	   if (ReadSectors(handle, SECTORSPERREADBUF,
			   fatstart + (unsigned) seeking * SECTORSPERREADBUF,
			   buffer) == -1)
           {
	      FreeSectors((SECTOR*)buffer);
              RETURN_FTEERR(FALSE);
           }

	   prevpart = seeking;
	}

	sector = ConvertToDataSector(handle, cluster);
	if (!GetFatLabel(handle, buffer, cluster, &cluster))
        {        
           FreeSectors((SECTOR*)buffer);
           RETURN_FTEERR(FALSE);
        }
        
        /*
            A bad cluster in a file chain is actually quite uncommon.
            
            It can only happen when:
            - When the file was written it was not marked as bad.
            - The cluster got bad afterwards
            - A disk scanning utility marked it as bad without relocating
              the cluster.
              
            Still we check for it.
        */
        
        gencluster = GeneralizeLabel(handle, cluster);
        if (FAT_BAD(gencluster) || FAT_FREE(gencluster))
        {
	   FreeSectors((SECTOR*)buffer);
           RETURN_FTEERR(FALSE);
        }

	switch (func(handle, gencluster, sector, structure))
	{
	   case FALSE:
		FreeSectors((SECTOR*)buffer);
		return TRUE;
	   case FAIL:
		FreeSectors((SECTOR*)buffer);
		RETURN_FTEERR(FALSE);
	}
    }

    FreeSectors((SECTOR*)buffer);
    return TRUE;
}

/*******************************************************************
**                        ReadFatLabel
********************************************************************
** Reads a label from the FAT at the indicated position.
********************************************************************/

BOOL ReadFatLabel(RDWRHandle handle, CLUSTER labelnr,
		 CLUSTER* label)
{
    int     labelsize;
    SECTOR  sectorstart, blockindex;
    int     labelsinbuf;
    BOOL    retVal;
    
    assert(label);

    labelsize = GetFatLabelSize(handle);
    if (!labelsize) RETURN_FTEERR(FALSE);
	
    assert((labelsize == FAT12) || (labelsize == FAT16) || (labelsize == FAT32));

    sectorstart = GetFatStart(handle);
    if (!sectorstart) RETURN_FTEERR(FALSE);

    labelsinbuf  = FATREADBUFSIZE * 8 / labelsize;
    blockindex   = labelnr / labelsinbuf;
    sectorstart += SECTORSPERREADBUF * blockindex;
    
    if ((!handle->FatCacheUsed) ||
        (handle->FatCacheStart != sectorstart))
    {
       if (ReadSectors(handle, SECTORSPERREADBUF, sectorstart,
                       handle->FatCache) == -1)
       {
           RETURN_FTEERR(FALSE);
       }
       
       handle->FatCacheStart = sectorstart;
    }
    
    retVal = GetFatLabel(handle, handle->FatCache, labelnr, label);

    handle->FatCacheUsed = retVal;

    return retVal;
}

/*******************************************************************
**                        WriteFatLabel
********************************************************************
** Writes a label to the FAT at the specified location.
**
** This is the only function that should ever be used to change a fat
** label. This is because it pertains the high four bits of a fat cluster
** in FAT32.
**
** Works for both generalized and none generalized labels.
********************************************************************/
   
int WriteFatLabel(RDWRHandle handle, CLUSTER labelnr,
		  CLUSTER label)
{
    int      retVal;
    int      labelsize;
    char*    buffer;
    SECTOR   sectorstart;
    unsigned long sectorsperfat, sectorsperbit, bit;
    int      labelsinbuf, temp, i;
    
    /* First invalidate FAT cache. */
    handle->FatCacheUsed = FALSE;
    
    labelsize = GetFatLabelSize(handle);
    if (!labelsize) RETURN_FTEERR(FALSE);

    sectorstart = GetFatStart(handle);
    if (!sectorstart) RETURN_FTEERR(FALSE);

    sectorsperfat = GetSectorsPerFat(handle);
    if (!sectorsperfat) RETURN_FTEERR(FALSE);

    labelsinbuf  = FATREADBUFSIZE * 8 / labelsize;
    sectorstart += ((SECTOR) labelnr / labelsinbuf) * SECTORSPERREADBUF;

    buffer = (char*) AllocateSectors(handle, SECTORSPERREADBUF);
    if (!buffer) RETURN_FTEERR(FALSE);
   
    if (ReadSectors(handle, SECTORSPERREADBUF, sectorstart, buffer) == -1)
    {
	FreeSectors((SECTOR*)buffer);
         RETURN_FTEERR(FALSE);
    }
 
    labelnr %= (FATREADBUFSIZE * 8 / labelsize);   
        
    if (labelsize == FAT12)
    {
       if (FAT_BAD(label)) label = FAT12_BAD_LABEL;
       if (FAT_LAST(label)) label = 0xfff;

       labelnr *= 12;                  /* The nth bit in the block */
       
       if (labelnr % 8 == 0) /* If this bit starts at a byte offset */
       {
	  memcpy(&temp, &buffer[(unsigned) labelnr / 8], 2);
	  temp &= 0xf000;
	  temp |= ((unsigned) label & 0xfff);
	  memcpy(&buffer[(unsigned) labelnr / 8], &temp, 2);
       }
       else
       {
	  memcpy(&temp, &buffer[(unsigned) labelnr / 8], 2);
	  temp &= 0x000f;
	  temp |= ((unsigned) label << 4);
	  memcpy(&buffer[(unsigned) labelnr / 8], &temp, 2);
       }
    }
    else if (labelsize == FAT16)
    {
       if (FAT_BAD(label)) label = FAT16_BAD_LABEL;
       if (FAT_LAST(label)) label = 0xffff;

       memcpy(&buffer[(unsigned) labelnr << 1], &label, 2);
    }
    else if (labelsize == FAT32)
    {
       /* Make sure the high for bits are pertained. */
       CLUSTER old;
       memcpy(&old, &buffer[(unsigned) labelnr << 2], 4);
       
       label &= 0x0fffffffL;
       old   &= 0xf0000000L;
       label += old;
       
       memcpy(&buffer[(unsigned) labelnr << 2], &label, 4); 
    }

    retVal = WriteSectors(handle, SECTORSPERREADBUF,
			  sectorstart, buffer, WR_FAT) != -1;
    FreeSectors((SECTOR*)buffer);                      

    /* Indicate which sectors contain labels that have changed. */
    handle->FatLabelsChanged = TRUE;
    if (!handle->FatChangeField)
       handle->FatChangeField = CreateBitField(sectorsperfat & 0xffff);

    if (handle->FatChangeField)
    {
       sectorsperbit = (sectorsperfat >> 16) + 1;

       for (i = 0; i < SECTORSPERREADBUF; i++)
       {
           bit = (sectorstart + i) / sectorsperbit;
           SetBitfieldBit(handle->FatChangeField, bit);
       }
    }

    return retVal;
}

/******************************************************************
**                        InvalidateFatCache
********************************************************************
** Invalidates the cache used for caching a part of the FAT.
********************************************************************/

void InvalidateFatCache(RDWRHandle handle)
{
   handle->FatCacheUsed = FALSE;
}

/*******************************************************************
**                        GetBytesInFat
********************************************************************
** Returns the number of bytes occupied by one FAT.
********************************************************************/

unsigned long GetBytesInFat(RDWRHandle handle)
{
    return ((unsigned long) GetFatLabelSize(handle) *
            (unsigned long) GetLabelsInFat(handle)) / 8L;
}

/*************************************************************************
**                           IsLabelValid
**************************************************************************
** Returns wether the given label is valid for a volume.
***************************************************************************/

BOOL IsLabelValid(RDWRHandle handle, CLUSTER label)
{
   unsigned long labelsinfat;

   /* Only normal FAT labels can be detected as wrong. */
   if (!FAT_NORMAL(label)) return TRUE;
   
   /* label 1 can never be a FAT label */
   if (label == 1) return FALSE;

   /* Check wether the label is in the right range */
   labelsinfat = GetLabelsInFat(handle);
   if (!labelsinfat)  RETURN_FTEERR(FAIL);

   return label < labelsinfat;
}

/*************************************************************************
**                         IsLabelValidInFile
**************************************************************************
** Returns wether the given label is valid as part of a file chain.
***************************************************************************/

BOOL IsLabelValidInFile(RDWRHandle handle, CLUSTER label)
{
    if (FAT_LAST(label)) 
	return TRUE;    
    
    if (!FAT_NORMAL(label)) 
	return FALSE;    
    
    return IsLabelValid(handle, label);
}

