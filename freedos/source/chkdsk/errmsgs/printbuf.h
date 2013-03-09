#ifndef PRINT_BUFS_H_
#define PRINT_BUFS_H_

char* GetFilePrintBuffer(void);
char* GetMessageBuffer(void);

void ShowDirectoryViolation(RDWRHandle handle,
                            struct DirectoryPosition* pos,
                            struct DirectoryEntry* entry,    /* Speed up */
                            char* message);
                            
void ShowClusterViolation(RDWRHandle, CLUSTER cluster, char* message);                            


void ShowFileNameViolation(RDWRHandle handle,
                           char* filename,
                           char* message);
                           
void ShowParentViolation(RDWRHandle handle,
                         char* filename,
                         char* message);                           

#endif