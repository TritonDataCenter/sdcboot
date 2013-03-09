#ifndef RELOCATE_CLUSTER_H_
#define RELOCATE_CLUSTER_H_

BOOL RelocateCluster(RDWRHandle handle, CLUSTER source, CLUSTER destination);

BOOL BreakFatMapReference(RDWRHandle handle, CLUSTER label);
BOOL CreateFatMapReference(RDWRHandle handle, CLUSTER label);
BOOL SwapFatMapReference(RDWRHandle handle, CLUSTER source, CLUSTER destination);

void DestroyFatReferedMap();

#endif
