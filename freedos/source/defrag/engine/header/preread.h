#ifndef PREREAD_H_
#define PREREAD_H_

BOOL PreReadClusterSequence(RDWRHandle handle, CLUSTER start,
                            unsigned long length);
  
BOOL PreReadClusterChain(RDWRHandle handle, CLUSTER start);                          
                            
#endif
