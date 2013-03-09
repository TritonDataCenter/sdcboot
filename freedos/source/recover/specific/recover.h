#ifndef RECOVERDISK_H_
#define RECOVERDISK_H_

void RecoverDisk(char* disk);
void RecoverFile(char* file);

#ifdef FAT_TRANSFORMATION_ENGINE

BOOL TruncateCrossLinkedFiles(RDWRHandle handle);
BOOL ConvertLostClustersToFiles(RDWRHandle handle);

BOOL IsStartOfChain(RDWRHandle handle, CLUSTER cluster);

BOOL RecoverFileChain(RDWRHandle handle, CLUSTER firstclust, unsigned long* newsize);

#endif

#endif
