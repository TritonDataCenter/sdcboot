#ifndef FS_INFO_H_
#define FS_INFO_H_

/*
	No idea wether microsoft did it on purpose, but this structure seems 
	to already be aligned to 32bit.
*/

struct FSInfoStruct
{
    unsigned long LeadSignature;
    char          reserved1[480];
    unsigned long StructSignature; 
    unsigned long FreeClusterCount;
    unsigned long FreeSearchStart;   /* Indicates where we should start 
                                        looking for free clusters.       */
    char          reserved2[12];  
    unsigned long TailSignature;
};

#ifndef __GNUC__
#if sizeof(struct FSInfoStruct) != BYTESPERSECTOR
#error Invalid FSInfo structure
#endif
#endif


BOOL ReadFSInfo(RDWRHandle handle, struct FSInfoStruct* info);
BOOL WriteFSInfo(RDWRHandle handle, struct FSInfoStruct* info);
BOOL CheckFSInfo(RDWRHandle handle, BOOL* isfaulty);

BOOL GetFreeClusterSearchStart(RDWRHandle handle, CLUSTER* startfrom); 
BOOL GetNumberOfFreeClusters(RDWRHandle handle, CLUSTER* freeclustercount);

void WriteFreeClusterStart(struct FSInfoStruct* info, CLUSTER startfrom);
void WriteFreeClusterCount(struct FSInfoStruct* info, CLUSTER count);

#define FSINFO_DONTKNOW 0xFFFFFFFFL /* Returned by GetNumberOfFreeClusters */

#define FSINFO_LEAD_SIGNATURE  0x41615252L
#define FSINFO_STRUC_SIGNATURE 0x61417272L
#define FSINFO_TRAIL_SIGNATURE 0XAA550000L

#endif
