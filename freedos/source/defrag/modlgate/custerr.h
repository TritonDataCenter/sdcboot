#ifndef CUSTOM_ERRORS
#define CUSTOM_ERRORS

#define CUSTOMOK                        0
#define HANDLENOTINITIALISED            1
#define FAT32NOTSUPPORTED               2
#define DISKCHECKINGFAILED              3
#define WRONG_LABELSINFAT               4    
#define WRONG_LABEL_IN_FAT              5
#define CLUSTER_RETRIEVE_ERROR          6
#define DRAW_DRIVEMAP_ERROR             7
#define FILL_DRIVE_MAP_ERROR            8
#define GET_DRIVE_SIZE_FAILED           9
#define VFS_ALLOC_FAILED                10
#define VFS_GET_FAILED                  11
#define VFS_SET_FAILED                  12
#define GET_FIXEDFILE_FAILED            13
#define GET_FATTYPE_FAILED              14
#define GET_FAT32_ROOTCLUSTER_FAILED    15
#define GET_FAT32_ROOTDIR_FAILED        16
#define GET_UNMOVABLES_FAILED           17
#define GET_DIRCOUNT_FAILED             18
#define DIRECTORY_READ_ERROR            19
#define DIRECTORY_WRITE_ERROR		21
#define GET_FIRSTFILES_FAILED           20
#define FIRST_CLUSTER_UPDATE_FAILED     21
#define GET_CONTINOUS_FILES_ERROR       22
#define INSUFFICIENT_FREE_DISK_SPACE    23
#define GET_FREESPACE_ERROR             24
#define VFS_RELOC_FAILED                25
#define RELOC_OVERLAPPING_FAILED        26
#define GET_FILESIZE_FAILED             27
#define GET_MAXRELOC_FAILED             28
#define GET_FILECLUSTER_FAILED          29
#define RELOC_SEQ_FAILED                30
#define IS_FILE_FRAGMENTED_FAILED       31

#define NUM_CUSTOMS 33

void SetCustomError(int error);

int GetCustomError(void);

void ClearCustomError(void);

#endif
