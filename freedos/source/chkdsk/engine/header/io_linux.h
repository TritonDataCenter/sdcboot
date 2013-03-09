#ifndef IO_H_
#define IO_H_

/* Linux specific device handling */
int fs_open(char *path,int modus);
BOOL fs_read(int fd, loff_t pos,int size,void *data);
BOOL fs_write(int fd, loff_t pos,int size,void *data);
void fs_close(int fd);

#endif
