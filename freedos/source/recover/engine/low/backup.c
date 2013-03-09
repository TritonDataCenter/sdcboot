#include <string.h>

#include "bool.h"
#include "rdwrsect.h"
#include "boot.h"
#include "fat.h"
#include "fatconst.h"
#include "direct.h"
#include "fsinfo.h"
#include "ftemem.h"
#include "backup.h"
#include "traversl.h"
#include "dtstrct.h"

/************************************************************
**                        BackupBoot                       **
*************************************************************
** On FAT32 this makes a copy of the boot at the location  **
** specified by the BPB.                                   **
**                                                         **
** On FAT12/16 this function does nothing, but may be      **
** called.                                                 **
*************************************************************/

BOOL BackupBoot(RDWRHandle handle)
{
   int fatlabelsize = GetFatLabelSize(handle);
   SECTOR BackupSector;
   struct BootSectorStruct* boot;
 
   if (fatlabelsize == FAT32)
   {
      boot = AllocateBootSector();
      if (!boot) return FALSE;
   
      if (!ReadBootSector(handle, boot) == -1)
      {
         FreeBootSector(boot);
         return FALSE;
      }
      
      BackupSector = GetFAT32BackupBootSector(handle);
      if (!BackupSector)
      {
         FreeBootSector(boot);
         return FALSE;
      }
               
      if (WriteSectors(handle, 1, BackupSector, (void*) boot, WR_UNKNOWN)
                                                                     == -1) 
      {
          FreeBootSector(boot);
          return FALSE;
      }    
   }     
   
   return TRUE;
}

/************************************************************
**                    MultipleBootCheck                    **
*************************************************************
** Checks wether the second FAT is the same as the first.  **
**                                                         **
** On FAT12/16 this function does nothing, but may be      **
** called.                                                 **
*************************************************************/

BOOL MultipleBootCheck(RDWRHandle handle)
{
   BOOL retval;
   int fatlabelsize = GetFatLabelSize(handle);
   SECTOR BackupSector;
   struct BootSectorStruct* boot1, *boot2;

   if (fatlabelsize == FAT32)
   {
      boot1 = AllocateBootSector();
      if (!boot1) return FAIL;
   
      boot2 = AllocateBootSector();
      if (!boot2)
      {         
         FreeBootSector(boot1);
         return FAIL;
      }

      if (!ReadBootSector(handle, boot1) == -1)
      {
         FreeBootSector(boot1);
         return FAIL;
      }
      
      BackupSector = GetFAT32BackupBootSector(handle);
      if (!BackupSector)
      {
         FreeBootSector(boot1);
         FreeBootSector(boot2);
         return FAIL;
      }
               
      if (!ReadSectors(handle, 1, BackupSector, (void*) boot2) == -1) 
      {
         FreeBootSector(boot1);
         FreeBootSector(boot2);
         return FAIL;
      }    
      
      retval = (memcmp(boot1, boot2, BYTESPERSECTOR) == 0);
      
      FreeBootSector(boot1);
      FreeBootSector(boot2);
      
      return retval;
   }     

   return (fatlabelsize) ? TRUE : FAIL;    
}

/************************************************************
**                      HasSectorChanged                   **
*************************************************************
** This function returns a certain sector has changed.     **
** If we don't know say that it has failed.                **
*************************************************************/

static BOOL HasSectorChanged(RDWRHandle handle, SECTOR sector,
                             unsigned long sectorsperfat)
{
    /* Notice that we have already checked wether there were
       any changes and that this function is only called if
       not the entire FAT needs to be copied. */
    unsigned long sectorsperbit;
    unsigned long bit;

    if (handle->FatChangeField)
    {
       sectorsperbit = (sectorsperfat >> 16) + 1;
       bit = sector / sectorsperbit;
       return GetBitfieldBit(handle->FatChangeField, bit);
    }
    else
    {
       return TRUE;
    }
}

/************************************************************
**                     PrivateBackupFat                    **
*************************************************************
** This function copies labels to the copies of the FAT    **
** either all of it or only what has changed.              **
*************************************************************/

static BOOL PrivateBackupFat(RDWRHandle handle, BOOL all)
{
    BOOL result = TRUE;
    unsigned i, NumberOfFats;
    unsigned long SectorsPerFat, bytesinlastsector;
    SECTOR sector, fatstart;
    char* sectbuf, *sectbuf1;
    unsigned long bytesinfat;

    bytesinfat = GetBytesInFat(handle);
    if (!bytesinfat) return FALSE;

    fatstart = GetFatStart(handle);
    if (!fatstart) return FALSE;
    
    NumberOfFats = GetNumberOfFats(handle);
    if (!NumberOfFats) return FALSE;
    
    SectorsPerFat = GetSectorsPerFat(handle);
    if (!SectorsPerFat) return FALSE;

    sectbuf  = (char*) FTEAlloc(BYTESPERSECTOR);
    if (!sectbuf) return FALSE;

    sectbuf1 = (char*) FTEAlloc(BYTESPERSECTOR);
    if (!sectbuf1)
    {
       FTEFree(sectbuf);
       return FALSE;
    }
        
    for (i = 1; i < NumberOfFats; i++)
    {
        for (sector = fatstart; sector < fatstart+SectorsPerFat-1; sector++)
        {
            if (all || HasSectorChanged(handle, sector, SectorsPerFat))
            {
               if (ReadSectors(handle, 1, sector, sectbuf) != -1)
               {
                  if (WriteSectors(handle, 1, sector + (i * SectorsPerFat),
                                   sectbuf, WR_FAT) == -1)
                  {
                     FTEFree(sectbuf);
                     FTEFree(sectbuf1);
                     return FALSE;
                  }
               }
               else
               {
                  FTEFree(sectbuf);
                  FTEFree(sectbuf1);
                  return FALSE;
               }
            }
        }

	if (all || HasSectorChanged(handle, sector, SectorsPerFat))
        {
	   if (ReadSectors(handle, 1, fatstart+SectorsPerFat-1, sectbuf) != -1)
           {
	      if (ReadSectors(handle, 1, (i * SectorsPerFat)+fatstart+SectorsPerFat-1,
                              sectbuf1) == -1)
              {
                 FTEFree(sectbuf);
                 FTEFree(sectbuf1);
                 return FALSE;
              }
                       
              bytesinlastsector = bytesinfat % BYTESPERSECTOR;
           
              memcpy(sectbuf1, sectbuf, (size_t) bytesinlastsector);
           
	      if (WriteSectors(handle, 1, (i * SectorsPerFat)+fatstart+SectorsPerFat-1,
                               sectbuf1, WR_FAT) == -1)
              {
                 FTEFree(sectbuf);
                 FTEFree(sectbuf1);
                 return FALSE;
              }
           }
           else
           {
              FTEFree(sectbuf);
              FTEFree(sectbuf1);
              return FALSE;
           }
        }
    }

    FTEFree(sectbuf);
    FTEFree(sectbuf1);
    return result;
}


/************************************************************
**                        BackupFat                       **
*************************************************************
** This function makes all FATs equal to the first.        **
*************************************************************/

BOOL BackupFat(RDWRHandle handle)
{
    return PrivateBackupFat(handle, TRUE);
}

/************************************************************
**                    SynchronizeFATs                      **
*************************************************************
** This function propagates the changes to the FAT to all  **
** the copies.                                             **
** After calling WriteFatLabel, this function MUST be      **
** called, otherwise the system is leaking memory.         **
*************************************************************/

BOOL SynchronizeFATs(RDWRHandle handle)
{
    BOOL result;
    
    if (handle->FatLabelsChanged)
    {
       result = PrivateBackupFat(handle, FALSE);

       /* structure no longer needed, release memory */
       if (handle->FatChangeField)
       {
          DestroyBitfield(handle->FatChangeField);
          handle->FatChangeField = NULL;
       }
       handle->FatLabelsChanged = FALSE;

       return result;
    }
    return TRUE;
}
