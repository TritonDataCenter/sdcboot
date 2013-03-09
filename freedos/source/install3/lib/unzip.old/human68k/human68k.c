/*---------------------------------------------------------------------------

  human68k.c

  Human68K-specific routines for use with Info-ZIP's UnZip 5.1 and later.

  Contains:  do_wild()
             mapattr()
             mapname()
             checkdir()
             close_outfile()
             version()
             TwentyOne()
             normalize_name()

  ---------------------------------------------------------------------------*/


#include <dirent.h>
#include <sys/dos.h>
#include <sys/xunistd.h>
#include <jstring.h>
#define UNZIP_INTERNAL
#include "unzip.h"


static void normalize_name(char *);

static int created_dir;        /* used in mapname(), checkdir() */
static int renamed_fullpath;   /* ditto */

#ifndef SFX

/**********************/
/* Function do_wild() */
/**********************/

char *do_wild(__G__ wildspec)
    __GDEF
    char *wildspec;         /* only used first time on a given dir */
{
    static DIR *dir = NULL;
    static char *dirname, *wildname, matchname[FILNAMSIZ];
    static int firstcall=TRUE, have_dirname, dirnamelen;
    struct dirent *file;


    /* Even when we're just returning wildspec, we *always* do so in
     * matchname[]--calling routine is allowed to append four characters
     * to the returned string, and wildspec may be a pointer to argv[].
     */
    if (firstcall) {        /* first call:  must initialize everything */
        firstcall = FALSE;

        /* break the wildspec into a directory part and a wildcard filename */
        if ((wildname = strrchr(wildspec, '/')) == NULL) {
            dirname = ".";
            dirnamelen = 1;
            have_dirname = FALSE;
            wildname = wildspec;
        } else {
            ++wildname;     /* point at character after '/' */
            dirnamelen = wildname - wildspec;
            if ((dirname = (char *)malloc(dirnamelen+1)) == NULL) {
                Info(slide, 1, ((char *)slide,
                  "warning:  can't allocate wildcard buffers\n"));
                strcpy(matchname, wildspec);
                return matchname;   /* but maybe filespec was not a wildcard */
            }
            strncpy(dirname, wildspec, dirnamelen);
            dirname[dirnamelen] = '\0';   /* terminate for strcpy below */
            have_dirname = TRUE;
        }

        if ((dir = opendir(dirname)) != NULL) {
            while ((file = readdir(dir)) != NULL) {
                if (file->d_name[0] == '.' && wildname[0] != '.')
                    continue;  /* Unix:  '*' and '?' do not match leading dot */
                if (match(file->d_name, wildname, 0)) {  /* 0 == case sens. */
                    if (have_dirname) {
                        strcpy(matchname, dirname);
                        strcpy(matchname+dirnamelen, file->d_name);
                    } else
                        strcpy(matchname, file->d_name);
                    return matchname;
                }
            }
            /* if we get to here directory is exhausted, so close it */
            closedir(dir);
            dir = NULL;
        }

        /* return the raw wildspec in case that works (e.g., directory not
         * searchable, but filespec was not wild and file is readable) */
        strcpy(matchname, wildspec);
        return matchname;
    }

    /* last time through, might have failed opendir but returned raw wildspec */
    if (dir == NULL) {
        firstcall = TRUE;  /* nothing left to try--reset for new wildspec */
        if (have_dirname)
            free(dirname);
        return (char *)NULL;
    }

    /* If we've gotten this far, we've read and matched at least one entry
     * successfully (in a previous call), so dirname has been copied into
     * matchname already.
     */
    while ((file = readdir(dir)) != NULL)
        if (match(file->d_name, wildname, 0)) {   /* 0 == don't ignore case */
            if (have_dirname) {
                /* strcpy(matchname, dirname); */
                strcpy(matchname+dirnamelen, file->d_name);
            } else
                strcpy(matchname, file->d_name);
            return matchname;
        }

    closedir(dir);     /* have read at least one dir entry; nothing left */
    dir = NULL;
    firstcall = TRUE;  /* reset for new wildspec */
    if (have_dirname)
        free(dirname);
    return (char *)NULL;

} /* end function do_wild() */

#endif /* !SFX */




/**********************/
/* Function mapattr() */
/**********************/

int mapattr(__G)
    __GDEF
{
    ulg  tmp = G.crec.external_file_attributes;

    if (G.pInfo->hostnum == UNIX_ || G.pInfo->hostnum == VMS_)
	G.pInfo->file_attr = _mode2dos(tmp >> 16);
    else
	/* set archive bit (file is not backed up): */
	G.pInfo->file_attr = (unsigned)(G.crec.external_file_attributes|32) &
          0xff;
    return 0;

} /* end function mapattr() */





/**********************/
/* Function mapname() */
/**********************/

int mapname(__G__ renamed)   /* return 0 if no error, 1 if caution (filename */
    __GDEF                   /* trunc), 2 if warning (skip file because dir */
    int renamed;             /* doesn't exist), 3 if error (skip file), 10 */
{                            /* if no memory (skip file) */
    char pathcomp[FILNAMSIZ];    /* path-component buffer */
    char *pp, *cp=(char *)NULL;  /* character pointers */
    char *lastsemi=(char *)NULL; /* pointer to last semi-colon in pathcomp */
    int quote = FALSE;           /* flags */
    int error = 0;
    register unsigned workch;    /* hold the character being tested */


/*---------------------------------------------------------------------------
    Initialize various pointers and counters and stuff.
  ---------------------------------------------------------------------------*/

    if (G.pInfo->vollabel)
        return IZ_VOL_LABEL;    /* can't set disk volume labels in Unix */

    /* can create path as long as not just freshening, or if user told us */
    G.create_dirs = (!G.fflag || renamed);

    created_dir = FALSE;        /* not yet */

    /* user gave full pathname:  don't prepend rootpath */
    renamed_fullpath = (renamed && (*filename == '/'));

    if (checkdir(__G__ (char *)NULL, INIT) == 10)
        return 10;              /* initialize path buffer, unless no memory */

    *pathcomp = '\0';           /* initialize translation buffer */
    pp = pathcomp;              /* point to translation buffer */
    if (G.jflag)                /* junking directories */
        cp = (char *)strrchr(G.filename, '/');
    if (cp == NULL)             /* no '/' or not junking dirs */
        cp = G.filename;        /* point to internal zipfile-member pathname */
    else
        ++cp;                   /* point to start of last component of path */

/*---------------------------------------------------------------------------
    Begin main loop through characters in filename.
  ---------------------------------------------------------------------------*/

    while ((workch = (uch)*cp++) != 0) {
	if (iskanji(workch)) {
	    *pp++ = (char)workch;
	    quote = TRUE;
	} else if (quote) {                 /* if character quoted, */
            *pp++ = (char)workch;    /*  include it literally */
            quote = FALSE;
        } else
            switch (workch) {
            case '/':             /* can assume -j flag not given */
                *pp = '\0';
                if ((error = checkdir(__G__ pathcomp, APPEND_DIR)) > 1)
                    return error;
                pp = pathcomp;    /* reset conversion buffer for next piece */
                lastsemi = NULL;  /* leave directory semi-colons alone */
                break;

            case ';':             /* VMS version (or DEC-20 attrib?) */
                lastsemi = pp;         /* keep for now; remove VMS ";##" */
                *pp++ = (char)workch;  /*  later, if requested */
                break;

            case '\026':          /* control-V quote for special chars */
                quote = TRUE;     /* set flag for next character */
                break;

            case ' ':             /* change spaces to underscore under */
                *pp++ = '_';      /*  MTS; leave as spaces under Unix */
                break;

            default:
                /* allow European characters in filenames: */
                if (isprint(workch) || (128 <= workch && workch <= 254))
                    *pp++ = (char)workch;
            } /* end switch */

    } /* end while loop */

    *pp = '\0';                   /* done with pathcomp:  terminate it */

    /* if not saving them, remove VMS version numbers (appended ";###") */
    if (!G.V_flag && lastsemi) {
        pp = lastsemi + 1;
        while (isdigit((uch)(*pp)))
            ++pp;
        if (*pp == '\0')          /* only digits between ';' and end:  nuke */
            *lastsemi = '\0';
    }

/*---------------------------------------------------------------------------
    Report if directory was created (and no file to create:  filename ended
    in '/'), check name to be sure it exists, and combine path and name be-
    fore exiting.
  ---------------------------------------------------------------------------*/

    if (G.filename[strlen(G.filename) - 1] == '/') {
        checkdir(__G__ G.filename, GETPATH);
        if (created_dir && QCOND2) {
            Info(slide, 0, ((char *)slide, "   creating: %s\n",
              G.filename));
            return IZ_CREATED_DIR;   /* set dir time (note trailing '/') */
        }
        return 2;   /* dir existed already; don't look for data to extract */
    }

    if (*pathcomp == '\0') {
        Info(slide, 1, ((char *)slide, "mapname:  conversion of %s failed\n",
          G.filename));
        return 3;
    }

    checkdir(__G__ pathcomp, APPEND_NAME);   /* returns 1 if truncated:  care? */
    checkdir(__G__ G.filename, GETPATH);

    return error;

} /* end function mapname() */





/***********************/
/* Function checkdir() */
/***********************/

int checkdir(__G__ pathcomp, flag)
    __GDEF
    char *pathcomp;
    int flag;
/*
 * returns:  1 - (on APPEND_NAME) truncated filename
 *           2 - path doesn't exist, not allowed to create
 *           3 - path doesn't exist, tried to create and failed; or
 *               path exists and is not a directory, but is supposed to be
 *           4 - path is too long
 *          10 - can't allocate memory for filename buffers
 */
{
    static int rootlen = 0;   /* length of rootpath */
    static char *rootpath;    /* user's "extract-to" directory */
    static char *buildpath;   /* full path (so far) to extracted file */
    static char *end;         /* pointer to end of buildpath ('\0') */

#   define FN_MASK   7
#   define FUNCTION  (flag & FN_MASK)



/*---------------------------------------------------------------------------
    APPEND_DIR:  append the path component to the path being built and check
    for its existence.  If doesn't exist and we are creating directories, do
    so for this one; else signal success or error as appropriate.
  ---------------------------------------------------------------------------*/

    if (FUNCTION == APPEND_DIR) {
        int too_long = FALSE;
        char *old_end = end;

        Trace((stderr, "appending dir segment [%s]\n", pathcomp));
        while ((*end = *pathcomp++) != '\0')
            ++end;
	normalize_name(old_end);

        /* GRR:  could do better check, see if overrunning buffer as we go:
         * check end-buildpath after each append, set warning variable if
         * within 20 of FILNAMSIZ; then if var set, do careful check when
         * appending.  Clear variable when begin new path. */

        if ((end-buildpath) > FILNAMSIZ-3)  /* need '/', one-char name, '\0' */
            too_long = TRUE;                /* check if extracting directory? */
        if (stat(buildpath, &G.statbuf))    /* path doesn't exist */
        {
            if (!G.create_dirs) { /* told not to create (freshening) */
                free(buildpath);
                return 2;         /* path doesn't exist:  nothing to do */
            }
            if (too_long) {
                Info(slide, 1, ((char *)slide,
                  "checkdir error:  path too long: %s\n",
                  buildpath));
                free(buildpath);
                return 4;         /* no room for filenames:  fatal */
            }
            if (MKDIR(buildpath, 0666) == -1) {   /* create the directory */
                Info(slide, 1, ((char *)slide,
                  "checkdir error:  can't create %s\n\
                 unable to process %s.\n",
                  buildpath, G.filename));
                free(buildpath);
                return 3;      /* path didn't exist, tried to create, failed */
            }
            created_dir = TRUE;
        } else if (!S_ISDIR(G.statbuf.st_mode)) {
            Info(slide, 1, ((char *)slide,
              "checkdir error:  %s exists but is not directory\n\
                 unable to process %s.\n",
              buildpath, G.filename));
            free(buildpath);
            return 3;          /* path existed but wasn't dir */
        }
        if (too_long) {
            Info(slide, 1, ((char *)slide,
              "checkdir error:  path too long: %s\n",
              buildpath));
            free(buildpath);
            return 4;         /* no room for filenames:  fatal */
        }
        *end++ = '/';
        *end = '\0';
        Trace((stderr, "buildpath now = [%s]\n", buildpath));
        return 0;

    } /* end if (FUNCTION == APPEND_DIR) */

/*---------------------------------------------------------------------------
    GETPATH:  copy full path to the string pointed at by pathcomp, and free
    buildpath.
  ---------------------------------------------------------------------------*/

    if (FUNCTION == GETPATH) {
        strcpy(pathcomp, buildpath);
        Trace((stderr, "getting and freeing path [%s]\n", pathcomp));
        free(buildpath);
        buildpath = end = (char *)NULL;
        return 0;
    }

/*---------------------------------------------------------------------------
    APPEND_NAME:  assume the path component is the filename; append it and
    return without checking for existence.
  ---------------------------------------------------------------------------*/

    if (FUNCTION == APPEND_NAME) {
        char *old_end = end;

        Trace((stderr, "appending filename [%s]\n", pathcomp));
        while ((*end = *pathcomp++) != '\0') {
            ++end;
            normalize_name(old_end);
            if ((end-buildpath) >= FILNAMSIZ) {
                *--end = '\0';
                Info(slide, 1, ((char *)slide,
                  "checkdir warning:  path too long; truncating\n\
                   %s\n                -> %s\n",
                  G.filename, buildpath));
                return 1;   /* filename truncated */
            }
        }
        Trace((stderr, "buildpath now = [%s]\n", buildpath));
        return 0;  /* could check for existence here, prompt for new name... */
    }

/*---------------------------------------------------------------------------
    INIT:  allocate and initialize buffer space for the file currently being
    extracted.  If file was renamed with an absolute path, don't prepend the
    extract-to path.
  ---------------------------------------------------------------------------*/

/* GRR:  for VMS and TOPS-20, add up to 13 to strlen */

    if (FUNCTION == INIT) {
        Trace((stderr, "initializing buildpath to "));
        if ((buildpath = (char *)malloc(strlen(G.filename)+rootlen+1)) ==
            (char *)NULL)
            return 10;
        if ((rootlen > 0) && !renamed_fullpath) {
            strcpy(buildpath, rootpath);
            end = buildpath + rootlen;
        } else {
            *buildpath = '\0';
            end = buildpath;
        }
        Trace((stderr, "[%s]\n", buildpath));
        return 0;
    }

/*---------------------------------------------------------------------------
    ROOT:  if appropriate, store the path in rootpath and create it if neces-
    sary; else assume it's a zipfile member and return.  This path segment
    gets used in extracting all members from every zipfile specified on the
    command line.
  ---------------------------------------------------------------------------*/

#if (!defined(SFX) || defined(SFX_EXDIR))
    if (FUNCTION == ROOT) {
        Trace((stderr, "initializing root path to [%s]\n", pathcomp));
        if (pathcomp == (char *)NULL) {
            rootlen = 0;
            return 0;
        }
        if ((rootlen = strlen(pathcomp)) > 0) {
            int had_trailing_pathsep=FALSE;

            if (pathcomp[rootlen-1] == '/') {
                pathcomp[--rootlen] = '\0';
                had_trailing_pathsep = TRUE;
            }
            if (rootlen > 0 && (SSTAT(pathcomp, &G.statbuf) ||
                !S_ISDIR(G.statbuf.st_mode)))          /* path does not exist */
            {
                if (!G.create_dirs /* || iswild(pathcomp) */ ) {
                    rootlen = 0;
                    return 2;   /* skip (or treat as stored file) */
                }
                /* create the directory (could add loop here to scan pathcomp
                 * and create more than one level, but why really necessary?) */
                if (MKDIR(pathcomp, 0777) == -1) {
                    Info(slide, 1, ((char *)slide,
                      "checkdir:  can't create extraction directory: %s\n",
                      pathcomp));
                    rootlen = 0;   /* path didn't exist, tried to create, and */
                    return 3;  /* failed:  file exists, or 2+ levels required */
                }
            }
            if ((rootpath = (char *)malloc(rootlen+2)) == NULL) {
                rootlen = 0;
                return 10;
            }
            strcpy(rootpath, pathcomp);
            rootpath[rootlen++] = '/';
            rootpath[rootlen] = '\0';
            Trace((stderr, "rootpath now = [%s]\n", rootpath));
        }
        return 0;
    }
#endif /* !SFX || SFX_EXDIR */

/*---------------------------------------------------------------------------
    END:  free rootpath, immediately prior to program exit.
  ---------------------------------------------------------------------------*/

    if (FUNCTION == END) {
        Trace((stderr, "freeing rootpath\n"));
        if (rootlen > 0)
            free(rootpath);
        return 0;
    }

    return 99;  /* should never reach */

} /* end function checkdir() */





/****************************/
/* Function close_outfile() */
/****************************/

void close_outfile(__G)
    __GDEF
{
#ifdef USE_EF_UX_TIME
    ztimbuf z_utime;

    /* The following DOS date/time structure is machine dependent as it
     * assumes "little endian" byte order. For MSDOS specific code, which
     * is run on ix86 CPUs (or emulators), this assumption is valid; but
     * care should be taken when using this code as template for other ports.
     */
    union {
        ulg z_dostime;
        struct {                /* date and time words */
            union {             /* DOS file modification time word */
                ush ztime;
                struct {
                    unsigned zt_se : 5;
                    unsigned zt_mi : 6;
                    unsigned zt_hr : 5;
                } _tf;
            } _t;
            union {             /* DOS file modification date word */
                ush zdate;
                struct {
                    unsigned zd_dy : 5;
                    unsigned zd_mo : 4;
                    unsigned zd_yr : 7;
                } _df;
            } _d;
        } zt;
    } dos_dt;
#endif /* USE_EF_UX_TIME */

    if (G.cflag) {
	fclose(G.outfile);
        return;
    }

#ifdef USE_EF_UX_TIME
    if (G.extra_field &&
        ef_scan_for_izux(G.extra_field, G.lrec.extra_field_length,
                         &z_utime, NULL) > 0) {
        struct tm *t;

        TTrace((stderr, "close_outfile:  Unix e.f. modif. time = %ld\n",
          z_utime.modtime));
        /* round up to even seconds */
        z_utime.modtime = (z_utime.modtime + 1) & (~1);
        t = localtime(&(z_utime.modtime));
        if (t->tm_year < 80) {
            dos_dt.zt._t._tf.zt_se = 0;
            dos_dt.zt._t._tf.zt_mi = 0;
            dos_dt.zt._t._tf.zt_hr = 0;
            dos_dt.zt._d._df.zd_dy = 1;
            dos_dt.zt._d._df.zd_mo = 1;
            dos_dt.zt._d._df.zd_yr = 0;
        } else {
            dos_dt.zt._t._tf.zt_se = t->tm_sec >> 1;
            dos_dt.zt._t._tf.zt_mi = t->tm_min;
            dos_dt.zt._t._tf.zt_hr = t->tm_hour;
            dos_dt.zt._d._df.zd_dy = t->tm_mday;
            dos_dt.zt._d._df.zd_mo = t->tm_mon + 1;
            dos_dt.zt._d._df.zd_yr = t->tm_year - 80;
        }
    } else {
        dos_dt.zt._t.ztime = G.lrec.last_mod_file_time;
        dos_dt.zt._d.zdate = G.lrec.last_mod_file_date;
    }
    _dos_filedate(fileno(G.outfile), dos_dt.z_dostime);
#else /* !USE_EF_UX_TIME */
    _dos_filedate(fileno(G.outfile),
      (ulg)G.lrec.last_mod_file_date << 16 | G.lrec.last_mod_file_time);
#endif /* ?USE_EF_UX_TIME */

    fclose(G.outfile);

    _dos_chmod(G.filename, G.pInfo->file_attr);

} /* end function close_outfile() */




#ifndef SFX

/************************/
/*  Function version()  */
/************************/

void version(__G)
    __GDEF
{
    int len;
#if 0
    char buf[40];
#endif

    len = sprintf((char *)slide, LoadFarString(CompiledWith),

#ifdef __GNUC__
      "gcc ", __VERSION__,
#else
#  if 0
      "cc ", (sprintf(buf, " version %d", _RELEASE), buf),
#  else
      "unknown compiler", "",
#  endif
#endif

      "Human68k", " (X68000)",

#ifdef __DATE__
      " on ", __DATE__
#else
      "", ""
#endif
      );

    (*G.message)((zvoid *)&G, slide, (ulg)len, 0);

} /* end function version() */

#endif /* !SFX */





/* Human68K-specific routines */

#define VALID_CHAR "&#()@_^{}!"

extern ulg TwentyOneOptions(void);

static int multi_period = 0;
static int special_char = 0;

void
InitTwentyOne(void)
{
    ulg stat;

    stat = TwentyOneOptions();
    if (stat == 0 || stat == (unsigned long) -1) {
	special_char = 0;
	multi_period = 0;
	return;
    }
    if (stat & (1UL << 29))
	special_char = 1;
    if (stat & (1UL << 28))
	multi_period = 1;
}

static void
normalize_name(char *name)
{
    char *dot;
    char *p;

    if (strlen(name) > 18) {	/* too long */
	char base[18 + 1];
	char ext[4 + 1];

	if ((dot = jstrrchr(name, '.')) != NULL)
	    *dot = '\0';
	strncpy(base, name, 18);
	base[18] = '\0';
	if (dot) {
	    *dot = '.';
	    strncpy(ext, dot, 4);
	    ext[4] = '\0';
	} else
	    *ext = '\0';
	strcpy(name, base);
	strcat(name, ext);
    }
    dot = NULL;
    for (p = name; *p; p++) {
	if (iskanji((unsigned char)*p) && p[1] != '\0')
	    p++;
	else if (*p == '.') {
	    if (!multi_period) {
		dot = p;
		*p = '_';
	    }
	} else if (!special_char && !isalnum (*p)
		   && strchr(VALID_CHAR, *p) == NULL)
	    *p = '_';
    }
    if (dot != NULL) {
	*dot = '.';
	if (strlen(dot) > 4)
	    dot[4] = '\0';
    }
}
