#ifndef SECTORCACHE_H_
#define SECTORCACHE_H_

/* Init and destroy */
void InitSectorCache(void);
void CloseSectorCache(void);

/* User lock of cache */
void LockSectorCache(void);
void UnlockSectorCache(void);

/* Algorithm defined lockout */
void StartSectorCache(void);
void StopSectorCache(void);

/* Trash all cache information */
void InvalidateCache(void);

/* Get/retreive/invalidate sectors */
int CacheSector(unsigned devid, SECTOR sector, char* buffer, BOOL dirty,
                unsigned area);
int  RetreiveCachedSector(unsigned devid, SECTOR sector, char* buffer);
void UncacheSector(unsigned devid, SECTOR sector);

/* Return wether the cache is active */
BOOL CacheActive(void);

/* Total shutdown */
void CacheIsCorrupt(void);

/* Commit the write back cache */
int CommitCache(void);

#endif
