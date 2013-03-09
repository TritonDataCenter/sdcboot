#ifndef DEFRAG_MAPPING_H_
#define DEFRAG_MAPPING_H_

BOOL  CreateFastSelectMap(RDWRHandle handle);
void  DestroyFastSelectMap(void);
VirtualDirectoryEntry* GetFastSelectMap(void);
void  SwapClustersInFastSelectMap(CLUSTER cluster1, CLUSTER cluster2);

BOOL  CreateNotFragmentedMap(RDWRHandle handle);
void  DestroyNotFragmentedMap(void);
VirtualDirectoryEntry* GetNotFragmentedMap(void);
void  MarkFileAsContinous(CLUSTER place, unsigned long length);
BOOL MoveFragmentBlock(CLUSTER source, CLUSTER destination, unsigned long length);

#endif