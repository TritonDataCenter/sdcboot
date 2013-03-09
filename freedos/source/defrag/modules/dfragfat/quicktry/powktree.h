#ifndef POST_ORDER_WALKTREE_H_
#define POST_ORDER_WALKTREE_H_

BOOL PostOrderWalkDirectoryTree(RDWRHandle handle,
		                int (*func) (RDWRHandle handle,
			                     struct DirectoryPosition* position,
				             void** structure),
		                void** structure);

#endif
