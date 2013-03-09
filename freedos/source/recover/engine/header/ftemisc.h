#ifndef FTE_MISC_H_
#define FTE_MISC_H_

/* Bufshift.c */
void RotateBufRight(char* begin, char* end, unsigned n);
void RotateBufLeft(char* begin, char* end, int n);
void SwapBuffer(char* p1, char* p2, unsigned len);

/* Entshift.c */
BOOL RotateEntriesRight(RDWRHandle handle, CLUSTER dirclust, 
                        unsigned long begin, unsigned long end,
                        unsigned long n);

BOOL RotateEntriesLeft(RDWRHandle handle, CLUSTER dirclust, 
                       unsigned long begin, unsigned long end,
                       unsigned long n);

BOOL SwapBasicEntries(RDWRHandle handle, 
                      struct DirectoryPosition* pos1,
                      struct DirectoryPosition* pos2);

BOOL SwapLFNEntries(RDWRHandle handle,
                    CLUSTER cluster,
                    unsigned long begin1,
                    unsigned long begin2);

#endif
