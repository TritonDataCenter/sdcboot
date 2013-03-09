
#include "FTE.h"

static BOOL FileCountRememberer(RDWRHandle handle, struct DirectoryPosition* position, void** structure);

static unsigned long FileCount=0;

void ResetFileCount(void)
{
    FileCount = 0;
}

void IncrementFileCount(void)
{
    FileCount++;
}

unsigned long GetRememberedFileCount(void)
{
    return FileCount;
}

BOOL RememberFileCount(RDWRHandle handle)
{
    ResetFileCount();

    if (!WalkDirectoryTree(handle, FileCountRememberer, NULL))
    {
        RETURN_FTEERR(FALSE);    
    }

    return TRUE;
}

static BOOL FileCountRememberer(RDWRHandle handle, struct DirectoryPosition* position, void** structure)
{
    struct DirectoryEntry entry;

    if(structure);

    if (!GetDirectory(handle, position, &entry))
        RETURN_FTEERR(FALSE);    
    
    if ((entry.attribute & FA_LABEL) ||
        IsCurrentDir(entry)  ||
        IsPreviousDir(entry) ||
        IsLFNEntry(&entry)   ||
        IsDeletedLabel(entry))
    {
        return TRUE;
    }        

    IncrementFileCount();
    return TRUE;
}

