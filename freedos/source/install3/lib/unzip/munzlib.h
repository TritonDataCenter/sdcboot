/* this is based on miniunz.c, modified by Kenneth J. Davis <jeremyd@computer.org> */
#ifndef __MUNZLIB_H__
#define __MUNZLIB_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <fcntl.h>

#ifdef unix
# include <unistd.h>
# include <utime.h>
#else
# include <direct.h>
# include <io.h>
#endif

#include "unzip.h"

#ifdef WIN32
#define USEWIN32IOAPI
#include "iowin32.h"
#endif


/* change_file_date : change the date/time of a file
    filename : the filename of the file where date/time must be modified
    dosdate : the new date at the MSDos format (4 bytes)
    tmu_date : the SAME new date at the tm_unz format */
void change_file_date(const char *filename, uLong dosdate, tm_unz tmu_date);

int mymkdir(const char *dirname);
int makedir (const char *newdir, int (* printErr)(const char *errmsg, ...) );


int do_extract_currentfile(
	unzFile uf,
	const int* popt_extract_without_path,
    int* popt_overwrite,
    int (* printErr)(const char *errmsg, ...), /* called if there is an error (eg printf) */
    int (* fileexistfunc)(void * extra, const char *filename), /* called to determine overwrite or not */
    /* if nonzero then clean up and return an error (user_interrupted) */
    int (* statusfunc)(void * extra, int bytesRead, char * msg, ...),
    void *extra  /* no used, simply passed to statusfunc */
);

int do_extract(
	unzFile uf,
	int opt_extract_without_path,
    int opt_overwrite,
    int (* printErr)(const char *errmsg, ...), /* called if there is an error (eg printf) */
    int (* fileexistfunc)(void * extra, const char *filename),  /* called to determine overwrite or not */
    /* if nonzero then clean up and return an error (user_interrupted) */
    int (* statusfunc)(void * extra, int bytesRead, char * msg, ...),
    void *extra  /* no used, simply passed to statusfunc */
);

int do_extract_onefile(
	unzFile uf,
	const char* filename,
	int opt_extract_without_path,
    int opt_overwrite,
    int (* printErr)(const char *errmsg, ...), /* called if there is an error (eg printf) */
    int (* fileexistfunc)(void * extra, const char *filename),  /* called to determine overwrite or not */
    /* if nonzero then clean up and return an error (user_interrupted) */
    int (* statusfunc)(void * extra, int bytesRead, char * msg, ...),
    void *extra  /* no used, simply passed to statusfunc */
);


#ifdef __cplusplus
}
#endif

#endif /* __MUNZLIB_H__ */
