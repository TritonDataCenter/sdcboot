#ifndef VOLUME_HANDLE_H_
#define VOLUME_HANDLE_H_

BOOL InstallOptimizationDrive(char* drive);
void RejectOptimizationDrive(void);
RDWRHandle GetCurrentVolumeHandle(void);

#endif