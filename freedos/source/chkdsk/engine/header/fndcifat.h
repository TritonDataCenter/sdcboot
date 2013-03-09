#ifndef FIND_CLUSTER_IN_FAT_H_
#define FIND_CLUSTER_IN_FAT_H_

BOOL FindClusterInFAT(RDWRHandle handle, CLUSTER tofind, CLUSTER* result);
BOOL IndicateFatClusterMoved(CLUSTER label, CLUSTER source, CLUSTER destination);

BOOL BreakFatTableReference(RDWRHandle handle, CLUSTER label);
BOOL CreateFatTableReference(RDWRHandle handle, CLUSTER destination, CLUSTER label);

BOOL WriteFatReference(RDWRHandle handle, CLUSTER cluster, CLUSTER label);

void DestroyFatReferedTable();

#endif
