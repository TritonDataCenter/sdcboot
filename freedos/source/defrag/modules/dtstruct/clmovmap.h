#ifndef FIXED_CLUSTER_MAP_H_
#define FIXED_CLUSTER_MAP_H_

BOOL CreateFixedClusterMap(RDWRHandle handle);
void DestroyFixedClusterMap(void);
BOOL IsClusterMovable(RDWRHandle handle, CLUSTER cluster, BOOL* isMovable);

#endif
