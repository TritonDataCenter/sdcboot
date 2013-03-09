/*
 * disklib.h    functions
 *
 * This file is part of the BETA version of DISKLIB
 * Copyright (C) 1998, Gregg Jennings
 *
 * See README.TXT for information about re-distribution.
 * See DISKLIB.TXT for information about usage.
 *
 */

#ifndef DISKLIB_H
#define DISKLIB_H

enum { UNKFS, FAT1216, FAT32, NTFS, CDFS };

/* VER.C */

int lib_ver(void);
int dos_ver(void);
int win_ver(void);
enum { LIB_DOS, LIB_WIN95, LIB_WIN98, LIB_WINNT };
enum { WIN_NT = 0x532, WIN_95 = 0x700, WIN_98 = 0x70a };
enum { WINDOWS_NT = 0x300, WINDOWS_95 = 0x400, WINDOWS_98 = 0x40a };
/* have to re-check those... */

/* ERR.C */

extern int disk_errno;

void lib_error(const char *s, int i);

/* ERROR.C */

void err_dump(const char *str);
const char *stroserror(void);
int os_error(void);
void poserror(void);
void doserror(int error);
void setdosioerror(int error);
void set_error(int error, int class, int action, int location);
#define setdoserror(e)  set_error(e,0,0,0)

/* GETDISK.C */

int disk_get_logical(int disk, int *t, int *s, int *h);
int disk_get_logical32(int disk, int *t, int *s, int *h);
int disk_getfilesys(int disk, char filesys[8]);
int disk_gettype(int disk);

/* BIOS.C */

int disk_reset(int disk);
int disk_status(int disk);
int disk_get_physical(int disk, int *t, int *s, int *h);
int disk_get_physical_ext(int disk, int *t, int *s, int *h);
int disk_read_p(int disk, int t, int s, int h, void *buf, int nsecs);
int disk_write_p(int disk, int t, int s, int h, void *buf, int nsecs);
int disk_verify_p(int disk, int trk, int sec, int head, int nsecs);
int disk_type(int disk);
int disk_part(int disk, int c, int h, int s, void *buf);
const char *disk_error(int error);

/* READ/WRITE.C */

int disk_read(int disk, long sector, void *buffer, int nsecs);
int disk_read_ext(int disk, long sector, void *buffer, int nsecs);
int disk_read_ioctl(int disk, int track, int sec, int head, void *buf, int n);
int disk_read32(int disk, long sector, void *buffer, int nsecs);
int disk_read_ioctl32(int disk, int track, int sec, int head, void *buf, int nsecs);

int disk_write(int disk, long sector, void *buffer, int nsecs);
int disk_write_ext(int disk, long sector, void *buffer, int nsecs);
int disk_write_ioctl(int disk, int track, int sec, int head, void *buf, int n);
int disk_write32(int disk, long sector, void *buffer, int nsecs);

/* XLATE.C */

long logical_sector(int t, int s, int h, long hidden, int maxsector, int maxhead);
void physical(int *t, int *s, int *h, long sector, long hidden, int maxsector, int maxhead);
int physical_track(long sector, long hidden, int maxsector, int maxhead);
int physical_sector(long sector, long hidden, int maxsector);
int physical_head(long sector, long hidden, int maxsector, int maxhead);
void physical_p(int *t, int *s, int *h, long sec, int maxs, int maxh);
int cluster_sector(long sector, int datasec, int secscluster);
long cluster_to_sector(long cluster, int datasec, int secscluster);
int sector_to_cluster(long sector, int datasec, int secscluster);

#endif
