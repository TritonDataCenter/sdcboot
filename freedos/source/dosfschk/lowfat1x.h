#ifndef _LOWFAT1X_H
#ifdef __DJGPP__

#include "common.h"
#include <unistd.h>
#include <stdlib.h>

struct DCB {
    __u32  sector;
    __u16  number;
    __u32  bufptr;
} __attribute__((packed));
/* control block for the > 32 MB disk access with "int" 25/26h */
            
/* returns 0 if okay, -1 otherwise. Old is for < 32 MB */
/* drive numbers are 0 based here, 0 = A: ...! */
int raw_read_old(int drive, loff_t pos, size_t size, void * data);
int raw_write_old(int drive, loff_t pos, size_t size, void * data);
int raw_read_new(int drive, loff_t pos, size_t size, void * data);
int raw_write_new(int drive, loff_t pos, size_t size, void * data);

#endif
#endif
