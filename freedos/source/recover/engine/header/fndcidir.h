#ifndef FIND_CLUSTER_IN_DIRECTORY_H_
#define FIND_CLUSTER_IN_DIRECTORY_H_

BOOL FindClusterInDirectories(RDWRHandle handle, CLUSTER tofind, 
                              struct DirectoryPosition* result,
                              BOOL* found);
                           
#endif
