#ifndef VIRTUAL_FILE_SYSTEM_H_
#define VIRTUAL_FILE_SYSTEM_H_

typedef struct VirtualDirectoryEntry 
{ 
	RDWRHandle handle; 

	BOOL dirty;
	BOOL used; 
	BOOL hasCache;
	CLUSTER start; 
        // CLUSTER fat; todo optimization

        int isFragmented;   // Quick and dirty optimization, remove later!

	unsigned long length; 

	char* cache;			/* [8kb] */
	unsigned long cachedBlock; 
    
	CLUSTER lowestClusterInvolved;

} VirtualDirectoryEntry; 

typedef struct VirtualFileSpace 
{ 
	CLUSTER next; 
	char* data;		/* [BYTESPERSECTOR*sectorspercluster()-sizeof(CLUSTER)]; */
} VirtualFileSpace; 

VirtualDirectoryEntry* CreateVirtualFile(RDWRHandle handle); 

void DeleteVirtualFile(VirtualDirectoryEntry* entry); 

BOOL WriteVirtualFile(VirtualDirectoryEntry* entry, unsigned long pos, char* data, unsigned long length);

BOOL ReadVirtualFile(VirtualDirectoryEntry* entry, unsigned long pos, char* data, unsigned long length); 

BOOL GetVirtualFileSpaceUsed(RDWRHandle handle, unsigned long* spaceUsed);

BOOL EnsureClusterFree(RDWRHandle handle, CLUSTER cluster);
BOOL EnsureClustersFree(RDWRHandle handle, CLUSTER cluster, unsigned long n);

BOOL PrepareVirtualFile(VirtualDirectoryEntry* entry, unsigned long length);
BOOL GetMaxRelocatableSize(RDWRHandle handle, unsigned long* size);

#endif
