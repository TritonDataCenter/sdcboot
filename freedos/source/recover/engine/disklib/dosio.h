/*
 * dosio.h      DOS data definitions
 *
 * This file is part of the BETA version of DISKLIB
 * Copyright (C) 1998, Gregg Jennings
 *
 * See README.TXT for information about re-distribution.
 * See DISKLIB.TXT for information about usage.
 *
 * 26-Nov-1998  greggj  MAX_FAT16_SECS
 * 04-Nov-1998  fixed the LOCK_n values
 *
 */

#ifndef DOSIO_H
#define DOSIO_H

/* Function return values */

#define DISK_OK     0
#define DEVICE_OK   0
#define DOS_ERR     (-1)
#define BIOS_ERR    (-2)
#define MEM_ERR     (-3)
#define DISK_ERR    (-4)
#define DEVICE_ERR  (-5)

/* fixed width data types for structures */

typedef unsigned char BYTE;         /* (I'm starting to dislike these...) */
typedef unsigned int UINT;
typedef short INT16;
typedef long INT32;
typedef unsigned short UINT16;
typedef unsigned long UINT32;


/* DOS Disk structure definitions */

#pragma pack(1)                 /* do not forget this! */

/* MS/PC/DR-DOS Boot Sector */

struct BOOT {
    BYTE    jump[3];            /* 3 byte Near Jump to boot code */
    BYTE    name[8];            /* 8 byte OEM name and version */
    UINT16  sec_size; /*BPB*/   /* Bytes per sector */
    BYTE    secs_cluster;       /* Sectors per allocation unit */
    UINT16  reserved_secs;      /* Reserved sectors (note 1) */
    BYTE    num_fats;           /* Number of FATs */
    UINT16  dir_entries;        /* Number of root dir entries */
    UINT16  num_sectors;        /* Number of sectors in logical image (note 2) */
    BYTE    media_desc;         /* Media descriptor */
    UINT16  secs_fat;           /* Number of FAT sectors */
    UINT16  secs_track;         /* Sectors per track */
    UINT16  num_heads;/*BPB*/   /* Number of heads */
 /* UINT16  hidden_sectors; */  /* Number of hidden sectors (note 3) */
    /* end DOS 2.0 */
 /* UINT16  large_sectors; */   /* High order number of hidden sectors */
 /* UINT32  total_sectors; */   /* Number of logical sectors */
    /* end DOS 3.0 */
    UINT32  hidden_sectors;     /* Number of hidden sectors (note 4) */
    UINT32  total_sectors;      /* Number of sectors if num_sectors == 0 */
    BYTE    drive_number;       /* 80h = harddrive else 00h */
    BYTE    reserved1;
    BYTE    signature;          /* 29h, 80h = WinNT */
    UINT32  volume_id;          /* Volume serial number */
    BYTE    volume_label[11];   /* Volume label */
    BYTE    file_system[8];/*EBPB*//* File system type */
    /* end DOS 6.0 */

    /*
       NOTES:

       1) Reserved sectors is usually the BOOT sector but can be more.
       2) If num_sectors is 0 use total_sectors.
       3) Sometime between 3.0 and 6.0 the WORD hidden_sectors entry
          merged with WORD large_sectors and became a DWORD, and
          total_sectors was renamed huge_sectors (in DOS parlance); also,
          the BPB grew in size. (I now use the name total_sectors.)
       4) Hidden sectors is usually the Partition track.
    */

};

/* FAT32 Boot sector */

struct BOOT_FAT32 {
    BYTE    jump[3];            /* 3 byte Near Jump to boot code */
    BYTE    name[8];            /* 8 byte OEM name and version */
    UINT16  sec_size;           /* Bytes per sector */
    BYTE    secs_cluster;       /* Sectors per allocation unit */
    UINT16  reserved_secs;      /* Reserved sectors (note 1) */
    BYTE    num_fats;           /* Number of FATs */
    UINT16  dir_entries;        /* 0 */
    UINT16  num_sectors;        /* 0 */
    BYTE    media_desc;         /* Media descriptor */
    UINT16  secs_fat;           /* 0 */
    UINT16  secs_track;         /* Sectors per track */
    UINT16  num_heads;          /* Number of heads */
    UINT32  hidden_sectors;     /* Number of hidden sectors */
    UINT32  total_sectors;      /* Number of sectors */
    UINT32  sectors_fat;        /* Sectors per FAT */
    UINT16  flags;
    UINT16  fs_version;         /* file system version number */
    UINT32  root_cluster;       /* cluster number of the first cluster of root */
    UINT16  info_sec;           /* file system information sector */
    UINT16  boot_sec;           /* backup boot sector sector */
    UINT16  reserved[6];
    BYTE    drive_number;       /* 80h = harddrive else 00h */
    BYTE    reserved1;
    BYTE    signature;          /* 29h */
    UINT32  volume_id;          /* Volume serial number */
    BYTE    volume_label[11];   /* Volume label */
    BYTE    file_system[8];     /* File system type */

    /*
        One would think that Microsoft would have put something in
        the BOOT sector to indicate that it is a FAT32/Win98 BOOT
        record. Perhaps if `secs_fat' == 0 means FAT32?
    */
};

/* NTFS Boot Sector */

struct BOOT_NTFS {
    BYTE    jump[3];            /* 3 byte Near Jump to boot code */
    BYTE    name[8];            /* 8 byte OEM name and version */
    UINT16  sec_size;           /* Bytes per sector */
    BYTE    secs_cluster;       /* Sectors per allocation unit */
    UINT16  reserved_secs;/*na*//* Reserved sectors */
    BYTE    num_fats;     /*na*//* Number of FATs */
    UINT16  dir_entries;  /*na*//* Number of root dir entries */
    UINT16  num_sectors;  /*na*//* Number of sectors in logical image */
    BYTE    media_desc;         /* Media descriptor */
    UINT16  secs_fat;     /*na*//* Number of FAT sectors */
    UINT16  secs_track;         /* Sectors per track */
    UINT16  num_heads;          /* Number of heads */
    UINT32  hidden_sectors;/*na*//* Number of hidden sectors */
    UINT32  total_sectors;/*na*//* Number of sectors if num_sectors == 0 */
    BYTE    drive_number;       /* 80h = harddrive else 00h */
    BYTE    reserved1;
    BYTE    signature;          /* 80h */
    BYTE    reserved2;
    UINT32  num_secs_lo;        /* Number of sectors in volume */
    UINT32  num_secs_hi;
    UINT32  mft_clus_lo;
    UINT32  mft_clus_hi;
    UINT32  mft2_clus_lo;
    UINT32  mft2_clus_hi;
    UINT32  rec_size;
    UINT32  buf_size;
    UINT32  volume_id;
};

/* BIOS Parameter Block definition */
/* (Part of Boot Sector) */

struct BPB {
    UINT16  sec_size; /*BPB*/   /* Bytes per sector */
    BYTE    secs_cluster;       /* Sectors per allocation unit */
    UINT16  reserved_secs;      /* Reserved sectors (note 1) */
    BYTE    num_fats;           /* Number of FATs */
    UINT16  dir_entries;        /* Number of root dir entries */
    UINT16  num_sectors;        /* Number of sectors in logical image (note 2) */
    BYTE    media_desc;         /* Media descriptor */
    UINT16  secs_fat;           /* Number of FAT sectors */
    UINT16  secs_track;         /* Sectors per track */
    UINT16  num_heads;/*BPB*/   /* Number of heads */
 /* UINT16  hidden_sectors; */  /* Number of hidden sectors (note 3) */
    /* end DOS 2.0 */
 /* UINT16  large_sectors; */   /* High order number of hidden sectors */
 /* UINT32  total_sectors; */   /* Number of logical sectors */
    /* end DOS 3.0 */
    UINT32  hidden_sectors;     /* Number of hidden sectors (note 4) */
    UINT32  total_sectors;/*BPB*//* Number of sectors if num_sectors == 0 */
};

/* Enhanced, DOS 6.0+, BIOS Parameter Block definition */

struct EBPB {
    UINT16  sec_size; /*BPB*/   /* Bytes per sector */
    BYTE    secs_cluster;       /* Sectors per allocation unit */
    UINT16  reserved_secs;      /* Reserved sectors */
    BYTE    num_fats;           /* Number of FATs */
    UINT16  dir_entries;        /* Number of root dir entries */
    UINT16  num_sectors;        /* Number of sectors in logical image */
    BYTE    media_desc;         /* Media descriptor */
    UINT16  secs_fat;           /* Number of FAT sectors */
    UINT16  secs_track;         /* Sectors per track */
    UINT16  num_heads;          /* Number of heads */
    UINT32  hidden_sectors;     /* Number of hidden sectors */
    UINT32  total_sectors;/*BPB*//* Number of sectors */
    BYTE    drive_number;       /* 80h = harddrive else 00h */
    BYTE    reserved1;
    BYTE    signature;          /* 29h */
    UINT32  volume_id;          /* Volume serial number */
    BYTE    volume_label[11];   /* Volume label */
    BYTE    file_system[8];/*EBPB*/ /* File system type */
};

/* Extended, or FAT32, BIOS Parameter Block definition */

struct EXT_BPB {
    UINT16  sec_size;           /* Bytes per sector */
    BYTE    secs_cluster;       /* Sectors per allocation unit */
    UINT16  reserved_secs;      /* Reserved sectors */
    BYTE    num_fats;           /* Number of FATs */
    UINT16  dir_entries;        /* Number of root dir entries */
    UINT16  num_sectors;        /* Number of sectors in logical image */
    BYTE    media_desc;         /* Media descriptor */
    UINT16  secs_fat;           /* Number of FAT sectors */
    UINT16  secs_track;         /* Sectors per track */
    UINT16  num_heads;          /* Number of heads */
    UINT32  hidden_sectors;     /* Number of hidden sectors */
    UINT32  total_sectors;      /* Number of sectors */
    UINT32  sectors_fat;        /* Sectors per FAT */
    UINT16  flags;
    UINT16  fs_version;         /* file system version number */
    UINT32  root_cluster;       /* cluster number of the first cluster of root */
    UINT16  info_sec;           /* file system information sector */
    UINT16  boot_sec;           /* backup boot sector sector */
    UINT16  reserved[6];

    /*
        One would think that Microsoft would have put something in
        the Ext BPB to indicate that it is an Ext BPB...
    */

};

/* MS-DOS Disk Parameter Block */
/* Int 21h 1Fh/32h - Get DPB */

struct DPB {
    BYTE    drive_num;          /* drive number */
    BYTE    unit_num;           /* for driver */
    UINT16  sec_size;           /* bytes per sector */
    BYTE    secs_cluster;       /* sectors per cluster */
    BYTE    secs_cluster2;      /* sectors per cluster as pow(2) */
    UINT16  fat_sector;         /* first FAT sector */
    BYTE    num_fats;           /* number of FATs */
    UINT16  dir_entries;        /* number of directory entries */
    UINT16  cluster_sec;        /* first sector of first cluster */
    UINT16  num_clusters;       /* number of clusters + 1 */
    UINT16  secs_fat;           /* sectors per FAT */
    UINT16  dir_sector;         /* first sector of ROOT directory */
    UINT32  driver_addr;        /* device driver address */
    BYTE    media_desc;         /* media descriptor */
    BYTE    first_access;       /* access of drive */
    UINT32  next_dpb;           /* next DPB */
    UINT16  last_cluster;       /* last allocated cluster */
    UINT16  free_clusters;      /* availible clusters */
};

/* MS-DOS Disk Parameter Block, FAT32 */
/* Int 21h 7302h - Get ExtDPB */

struct EXT_DPB {
  /*UINT16  size;*/             /* size of the structure minus this member */
    BYTE    drive_num;          /* drive number (A: = 0) */
    BYTE    unit_num;           /* for driver */
    UINT16  sec_size;           /* bytes per sector */
    BYTE    secs_cluster;       /* sectors per cluster */
    BYTE    secs_cluster2;      /* sectors per cluster as pow(2) */
    UINT16  fat_sector;         /* first FAT sector */
    BYTE    num_fats;           /* number of FATs */
    UINT16  dir_entries;        /* number of directory entries */
    UINT16  cluster_sec;        /* first sector of first cluster */
    UINT16  num_clusters;       /* number of clusters + 1 */
    UINT16  secs_fat;           /* sectors per FAT */
    UINT16  dir_sector;         /* first sector of ROOT directory */
    UINT32  driver_addr;        /* device driver address */
    BYTE    media_desc;         /* media descriptor */
    BYTE    first_access;       /* access of drive */
    UINT32  next_dpb;           /* next DPB */
    UINT16  last_cluster;       /* last allocated cluster */
    UINT32  free_clusters;      /* availible clusters */
    UINT16  flags;
    UINT16  info_sec;           /* file system information sector */
    UINT16  boot_sec;           /* backup boot sector sector */
    UINT32  cluster_sector;     /* first sector of first cluster */
    UINT32  max_clusters;       /* number of clusters + 1 */
    UINT32  sectors_fat;        /* sectors per FAT */
    UINT32  root_cluster;       /* cluster number of the first cluster of root */
    UINT32  next_free_cluster;  /* most recently allocated cluster */

    /*
        When making the call to fill this structure the buffer must be
        preceded by a WORD which will receive the size of the ExtDPB.
        Which begs the question: Should that size field be part of the
        structure?
    */
};

/* MS-DOS Device Parameter Block */
/* IOCTL Int 21h 440Dh/60h - Get Device Parameters */

struct DEVICEPARAMS {
    BYTE    special;
    BYTE    dev_type;
    UINT16  dev_attrib;
    UINT16  cylinders;
    BYTE    media;
    UINT16  sec_size;       /*BPB*/
    BYTE    secs_cluster;
    UINT16  reserved_secs;
    BYTE    num_fats;
    UINT16  dir_entries;
    UINT16  num_sectors;
    BYTE    media_desc;
    UINT16  secs_fat;
    UINT16  secs_track;
    UINT16  num_heads;
    UINT32  hidden_sectors;
    UINT32  total_sectors;  /*BPB*/
};

/* MS-DOS Device Parameter Block, FAT32 */
/* IOCTL Int 21h 440Dh/60h, Category 48h - Get Device Parameters */

struct EXT_DEVICEPARAMS {
    BYTE    special;
    BYTE    dev_type;
    UINT16  dev_attrib;
    UINT16  cylinders;
    BYTE    media;
    UINT16  sec_size;       /*BPB*/
    BYTE    secs_cluster;
    UINT16  reserved_secs;
    BYTE    num_fats;
    UINT16  dir_entries;
    UINT16  num_sectors;
    BYTE    media_desc;
    UINT16  secs_fat;
    UINT16  secs_track;
    UINT16  num_heads;
    UINT32  hidden_sectors;
    UINT32  total_sectors;  /*BPB*/
    UINT32  sectors_fat;        /* Sectors per FAT */
    UINT16  flags;
    UINT16  fs_version;         /* file system version number */
    UINT32  root_cluster;       /* cluster number of the first cluster of root */
    UINT16  info_sec;           /* file system information sector */
    UINT16  boot_sec;           /* backup boot sector sector */
    UINT16  reserved[6];
    BYTE    reserved2[32];
  /*UINT16  table_entries;*/
  /*BYTE    sector_table[0];*/
};

#define DIR_ENTRY_SIZE  32
#define MAX_FAT16_SECS  4194304 /* ((1024*1024*1024*2)/512) */

/* MS-DOS Disk Control Block - Int 25h/26h */
/* (For drives larger than 32M) */

struct DCB {
    UINT32  sector;
    UINT16  number;
    BYTE    *buffer;
};

/* MS-DOS Media ID structure */
/* INT 21h Function 440Dh/66h - Get Media ID */

struct MID {
    UINT16  infolevel;
    UINT32  serialnum;
    char    vollabel[11];
    char    filesys[8];
};

/* MS-DOS Read/Write Block */
/* INT 21h Function 440Dh/41h/61h - Write/Read Track on Logical Drive */

struct RWBLOCK {
    BYTE    special;
    UINT16  head;
    UINT16  track;
    UINT16  sector;
    UINT16  nsecs;
    BYTE    *buffer;
};

/* MS-DOS Format and Verify Block */
/* INT 21h Function 440Dh/42h - Format and Verify Track on Logical Drive */

struct FVBLOCK {
    BYTE    special;
    UINT16  head;
    UINT16  track;
    UINT16  tracks;
};

/* MS-DOS Free Space */
/* INT 21h Function 36h - Get Disk Free Space */

struct FREESPACE {
    UINT16  secs_cluster;
    UINT16  avail_clusters;
    UINT16  sec_size;
    UINT16  num_clusters;
};

/* MS-DOS Free Space, FAT32 */
/* INT 21h Function 7303h - Get Disk Free Space */

struct EXT_FREESPACE {
   UINT16   size;           /* size of this struct (R/O) */
   UINT16   level;          /* ? must be zero */
   UINT32   secs_cluster;   /* sectors per cluster */
   UINT32   sec_size;       /* sector size in bytes */
   UINT32   avail_clusters; /* number of available clusters */
   UINT32   num_clusters;   /* number of clusters */
   UINT32   phys_asectors;  /* number of available physical sectors */
   UINT32   phys_sectors;   /* number of physical sectors */
   UINT32   avail_alloc;    /* number of available allocation units */
   UINT32   alloc_units;    /* number of allocation units */
   UINT32   reserved[2];
};

/* MS-DOS Get Lock State */
/* INT 21h Function 440Dh/48h - Lock/Unlock Removable Media */

struct PARAMBLOCK {
    unsigned char operation;
    unsigned char numlocks;
};

enum LOCK_OPERATION { LOCK_VOLUME, UNLOCK_VOLUME, LOCK_STATE };
#define LOCK_INVALID_FUNC   0x01
#define LOCK_NOT_LOCKED     0xB0
#define LOCK_NOT_REMOVABLE  0xB2
#define LOCK_COUNT_EXCEEDED 0xB4

#define LOCK_0  0x0000
#define LOCK_1  0x0100
#define LOCK_2  0x0200
#define LOCK_3  0x0300

#define LOCK_PERM_WRT   0x0001
#define LOCK_PERM_MAP   0x0002
#define LOCK_PERM_FMT   0x0004


/* Enhanced BIOS Interface */

struct DISK_PACKET {
    unsigned short size;
    unsigned short flags;
    unsigned long cylinders;
    unsigned long heads;
    unsigned long sectors;
    unsigned long abssectors1;
    unsigned long abssectors2;
    unsigned short secsize;
    void *edd;
};

/* disk packet flags */

#define F_DMAE  0x0001
#define F_GEOV  0x0002
#define F_REMV  0x0004
#define F_WRTV  0x0008
#define F_CHGL  0x0010
#define F_LOCK  0x0020
#define F_NOME  0x0040

/* Partition Table */

struct partition {
   unsigned char bootable;          /* 80h or 0 */
   unsigned char start_head;        /* dh location of first sector (boot_sector) */
   unsigned char start_sector;      /* ch */
   unsigned char start_cylinder;    /* cl */
   unsigned char system;
   unsigned char end_head;          /* location of last sector */
   unsigned char end_sector;
   unsigned char end_cylinder;
   unsigned long start_sector_abs;  /* start_cylinder * heads * sectors */
                                    /*  + start_head * sectors + start_sector - 1 */
   unsigned long no_of_sectors_abs; /* end_cylinder * heads * sectors + end_head * sectors */
                                    /*  + end_sector - start_sector_abs */
};

/* Master Boot Record */

struct mbr {
    unsigned char code[446];
    struct partition part[4];
    unsigned short sig;
};

#pragma pack()

struct PARTITION_TYPE {
    int type;
    char *name;
    char *desc;
};

/* Typedefs for constants and function numbers */

/* MS-DOS function return values */

typedef enum {
    DOS_OK,             /* Error 0  (not an error)  */
    DOS_EINVFNC,        /* Invalid function number  */
    DOS_ENOFILE,        /* File not found           */
    DOS_ENOPATH,        /* Path not found           */
    DOS_EMFILE,         /* Too many open files      */
    DOS_EACCES,         /* Permission denied        */
    DOS_EBADF,          /* Bad file number          */
    DOS_ECONTR,         /* Memory blocks destroyed  */
    DOS_ENOMEM,         /* Not enough core          */
    DOS_EINVMEM,        /* Invalid memory block address */
    DOS_EINVENV,        /* Invalid environment      */
    DOS_EINVFMT,        /* Invalid format           */
    DOS_EINVACC,        /* Invalid access code      */
    DOS_EINVDAT,        /* Invalid data             */
    DOS_EFAULT,         /* Unknown error            */
    DOS_EINVDRV,        /* Invalid drive specified  */
    DOS_ECURDIR,        /* Attempt to remove CurDir */
    DOS_ENOTSAM,        /* Not same device          */
    DOS_ENMFILE,        /* No more files            */
    DOS_EWRITE,         /* Write protect            */
    DOS_EBADU,          /* Bad unit                 */
    DOS_ENREADY,        /* Drive not ready          */
    DOS_EBADCMD,        /* Bad command              */
    DOS_ECRC,           /* CRC error                */
    DOS_EBADLEN,        /* Bad length               */
    DOS_ESEEK,          /* Seek error               */
    DOS_ENONDOS,        /* Non DOS disk             */

/* there is more of course, but the above are the most common */

    DOS_EMAXERRNO
} DOS_ERROR;

/* MS-DOS functions used by this library */

typedef enum {
    DOS_GET_VER = 0x3000,
    DOS_GET_VER_5 = 0x3306,
    DOS_GET_DPB = 0x3200,
    DOS_GET_FREE_SPACE = 0x3600,
    DOS_EXT_GET_FREE_SPACE = 0x7303,
    DOS_EXT_ABS_READ_WRITE = 0x7305
} DOS_FUNCTION;

typedef enum {
    DOS_DEV_REMOVE = 0x08,
    DOS_DRV_REMOTE = 0x09,
    DOS_DEV_IOCTL = 0x0D
} DOS_SUBFUNCTION;

typedef enum {
    DOS_MINOR_NONE = 0,
    DOS_MINOR_SET_DEVICE = 0x40,
    DOS_MINOR_WRITE_TRACK = 0x41,
    DOS_MINOR_FORMAT_TRACK = 0x42,
    DOS_MINOR_SET_MEDIA = 0x46,
    DOS_MINOR_LOCK_LOGICAL = 0x4A,
    DOS_MINOR_LOCK_PHYSICAL = 0x4B,
    DOS_MINOR_GET_DEVICE = 0x60,
    DOS_MINOR_READ_TRACK = 0x61,
    DOS_MINOR_GET_MEDIA = 0x66,
    DOS_MINOR_UNLOCK_LOGICAL = 0x6A,
    DOS_MINOR_UNLOCK_PHYSICAL = 0x6B
} DOS_MINOR_CODE;


/* Function prototypes */

/* FREE.C */

int get_drive(void);
int drive_size(int disk, long *size);
int disk_free_space(int disk, struct FREESPACE *fs);

/* IOCTL.C */

int dos_ioctl(DOS_SUBFUNCTION subfunc, DOS_MINOR_CODE code, int device, void *data);
int dos_ioctl32(DOS_SUBFUNCTION subfunc, DOS_MINOR_CODE code, int device, void *data);
int win_ioctl(DOS_MINOR_CODE func, int data, void *buf);
int win_ioctl_ext(DOS_MINOR_CODE func, int data, void *buf);

int doscall(DOS_FUNCTION ax, int bx, int cx, int dx, void *data);
#define GET_DPB(disk,dpb)   doscall(DOS_GET_DPB,0,0,disk+1,dpb)

/* GETDISK.C */

int disk_getparamblk(int disk, struct DPB *dpb);
int disk_getparams(int disk, struct DEVICEPARAMS *dp);
int disk_get_logical(int disk, int *t, int *s, int *h);
int disk_getmedia(int disk, struct MID *mid);
int disk_getparams32(int disk, struct EXT_DEVICEPARAMS *dp);
int disk_getmedia32(int disk, struct MID *mid);
int disk_setmedia(int disk, struct MID *mid);
int disk_setmedia32(int disk, struct MID *mid);

/* XLATE.C */

long disk_size(struct DEVICEPARAMS *dp);
#if defined _WIN32
#if defined __WATCOMC__ && __WATCOMC__ >= 1100
__int64 disk_size32(struct DEVICEPARAMS *dp);
#endif
#endif
int dir_sectors(struct DEVICEPARAMS *dp);
int data_sector(struct DEVICEPARAMS *dp);
int dir_sector(struct DEVICEPARAMS *dp);
unsigned int num_clusters(struct DEVICEPARAMS *dp);
int max_track(struct DEVICEPARAMS *dp);

/* PART.C */

struct PARTITION_TYPE *partition_type(int type);

/* LOCK.C */

struct VOLUMEINFO {
    int flags;
    int maxfname;
    int maxpath;
};

int disk_get_volinfo(int disk, struct VOLUMEINFO *vi);
int disk_enum_files(int disk, char *files, int index);
void disk_flush(int disk);
int disk_lock_flag(int disk);
/*int disk_lock_unlock(int disk, int op, int *numlocks);*/
int disk_lock_state(int disk, int *level, int *perm);
int disk_lock_state32(int disk, int *level, int *perm);
int disk_lock_logical(int disk, int level, int perm);
int disk_lock_logical32(int disk, int level, int perm);
int disk_unlock_logical(int disk);
int disk_unlock_logical32(int disk);

#endif
