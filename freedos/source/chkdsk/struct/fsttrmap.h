#ifndef FAST_TREE_MAP
#define FAST_TREE_MAP

BOOL CreateFastTreeMap(RDWRHandle handle);
BOOL CompressFastTreeMap(RDWRHandle handle);
BOOL MarkDirectoryEntry(RDWRHandle handle, struct DirectoryPosition* pos);
void DestroyFastTreeMap(void);
BOOL FastWalkDirectoryTree(RDWRHandle handle,
                           BOOL (*func)(RDWRHandle handle, 
                                        struct DirectoryPosition* pos,
                                        struct DirectoryEntry* entry,
                                        void** structure), 
                           void** structure);

void IndicateTreeIncomplete(void);
BOOL IsTreeIncomplete(void);                           
                           
#endif