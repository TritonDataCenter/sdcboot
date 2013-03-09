#ifndef LOW_FAT32_H_
#define LOW_FAT32_H_

int ExtendedAbsReadWrite(int drive, int nsects, unsigned long lsect,
                         void* buffer, unsigned area);

unsigned GetFAT32SectorSize(int drivenum);

#endif
