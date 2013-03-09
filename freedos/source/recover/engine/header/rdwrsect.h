#ifndef RDWRSECT_H_
#define RDWRSECT_H_

typedef unsigned long SECTOR; /* Supports 2048 GB. */

struct RDWRHandleStruct
{
   int handle;                /* Drive number or file handle of image  */
                              /* file.                                 */
   int IsImageFile;           /* Remembers wether an image file is     */
                              /* intended.                             */
   SECTOR dirbegin;           /* First sector with directory entries.  */

   int FATtype;               /* Actual file system for this volume:
                                 FAT12, FAT16 or FAT32                 */
                                 
   int ReadWriteMode;         /* Wether the volume was opened for 
                                 READ or READ/WRITE                   */ 

   /* Specific cached information from boot sector,
      except for NumberOfFiles all fields indicate the calculated values
      according to the specific FAT type.                                   */
   unsigned short BytesPerSector;     /* Number of bytes per sector.        */
   unsigned char  SectorsPerCluster;  /* sectors per cluster.               */
   unsigned short ReservedSectors;    /* number of reserved sectors.        */
   unsigned char  Fats;               /* number of fats.                    */
   unsigned short NumberOfFiles;      /* number of files or directories in  */
                                      /* the root directory.                */
   unsigned long  NumberOfSectors;    /* number of sectors in the volume.   */
   unsigned char  descriptor;         /* media descriptor.                  */
   unsigned long  SectorsPerFat;      /* number of sectors per fat.         */
   unsigned short SectorsPerTrack;    /* sectors per track.                 */
   unsigned short Heads;              /* number of read/write heads.        */

   /* Only filled with relevant data on FAT32 */
   unsigned long  RootCluster;        /* Starting cluster of root directory */
   unsigned short FSInfo;             /* sector FSInfo                      */

   /* Cache for the FAT information (3 sectors) */
   int  FatCacheUsed;                 /* 1 if the cache contains valid data,
                                         0 if not. */
   SECTOR  FatCacheStart;             /* The first of the three cached sectors. */
   char FatCache[3*512];              /* The cache. */

   /* Structure to keep track of which labels in the FAT have been changed. */
   int   FatLabelsChanged;            /* Have the FAT labels changed. */
   char* FatChangeField;              /* Bitfield that keeps track of which
                                         clusters have changed, max 8Kb. */

   /* Function to read from drive or image file. */
   int (*ReadFunc)  (int handle, int nsects, SECTOR lsect, void* buffer);
   
   /* Function to write to drive or image file. */
   int (*WriteFunc) (int handle, int nsects, SECTOR lsect, void* buffer,
                     unsigned area);

#ifdef USE_SECTOR_CACHE   
   unsigned UniqueNumber;             /* Unique number used for caching    */
#endif
}; 
typedef struct RDWRHandleStruct* RDWRHandle;

int ReadSectors(RDWRHandle handle, int nsects, SECTOR lsect, void* buffer);
int WriteSectors(RDWRHandle handle, int nsects, SECTOR lsect, void* buffer,
                 unsigned area);

int InitReadWriteSectors(char* driveorfile, RDWRHandle* handle);
int InitReadSectors     (char* driveorfile, RDWRHandle* handle);

/* Reasonably stupid. */
int InitWriteSectors    (char* driveorfile, RDWRHandle* handle);

void CloseReadWriteSectors(RDWRHandle* handle);

int GetReadWriteModus(RDWRHandle handle);

int IsWorkingOnImageFile(RDWRHandle handle);

char *ReadSectorsAddress(RDWRHandle handle, unsigned long position);

/* Defines to indicate which part of the disk is being written to,
   match the value expected in SI by FAT32 absolute read/write.    */
#define WR_UNKNOWN   0x0001
#define WR_FAT       0x2001
#define WR_DIRECT    0x4001
#define WR_DATA      0x6001

/* Defines for the read/write mode */
#define READING           1
#define READINGANDWRITING 2

#endif
