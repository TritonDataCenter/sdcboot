#ifndef RELOCATE_FILE_H_
#define RELOCATE_FILE_H_

BOOL RelocateKnownFile(RDWRHandle handle, 
                       struct DirectoryPosition* pos,
                       struct DirectoryEntry* entry,
                       CLUSTER destination); 

#endif
