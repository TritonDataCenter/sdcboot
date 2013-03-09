#ifndef LOGICAL_CACHE_H_
#define LOGICAL_CACHE_H_

unsigned long InitialiseLogicalCache(void);
void CloseLogicalCache(void);

char* GetLogicalBlockAddress(int logicalblock);
unsigned long GetMappedPhysicalBlock(int logicalblock);
char* EnsureBlockMapped(unsigned long physicalblock);

#endif
