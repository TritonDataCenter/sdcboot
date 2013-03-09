#ifndef PHYSICAL_CACHE_H_
#define PHYSICAL_CACHE_H_

unsigned InitialisePhysicalCache(void);
void ClosePhysicalCache(void);
int GetPhysicalMemType(unsigned physblock);
BOOL IsEMSCached(void);
BOOL RemapXMSBlock(int logblock, unsigned physblock);
BOOL RemapEMSBlock(int logblock, unsigned physblock);

#define CACHEBLOCKSIZE EMS_PAGE_SIZE

#define NOTEXT 0
#define XMS    1
#define EMS    2

#endif
