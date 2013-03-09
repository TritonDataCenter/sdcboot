#ifndef RELOC_SEQ_H_
#define RELOC_SEQ_H_

BOOL RelocateClusterSequence(RDWRHandle handle, CLUSTER source, CLUSTER destination, unsigned long length);
BOOL RelocateOverlapping(RDWRHandle handle, CLUSTER source, CLUSTER target, unsigned long length);

#endif
