#ifndef FILESSUMMARY_H_
#define FILESSUMMARY_H_

struct FilesSummary
{
   unsigned long TotalFileCount;       /* Total number of files.        */
   unsigned long TotalSizeofFiles[2];  /* Total number of bytes used by
                                          the files.                    */
   unsigned long SizeOfAllFiles[2];    /* Number of clusters taken up by
                                          all the files. */
   unsigned long HiddenFileCount;      /* Total number of hidden files. */
   unsigned long SizeOfHiddenFiles[2]; /* Number of clusters taken up by
                                          hidden files. */
   unsigned long SystemFileCount;      /* Total number of system files. */
   unsigned long SizeOfSystemFiles[2]; /* Number of clusters taken up by
                                          system files. */
   unsigned long DirectoryCount;       /* Total number of directories. */
   unsigned long SizeOfDirectories[2]; /* Total number of clusters taken
                                          up by directories.             */
};

BOOL GetFilesSummary(RDWRHandle handle, struct FilesSummary* info);

#endif
