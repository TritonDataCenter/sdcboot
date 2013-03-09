#ifndef BLOCK_CACHE_H_
#define BLOCK_CACHE_H_

BOOL InitialisePerBlockCaching(void);
void ClosePerBlockCaching(void);
int CacheBlockSector(unsigned devid, SECTOR sector, char* buffer, BOOL dirty,
                      unsigned area);
int RetreiveBlockSector(unsigned devid, SECTOR sector, char* buffer);
void UncacheBlockSector(unsigned devid, SECTOR sector);
void InvalidateBlockCache(void);
int WriteBackCache(void);

#endif