#ifndef FREESPACE_H_
#define FREESPACE_H_

BOOL FindFirstFreeSpace(RDWRHandle handle, CLUSTER* space,
                        unsigned long* length);

BOOL FindFirstFittingFreeSpace(RDWRHandle handle, 
                               unsigned long size,
                               CLUSTER* freespace);
			       
BOOL FindNextFreeSpace(RDWRHandle handle, CLUSTER* space,
                       unsigned long* length);			       
		       
BOOL FindLastFreeCluster(RDWRHandle handle, CLUSTER* result);		       
        
BOOL GetFreeDiskSpace(RDWRHandle handle, unsigned long* length);

BOOL GetPreviousFreeCluster(RDWRHandle handle, CLUSTER cluster, CLUSTER* previous);

BOOL CalculateFreeSpace(RDWRHandle handle, CLUSTER start, unsigned long* length);

#endif
