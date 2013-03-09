#ifndef TRACE_PATH_H_
#define TRACE_PATH_H_

CLUSTER GetSurroundingCluster(RDWRHandle handle, struct DirectoryPosition* pos);
void AddToFrontOfString(char* string, char* front);
BOOL FindPointingPosition(RDWRHandle handle, CLUSTER parentcluster, 
                          CLUSTER cluster, struct DirectoryPosition* pos);
BOOL TraceFullPathName(RDWRHandle handle, struct DirectoryPosition* pos,
                       char* result);
BOOL GetSurroundingFile(RDWRHandle handle, CLUSTER cluster,
                           struct DirectoryPosition* result);   
BOOL TraceClusterPathName(RDWRHandle handle, CLUSTER cluster,
                          char* result);                           
BOOL TracePreviousDir(RDWRHandle handle, struct DirectoryPosition* thisPos,
                      struct DirectoryPosition* previousPos);
                          
#endif
