#ifndef SORT_FAT_FUNCTIONS_H_
#define SORT_FAT_FUNCTIONS_H_

/* sorttree.h */
int SortDirectoryTree(RDWRHandle handle);

/* srtentrs.h */
int SortEntries(RDWRHandle handle, CLUSTER firstcluster);

int SortSubdir(RDWRHandle handle,
	       struct DirectoryPosition* pos,
	       void** buffer);
               
/* ssortdir.c */
int SelectionSortEntries(void* entries, int amountofentries);

/* cmpfncs.c */
int CompareNames(struct DirectoryEntry* entry1,
		 struct DirectoryEntry* entry2);
                 
int CompareExtension(struct DirectoryEntry* entry1,
		     struct DirectoryEntry* entry2);

int CompareSize(struct DirectoryEntry* entry1,
		struct DirectoryEntry* entry2);

int CompareDateTime(struct DirectoryEntry* entry1,
		    struct DirectoryEntry* entry2);

int AscendingFilter (int x);
int DescendingFilter(int x);

/* dskcfg\dsksort.c */
BOOL DiskSortEntries(RDWRHandle handle, CLUSTER cluster);

/* memcfg\memsort.c */
void MemorySortEntries(struct DirectoryEntry* entries,
                       int amofentries);

/* memcfg\rdentrs.c */
int ReadEntriesToSort(RDWRHandle handle, CLUSTER cluster,
                      struct DirectoryEntry* entries, CLUSTER* clusters);
                      
/* memcfg\wrentrs.c */
int WriteSortedEntries(RDWRHandle handle, 
                       struct DirectoryEntry* entries,
                       CLUSTER* clusters,
                       int amofentries);
                       
#endif 
