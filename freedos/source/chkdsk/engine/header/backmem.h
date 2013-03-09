#ifndef BACKUP_MEM_H_
#define BACKUP_MEM_H_

BOOL AllocateBackupMemory(unsigned size);
void FreeBackupMemory(void);

void* BackupAlloc(unsigned size);
void  BackupFree(void* block);

BOOL InBackupMemoryRange(void* block);

#endif
