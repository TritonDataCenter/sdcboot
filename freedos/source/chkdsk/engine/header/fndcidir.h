#ifndef FIND_CLUSTER_IN_DIRECTORY_H_
#define FIND_CLUSTER_IN_DIRECTORY_H_

BOOL FindClusterInDirectories(RDWRHandle handle, CLUSTER tofind, 
                              struct DirectoryPosition* result,
                              BOOL* found);
 
BOOL IndicateDirEntryMoved(CLUSTER source, CLUSTER destination);

BOOL IndicateDirClusterMoved(RDWRHandle handle, CLUSTER source, CLUSTER destination);

void DestroyDirReferedTable();

#endif
