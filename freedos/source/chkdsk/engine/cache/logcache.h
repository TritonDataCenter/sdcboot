#ifndef LOGICAL_CACHE_H_
#define LOGICAL_CACHE_H_

struct LogicalBlockInfo
{
   char*    address;
   unsigned age;
   unsigned PhysicalBlock;
};

unsigned InitialiseLogicalCache(void);
void CloseLogicalCache(void);
char* EnsureBlockMapped(unsigned physicalblock);

#endif
