#ifndef DIRECT_H_
#define DIRECT_H_

#ifndef __GNUC__
#include <dos.h>
#endif

#include <time.h>

#include "../../engine/header/rdwrsect.h"
#include "../../engine/header/fat.h"

struct PackedDate
{
       unsigned short day:   5;
       unsigned short month: 4;
       unsigned short year:  7;
}
#ifdef __GNUC__
__attribute__((packed))
#endif
;

#ifndef __GNUC__
#ifndef _WIN32
#if sizeof(struct PackedDate) != 2
#error Wrong Packed Date Size
#endif
#endif
#endif

struct PackedTime
{
       unsigned short second: 5;
       unsigned short minute: 6;
       unsigned short hours:  5;
}
#ifdef __GNUC__
__attribute__((packed))
#endif
;

#ifndef __GNUC__
#ifndef _WIN32
#if sizeof(struct PackedTime) != 2
#error Wrong Packed Time Size
#endif
#endif
#endif

struct DirectoryEntry
{
    char              filename[8];           /* file name.                  */
    char              extension[3];          /* extension.                  */
    unsigned char     attribute;             /* file attribute.             */
    char              NTReserved;            /* reserved for Windows NT     */    
    char              MilisecondStamp;       /* Milisecond stamp at file
                                                creation.                   */
    struct PackedTime timestamp;             /* time created.               */
    struct PackedDate datestamp;             /* date created.               */
    struct PackedDate LastAccessDate;        /* Date last accessed.         */
    unsigned short    firstclustHi;          /* hi part of first cluster of
                                                file only FAT32.           */
    struct PackedTime LastWriteTime;         /* time of last write
                                                (or creation).              */
    struct PackedDate LastWriteDate;         /* date of last write
                                                (or creation).                         */
    unsigned short    firstclustLo;          /* first cluster of file.      */
    unsigned long     filesize;              /* file size.                  */
}
#ifdef __GNUC__
__attribute__((packed))
#endif
;

#ifndef __GNUC__
#ifndef _WIN32
#if sizeof(struct DirectoryEntry) != 32
#error Wrong directory entry structure
#endif
#endif
#endif

struct DirectoryPosition
{
        SECTOR sector;
        int    offset;  /* May be a value between 0 and 4096 / 32 */
};

struct LongFileNameEntry
{
       unsigned char  NameIndex;   /* LFN record sequence and flags byte */
       unsigned short Part1[5];
       unsigned char  Attributes;  /* Attributes (0Fh)                   */
       unsigned char  reserved;
       unsigned char  checksum;
       unsigned short Part2[6];
       unsigned short firstclust;  /* First cluster number
                                    (always 0000h for LFN records)      */
       unsigned short Part3[2];
}
#ifdef __GNUC__
__attribute__((packed))
#endif
;

#ifndef __GNUC__
#ifndef _WIN32
#if sizeof(struct LongFileNameEntry) != 32
#error Wrong LFN entry structure
#endif
#endif
#endif


#define LASTLABEL    0x00
#define CHARE5hLABEL 0x05
#define DOTLABEL     0x2E
#define DELETEDLABEL 0xE5

#define IsLastLabel(entry) ((entry).filename[0] == LASTLABEL)
#define IsDeletedLabel(entry) (((unsigned char) (entry).filename[0]) == (unsigned char)DELETEDLABEL)

#define IsCurrentDir(entry) (((entry).filename[0] == '.') && \
                            ((entry).filename[1] == ' '))

#define IsPreviousDir(entry) (((entry).filename[0] == '.') && \
                              ((entry).filename[1] == '.') && \
                              ((entry).filename[2] == ' '))

#define MarkEntryAsDeleted(entry) 							\
{															\
    unsigned char x = 0xe5;									\
	((unsigned char)((entry).filename[0])) = x;				\
}
	
/*
   macro implementing int IsLFNEntry (DirectoryEntry* entry);
*/

#define LFN_ATTRIBUTES (FA_RDONLY | FA_HIDDEN | FA_SYSTEM | FA_LABEL)

#define IsLFNEntry(x) ((x)->attribute == LFN_ATTRIBUTES)

#define IsFirstLFNEntry(x) (((x)->NameIndex & 0x40) != 0)

/* Use the following macro only on the first entry of a series of long
   directory entries.
*/
#define GetNrOfLFNEntries(x) ((x)->NameIndex & 0x3F)

SECTOR GetDirectoryStart(RDWRHandle handle);
BOOL ReadDirEntry(RDWRHandle handle, unsigned short index,
                  struct DirectoryEntry* entry);
BOOL WriteDirEntry(RDWRHandle handle, unsigned short index,
                   struct DirectoryEntry* entry);
BOOL GetRootDirPosition(RDWRHandle handle, unsigned short index,
                       struct DirectoryPosition* pos);
BOOL IsRootDirPosition(RDWRHandle handle, struct DirectoryPosition* pos);
CLUSTER GetFirstCluster(struct DirectoryEntry* entry);
void SetFirstCluster(CLUSTER cluster, struct DirectoryEntry* entry);

CLUSTER LocatePreviousDir(RDWRHandle handle, CLUSTER firstdircluster);


/*
    Macros to convert to/from a user printable date/time to a entry stored date/time.
*/

#define ENTRYSECOND(time) ((time.second >= 60) ? 29 : (time.second >> 1))
#define ENTRYMINUTE(time) (time.minute)
#define ENTRYHOUR(time)   (time.hours)

#define ENTRYDAY(date)    (date.day)
#define ENTRYMONTH(date)  (date.month)
#define ENTRYYEAR(date)   (date.year-1980)

#define USERSECOND(time)  (time.second << 1)
#define USERMINUTE(time)  (time.minute)
#define USERHOUR(time)    (time.hours)

#define USERDAY(date)     (date.day)
#define USERMONTH(date)   (date.month)
#define USERYEAR(date)    (date.year+1980)


unsigned long EntryLength(struct DirectoryEntry* entry);

#define DIRLEN2BYTES(x) (x << 5)

/* File attributes */
#ifndef FA_RDONLY
#define FA_RDONLY	 0x0001
#endif

#ifndef FA_HIDDEN
#define FA_HIDDEN	 0x0002
#endif 

#ifndef FA_SYSTEM
#define FA_SYSTEM	 0x0004
#endif

#ifndef FA_LABEL
#define FA_LABEL	 0x0008
#endif

#ifndef FA_DIREC
#define FA_DIREC	 0x0010
#endif

#ifndef FA_ARCH
#define FA_ARCH		 0x0020
#endif


#endif
