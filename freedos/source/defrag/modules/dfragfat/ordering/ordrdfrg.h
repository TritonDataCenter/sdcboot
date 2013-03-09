#ifndef ORDERED_DEFRAGMENTATION_H_
#define ORDERED_DEFRAGMENTATION_H_

BOOL FilesFirstDefrag(RDWRHandle handle);
BOOL DirectoriesFirstDefrag(RDWRHandle handle);
BOOL DirectoriesWithFilesDefrag(RDWRHandle handle);

#endif
