#ifndef WALKTREE_H_
#define WALKTREE_H_

int WalkDirectoryTree(RDWRHandle handle,
                      int (*func) (RDWRHandle handle,
                                   struct DirectoryPosition* position,
                                   void** structure),
                      void** structure);

#endif
