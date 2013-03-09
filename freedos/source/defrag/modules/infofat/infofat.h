#ifndef INFO_FAT_H_
#define INFO_FAT_H_

int FillDriveMap    (RDWRHandle handle);
int GetDefragFactor (RDWRHandle handle);
int MarkUnmovables  (RDWRHandle handle);
BOOL RememberFileCount(RDWRHandle handle);

void ResetFileCount(void);
void IncrementFileCount(void);

unsigned long GetRememberedFileCount(void);

#endif
