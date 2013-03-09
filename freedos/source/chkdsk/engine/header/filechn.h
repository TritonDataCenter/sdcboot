#ifndef FILE_CHAIN_H_
#define FILE_CHAIN_H_

BOOL CreateFileChain(RDWRHandle handle, struct DirectoryPosition* pos,
                     CLUSTER *newcluster);
                     
BOOL ExtendFileChain(RDWRHandle handle, CLUSTER firstcluster,
                     CLUSTER* newcluster);
                     
BOOL AddDirectory(RDWRHandle handle, CLUSTER firstcluster,
                  struct DirectoryPosition* pos);

#endif
