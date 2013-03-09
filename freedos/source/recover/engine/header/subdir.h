#ifndef SUBDIR_H_
#define SUBDIR_H_

int TraverseSubdir(RDWRHandle handle, CLUSTER fatcluster,
		   int (*func) (RDWRHandle handle,
		                struct DirectoryPosition* pos,
				void** buffer),
		   void** buffer, int exact);

int TraverseRootDir(RDWRHandle handle,
               	    int (*func) (RDWRHandle handle,
               	                 struct DirectoryPosition* pos,
	         		 void** buffer),
                    void** buffer, int exact);

int GetDirectory(RDWRHandle handle,
                 struct DirectoryPosition* position,
		 struct DirectoryEntry* direct);

int DirectoryEquals(struct DirectoryPosition* pos1,
		    struct DirectoryPosition* pos2);

int WriteDirectory(RDWRHandle handle, struct DirectoryPosition* pos,
		   struct DirectoryEntry* direct);

#endif
