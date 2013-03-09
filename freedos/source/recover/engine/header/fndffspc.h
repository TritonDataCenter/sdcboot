#ifndef FIND_FIRST_FREE_SPACE_H_
#define FIND_FIRST_FREE_SPACE_H_

BOOL FindFirstFreeSpace(RDWRHandle handle, CLUSTER* space,
                        unsigned long* length);
                        
BOOL FindNextFreeSpace(RDWRHandle handle, CLUSTER* space,
                       unsigned long* length);

#endif
