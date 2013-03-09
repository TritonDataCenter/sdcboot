#ifdef __DJGPP__ /* DOS specific functions everywhere */
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
 */

#include <stdio.h>
#include <fcntl.h>
#include <sys/stat.h>	/* for mode definitions */
#include <string.h>	/* strlen */
#include <ctype.h>	/* toupper */
#include <unistd.h>
#include <stdlib.h>

#include "common.h"
#include "volume.h"
#include <io.h>		/* _is_remote_drive()...  */
#include <dos.h>	/* _flush_disk_cache()... */
#include "lowfat32.h"
#include "lowfat1x.h"

#define TRUE  1
#define FALSE 0

/* Defines to indicate which part of the disk is being written to,
   match the value expected in SI by FAT32 absolute read/write.    */
#define WR_UNKNOWN   0x0001
#define WR_FAT       0x2001
#define WR_DIRECT    0x4001
#define WR_DATA      0x6001

#define SECTOR unsigned long

static unsigned BytesPerSector;

static int IsImageFile;			/* Whether we are working on image files */
static int handle;			/* drive letter - 'A' or file handle */
static __u8 * buf;			/* sector buffer - just allocated once */

/* Function to read from drive or image file. */
static int (*ReadFunc)  (int handle, int nsects, SECTOR lsect, void* buffer);

/* Function to write to drive or image file. */
static int (*WriteFunc) (int handle, int nsects, SECTOR lsect, void* buffer);

/* Emulated file pointer */
static loff_t VolumePointer;


static int WriteToNoDisk(int handle, int nsects, SECTOR lsect, void* buffer);
static int ReadFromRealDisk12(int handle, int nsects, SECTOR lsect, void* buffer);
static int WriteToRealDisk12(int handle, int nsects, SECTOR lsect, void* buffer);
static int ReadFromRealDisk16(int handle, int nsects, SECTOR lsect, void* buffer);
static int WriteToRealDisk16(int handle, int nsects, SECTOR lsect, void* buffer);
static int ReadFromRealDisk32(int handle, int nsects, SECTOR lsect, void* buffer);
static int WriteToRealDisk32(int handle, int nsects, SECTOR lsect, void* buffer);
			
/**************************************************************************************
***			OpenVolume
***************************************************************************************
*** Opens a volume (drive or image file)
***************************************************************************************/

int OpenVolume(char* driveorfile, int openmode)
{
    int driveletter = -1;

    if ((strlen(driveorfile) == 2) && (driveorfile[1] == ':'))
       driveletter = toupper(driveorfile[0]) - 'A';

    if ((driveletter >= 0) && (driveletter < 26))
    {
	__u32 clusters;
	__u32 sectors;

	handle = driveletter;	/* "handle" is drive letter for real disks */
	IsImageFile = FALSE;

#ifdef _is_remote_drive	/* only for newer DJGPP versions? */
	if (_is_remote_drive(driveletter))
	{
	    fprintf(stderr, "Drive %c: is no normal disk, cannot check.\n",
        	driveletter + 'A');
	    return -1;
	}
#endif

	BytesPerSector = GetFAT32SectorSize(driveletter, &clusters, &sectors);
	if (!BytesPerSector) {
	    fprintf(stderr, "No FAT disk\n");
	    return -1;	/* no FAT disk */
	}

	if (clusters > 65525UL) {	/* too big for FAT16 */
	    ReadFunc  = ReadFromRealDisk32;
	    WriteFunc = WriteToRealDisk32;
	}
	else				/* FAT16, 32 MB+ or smaller */
	{
	    if (BytesPerSector != 512) {
	        fprintf(stderr, "Only 512 bytes per sector supported for FAT1x\n");
	        return -1;	/* we cannot use other FAT1x sector sizes yet */
	     }

	    if (sectors < 65536UL)
	    {
		ReadFunc  = ReadFromRealDisk12;	/* somehow misleading name... */
		WriteFunc = WriteToRealDisk12;  /* ...indeed MOSTLY for FAT12 */
	    }
	    else
	    {
		ReadFunc  = ReadFromRealDisk16;
		WriteFunc = WriteToRealDisk16;
	    }
#if 0
//	    WriteFunc = WriteToRealDisk32;  /* FAT1x disk write not ready yet */
#endif					   /* "int" 26 write ready: 15aug2004 */
	}
	if (openmode == O_RDONLY)
	    WriteFunc = WriteToNoDisk;
	Lock_Unlock_Drive(handle /* drive */, 1);	/* normal lock */
	Lock_Unlock_Drive(handle /* drive */, 2);	/* formatting lock */
	/* *** is formatting lock needed? For boot sector edit, yes!? *** */
    }
    else	/* if not a drive letter */
    {
	handle = open (driveorfile, openmode|O_BINARY);
	IsImageFile = TRUE;
    }

    return handle;	/* Emulate 2 for the file descriptor */
}

/**************************************************************************************
***			ReadVolume
***************************************************************************************
*** Reads from a volume, making the translation between byte oriented and sector
*** oriented logic, returns -1 on error
***************************************************************************************/

int ReadVolume(void * data, size_t size)
{
    size_t origsize = size;

    if (IsImageFile)
    {
	/* Just read from file */
	if (lseek(handle, VolumePointer, SEEK_SET) == -1L) /* DOS only has lseek */
	   return -1;

	return read(handle, data, size);
    }

    /* else: real disk */
    /* Read from real disk */
    unsigned sector = VolumePointer / BytesPerSector;
    unsigned offset = VolumePointer % BytesPerSector;
    unsigned rest   = BytesPerSector - offset;

    if (buf == NULL)
        buf = (__u8*) malloc(BytesPerSector);
    if (!buf)
	return -1;

    if (rest >= size)	/* access only in this sector */
    {
        if (ReadFunc(handle, 1, sector, buf) == -1)
	    return -1;
        memcpy(data, buf + offset, size);
    }
    else	/* access crosses sector boundary */
    {
	size_t i;
	__u8 * cdata = (__u8 *) data;
	
	if (rest)
	{
	    if (ReadFunc(handle, 1, sector, buf) == -1)
		return -1;
	
	    memcpy(data, buf + offset, rest);
	    sector++;
	}

	cdata += rest;
	size  -= rest;
	
	for (i = 0; i < size / BytesPerSector; i++)
	{
	    if (ReadFunc(handle, 1, sector, cdata) == -1)
		return -1;	

	    cdata += BytesPerSector;
	    sector++;
	}
	
	size %= BytesPerSector;
	if (size)
	{
	    if (ReadFunc(handle, 1, sector, buf) == -1)
	        return -1;
	
	    memcpy(cdata, buf, size);
	}	
    }

    return origsize;
}

/**************************************************************************************
***			WriteVolume
***************************************************************************************
*** Writes to a volume, making the translation between byte oriented and sector
*** oriented logic.
***
*** Returns -1 on error
***************************************************************************************/

int WriteVolume(void * data, size_t size)
{
    int i;
    __u8 * cdata;
    size_t origsize = size;

    if (IsImageFile)
    {
	/* Just write to file */
	if (lseek(handle, VolumePointer, SEEK_SET) == -1L) /* DOS only has lseek */
	   return -1;

	return write(handle, data, size);
    }

    /* else: real disk */
    /* Write to real disk */
    unsigned sector = VolumePointer / BytesPerSector;
    unsigned offset = VolumePointer % BytesPerSector;
    unsigned rest   = BytesPerSector - offset;

    if (buf == NULL)
        buf = (__u8*) malloc(BytesPerSector);
    if (!buf)
	return -1;

    if (rest >= size)	/* access only inside this sector */
    {
        if (ReadFunc(handle, 1, sector, buf) == -1)
	    return -1;

	memcpy(buf + offset, data, size);
	
	if (WriteFunc(handle, 1, sector, buf) == -1)
	    return -1;	
    }
    else	/* access crosses sector boundary */
    {
        cdata = (__u8 *) data;

	if (rest)
	{
	    if (ReadFunc(handle, 1, sector, buf) == -1)
		return -1;

	    memcpy(buf + offset, data, rest);

	    if (WriteFunc(handle, 1, sector, buf) == -1)
		return -1;	

	    sector++;
	}
	
	cdata += rest;
	size  -= rest;
	
	for (i = 0; i < size / BytesPerSector; i++)
	{
	    if (WriteFunc(handle, 1, sector, cdata) == -1)
	        return -1;	

	    cdata += BytesPerSector;
	    sector++;
	}

        size %= BytesPerSector;

        if (size)
	{
	    if (ReadFunc(handle, 1, sector, buf) == -1)
		return -1;

	    memcpy(buf, cdata, size);

	    if (WriteFunc(handle, 1, sector, buf) == -1)
		    return -1;
	}	
    }

    return origsize;
}

/**************************************************************************************
***			CloseVolume
***************************************************************************************
*** Closes a volume, i.e. if this is an image file, then close the file handle.
***************************************************************************************/

int CloseVolume(int dummy) /* we do not really pass the handle */
{
    if (IsImageFile) {
        sync();
	return close(handle);
    }

    Lock_Unlock_Drive(handle /* drive */, 0);	/* format unlock */
    Lock_Unlock_Drive(handle /* drive */, 0);	/* normal unlock */
    _flush_disk_cache();			/* hopefully the right place */
    return 0;
}

/**************************************************************************************
***			SeekVolume
***************************************************************************************
*** Sets the virtual file pointer
***************************************************************************************/

loff_t VolumeSeek(loff_t offset)
{
    VolumePointer = offset;
    return offset;
}

/**************************************************************************************
***			IsWorkingOnImageFile
***************************************************************************************
*** Returns wether we are dealing with an image file
***************************************************************************************/

int IsWorkingOnImageFile()
{
    return IsImageFile;
}

/**************************************************************************************
***			GetVolumeHandle
***************************************************************************************
*** Returns the volume handle. Or returns the drive, 0 = A:, if a real disk!
***************************************************************************************/

int GetVolumeHandle()
{
    return handle;
}


/* Real disk sector modifiers. */
/*
   There are different read/write functions:
   - ReadFromRealDisk12, WriteToRealDisk12 are used for FAT12 and FAT16
     volumes <= 32 MB
   - ReadFromRealDisk16, WriteToRealDisk16 are used for FAT16
     volumes > 32 MB
   - ReadFromRealDisk32, WriteToRealDisk32 are used for systems supporting the FAT32 API,
     (like FreeDOS FAT32 enabled kernels)

   Notice also that, despite the names absread/abswrite work independent
   of the file system used (i.e. the naming here is a litle unfortunate)
*/

static int WriteToNoDisk(int drive, int nsects, SECTOR lsect, void* buffer)
{
  if (buffer != NULL)
      fprintf(stderr, "Writes to %c: disabled: %d sectors from %lu on\n",
          drive + 'A', nsects, lsect);
  return -1;
}

static int ReadFromRealDisk12(int drive, int nsects, SECTOR lsect, void* buffer)
{
  return raw_read_old(drive, lsect << 9, nsects << 9, buffer);
}

static int WriteToRealDisk12(int drive, int nsects, SECTOR lsect, void* buffer)
{
  return raw_write_old(drive, lsect << 9, nsects << 9, buffer);
}

static int ReadFromRealDisk16(int drive, int nsects, SECTOR lsect, void* buffer)
{
  return raw_read_new(drive, lsect << 9, nsects << 9, buffer);
}

static int WriteToRealDisk16(int drive, int nsects, SECTOR lsect, void* buffer)
{
  return raw_write_new(drive, lsect << 9, nsects << 9, buffer);
}

static int ReadFromRealDisk32(int drive, int nsects, SECTOR lsect, void* buffer)
{
   /* Don't use DISKLIB here (it uses ioctl for FAT12/16 which according to
      microsoft docs does not work!) */

   return ExtendedAbsReadWrite(drive+1, nsects, lsect, buffer, 0, BytesPerSector);
}

static int WriteToRealDisk32(int drive, int nsects, SECTOR lsect, void* buffer)
{
   unsigned area = WR_UNKNOWN;	/* mark as write (not yet telling which data type) */

   return ExtendedAbsReadWrite(drive+1, nsects, lsect, buffer, area, BytesPerSector);
}


#endif /* DJGPP only */
