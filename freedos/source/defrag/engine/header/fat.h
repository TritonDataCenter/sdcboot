#ifndef FAT_H_
#define FAT_H_

#include "../../engine/header/rdwrsect.h"

#define FAT12 12
#define FAT16 16
#define FAT32 32

/* FAT labels for FAT12. */
#define FAT12_FREE_LABEL     0x000
#define FAT12_BAD_LABEL      0xff7

#define FAT12_FREE(x)     (x == FAT12_FREE_LABEL)
#define FAT12_BAD(x)      (x == FAT12_BAD_LABEL)
#define FAT12_RESERVED(x) ((x >= 0xff0) && (x <= 0xff8))
#define FAT12_LAST(x)     ((x >= 0xff8) && (x <= 0xfff))
#define FAT12_NORMAL(x)   ((x != 0) && (x < 0xff0))

/* FAT labels for FAT16. */
#define FAT16_FREE_LABEL     0x0000
#define FAT16_BAD_LABEL      0xfff7

#define FAT16_FREE(x)     (x == FAT16_FREE_LABEL)
#define FAT16_BAD(x)      (x == FAT16_BAD_LABEL)
#define FAT16_RESERVED(x) ((x >= 0xfff0) && (x <= 0xfff8))
#define FAT16_LAST(x)     ((x >= 0xfff8) && (x <= 0xffff))
#define FAT16_NORMAL(x)   ((x != 0) && (x < 0xfff0))


/* FAT labels for FAT32 */
/* Takes the 28bit part of the cluster value that indicates the
   real cluster. */
#define FAT32_CLUSTER(x) (x & 0x0fffffffL)

#define FAT32_FREE_LABEL   0x00000000L
#define FAT32_BAD_LABEL    0x0ffffff7L
#define FAT32_LAST_LABEL   0x0fffffffL

#define FAT32_FREE(x)      (FAT32_CLUSTER(x) == FAT32_FREE_LABEL)
#define FAT32_BAD(x)       (FAT32_CLUSTER(x) == FAT32_BAD_LABEL)
#define FAT32_LAST(x)      ((FAT32_CLUSTER(x) >= 0xFFFFFF8L) && \
			    (FAT32_CLUSTER(x) <= 0xFFFFFFFL))

/* Generalised FAT labels. */
/* Notice that we do not give atention to the first two clusters in the
   FAT that form the reserved clusters. */
#define FAT_CLUSTER FAT32_CLUSTER
   
#define FAT_FREE_LABEL       0x00000000L
#define FAT_BAD_LABEL        0x0ffffff7L
#define FAT_LAST_LABEL       0x0fffffffL

#define FAT_FREE(x)     (FAT_CLUSTER(x) == FAT_FREE_LABEL)
#define FAT_BAD(x)      (FAT_CLUSTER(x) == FAT_BAD_LABEL)
#define FAT_LAST(x)     (FAT_CLUSTER(x) == FAT_LAST_LABEL)
#define FAT_NORMAL(x)   (!FAT_FREE(x) && !FAT_BAD(x) && !FAT_LAST(x))

typedef unsigned long CLUSTER;

SECTOR GetFatStart(RDWRHandle handle);
int GetFatLabelSize(RDWRHandle handle);
SECTOR GetDataAreaStart(RDWRHandle handle);


SECTOR ConvertToDataSector(RDWRHandle handle,
                           CLUSTER fatcluster);

int LinearTraverseFat(RDWRHandle handle,
                      int (*func) (RDWRHandle handle,
                                   CLUSTER label,
                                   SECTOR  datasector,
                                   void** structure),
                      void** structure);

int FileTraverseFat(RDWRHandle handle, CLUSTER startcluster,
                    int (*func) (RDWRHandle handle,
                                 CLUSTER label,
                                 SECTOR  datasector,
                                 void** structure),
                    void** structure);

int ReadFatLabel(RDWRHandle handle, CLUSTER labelnr,
                 CLUSTER* label);

int WriteFatLabel(RDWRHandle handle, CLUSTER labelnr,
                  CLUSTER label);

unsigned long GetBytesInFat(RDWRHandle handle);

CLUSTER DataSectorToCluster(RDWRHandle handle, SECTOR sector);
SECTOR  ConvertToDataSector(RDWRHandle handle, CLUSTER fatcluster);

void InvalidateFatCache(RDWRHandle handle);

BOOL IsLabelValid(RDWRHandle handle, CLUSTER label);
BOOL IsLabelValidInFile(RDWRHandle handle, CLUSTER label);

#endif
