#ifndef __MISC__H__
#define __MISC__H__

#ifdef __WATCOMC__
#define MAXPATH _MAX_PATH
#define MAXFILE _MAX_FNAME
#define MAXEXT _MAX_EXT
#define MAXDRIVE _MAX_DRIVE
#define MAXDIR _MAX_DIR

#define FA_RDONLY _A_RDONLY
#define FA_ARCH _A_ARCH
#define FA_SYSTEM _A_SYSTEM
#define FA_HIDDEN _A_HIDDEN
#define FA_DIREC _A_SUBDIR
#define FA_LABEL _A_VOLID

#define findfirst(x,y,z) _dos_findfirst(x,z,y)
#define findnext _dos_findnext
#define ffblk find_t
#define ff_name name
#define ff_attrib attrib
#define ff_reserved reserved
#define ff_ftime wr_time
#define ff_date wr_date
#define ff_fsize size

#define getdfree _dos_getdiskfree
#define dfree diskfree_t
#define df_avail avail_clusters
#define df_sclus sectors_per_cluster
#define df_bsec bytes_per_sector

#define fnsplit(a,b,c,d,e) _splitpath(a,b,c,d,e)

struct ftime /* As defined by Borland C */
{
    unsigned    ft_tsec  : 5;   /* Two second interval */
    unsigned    ft_min   : 6;   /* Minutes */
    unsigned    ft_hour  : 5;   /* Hours */
    unsigned    ft_day   : 5;   /* Days */
    unsigned    ft_month : 4;   /* Months */
    unsigned    ft_year  : 7;   /* Year */
};

#define getdisk _getdrive

#endif /* __WATCOMC__ */

#define DIR_SEPARATOR "\\"

extern char *addsep(char *);
extern int dir_exists(const char *);
extern int makedir(char *);
extern char *strmcpy(char *dest, const char *src, const unsigned int maxlen);
extern char *strmcat(char *dest, const char *src, const unsigned int maxlen);
extern int copy_file(const char *src_filename, const char *dest_filename);
extern void build_filename(char *, const char *, const char *);

#ifdef INLINE
#define SplitPath(path, drive, dir, fname, ext)    (fnsplit((path), (drive), (dir), (fname), (ext)))
#ifdef USE_KITTEN
#define error(x,y,error) fprintf(stderr, "[%s]\n", kittengets(x,y,error))
#else
#define error(error)    (fprintf(stderr, " [%s]\n", (error)))
#endif /* USE_KITTEN */
#else
extern void SplitPath(const char* path, char* drive, char* dir, char* fname, char* ext);
#ifdef USE_KITTEN
extern void error(int, int, const char *);
#else
extern void error(const char *);
#endif /* USE_KITTEN */
#endif /* INLINE */
#endif /* __MISC__H__ */
