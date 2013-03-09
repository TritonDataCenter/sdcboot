#ifndef BACKUP_STRUCTURES_H_
#define BACKUP_STRUCTURES_H_

BOOL BackupBoot(RDWRHandle handle);
BOOL BackupFat(RDWRHandle handle);
BOOL SynchronizeFATs(RDWRHandle handle);

BOOL MultipleBootCheck(RDWRHandle handle);

#endif

