#ifndef BOOT_H_
#define BOOT_H_

struct FAT32Specific
{ 
   unsigned long  SectorsPerFat;  /* Sectors per FAT (32 bit)                 */
   unsigned short ExtendedFlags;  /* Bits 0-3 -- Zero-based number of active FAT. 
                                                 Only valid if mirroring is disabled.
                                     Bits 4-6 -- Reserved.    
                                     Bit 7 -- 0 means the FAT is mirrored at runtime into all FATs.
                                           -- 1 means only one FAT is active; it is the one referenced
                                              in bits 0-3.
                                     Bits 8-15 -- Reserved.*/
   unsigned short FSVersion;      /* File system version, MUST be 0.  */
   unsigned long  RootCluster;    /* First cluster of ROOT directory. */                                    
   unsigned short FSInfo;         /* Sector number of FSINFO structure in the
                                     reserved area of the FAT32 volume. Usually 1.*/
   unsigned short BackupBoot;     /* Sector of boot sector backup         */
   unsigned char  Reserved[12];   /* Should be all 0's                    */
   unsigned char DriveNumber;     /* BIOS drive number                        */
   unsigned char Reserved1;       /* Used by Windows NT (must be 0)           */
   unsigned char Signature;       /* Indicates wether the following fields 
                                     are present                              */  
   unsigned long VolumeID;        /* Disk serial number                       */
   unsigned char VolumeLabel[11]; /* Volume label                             */
   unsigned char FSType[8];       /* Informational: "FAT12", "FAT16", "FAT32" */
                                  /* Has nothing to do with FAT type
                                     determination                            */
   unsigned char UnUsed[420];
   unsigned short LastTwoBytes;   /* Value of the last two bytes if
                                     BYTESPERSECTOR == 512                   */
} 
#ifdef __GNUC__
__attribute__((packed))
#endif
;

#ifndef __GNUC__
#ifndef _WIN32
#if sizeof(struct FAT32Specific) != 476
#error Wrong struct FAT32Specific
#endif
#endif
#endif

struct FAT1216Specific
{
   unsigned char DriveNumber;    /* BIOS drive number                        */
   unsigned char Reserved;       /* Used by Windows NT (must be 0)           */
   unsigned char Signature;      /* Indicates wether the following fields 
                                    are present                              */  
   unsigned long VolumeID;       /* Disk serial number                       */
   unsigned char VolumeLabel[11];/* Volume label                             */
   unsigned char FSType[8];      /* Informational: "FAT12", "FAT16", "FAT32" */       
   
   unsigned char UnUsed[448];

   unsigned short LastTwoBytes;  /* Value of the last two bytes if
                                    BYTESPERSECTOR == 512                   */
} 
#ifdef __GNUC__
__attribute__((packed))
#endif
;

#ifndef __GNUC__
#ifndef _WIN32
#if sizeof(struct FAT1216Specific) != 476
#error Wrong struct FAT1216Specific
#endif
#endif
#endif

union FATSpecific
{
   struct FAT32Specific spc32;
   struct FAT1216Specific spc1216;
} 
#ifdef __GNUC__
__attribute__((packed))
#endif
;

struct BootSectorStruct
{
   char     Jump[3];                 /* Jump instruction in boot routine. */
   char     Identification[8];       /* Identification code.              */
   unsigned short BytesPerSector;    /* bytes per sector.                 */
   unsigned char  SectorsPerCluster; /* sectors per cluster.              */
   unsigned short ReservedSectors;   /* number of reserved sectors.       */
   unsigned char  Fats;              /* number of fats.                   */
   unsigned short NumberOfFiles;     /* number of files or directories in */
                                     /* the root directory.               */
   unsigned short NumberOfSectors;   /* number of sectors in the volume.  */
   unsigned char  descriptor;        /* media descriptor.                 */
   unsigned short SectorsPerFat;     /* number of sectors per fat.        */
   unsigned short SectorsPerTrack;   /* sectors per track.                */
   unsigned short Heads;             /* number of read/write heads.       */
   unsigned long  HiddenSectors;     /* number of hidden sectors in the 
                                        partition table                   */
   unsigned long  NumberOfSectors32; /* Number of sectors if the total
                                        number of sectors > 0xffff, or
                                        if this is FAT 32                 */
   union FATSpecific fs;             /* Fields that are different for
                                        FAT 12/16 and FAT32               */
} 
#ifdef __GNUC__
__attribute__((packed))
#endif
;

#ifndef __GNUC__
#ifndef _WIN32
#if sizeof(struct BootSectorStruct) != 512
#error WRONG BOOT SECTOR STRUCT LENGTH
#endif
#endif
#endif

BOOL ReadBootSector(RDWRHandle handle, struct BootSectorStruct* buffer);
BOOL WriteBootSector(RDWRHandle handle, struct BootSectorStruct* buffer);

/* Only FAT12/16 */
unsigned short GetNumberOfRootEntries(RDWRHandle handle); 

/* Return the right fields looking at what type of FAT is realy used. */
unsigned long  GetSectorsPerCluster(RDWRHandle handle);
unsigned short GetReservedSectors(RDWRHandle handle);
unsigned char  GetNumberOfFats(RDWRHandle handle);
unsigned char  GetMediaDescriptor(RDWRHandle handle);
unsigned long  GetNumberOfSectors(RDWRHandle handle);
unsigned long  GetSectorsPerFat(RDWRHandle handle);
unsigned short GetSectorsPerTrack(RDWRHandle handle);
unsigned short GetReadWriteHeads(RDWRHandle handle);
unsigned long  GetClustersInDataArea(RDWRHandle handle);
unsigned long  GetLabelsInFat(RDWRHandle handle);
unsigned char  GetBiosDriveNumber(RDWRHandle handle);

/* Returns wether the signature field is equal to 0x29 */
BOOL IsVolumeDataFilled(RDWRHandle handle);
/* Returns wether the signature field is equal to 0x80 */
BOOL IsVolumeDataFilled_NT(RDWRHandle handle);

unsigned long GetDiskSerialNumber(RDWRHandle handle);
BOOL GetBPBVolumeLabel(RDWRHandle handle, char* label);
BOOL GetBPBFileSystemString(RDWRHandle handle, char* label);

#define VOLUME_LABEL_LENGTH  11
#define FILESYS_LABEL_LENGTH  8

/* FAT32 specific, only returns relevant data for FAT32 */
unsigned short GetFAT32Version(RDWRHandle handle);
unsigned long  GetFAT32RootCluster(RDWRHandle handle);
unsigned short GetFSInfoSector(RDWRHandle handle);
unsigned short GetFAT32BackupBootSector(RDWRHandle handle);

int DetermineFATType(struct BootSectorStruct* boot);

/*
   The following functions write boot sector info to a BootSectorStruct.

   Notice that they do not write to the drive directly. This is in order
   not to end up with a faulty boot sector on disk with only some of the
   fields in the boot sector changed (might happen if the drive controller
   decides not to work any more, all information is written at once or not
   at all).

   The changed boot sector can be written to disk with WriteBootSector()

   These functions do however fill in the structure correctly depending on
   the FAT type found.
*/

/* Only FAT12/16! */
void WriteNumberOfRootEntries(struct BootSectorStruct* boot, unsigned short numentries);

void WriteSectorsPerCluster(struct BootSectorStruct* boot, unsigned char sectorspercluster);
void WriteReservedSectors(struct BootSectorStruct* boot, unsigned short reserved);
void WriteNumberOfFats(struct BootSectorStruct* boot, unsigned char numberoffats);
void WriteMediaDescriptor(struct BootSectorStruct* boot, unsigned char descriptor);
void WriteNumberOfSectors(struct BootSectorStruct* boot, unsigned long count);
void WriteSectorsPerFat(struct BootSectorStruct* boot, unsigned long sectorsperfat);
void WriteSectorsPerTrack(struct BootSectorStruct* boot, unsigned short sectorspertrack);
void WriteReadWriteHeads(struct BootSectorStruct* boot, unsigned short heads);
void WriteBiosDriveNumber(struct BootSectorStruct* boot, unsigned char drivenumber);

void IndicateVolumeDataFilled_NT(struct BootSectorStruct* boot);
void IndicateVolumeDataFilled(struct BootSectorStruct* boot);

void WriteDiskSerialNumber(struct BootSectorStruct* boot,
                           unsigned long serialnumber);

unsigned long CalculateNewSerialNumber(void);

void WriteBPBVolumeLabel(struct BootSectorStruct* boot, char* label);
void WriteBPBFileSystemString(struct BootSectorStruct* boot, char* label);

/* Only FAT32! */
void WriteFAT32Version(struct BootSectorStruct* boot, unsigned short version);
void WriteFAT32RootCluster(struct BootSectorStruct* boot, unsigned long RootCluster);
void WriteFSInfoSector(struct BootSectorStruct* boot, unsigned short sector);
void WriteFAT32BackupBootSector(struct BootSectorStruct* boot, unsigned short backupsect);

#define EXTENDED_BOOT_SIGNATURE 0x29 /* indicates that the following three 
                                        fields in the boot sector are present.*/
#define EXTENDED_BOOT_SIGNATURE_NT 0X80 /* the same on NT? */
                                        
#define CHECK_LAST_TWO_BYTES(bootsect)                          \
        ((((unsigned char*)bootsect)[510] == 0x55) &&           \
         (((unsigned char*)bootsect)[511] == 0xAA))                     
                                        
#endif
