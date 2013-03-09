#ifndef LOCATE_PATH_H_
#define LOCATE_PATH_H_

BOOL LocateFilePosition(RDWRHandle handle, CLUSTER firstcluster,
                        char* filename, char* extension,
                        struct DirectoryPosition* pos);

BOOL LocatePathPosition(RDWRHandle handle, char* abspath,
                        struct DirectoryPosition* pos);

BOOL LocateRelPathPosition(RDWRHandle handle, char* relpath,
                           CLUSTER firstcluster, struct DirectoryPosition* pos);

#endif
