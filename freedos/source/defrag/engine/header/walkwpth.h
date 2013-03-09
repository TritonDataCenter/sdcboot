#ifndef WALK_WILDCARD_PATH_H_
#define WALK_WILDCARD_PATH_H_

BOOL WalkWildcardPath(RDWRHandle handle, char* path, CLUSTER firstcluster,
                      BOOL (*func)(RDWRHandle handle,
                                   struct DirectoryPosition* pos,
                                   void** structure), void** structure);


#endif
