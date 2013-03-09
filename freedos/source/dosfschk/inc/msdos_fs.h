#ifndef _LINUX_MSDOS_FS_H
#define _LINUX_MSDOS_FS_H

/*
 * Changed October 2002 by Eric Auer for inclusion in the DOS
 * port of DOSFSCK. Now, only the most important parts are left.
 */

/*
 * The MS-DOS filesystem constants/structures
 */
/* *** seems not to be used anyway #include "fd.h" *** */
#include "types.h" /* *** __u32 etc. *** */
#ifndef _LOFF_DEF     
 typedef long long loff_t;       
#define _LOFF_DEF
#endif

#define FD_FILL_BYTE 0x0fd /* actually used... */

#define SECTOR_SIZE     512 /* sector size (bytes) */
#define MSDOS_DPB	(MSDOS_DPS) /* dir entries per block */
#define MSDOS_DPS	(SECTOR_SIZE/sizeof(struct msdos_dir_entry))
#define MSDOS_DIR_BITS	5 /* log2(sizeof(struct msdos_dir_entry)) */

#define ATTR_RO      1  /* read-only */
#define ATTR_HIDDEN  2  /* hidden */
#define ATTR_SYS     4  /* system */
#define ATTR_VOLUME  8  /* volume label */
#define ATTR_DIR     16 /* directory */
#define ATTR_ARCH    32 /* archived */

#define ATTR_NONE    0 /* no attribute bits */
#define ATTR_UNUSED  (ATTR_VOLUME | ATTR_ARCH | ATTR_SYS | ATTR_HIDDEN)
	/* attribute bits that are copied "as is" */
#define ATTR_EXT     (ATTR_RO | ATTR_HIDDEN | ATTR_SYS | ATTR_VOLUME)
	/* bits that are used by the Windows 95/Windows NT extended FAT */

#define ATTR_DIR_READ_BOTH 512 /* read both short and long names from the
				* vfat filesystem.  This is used by Samba
				* to export the vfat filesystem with correct
				* shortnames. */
#define ATTR_DIR_READ_SHORT 1024

#define DELETED_FLAG 0xe5 /* marks file as deleted when in name[0] */
#define IS_FREE(n) (!*(n) || *(const unsigned char *) (n) == DELETED_FLAG || \
  *(const unsigned char *) (n) == FD_FILL_BYTE)

#define MSDOS_SB(s) (&((s)->u.msdos_sb))

#define MSDOS_NAME 11 /* maximum name length */
#define MSDOS_LONGNAME 256 /* maximum name length */
#define MSDOS_SLOTS 21  /* max # of slots needed for short and long names */
#define MSDOS_DOT    ".          " /* ".", padded to MSDOS_NAME chars */
#define MSDOS_DOTDOT "..         " /* "..", padded to MSDOS_NAME chars */

#define MSDOS_FAT12 4084 /* maximum number of clusters in a 12 bit FAT */

#define EOF_FAT12 0xFF8		/* standard EOF */
#define EOF_FAT16 0xFFF8
#define EOF_FAT32 0xFFFFFF8
#define EOF_FAT(s) (MSDOS_SB(s)->fat_bits == 32 ? EOF_FAT32 : \
	MSDOS_SB(s)->fat_bits == 16 ? EOF_FAT16 : EOF_FAT12)

/*
 * Conversion from and to little-endian byte order. (no-op on i386/i486)
 *
 * Naming: Ca_b_c, where a: F = from, T = to, b: LE = little-endian,
 * BE = big-endian, c: W = word (16 bits), L = longword (32 bits)
 */

#define CF_LE_W(v) v /* *** le16_to_cpu(v) *** */
#define CF_LE_L(v) v /* *** le32_to_cpu(v) *** */
#define CT_LE_W(v) v /* *** cpu_to_le16(v) *** */
#define CT_LE_L(v) v /* *** cpu_to_le32(v) *** */

struct fat_boot_sector {
	__s8	ignored[3];	/* Boot strap short or near jump */
	__s8	system_id[8];	/* Name - can be used to special case
				   partition manager volumes */
	__u8	sector_size[2];	/* bytes per logical sector */
	__u8	cluster_size;	/* sectors/cluster */
	__u16	reserved;	/* reserved sectors */
	__u8	fats;		/* number of FATs */
	__u8	dir_entries[2];	/* root directory entries */
	__u8	sectors[2];	/* number of sectors */
	__u8	media;		/* media code (unused) */
	__u16	fat_length;	/* sectors/FAT */
	__u16	secs_track;	/* sectors per track */
	__u16	heads;		/* number of heads */
	__u32	hidden;		/* hidden sectors (unused) */
	__u32	total_sect;	/* number of sectors (if sectors == 0) */

	/* The following fields are only used by FAT32 */
	__u32	fat32_length;	/* sectors/FAT */
	__u16	flags;		/* bit 8: fat mirroring, low 4: active fat */
	__u8	version[2];	/* major, minor filesystem version */
	__u32	root_cluster;	/* first cluster in root directory */
	__u16	info_sector;	/* filesystem info sector */
	__u16	backup_boot;	/* backup boot sector */
	__u16	reserved2[6];	/* Unused */
};

struct fat_boot_fsinfo {
	__u32   reserved1;	/* Nothing as far as I can tell */
	__u32   signature;	/* 0x61417272L */
	__u32   free_clusters;	/* Free cluster count.  -1 if unknown */
	__u32   next_cluster;	/* Most recently allocated cluster.
				 * Unused under Linux. */
	__u32   reserved2[4];
};

struct msdos_dir_entry {
	__s8	name[8],ext[3];	/* name and extension */
	__u8	attr;		/* attribute bits */
	__u8    lcase;		/* Case for base and extension */
	__u8	ctime_ms;	/* Creation time, milliseconds */
	__u16	ctime;		/* Creation time */
	__u16	cdate;		/* Creation date */
	__u16	adate;		/* Last access date */
	__u16   starthi;	/* High 16 bits of cluster in FAT32 */
	__u16	time,date,start;/* time, date and first cluster */
	__u32	size;		/* file size (in bytes) */
};

/* Up to 13 characters of the name */
struct msdos_dir_slot {
	__u8    id;		/* sequence number for slot */
	__u8    name0_4[10];	/* first 5 characters in name */
	__u8    attr;		/* attribute byte */
	__u8    reserved;	/* always 0 */
	__u8    alias_checksum;	/* checksum for 8.3 alias */
	__u8    name5_10[12];	/* 6 more characters in name */
	__u16   start;		/* starting cluster number, 0 in long slots */
	__u8    name11_12[4];	/* last 2 characters in name */
};

struct vfat_slot_info {
	int is_long;		       /* was the found entry long */
	int long_slots;		       /* number of long slots in filename */
	int total_slots;	       /* total slots (long and short) */
	loff_t longname_offset;	       /* dir offset for longname start */
	loff_t shortname_offset;       /* dir offset for shortname start */
	int ino;		       /* ino for the file */
};

#endif
