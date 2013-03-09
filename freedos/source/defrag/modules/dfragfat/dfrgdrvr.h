#ifndef DEFRAGMENTATION_DRIVER_H_
#define DEFRAGMENTATION_DRIVER_H_

BOOL DefragmentVolume(RDWRHandle handle,
                      BOOL (*select)(RDWRHandle handle,
                                     CLUSTER startfrom,
                                     CLUSTER* clustertoplace),
                      BOOL (*place)(RDWRHandle handle,
                                    CLUSTER clustertoplace,
                                    CLUSTER startfrom,
                                    CLUSTER* wentto));

#endif