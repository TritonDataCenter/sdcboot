#ifndef FTE_MEMORY_H_
#define FTE_MEMORY_H_

#ifndef __GNUC__
#ifndef _WIN32
#include <alloc.h>
#endif
#endif

#define PRE_FTE_ALLOC  1
#define POST_FTE_ALLOC 2

#define ALLOCRES int

#define TOTAL_FAIL    1
#define BACKUP_FAIL   2
#define ALLOC_SUCCESS 3

/* Memory subsystem initialisation/shutdown */
ALLOCRES AllocateFTEMemory(unsigned guaranteed, unsigned char guaranteedblocks,
                           unsigned backupbytes);
void DeallocateFTEMemory(void);

/* Generic allocation */
void* FTEAlloc(size_t bytes);
void  FTEFree(void* tofree);

/* Sectors */
SECTOR* AllocateSector(RDWRHandle handle);
SECTOR* AllocateSectors(RDWRHandle handle, int count);
void    FreeSectors(SECTOR* sector);

/* Boot */
struct BootSectorStruct* AllocateBootSector(void);
void   FreeBootSector(struct BootSectorStruct* boot);

/* Directories */
struct DirectoryEntry* AllocateDirectoryEntry(void);
void   FreeDirectoryEntry(struct DirectoryEntry* entry);

struct FSInfoStruct* AllocateFSInfo(void);
void FreeFSInfo(struct FSInfoStruct* info);

#endif
