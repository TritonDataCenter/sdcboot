#ifndef FULLDEFRAG_H_
#define FULLDEFRAG_H_

BOOL FullyDefragmentVolume(RDWRHandle handle);

BOOL FullDefragPlace(RDWRHandle handle, CLUSTER clustertoplace,
                     CLUSTER clustertobereplaced, CLUSTER* stop);


#endif
