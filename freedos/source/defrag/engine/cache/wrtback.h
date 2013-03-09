#ifndef WRITE_BACK_CACHE_H_
#define WRITE_BACK_CACHE_H_

void InstallWriteFunction(unsigned devid, 
                          int (*write)(int handle, int nsects, SECTOR lsect,
                                       void* buffer, unsigned area),
                          int handle);
                          
void UninstallWriteFunction(unsigned devid);   

int WriteBackSectors(unsigned devid, int nsects, SECTOR lsect, 
                     void* buffer, unsigned area);                       

#endif