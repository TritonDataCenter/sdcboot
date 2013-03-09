#ifndef SORT_CONFIGURATION_H_
#define SORT_CONFIGURATION_H_

struct CompareConfiguration
{
       int (*cmpfunc)(struct DirectoryEntry* e1,
		      struct DirectoryEntry* e2);
       int (*filterfunc)(int x);
};

struct ResourceConfiguration
{
       int (*getentry)(void* entries, unsigned pos, 
                       struct DirectoryEntry* result);
       int (*swapentry)(void* entries, unsigned pos1, unsigned pos2);
       int (*getslot) (void* entries, int entrynr, int slot,
                       struct DirectoryEntry* entry); 
};

void SetCompareFunction(int (*cmpfunc)(struct DirectoryEntry* e1,
		                       struct DirectoryEntry* e2));
void SetFilterFunction(int (*filterfunc)(int x));

void SetResourceConfiguration(struct ResourceConfiguration* config);

int CompareEntriesToSort(struct DirectoryEntry* e1,
			 struct DirectoryEntry* e2);
int FilterSortResult(int x);
int GetEntryToSort(void* entries, unsigned pos, struct DirectoryEntry* result);
int SwapEntryForSort(void* entries, unsigned pos1, unsigned pos2);
int GetSlotToSort(void* entries, int entrynr, int slot,
                  struct DirectoryEntry* entry);

#endif
