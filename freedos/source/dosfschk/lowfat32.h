#ifndef LOW_FAT32_H_
#define LOW_FAT32_H_

/* Implementation of some FAT32 API functions */
/* area 0 means read, odd area means write */
int ExtendedAbsReadWrite(int drive, int nsects, unsigned long lsect,
			 void* buffer, unsigned area, unsigned BytesPerSector);

/* returns sector size and cluster / sector count (int 21.7303 or int 21.36) */
unsigned GetFAT32SectorSize(int drivenum, __u32 * clusters, __u32 * sectors);

#endif
