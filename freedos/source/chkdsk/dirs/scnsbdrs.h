#ifndef SCAN_SUBDIRECTORIES_H_
#define SCAN_SUBDIRECTORIES_H_

struct ScanDirFunction
{
     BOOL (*Init)(RDWRHandle handle, CLUSTER cluster, char* filename);
     BOOL (*PreProcess)(RDWRHandle handle, CLUSTER cluster, char* filename, BOOL fixit);
     BOOL (*Check)(RDWRHandle handle, 
                   struct DirectoryPosition* dirpos, 
                   struct DirectoryEntry* direct, 
                   char* filename,
                   BOOL fixit);
     BOOL (*PostProcess)(RDWRHandle handle, CLUSTER cluster, char* filename, BOOL fixit);
     void (*Destroy)(RDWRHandle handle, CLUSTER cluster, char* filename);
     
#ifdef ENABLE_LOGGING
     char* TestName; 
#endif
};

BOOL InitClusterEntryChecking(RDWRHandle handle, CLUSTER cluster, char* filename);
BOOL PreProcessClusterEntryChecking(RDWRHandle handle, CLUSTER cluster, char* filename, BOOL fixit);
BOOL CheckEntriesInCluster(RDWRHandle handle, CLUSTER cluster, char* filename, BOOL fixit);
BOOL PostProcessClusterEntryChecking(RDWRHandle handle, CLUSTER cluster, char* filename, BOOL fixit);
void DestroyClusterEntryChecking(RDWRHandle handle, CLUSTER cluster, char* filename);

BOOL PerformRootDirChecks(RDWRHandle handle, BOOL fixit);

BOOL PerformEntryChecks(RDWRHandle handle, 
                        struct DirectoryPosition* dirpos, 
                        struct DirectoryEntry* direct, 
                        char* filename,
                        BOOL fixit);

#endif