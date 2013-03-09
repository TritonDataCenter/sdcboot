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
#include "fteerr.h"

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
   BOOL retVal = TRUE;
  
   if (fatlabelsize == FAT32)
   {
      BackupSector = GetFAT32BackupBootSector(handle);
      if (!BackupSector)
      {
         RETURN_FTEERR(FALSE);
      }       
              
      boot = AllocateBootSector();
      if (!boot) RETURN_FTEERR(FALSE);
   
      if (!ReadBootSector(handle, boot) ||
	  (WriteSectors(handle, 1, BackupSector, (void*) boot, WR_UNKNOWN)
                                                                     == -1))
      {
	  RETURN_FTEERR(FALSE);
      }    
      
      FreeBootSector(boot);
   }     

   RETURN_FTEERR(retVal);   
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
      BackupSector = GetFAT32BackupBootSector(handle);
      if (!BackupSector)
      {
         RETURN_FTEERR(FAIL);
      }       
       
      boot1 = AllocateBootSector();
      if (!boot1) RETURN_FTEERR(FAIL);
   
      boot2 = AllocateBootSector();
      if (!boot2)
      {         
         FreeBootSector(boot1);
         RETURN_FTEERR(FAIL);
      }

      if (!ReadBootSector(handle, boot1) == -1)
      {
         FreeBootSector(boot1);
	 RETURN_FTEERR(FAIL);
      }      
               
      if (!ReadSectors(handle, 1, BackupSector, (void*) boot2) == -1) 
      {
         FreeBootSector(boot1);
         FreeBootSector(boot2);
         RETURN_FTEERR(FAIL);
      }    
      
      retval = (memcmp(boot1, boot2, BYTESPERSECTOR) == 0);
      
      FreeBootSector(boot1);
      FreeBootSector(boot2);
      
      return retval;
   }        

   RETURN_FTEERR((fatlabelsize) ? TRUE : FAIL);    
}

/************************************************************
**                      HasSectorChanged                   **
*************************************************************
** This function returns a certain sector has changed.     **
** If we don't know say that it has changed.               **
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
    if (!bytesinfat) RETURN_FTEERR(FALSE);

    fatstart = GetFatStart(handle);
    if (!fatstart) RETURN_FTEERR(FALSE);
    
    NumberOfFats = GetNumberOfFats(handle);
    if (!NumberOfFats) RETURN_FTEERR(FALSE);
    
    SectorsPerFat = GetSectorsPerFat(handle);
    if (!SectorsPerFat) RETURN_FTEERR(FALSE);

    sectbuf  = (char*) FTEAlloc(BYTESPERSECTOR);
    if (!sectbuf) RETURN_FTEERR(FALSE);

    sectbuf1 = (char*) FTEAlloc(BYTESPERSECTOR);
    if (!sectbuf1)
    {
       FTEFree(sectbuf);
       RETURN_FTEERR(FALSE);
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
		     RETURN_FTEERR(FALSE);
                  }
               }
               else
               {
                  FTEFree(sectbuf);
                  FTEFree(sectbuf1);
		  RETURN_FTEERR(FALSE); 
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
                 RETURN_FTEERR(FALSE);
              }
                       
              bytesinlastsector = bytesinfat % BYTESPERSECTOR;
           
              memcpy(sectbuf1, sectbuf, (size_t) bytesinlastsector);
           
	      if (WriteSectors(handle, 1, (i * SectorsPerFat)+fatstart+SectorsPerFat-1,
                               sectbuf1, WR_FAT) == -1)
              {
                 FTEFree(sectbuf);
                 FTEFree(sectbuf1);
		 RETURN_FTEERR(FALSE); 
              }
           }
           else
           {
              FTEFree(sectbuf);
              FTEFree(sectbuf1);
	      RETURN_FTEERR(FALSE); 
           }
        }
    }

    FTEFree(sectbuf);
    FTEFree(sectbuf1);
    RETURN_FTEERR(result);
}


/************************************************************
**                        BackupFat                       **
*************************************************************
** This function makes all FATs equal to the first.        **
*************************************************************/

BOOL BackupFat(RDWRHandle handle)
{ 
    BOOL retVal;
    
    retVal = PrivateBackupFat(handle, TRUE);
    RETURN_FTEERR(retVal);
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

       RETURN_FTEERR(result);
    }
    return TRUE;
}
