#ifndef VOLUME_H_
#define VOLUME_H_

#include <unistd.h>	/* size_t ...!? */
#include "common.h"	/* e.g. for typedefs */

int OpenVolume(char* driveorfile, int openmode);
int ReadVolume(void * data, size_t size);
int WriteVolume(void * data, size_t size);
int CloseVolume(int dummy);
loff_t VolumeSeek(loff_t offset);
int IsWorkingOnImageFile(void);
int GetVolumeHandle(void);

/* from lock.c */
void Lock_Unlock_Drive(int drive, int lock); /* 0 based drive number 0 = A: */
/* not yet available: */
void Enable_Disk_Access(int drive); /* DOS 4.0+ drive access flag / locking */

#endif
