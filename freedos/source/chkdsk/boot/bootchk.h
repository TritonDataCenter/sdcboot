#ifndef BOOTCHECK_H_
#define BOOTCHECK_H_

RETVAL CheckDescriptorInFat(RDWRHandle handle);
RETVAL PlaceDescriptorInFat(RDWRHandle handle);

int DescriptorCheck(RDWRHandle handle);

RETVAL CheckFSString(RDWRHandle handle);
RETVAL CorrectFSString(RDWRHandle handle);

#endif
