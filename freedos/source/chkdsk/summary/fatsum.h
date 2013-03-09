#ifndef FAT_SUMMARY_H_
#define FAT_SUMMARY_H_

struct FatSummary
{
   CLUSTER totalnumberofclusters;
   CLUSTER numoffreeclusters;
   CLUSTER numofbadclusters;
};

BOOL GetFATSummary(RDWRHandle handle, struct FatSummary* info);

#endif
