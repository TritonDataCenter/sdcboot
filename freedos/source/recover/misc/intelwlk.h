#ifndef INTELIWALK_H_
#define INTELIWALK_H_

BOOL IntelligentWalkDirectoryTree(RDWRHandle handle,
		                  int (*func) (RDWRHandle handle,
				               struct DirectoryPosition* position,
                                               struct DirectoryEntry* entry,
                                               char* filename,
				               void** structure),
		                  void** structure);
											   
BOOL HasValidDots(RDWRHandle handle, CLUSTER cluster);											   


#endif
