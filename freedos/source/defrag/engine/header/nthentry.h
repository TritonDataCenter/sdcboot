#ifndef NTH_ENTRY_H_
#define NTH_ENTRY_H_

BOOL GetNthDirectoryPosition(RDWRHandle handle, CLUSTER cluster,
                             unsigned long n, struct DirectoryPosition* result);

BOOL GetNthDirectorySector(RDWRHandle handle, CLUSTER cluster, 
                               unsigned long n, char* sector);

BOOL GetNthSubDirectoryPosition(RDWRHandle handle, CLUSTER cluster, 
                                unsigned long n, 
                                struct DirectoryPosition* result);   
                            
#endif
