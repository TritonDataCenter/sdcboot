#ifndef FAT_CHECKS_H_
#define FAT_CHECKS_H_

int MultipleFatCheck(RDWRHandle handle);

RETVAL CheckFatLabelValidity(RDWRHandle handle);
RETVAL MakeFatLabelsValid(RDWRHandle handle);

RETVAL CheckFilesForLoops(RDWRHandle handle);
RETVAL TruncateLoopingFiles(RDWRHandle handle);

RETVAL FindCrossLinkedFiles(RDWRHandle handle);
RETVAL SplitCrossLinkedFiles(RDWRHandle handle);

RETVAL FindLostClusters(RDWRHandle handle);
RETVAL ConvertLostClustersToFiles(RDWRHandle handle);


#endif
