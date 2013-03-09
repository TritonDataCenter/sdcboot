#ifndef DIRECTORY_CHECKS_H_
#define DIRECTORY_CHECKS_H_

RETVAL CheckFileChain(RDWRHandle handle);
RETVAL BreakInvalidChain(RDWRHandle handle);

RETVAL CheckFirstClusters(RDWRHandle handle);
RETVAL DeleteInvalidFirstClusters(RDWRHandle handle);

RETVAL FindDoubleFiles(RDWRHandle handle);
RETVAL RenameDoubleFiles(RDWRHandle handle);

RETVAL FindNon0Dirs(RDWRHandle handle);
RETVAL SetDirSizeTo0(RDWRHandle handle);

RETVAL CheckDotPointer(RDWRHandle handle);
RETVAL AdjustDotPointer(RDWRHandle handle);

RETVAL CheckDotDotPointer(RDWRHandle handle);
RETVAL AdjustDotDotPointer(RDWRHandle handle);

RETVAL CheckFileLengths(RDWRHandle handle);
RETVAL AdjustFileLengths(RDWRHandle handle);

RETVAL FindNonDirectoryDots(RDWRHandle handle);
RETVAL RenameNonDirectoryDots(RDWRHandle handle);

RETVAL RootDirDOTFinder(RDWRHandle handle);
RETVAL DropRootDirDOTs(RDWRHandle handle);

RETVAL CheckLFNs(RDWRHandle handle);
RETVAL EraseInvalidLFNs(RDWRHandle handle);

RETVAL CheckDirectoryEntries(RDWRHandle handle);
RETVAL FixDirectoryEntries(RDWRHandle handle);

BOOL ScanFileChain(RDWRHandle handle, CLUSTER firstcluster,
                   struct DirectoryPosition* pos, 
                   struct DirectoryEntry* entry, 
                   char* filename,
                   BOOL fixit);
                   
BOOL InitLFNChecking(RDWRHandle handle, CLUSTER cluster, char* filename);
BOOL PreProcesLFNChecking(RDWRHandle handle, CLUSTER cluster, char* filename, BOOL fixit);
BOOL CheckLFNInDirectory(RDWRHandle handle,                   
                         struct DirectoryPosition* pos, 
                         struct DirectoryEntry* entry, 
                         char* filename,
                         BOOL fixit);
BOOL PostProcesLFNChecking(RDWRHandle handle, CLUSTER cluster, char* filename, BOOL fixit);
void DestroyLFNChecking(RDWRHandle handle, CLUSTER cluster, char* filename);

BOOL InitDblNameChecking(RDWRHandle handle, CLUSTER cluster, char* filename);
BOOL PreProcesDblNameChecking(RDWRHandle handle, CLUSTER cluster, char* filename, BOOL fixit);
BOOL CheckDirectoryForDoubles(RDWRHandle handle,                                   
                              struct DirectoryPosition* pos, 
                              struct DirectoryEntry* entry, 
                              char* filename,
                              BOOL fixit);
BOOL PostProcesDblNameChecking(RDWRHandle handle, CLUSTER cluster, char* filename, BOOL fixit);
void DestroyDblNameChecking(RDWRHandle handle, CLUSTER cluster, char* filename);

BOOL InitDblDotChecking(RDWRHandle handle, CLUSTER cluster, char* filename);
BOOL PreProcesDblDotChecking(RDWRHandle handle, CLUSTER cluster, char* filename, BOOL fixit);
BOOL CheckDirectoryForInvalidDblDots(RDWRHandle handle,                                   
                                     struct DirectoryPosition* pos, 
                                     struct DirectoryEntry* entry, 
                                     char* filename,
                                     BOOL fixit);
BOOL PostProcesDblDotChecking(RDWRHandle handle, CLUSTER cluster, char* filename, BOOL fixit);
void DestroyDblDotChecking(RDWRHandle handle, CLUSTER cluster, char* filename); 

BOOL IntelligentWalkDirectoryTree(RDWRHandle handle,
		                  int (*func) (RDWRHandle handle,
				               struct DirectoryPosition* position,
                                               struct DirectoryEntry* entry,
                                               char* filename,
				               void** structure),
		                  void** structure);      
                                  
BOOL InitInvalidCharChecking(RDWRHandle handle, CLUSTER cluster, char* filename);
BOOL PreProcesInvalidCharChecking(RDWRHandle handle, CLUSTER cluster, char* filename, BOOL fixit);
BOOL CheckEntryForInvalidChars(RDWRHandle handle,                   
                               struct DirectoryPosition* pos, 
                               struct DirectoryEntry* entry, 
                               char* filename,
                               BOOL fixit);
BOOL PostProcesInvalidCharChecking(RDWRHandle handle, CLUSTER cluster, char* filename, BOOL fixit);
void DestroyInvalidCharChecking(RDWRHandle handle, CLUSTER cluster, char* filename);                                  
                   
BOOL InitFirstClusterChecking(RDWRHandle handle, CLUSTER cluster, char* filename);
BOOL PreProcesFirstClusterChecking(RDWRHandle handle, CLUSTER cluster, char* filename, BOOL fixit);
BOOL CheckFirstCluster(RDWRHandle handle,                   
                       struct DirectoryPosition* pos, 
                       struct DirectoryEntry* entry, 
                       char* filename,
                       BOOL fixit);
BOOL PostProcesFirstClusterChecking(RDWRHandle handle, CLUSTER cluster, char* filename, BOOL fixit);
void DestroyFirstClusterChecking(RDWRHandle handle, CLUSTER cluster, char* filename);                       

#endif