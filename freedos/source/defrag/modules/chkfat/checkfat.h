#ifndef CHECKFAT_H_
#define CHECKFAT_H_

/* Main check controller. */
int CheckVolume(RDWRHandle handle);

/* Checks. */
int DescriptorCheck(RDWRHandle handle);
int CheckDescriptorInFat(RDWRHandle handle);
int MultipleFatCheck(RDWRHandle handle);

#endif
