#ifndef LOW_CHDIR_H_
#define LOW_CHDIR_H_

struct DirectoryPosition low_getcwd(void);

int low_chdir(RDWRHandle handle, char* path,
              CLUSTER* fatcluster, struct DirectoryPosition* pos);

int ChangeDirectory(RDWRHandle handle, char* dir, CLUSTER* fatcluster,
                    struct DirectoryPosition* pos);


#endif
