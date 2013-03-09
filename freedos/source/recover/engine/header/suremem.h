#ifndef GUARANTEEDMEMORY_H_
#define GUARANTEEDMEMORY_H_

ALLOCRES AllocateGuaranteedMemory(unsigned guaranteed,
                                  unsigned char guaranteedblocks);
                              
int   AllocateGuaranteedBlock(unsigned length);
void* LockGuaranteedBlock(int handle);
void  UnlockGuaranteedBlock(void* memory);
void  FreeGuaranteedBlock(int handle);
void  FreeGuaranteedMemory(void);

unsigned char CountGuaranteedBlocks(void);
unsigned char CountFreeGuaranteedBlocks(void);

#endif
