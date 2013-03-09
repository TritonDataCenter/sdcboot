#ifndef PHYSICAL_CACHE_H_
#define PHYSICAL_CACHE_H_

unsigned long InitialisePhysicalCache(void);
void ClosePhysicalCache(void);
int GetPhysicalMemType(unsigned long physblock);
BOOL IsEMSCached(void);
BOOL RemapXMSBlock(int logblock, unsigned long physblock);
BOOL RemapEMSBlock(int logblock, unsigned long physblock);

#define CACHEBLOCKSIZE EMS_PAGE_SIZE

#define NOTEXT 0
#define XMS    1
#define EMS    2

#endif
