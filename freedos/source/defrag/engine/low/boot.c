/*	
   boot.c - boot sector manipulation code.
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
#include <time.h>

#ifdef __GNUC__
#include <sys/time.h>
#endif

#include "../../misc/bool.h"
#include "../header/fat.h"
#include "../header/rdwrsect.h"
#include "../header/direct.h"
#include "../header/boot.h"
#include "../header/fatconst.h"
#include "../header/fsinfo.h"
#include "../header/ftemem.h"
#include "../../misc/useful.h"
#include "../header/fteerr.h"

/************************************************************
**                  UpdateHandleStruct
*************************************************************
** This function updates the fields in the RDWRHandle with
** the data stored in the BPB.
**
** Notice: - not all data is cached in the RDWRHandle structure.
**         - For the fields that are cached it takes the right
**           values depending on the kind of FAT.
*************************************************************/

static void UpdateHandleStruct(RDWRHandle handle,
                               struct BootSectorStruct* buffer)
{
       /* Update the info in the handle structure */
       handle->BytesPerSector    = buffer->BytesPerSector;
       handle->SectorsPerCluster = buffer->SectorsPerCluster;
       handle->ReservedSectors   = buffer->ReservedSectors;
       handle->Fats              = buffer->Fats;
       handle->NumberOfFiles     = buffer->NumberOfFiles;
       
       if (buffer->NumberOfSectors)
          handle->NumberOfSectors = (unsigned long) buffer->NumberOfSectors;
       else
          handle->NumberOfSectors = (unsigned long) buffer->NumberOfSectors32;
          
       handle->descriptor = buffer->descriptor;
       
       if (DetermineFATType(buffer) == FAT32)
       {
          handle->SectorsPerFat = buffer->fs.spc32.SectorsPerFat;
          handle->NumberOfFiles = 0;
       }
       else
       {
          handle->SectorsPerFat = buffer->SectorsPerFat;
          handle->NumberOfFiles = buffer->NumberOfFiles;
       }
          
       handle->SectorsPerTrack = buffer->SectorsPerTrack;
       handle->Heads = buffer->Heads;

       /* For FAT12/16 this is garbage. */
       handle->RootCluster = buffer->fs.spc32.RootCluster;
       handle->FSInfo = buffer->fs.spc32.FSInfo;
}

/************************************************************
**                      ReadBootSector
*************************************************************
** This function reads the boot sector into a location in
** memory and updates the handle structure accordingly.
*************************************************************/

BOOL ReadBootSector(RDWRHandle handle, struct BootSectorStruct* buffer)
{
    if (ReadSectors(handle, 1, 0, buffer) != -1)
    {
       /* Update the info in the handle structure */
       UpdateHandleStruct(handle, buffer);          
       return TRUE;
    }
    else
       RETURN_FTEERR(FALSE);	    
}

/************************************************************
**                      WriteBootSector
*************************************************************
** This function writes the boot sector to the volume and
** updates the handle structure to whatever changes may have
** been made.
*************************************************************/

BOOL WriteBootSector(RDWRHandle handle, struct BootSectorStruct* buffer)
{
    BOOL retVal;
    
    assert(buffer);
    
    /* Synchronize cache values. */
    UpdateHandleStruct(handle, buffer); 
    
    retVal = WriteSectors(handle, 1, 0, buffer, WR_UNKNOWN) != -1;
    RETURN_FTEERR(retVal);
}

/************************************************************
**                    ReadBootSectorIfNeeded
*************************************************************
** When the boot sector information is not cached then this
** function updates the handle structure.
**
** It reads the boot sector in a temporary location that
** is freed before returning (all information is stored
** in the handle structure).
**
** This function does not use any memory when the handle
** structure needs to be updated.
*************************************************************/

STATIC BOOL ReadBootSectorIfNeeded(RDWRHandle handle)
{
    BOOL retVal = TRUE;
    struct BootSectorStruct* boot;

    if (handle->SectorsPerCluster == 0)
    {
       boot = AllocateBootSector();
       if (!boot) RETURN_FTEERR(FALSE);
    
       retVal = ReadBootSector(handle, boot);
    
       FreeBootSector(boot);
    }

    RETURN_FTEERR(retVal);
}

/************************************************************
**                      InvalidateBootInfo
*************************************************************
** This function invalidates the boot sector so that the
** next function call to a boot function forces a reread
** of the BPB from disk.
*************************************************************/

void InvalidateBootInfo(RDWRHandle handle)
{
    handle->SectorsPerCluster = 0;
    handle->FATtype = 0;
}

/************************************************************
**                      GetSectorsPerCluster
*************************************************************
** Returns the number of sectors per cluster.
*************************************************************/

unsigned long GetSectorsPerCluster(RDWRHandle handle)
{
    if (ReadBootSectorIfNeeded(handle))
       return handle->SectorsPerCluster;
    else
       RETURN_FTEERR(FALSE);
}

/************************************************************
**                      GetReservedSectors
*************************************************************
** Returns the number of reserved sectors.
*************************************************************/

unsigned short GetReservedSectors(RDWRHandle handle)
{
    if (ReadBootSectorIfNeeded(handle))
       return handle->ReservedSectors;
    else
       RETURN_FTEERR(FALSE); 	
}

/************************************************************
**                      GetNumberOfFats
*************************************************************
** Returns the number of FATs.
*************************************************************/

unsigned char GetNumberOfFats(RDWRHandle handle)
{
    if (ReadBootSectorIfNeeded(handle))
       return handle->Fats;
    else
       RETURN_FTEERR(FALSE);
}

/************************************************************
**                GetNumberOfRootEntries
*************************************************************
** Returns the number of root entries
**
** Returns 0 on FAT32
*************************************************************/
unsigned short GetNumberOfRootEntries(RDWRHandle handle)
{
    if (ReadBootSectorIfNeeded(handle))
       return handle->NumberOfFiles;
    else
       RETURN_FTEERR(FALSE);	
}

/************************************************************
**                    GetMediaDescriptor
*************************************************************
** Returns the media descriptor
*************************************************************/

unsigned char GetMediaDescriptor(RDWRHandle handle)
{
    if (ReadBootSectorIfNeeded(handle))
       return handle->descriptor;
    else
       RETURN_FTEERR(FALSE);	
}

/************************************************************
**                    GetNumberOfSectors
*************************************************************
** Returns the number of sectors
*************************************************************/

unsigned long GetNumberOfSectors(RDWRHandle handle)
{
    if (ReadBootSectorIfNeeded(handle))
       return handle->NumberOfSectors;
    else
       RETURN_FTEERR(FALSE); 	
}

/************************************************************
**                    GetSectorsPerFat
*************************************************************
** Returns the number of sectors per FAT
*************************************************************/

unsigned long GetSectorsPerFat(RDWRHandle handle)
{
    if (ReadBootSectorIfNeeded(handle))
       return handle->SectorsPerFat;
    else
       RETURN_FTEERR(FALSE);	
}

/************************************************************
**                    GetSectorsPerTrack
*************************************************************
** Returns the number of sectors per track
*************************************************************/

unsigned short GetSectorsPerTrack(RDWRHandle handle)
{
    if (ReadBootSectorIfNeeded(handle))
       return handle->SectorsPerTrack;
    else
       RETURN_FTEERR(FALSE);	    
}

/************************************************************
**                    GetReadWriteHeads
*************************************************************
** Returns the number of read/write heads
*************************************************************/

unsigned short GetReadWriteHeads(RDWRHandle handle)
{
    if (ReadBootSectorIfNeeded(handle))
       return handle->Heads;
    else
       RETURN_FTEERR(FALSE);	    	           
}

/************************************************************
**                    GetClustersInDataArea
*************************************************************
** Returns the number of clusters in the data area.
**
** This is not stored in the BPB but is calculated as follows:
**      total number of sectors -
**            number of reserved sectors (BPB, FSInfo, backup boot, ...) -
**            number of sectors in the root directory -
**            number of sectors in all the FAT's
**      / sectors per cluster
*************************************************************/

unsigned long GetClustersInDataArea(RDWRHandle handle)
{
    unsigned char sectorspercluster = GetSectorsPerCluster(handle);

    if (!sectorspercluster) RETURN_FTEERR(FALSE);	    

    return (GetNumberOfSectors(handle) -
            GetReservedSectors(handle) -
            (GetNumberOfRootEntries(handle) / 16) -
            (GetNumberOfFats(handle) * GetSectorsPerFat(handle))) /
           GetSectorsPerCluster(handle);
}

/************************************************************
**                   GetLabelsInFat
*************************************************************
** Returns the number of labels in the FAT which is the number
** of clusters in the data area + the clusters in the two
** first labels of the FAT.
*************************************************************/

unsigned long GetLabelsInFat(RDWRHandle handle)
{
    unsigned long clustersindataarea = GetClustersInDataArea(handle);
    
    if (!clustersindataarea) RETURN_FTEERR(FALSE);
    
    return clustersindataarea+2;
}

/************************************************************
**                    GetBiosDriveNumber
*************************************************************
** Returns the drive number of the volume as known by the BIOS
** or 0xff on error
**
** Notice: not all volumes are known by the BIOS
*************************************************************/

unsigned char GetBiosDriveNumber(RDWRHandle handle)
{
    /* Not cached in the handle structure because this function
       should not be called anyway.                             */
    char retVal;
    struct BootSectorStruct* boot;
    
    boot = AllocateBootSector();
    if (!boot) RETURN_FTEERR(0xff);
    
    if (!ReadBootSector(handle, boot))
    {
       FreeBootSector(boot);     
       RETURN_FTEERR(0xff);
    }
    
    if (DetermineFATType(boot) == FAT32)
    {        
       retVal = boot->fs.spc32.DriveNumber;
    }
    else        
    {
       retVal = boot->fs.spc1216.DriveNumber;
    }
    
    FreeBootSector(boot);     
    return retVal;
}

/************************************************************
**                PrivateIsVolumeDataFilled
*************************************************************
** Returns wether the optional fields in the BPB are filled
*************************************************************/

unsigned BOOL PrivateIsVolumeDataFilled(RDWRHandle handle,
                                        unsigned char signature)
{
    /* Not cached in the handle structure because this function
       should not be called anyway.                             */
    BOOL retVal;
    struct BootSectorStruct* boot;
    
    boot = AllocateBootSector();
    if (!boot) RETURN_FTEERR(0xff);
    
    if (!ReadBootSector(handle, boot))
    {
       FreeBootSector(boot);     
       RETURN_FTEERR(0xff);
    }
    
    if (DetermineFATType(boot) == FAT32)
       retVal = boot->fs.spc32.Signature == signature;
    else        
       retVal = boot->fs.spc1216.Signature == signature;
       
    FreeBootSector(boot);   
    return retVal;
}

/************************************************************
**                IsVolumeDataFilled
*************************************************************
** Returns wether the optional fields in the BPB are filled
*************************************************************/

BOOL IsVolumeDataFilled(RDWRHandle handle)
{
    return PrivateIsVolumeDataFilled(handle, EXTENDED_BOOT_SIGNATURE);
}

/************************************************************
**                IsVolumeDataFilled_NT
*************************************************************
** Returns wether the optional fields in the BPB are filled.
*************************************************************/

BOOL IsVolumeDataFilled_NT(RDWRHandle handle)
{
    return PrivateIsVolumeDataFilled(handle, EXTENDED_BOOT_SIGNATURE_NT);
}

/************************************************************
**                GetDiskSerialNumber
*************************************************************
** Returns the disk serial number.
*************************************************************/

unsigned long GetDiskSerialNumber(RDWRHandle handle)
{
    long retVal;    
    struct BootSectorStruct* boot;
    
    boot = AllocateBootSector();
    if (!boot) RETURN_FTEERR(0xff);
    
    if (!ReadBootSector(handle, boot))
    {
       FreeBootSector(boot);     
       RETURN_FTEERR(0xff); 
    }
    
    if (DetermineFATType(boot) == FAT32)
       retVal = boot->fs.spc32.VolumeID;
    else        
       retVal = boot->fs.spc1216.VolumeID;
              
    FreeBootSector(boot);   
    return retVal;
}

/************************************************************
**                GetBPBVolumeLabel
*************************************************************
** Returns volume label stored in the BPB
*************************************************************/

BOOL GetBPBVolumeLabel(RDWRHandle handle, char* label)
{ 
    struct BootSectorStruct* boot;
  
    boot = AllocateBootSector();
    if (!boot) RETURN_FTEERR(FALSE);
    
    if (!ReadBootSector(handle, boot))
    {
       FreeBootSector(boot);     
       RETURN_FTEERR(FALSE);
    }
    
    if (DetermineFATType(boot) == FAT32)
       memcpy(label, boot->fs.spc32.VolumeLabel, VOLUME_LABEL_LENGTH);
    else        
       memcpy(label, boot->fs.spc1216.VolumeLabel, VOLUME_LABEL_LENGTH);

    FreeBootSector(boot);   
    return TRUE;
}

/************************************************************
**                GetBPBFileSystemString
*************************************************************
** Returns the BPB file system string.
**
** This value can not be used to assume which kind of FAT
** is being used!
*************************************************************/

BOOL GetBPBFileSystemString(RDWRHandle handle, char* label)
{
    struct BootSectorStruct* boot;
    
    boot = AllocateBootSector();
    if (!boot) RETURN_FTEERR(FALSE);
    
    if (!ReadBootSector(handle, boot))
    {
       FreeBootSector(boot);     
       RETURN_FTEERR(FALSE);
    }
    
    if (DetermineFATType(boot) == FAT32)
       memcpy(label, boot->fs.spc32.FSType, FILESYS_LABEL_LENGTH);
    else
       memcpy(label, boot->fs.spc1216.FSType, FILESYS_LABEL_LENGTH);

    FreeBootSector(boot);   
    return TRUE;
}

/************************************************************
**                Determine FAT type
*************************************************************
** Returns the kind of FAT being used.
**
** Notice that the previous function returns the FAT
** determination string the value returned there is only
** informative and has no real value. The type of FAT is ONLY
** determined by the number of clusters.
*************************************************************/

int DetermineFATType(struct BootSectorStruct* boot)
{
    unsigned long RootDirSectors, FatSize, TotalSectors, DataSector;
    unsigned long CountOfClusters;
    
    assert(boot->BytesPerSector);
    assert(boot->SectorsPerCluster);
    
    RootDirSectors = ((boot->NumberOfFiles * 32) +
                      (boot->BytesPerSector-1)) /
                      (boot->BytesPerSector);
                      
    if (boot->SectorsPerFat != 0)
        FatSize = boot->SectorsPerFat;
    else
        FatSize = boot->fs.spc32.SectorsPerFat;
        
    if (boot->NumberOfSectors != 0)
       TotalSectors = boot->NumberOfSectors;
    else
       TotalSectors = boot->NumberOfSectors32;
       
    DataSector = TotalSectors - 
                     (boot->ReservedSectors + 
                          (boot->Fats * FatSize) +
                             RootDirSectors);
                             
   CountOfClusters = DataSector / boot->SectorsPerCluster;
   
   if (CountOfClusters < 4085)
   {
      return FAT12;
   }
   else if (CountOfClusters < 65525L)
   {
      return FAT16;
   }
   else
   {
      return FAT32;
   }    
}

/* The Next few functions are only for FAT32 */

/************************************************************
**                GetFAT32Version
*************************************************************
** Returns the version of FAT32.
*************************************************************/

unsigned short GetFAT32Version(RDWRHandle handle)
{
    short retVal;    
    struct BootSectorStruct* boot;

    assert(GetFatLabelSize(handle) == FAT32);
    
    boot = AllocateBootSector();
    if (!boot) RETURN_FTEERR(FALSE); 
   
    if (!ReadBootSector(handle, boot))
    {
       retVal = FALSE;
    }
    else
    {        
       retVal = boot->fs.spc32.FSVersion;
    }
    
    FreeBootSector(boot);
    RETURN_FTEERR(retVal);
}

/************************************************************
**                GetFAT32RootCluster
*************************************************************
** Returns the cluster of the root directory.
*************************************************************/

unsigned long GetFAT32RootCluster(RDWRHandle handle)
{
    assert(GetFatLabelSize(handle) == FAT32);
    
    if (ReadBootSectorIfNeeded(handle))
       return handle->RootCluster;
    else
       RETURN_FTEERR(FALSE);
}

/************************************************************
**                GetFSInfoSector
*************************************************************
** Returns the location of the FSInfo structure.
*************************************************************/

unsigned short GetFSInfoSector(RDWRHandle handle)
{
    assert(GetFatLabelSize(handle) == FAT32);
    
    if (ReadBootSectorIfNeeded(handle))
       return handle->FSInfo;
    else
       RETURN_FTEERR(FALSE);
}

/************************************************************
**                GetFAT32BackupBootSector
*************************************************************
** Returns the location of the backup boot.
*************************************************************/

unsigned short GetFAT32BackupBootSector(RDWRHandle handle)
{
    short retVal;
    struct BootSectorStruct* boot;
  
    assert(GetFatLabelSize(handle) == FAT32);
    
    boot = AllocateBootSector();
    if (!boot) RETURN_FTEERR(FALSE);
    
    if (!ReadBootSector(handle, boot))
    {
       retVal = FALSE;
    }
    else
    {        
       retVal = boot->fs.spc32.BackupBoot;
    }
    
    FreeBootSector(boot);
    RETURN_FTEERR(retVal);
}

/*
   The following functions take a boot sector structure 
   and make the changes to the boot sector in memory.

   This reduces the disk write overhead and the chance of disk corruption.
   
   Notice that the handle structure is not updated until you commit the
   changes with WriteBootSector().
*/

/************************************************************
**                IndicateVolumeDataFilled_NT
*************************************************************
** Indicates that the optional fields in the BPB are filled.
*************************************************************/

void IndicateVolumeDataFilled_NT(struct BootSectorStruct* boot)
{
    if (DetermineFATType(boot) == FAT32)
       boot->fs.spc32.Signature = EXTENDED_BOOT_SIGNATURE_NT;
    else
       boot->fs.spc1216.Signature = EXTENDED_BOOT_SIGNATURE_NT;
}

/************************************************************
**                IndicateVolumeDataFilled
*************************************************************
** Indicates that the optional fields in the BPB are filled.
*************************************************************/

void IndicateVolumeDataFilled(struct BootSectorStruct* boot)
{
    if (DetermineFATType(boot) == FAT32)
       boot->fs.spc32.Signature = EXTENDED_BOOT_SIGNATURE;
    else
       boot->fs.spc1216.Signature = EXTENDED_BOOT_SIGNATURE;
}

/************************************************************
**                WriteNumberOfRootEntries
*************************************************************
** Changes the number of root entries
**
** Only FAT12/16!
*************************************************************/

void WriteNumberOfRootEntries(struct BootSectorStruct* boot,
                              unsigned short numentries)
{
    boot->NumberOfFiles = numentries;
}

/************************************************************
**                WriteSectorsPerCluster
*************************************************************
** Changes the number of sectors per cluster
*************************************************************/

void WriteSectorsPerCluster(struct BootSectorStruct* boot,
                            unsigned char sectorspercluster)
{
    boot->SectorsPerCluster = sectorspercluster;    
}

/************************************************************
**                WriteReservedSectors
*************************************************************
** Changes the number of reserved sectors
*************************************************************/

void WriteReservedSectors(struct BootSectorStruct* boot,
                          unsigned short reserved)
{
    boot->ReservedSectors = reserved;
}

/************************************************************
**                WriteNumberOfFats
*************************************************************
** Changes the number of FATs
*************************************************************/

void WriteNumberOfFats(struct BootSectorStruct* boot,
                       unsigned char numberoffats)
{
    boot->Fats = numberoffats;    
}

/************************************************************
**                WriteMediaDescriptor
*************************************************************
** Changes the media descriptor
*************************************************************/

void WriteMediaDescriptor(struct BootSectorStruct* boot,
                          unsigned char descriptor)
{
    boot->descriptor = descriptor;
}

/************************************************************
**                WriteNumberSectors
*************************************************************
** Changes the number of sectors
*************************************************************/

void WriteNumberOfSectors(struct BootSectorStruct* boot,
                          unsigned long count)
{
    boot->NumberOfSectors = count;    
}

/************************************************************
**                WriteSectorsPerFat
*************************************************************
** Changes the number of sectors per FAT
*************************************************************/

void WriteSectorsPerFat(struct BootSectorStruct* boot,
                        unsigned long sectorsperfat)
{
    if (DetermineFATType(boot) == FAT32) 
       boot->fs.spc32.SectorsPerFat = sectorsperfat;
    else
       boot->SectorsPerFat = sectorsperfat;    
}

/************************************************************
**                WriteSectorsPerTrack
*************************************************************
** Changes the number of sectors per track
*************************************************************/

void WriteSectorsPerTrack(struct BootSectorStruct* boot,
                          unsigned short sectorspertrack)
{
    boot->SectorsPerTrack = sectorspertrack;    
}

/************************************************************
**                WriteReadWriteHeads
*************************************************************
** Changes the number of read/write heads
*************************************************************/

void WriteReadWriteHeads(struct BootSectorStruct* boot,
                         unsigned short heads)
{
    boot->Heads = heads;    
}

/************************************************************
**                WriteBiosDriveNumber
*************************************************************
** Changes the BIOS drive number
*************************************************************/

void WriteBiosDriveNumber(struct BootSectorStruct* boot,
                          unsigned char drivenumber)
{
    if (DetermineFATType(boot) == FAT32) 
       boot->fs.spc32.DriveNumber = drivenumber;
    else
       boot->fs.spc1216.DriveNumber = drivenumber;        
}

/************************************************************
**                WriteDiskSerialNumber
*************************************************************
** Changes the disk serial number
*************************************************************/

void WriteDiskSerialNumber(struct BootSectorStruct* boot,
                           unsigned long serialnumber)
{
    if (DetermineFATType(boot) == FAT32) 
       boot->fs.spc32.VolumeID = serialnumber;
    else
       boot->fs.spc1216.VolumeID = serialnumber;        
}

/************************************************************
**                CalculateNewSerialNumber
*************************************************************
** Calculates a new serial number according to the current date
** and time.
**
** Calculation method comes from RBIL.
*************************************************************/

#ifdef __TURBOC__

unsigned long CalculateNewSerialNumber(void)
{
  struct time theTime;
  struct date theDate;
  unsigned first, second;
  unsigned long result;

  gettime (&theTime);
  getdate (&theDate);

  first = ((theTime.ti_hour << 8) + theTime.ti_min) +
           (theDate.da_year + 1980);

  second = ((theTime.ti_sec << 8) + theTime.ti_hund) +
            ((theDate.da_mon << 8) + theDate.da_day);

  result = first + (second << 16);
  
  return result;
}

#else	/* UNIX version: not completely correct */

unsigned long CalculateNewSerialNumber(void)
{ 
  unsigned long result;
#ifndef _WIN32	// REMOVE!!!!!!!!!!!!
  time_t now;
  struct tm* nowtm;
  unsigned short first, second;

	  
  struct timeval  tval;
  struct timezone tzone;
  
  gettimeofday(&tval, &tzone);
  
  now = time(NULL);
  nowtm = localtime(&now);

  first = ((nowtm->tm_hour << 8) + nowtm->tm_min) +
           (nowtm->tm_year + 1900);					
  
  second = ((nowtm->tm_sec << 8) + (tval.tv_usec / 10)) +
            ((nowtm->tm_mon << 8) + nowtm->tm_mday);
  
  result = first + (second << 16);
#endif  
  return result;	
}


#endif

/************************************************************
**                WriteBPBVolumeLabel
*************************************************************
** Changes the BPB volume label.
*************************************************************/

void WriteBPBVolumeLabel(struct BootSectorStruct* boot, char* label)
{
    int i, j;
    char buffer[VOLUME_LABEL_LENGTH];
    
    for (i = 0; i < VOLUME_LABEL_LENGTH; i++)
    {
        if (label[i] == 0)
        {
           for (j = i; j < VOLUME_LABEL_LENGTH; j++)
               buffer[j] = ' ';
           break;
        }
        else
        {
           buffer[i] = label[i];
        }
    }
    
    if (DetermineFATType(boot) == FAT32) 
       memcpy(boot->fs.spc32.VolumeLabel, buffer, VOLUME_LABEL_LENGTH);
    else
       memcpy(boot->fs.spc1216.VolumeLabel, buffer, VOLUME_LABEL_LENGTH);        
}

/************************************************************
**                WriteBPBFileSystemString
*************************************************************
** Changes the BPB file system string.
*************************************************************/

void WriteBPBFileSystemString(struct BootSectorStruct* boot, char* label)
{
    int i, j;
    char buffer[FILESYS_LABEL_LENGTH];
    
    for (i = 0; i < FILESYS_LABEL_LENGTH; i++)
    {
        if (label[i] == 0)
        {
           for (j = i; j < FILESYS_LABEL_LENGTH; j++)
               buffer[j] = ' ';
           break;
        }
        else
        {
           buffer[i] = label[i];
        }
    }
    
    if (DetermineFATType(boot) == FAT32) 
       memcpy(boot->fs.spc32.FSType, buffer, FILESYS_LABEL_LENGTH);
    else
       memcpy(boot->fs.spc1216.FSType, buffer, FILESYS_LABEL_LENGTH);        
}

/************************************************************
**                WriteFAT32Version
*************************************************************
** Changes the FAT32 version.
**
** According to fatgen a FAT32 volume with an incorrect version
** number is not mounted.
**
** Only FAT32
*************************************************************/

void WriteFAT32Version(struct BootSectorStruct* boot, unsigned short version)
{
     boot->fs.spc32.FSVersion = version;
}

/************************************************************
**                WriteFAT32RootCluster
*************************************************************
** Changes the root cluster of the FAT32 volume.
**
** Only FAT32
*************************************************************/

void WriteFAT32RootCluster(struct BootSectorStruct* boot, CLUSTER RootCluster)
{
     boot->fs.spc32.RootCluster = RootCluster;
}

/************************************************************
**                WriteFSInfoSector
*************************************************************
** Changes the sector of the FSInfo structure.
**
** Only FAT32
*************************************************************/

void WriteFSInfoSector(struct BootSectorStruct* boot, unsigned short sector)
{
     boot->fs.spc32.FSInfo = sector;
}

/************************************************************
**                WriteFAT32BackupBootSector
*************************************************************
** Changes the backup boot sector of the FAT32 volume.
**
** Only FAT32
*************************************************************/

void WriteFAT32BackupBootSector(struct BootSectorStruct* boot,
                                unsigned short backupsect)
{
     boot->fs.spc32.BackupBoot = backupsect;
}
