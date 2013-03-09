/*---------------------------------------------------------------------------

  msdos.c

  MSDOS-specific routines for use with Info-ZIP's UnZip 5.2 and later.

  Contains:  Opendir()                      (from zip)
             Readdir()                      (from zip)
             do_wild()
             mapattr()
             mapname()
             map2fat()
             checkdir()
             isfloppy()
             volumelabel()                  (non-djgpp, non-emx)
             close_outfile()
             dateformat()
             version()
             _dos_getcountryinfo()          (djgpp, emx)
            [_dos_getftime()                (djgpp, emx)   to be added]
             _dos_setftime()                (djgpp, emx)
             _dos_setfileattr()             (djgpp, emx)
             _dos_getdrive()                (djgpp, emx)
             _dos_creat()                   (djgpp, emx)
             _dos_close()                   (djgpp, emx)
             volumelabel()                  (djgpp, emx)
             __crt0_glob_function()         (djgpp 2.x)
             __crt_load_environment_file()  (djgpp 2.x)

  ---------------------------------------------------------------------------*/



#define UNZIP_INTERNAL
#include "unzip.h"

#ifndef USE_VFAT
static void map2fat OF((char *pathcomp, char *last_dot));
#endif
static int isfloppy OF((int nDrive));
static int volumelabel OF((char *newlabel));

static int created_dir;        /* used by mapname(), checkdir() */
static int renamed_fullpath;   /* ditto */
static unsigned nLabelDrive;   /* ditto, plus volumelabel() */

/*****************************/
/*  Strings used in msdos.c  */
/*****************************/

#ifndef SFX
  static char Far CantAllocateWildcard[] =
    "";
#endif
static char Far Creating[] = "";
static char Far ConversionFailed[] = "";
static char Far Labelling[] = "";
static char Far ErrSetVolLabel[] = "";
static char Far PathTooLong[] = "";
static char Far CantCreateDir[] = "";
static char Far DirIsntDirectory[] =
  "";
static char Far PathTooLongTrunc[] =
  "";
#if (!defined(SFX) || defined(SFX_EXDIR))
   static char Far CantCreateExtractDir[] =
     "";
#endif
#ifdef __TURBOC__
   static char Far AttribsMayBeWrong[] =
     "";
#endif

#ifdef WATCOMC_386
#  define WREGS(v,r) (v##.w.##r)
#  define int86x int386x
#else
#  define WREGS(v,r) (v##.x.##r)
#endif

#if (defined(__GO32__) || defined(__EMX__))
#  include <dirent.h>        /* use readdir() */
#  define MKDIR(path,mode)   mkdir(path,mode)
#  define Opendir  opendir
#  define Readdir  readdir
#  define Closedir closedir
#  define zdirent  dirent
#  define zDIR     DIR
#  ifdef __EMX__
#    include <dos.h>
#    define GETDRIVE(d)      d = _getdrive()
#    define FA_LABEL         A_LABEL
#  else
#    define GETDRIVE(d)      _dos_getdrive(&d)
#  endif
#else /* !(__GO32__ || __EMX__) */
#  define MKDIR(path,mode)   mkdir(path)
#  ifdef __TURBOC__
#    define FATTR            FA_HIDDEN+FA_SYSTEM+FA_DIREC
#    define FVOLID           FA_VOLID
#    define FFIRST(n,d,a)    findfirst(n,(struct ffblk *)d,a)
#    define FNEXT(d)         findnext((struct ffblk *)d)
#    define GETDRIVE(d)      d=getdisk()+1
#    include <dir.h>
#  else /* !__TURBOC__ */
#    define FATTR            _A_HIDDEN+_A_SYSTEM+_A_SUBDIR
#    define FVOLID           _A_VOLID
#    define FFIRST(n,d,a)    _dos_findfirst(n,a,(struct find_t *)d)
#    define FNEXT(d)         _dos_findnext((struct find_t *)d)
#    define GETDRIVE(d)      _dos_getdrive(&d)
#    include <direct.h>
#  endif /* ?__TURBOC__ */
   typedef struct zdirent {
       char d_reserved[30];
       char d_name[13];
       int d_first;
   } zDIR;
   zDIR *Opendir OF((const char *));
   struct zdirent *Readdir OF((zDIR *));
#  define Closedir free




#ifndef SFX

/**********************/   /* Borland C++ 3.x has its own opendir/readdir */
/* Function Opendir() */   /*  library routines, but earlier versions don't, */
/**********************/   /*  so use ours regardless */

zDIR *Opendir(name)
    const char *name;        /* name of directory to open */
{
    zDIR *dirp;              /* malloc'd return value */
    char *nbuf;              /* malloc'd temporary string */
    int len = strlen(name);  /* path length to avoid strlens and strcats */


    if ((dirp = (zDIR *)malloc(sizeof(zDIR))) == (zDIR *)NULL)
        return (zDIR *)NULL;
    if ((nbuf = malloc(len + 5)) == (char *)NULL) {
        free(dirp);
        return (zDIR *)NULL;
    }
    strcpy(nbuf, name);
    if (nbuf[len-1] == ':') {
        nbuf[len++] = '.';
    } else if (nbuf[len-1] == '/' || nbuf[len-1] == '\\')
        --len;
    strcpy(nbuf+len, "/*.*");
    Trace((stderr, "Opendir:  nbuf = [%s]\n", nbuf));

    if (FFIRST(nbuf, dirp, FATTR)) {
        free((zvoid *)nbuf);
        return (zDIR *)NULL;
    }
    free((zvoid *)nbuf);
    dirp->d_first = 1;
    return dirp;
}





/**********************/
/* Function Readdir() */
/**********************/

struct zdirent *Readdir(d)
    zDIR *d;        /* directory stream from which to read */
{
    /* Return pointer to first or next directory entry, or NULL if end. */

    if (d->d_first)
        d->d_first = 0;
    else
        if (FNEXT(d))
            return (struct zdirent *)NULL;
    return (struct zdirent *)d;
}

#endif /* !SFX */
#endif /* ?(__GO32__ || __EMX__) */





#ifndef SFX

/************************/
/*  Function do_wild()  */   /* identical to OS/2 version */
/************************/

char *do_wild(__G__ wildspec)
    __GDEF
    char *wildspec;          /* only used first time on a given dir */
{
    static zDIR *dir = (zDIR *)NULL;
    static char *dirname, *wildname, matchname[FILNAMSIZ];
    static int firstcall=TRUE, have_dirname, dirnamelen;
    struct zdirent *file;


    /* Even when we're just returning wildspec, we *always* do so in
     * matchname[]--calling routine is allowed to append four characters
     * to the returned string, and wildspec may be a pointer to argv[].
     */
    if (firstcall) {        /* first call:  must initialize everything */
        firstcall = FALSE;

        if (!iswild(wildspec)) {
            strcpy(matchname, wildspec);
            have_dirname = FALSE;
            dir = NULL;
            return matchname;
        }

        /* break the wildspec into a directory part and a wildcard filename */
        if ((wildname = strrchr(wildspec, '/')) == (char *)NULL &&
            (wildname = strrchr(wildspec, ':')) == (char *)NULL) {
            dirname = ".";
            dirnamelen = 1;
            have_dirname = FALSE;
            wildname = wildspec;
        } else {
            ++wildname;     /* point at character after '/' or ':' */
            dirnamelen = (int)(wildname - wildspec);
            if ((dirname = (char *)malloc(dirnamelen+1)) == (char *)NULL) {
                Info(slide, 1, ((char *)slide,
                  LoadFarString(CantAllocateWildcard)));
                strcpy(matchname, wildspec);
                return matchname;   /* but maybe filespec was not a wildcard */
            }
/* GRR:  can't strip trailing char for opendir since might be "d:/" or "d:"
 *       (would have to check for "./" at end--let opendir handle it instead) */
            strncpy(dirname, wildspec, dirnamelen);
            dirname[dirnamelen] = '\0';       /* terminate for strcpy below */
            have_dirname = TRUE;
        }
        Trace((stderr, "do_wild:  dirname = [%s]\n", dirname));

        if ((dir = Opendir(dirname)) != (zDIR *)NULL) {
            while ((file = Readdir(dir)) != (struct zdirent *)NULL) {
                Trace((stderr, "do_wild:  readdir returns %s\n", file->d_name));
                if (match(file->d_name, wildname, 1)) {  /* 1 == ignore case */
                    Trace((stderr, "do_wild:  match() succeeds\n"));
                    if (have_dirname) {
                        strcpy(matchname, dirname);
                        strcpy(matchname+dirnamelen, file->d_name);
                    } else
                        strcpy(matchname, file->d_name);
                    return matchname;
                }
            }
            /* if we get to here directory is exhausted, so close it */
            Closedir(dir);
            dir = (zDIR *)NULL;
        }
        Trace((stderr, "do_wild:  Opendir(%s) returns NULL\n", dirname));

        /* return the raw wildspec in case that works (e.g., directory not
         * searchable, but filespec was not wild and file is readable) */
        strcpy(matchname, wildspec);
        return matchname;
    }

    /* last time through, might have failed opendir but returned raw wildspec */
    if (dir == (zDIR *)NULL) {
        firstcall = TRUE;  /* nothing left to try--reset for new wildspec */
        if (have_dirname)
            free(dirname);
        return (char *)NULL;
    }

    /* If we've gotten this far, we've read and matched at least one entry
     * successfully (in a previous call), so dirname has been copied into
     * matchname already.
     */
    while ((file = Readdir(dir)) != (struct zdirent *)NULL)
        if (match(file->d_name, wildname, 1)) {   /* 1 == ignore case */
            if (have_dirname) {
                /* strcpy(matchname, dirname); */
                strcpy(matchname+dirnamelen, file->d_name);
            } else
                strcpy(matchname, file->d_name);
            return matchname;
        }

    Closedir(dir);     /* have read at least one dir entry; nothing left */
    dir = (zDIR *)NULL;
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
    /* set archive bit (file is not backed up): */
    G.pInfo->file_attr = (unsigned)(G.crec.external_file_attributes | 32) &
      0xff;
    return 0;

} /* end function mapattr() */





/************************/
/*  Function mapname()  */
/************************/

int mapname(__G__ renamed)   /* return 0 if no error, 1 if caution (filename */
    __GDEF                   /* trunc), 2 if warning (skip file because dir */
    int renamed;             /* doesn't exist), 3 if error (skip file), 10 */
{                            /* if no memory (skip file) */
    char pathcomp[FILNAMSIZ];    /* path-component buffer */
    char *pp, *cp=(char *)NULL;  /* character pointers */
    char *lastsemi=(char *)NULL; /* pointer to last semi-colon in pathcomp */
#ifndef USE_VFAT
    char *last_dot=(char *)NULL; /* last dot not converted to underscore */
#endif
#ifndef USE_VFAT
    int dotname = FALSE;         /* flag:  path component begins with dot */
#endif
    int error = 0;
    register unsigned workch;    /* hold the character being tested */


/*---------------------------------------------------------------------------
    Initialize various pointers and counters and stuff.
  ---------------------------------------------------------------------------*/

    /* can create path as long as not just freshening, or if user told us */
    G.create_dirs = (!G.fflag || renamed);

    created_dir = FALSE;        /* not yet */
    renamed_fullpath = FALSE;

    if (renamed) {
        cp = G.filename - 1;    /* point to beginning of renamed name... */
        while (*++cp)
            if (*cp == '\\')    /* convert backslashes to forward */
                *cp = '/';
        cp = G.filename;
        /* use temporary rootpath if user gave full pathname */
        if (G.filename[0] == '/') {
            renamed_fullpath = TRUE;
            pathcomp[0] = '/';  /* copy the '/' and terminate */
            pathcomp[1] = '\0';
            ++cp;
        } else if (isalpha(G.filename[0]) && G.filename[1] == ':') {
            renamed_fullpath = TRUE;
            pp = pathcomp;
            *pp++ = *cp++;      /* copy the "d:" (+ '/', possibly) */
            *pp++ = *cp++;
            if (*cp == '/')
                *pp++ = *cp++;  /* otherwise add "./"? */
            *pp = '\0';
        }
    }

    /* pathcomp is ignored unless renamed_fullpath is TRUE: */
    if ((error = checkdir(__G__ pathcomp, INIT)) != 0)    /* initialize path buffer */
        return error;           /* ...unless no mem or vol label on hard disk */

    *pathcomp = '\0';           /* initialize translation buffer */
    pp = pathcomp;              /* point to translation buffer */
    if (!renamed) {             /* cp already set if renamed */
        if (G.jflag)            /* junking directories */
            cp = (char *)strrchr(G.filename, '/');
        if (cp == (char *)NULL) /* no '/' or not junking dirs */
            cp = G.filename;    /* point to internal zipfile-member pathname */
        else
            ++cp;               /* point to start of last component of path */
    }

/*---------------------------------------------------------------------------
    Begin main loop through characters in filename.
  ---------------------------------------------------------------------------*/

    while ((workch = (uch)*cp++) != 0) {

        switch (workch) {
        case '/':             /* can assume -j flag not given */
            *pp = '\0';
#ifndef USE_VFAT
            map2fat(pathcomp, last_dot);   /* 8.3 truncation (in place) */
            last_dot = (char *)NULL;
#endif
            if ((error = checkdir(__G__ pathcomp, APPEND_DIR)) > 1)
                return error;
            pp = pathcomp;    /* reset conversion buffer for next piece */
            lastsemi = (char *)NULL; /* leave directory semi-colons alone */
            break;

        /* drive names are not stored in zipfile, so no colons allowed;
         *  no brackets or most other punctuation either (all of which
         *  can appear in Unix-created archives; backslash is particularly
         *  bad unless all necessary directories exist) */
        case ':':
        case '\\':
        case '"':
#ifndef USE_VFAT
        case '[':
        case ']':
        case '+':
        case ',':
        case '=':
#endif
        case '<':
        case '>':
        case '|':
        case '*':
        case '?':
            *pp++ = '_';
            break;

#ifndef USE_VFAT
        case '.':
            if (pp == pathcomp) {     /* nothing appended yet... */
                if (*cp == '/') {     /* don't bother appending a "./" */
                    ++cp;             /*  component to the path:  skip */
                    break;            /*  to next char after the '/' */
                } else if (*cp == '.' && cp[1] == '/') {   /* "../" */
                    *pp++ = '.';      /* add first dot, unchanged... */
                    ++cp;             /* skip second dot, since it will */
                } else {              /*  be "added" at end of if-block */
                    *pp++ = '_';      /* FAT doesn't allow null filename */
                    dotname = TRUE;   /*  bodies, so map .exrc -> _.exrc */
                }                     /*  (extra '_' now, "dot" below) */
            } else if (dotname) {     /* found a second dot, but still */
                dotname = FALSE;      /*  have extra leading underscore: */
                *pp = '\0';           /*  remove it by shifting chars */
                pp = pathcomp + 1;    /*  left one space (e.g., .p1.p2: */
                while (pp[1]) {       /*  __p1 -> _p1_p2 -> _p1.p2 when */
                    *pp = pp[1];      /*  finished) [opt.:  since first */
                    ++pp;             /*  two chars are same, can start */
                }                     /*  shifting at second position] */
            }
            last_dot = pp;    /* point at last dot so far... */
            *pp++ = '_';      /* convert dot to underscore for now */
            break;
#endif

        case ';':             /* start of VMS version? */
#ifdef USE_VFAT
            lastsemi = pp;
            *pp++ = ';';      /* keep for now; remove VMS ";##" */
#else
            lastsemi = pp;    /* omit for now; remove VMS vers. later */
#endif
            break;

#ifndef USE_VFAT
        case ' ':             /* change spaces to underscore only */
            if (G.sflag)      /*  if specifically requested */
                *pp++ = '_';
            else
                *pp++ = (char)workch;
            break;
#endif

        default:
            /* allow ASCII 255 and European characters in filenames: */
            if (isprint(workch) || workch >= 127)
                *pp++ = (char)workch;
        } /* end switch */
    } /* end while loop */

    *pp = '\0';                   /* done with pathcomp:  terminate it */

    /* if not saving them, remove VMS version numbers (appended ";###") */
    if (!G.V_flag && lastsemi) {
#ifdef USE_VFAT
        pp = lastsemi + 1;
#else
        pp = lastsemi;            /* semi-colon was omitted:  expect all #'s */
#endif
        while (isdigit((uch)(*pp)))
            ++pp;
        if (*pp == '\0')          /* only digits between ';' and end:  nuke */
            *lastsemi = '\0';
    }

#ifndef USE_VFAT
    map2fat(pathcomp, last_dot);  /* 8.3 truncation (in place) */
#endif

/*---------------------------------------------------------------------------
    Report if directory was created (and no file to create:  filename ended
    in '/'), check name to be sure it exists, and combine path and name be-
    fore exiting.
  ---------------------------------------------------------------------------*/

    if (G.filename[strlen(G.filename) - 1] == '/') {
        checkdir(__G__ G.filename, GETPATH);
        if (created_dir && QCOND2) {
            Info(slide, 0, ((char *)slide, LoadFarString(Creating),
              G.filename));
            return IZ_CREATED_DIR;   /* set dir time (note trailing '/') */
        }
        return 2;   /* dir existed already; don't look for data to extract */
    }

    if (*pathcomp == '\0') {
        Info(slide, 1, ((char *)slide, LoadFarString(ConversionFailed),
          G.filename));
        return 3;
    }

    checkdir(__G__ pathcomp, APPEND_NAME);   /* returns 1 if truncated: care? */
    checkdir(__G__ G.filename, GETPATH);

    if (G.pInfo->vollabel) {    /* set the volume label now */
        if (QCOND2)
            Info(slide, 0, ((char *)slide, LoadFarString(Labelling),
              (nLabelDrive + 'a' - 1), G.filename));
        if (volumelabel(G.filename)) {
            Info(slide, 1, ((char *)slide, LoadFarString(ErrSetVolLabel)));
            return 3;
        }
        return 2;   /* success:  skip the "extraction" quietly */
    }

    return error;

} /* end function mapname() */





#ifndef USE_VFAT
/**********************/
/* Function map2fat() */
/**********************/

static void map2fat(pathcomp, last_dot)
    char *pathcomp, *last_dot;
{
    char *pEnd = pathcomp + strlen(pathcomp);

/*---------------------------------------------------------------------------
    Case 1:  filename has no dot, so figure out if we should add one.  Note
    that the algorithm does not try to get too fancy:  if there are no dots
    already, the name either gets truncated at 8 characters or the last un-
    derscore is converted to a dot (only if more characters are saved that
    way).  In no case is a dot inserted between existing characters.

              GRR:  have problem if filename is volume label??

  ---------------------------------------------------------------------------*/

    /* pEnd = pathcomp + strlen(pathcomp); */
    if (last_dot == (char *)NULL) {   /* no dots:  check for underscores... */
        char *plu = strrchr(pathcomp, '_');   /* pointer to last underscore */

        if (plu == (char *)NULL) {  /* no dots, no underscores:  truncate at */
            if (pEnd > pathcomp+8)  /* 8 chars (could insert '.' and keep 11) */
                *(pEnd = pathcomp+8) = '\0';
        } else if (MIN(plu - pathcomp, 8) + MIN(pEnd - plu - 1, 3) > 8) {
            last_dot = plu;       /* be lazy:  drop through to next if-block */
        } else if ((pEnd - pathcomp) > 8)    /* more fits into just basename */
            pathcomp[8] = '\0';    /* than if convert last underscore to dot */
        /* else whole thing fits into 8 chars or less:  no change */
    }

/*---------------------------------------------------------------------------
    Case 2:  filename has dot in it, so truncate first half at 8 chars (shift
    extension if necessary) and second half at three.
  ---------------------------------------------------------------------------*/

    if (last_dot != (char *)NULL) {   /* one dot (or two, in the case of */
        *last_dot = '.';              /*  "..") is OK:  put it back in */

        if ((last_dot - pathcomp) > 8) {
            char *p=last_dot, *q=pathcomp+8;
            int i;

            for (i = 0;  (i < 4) && *p;  ++i)  /* too many chars in basename: */
                *q++ = *p++;                   /*  shift extension left and */
            *q = '\0';                         /*  truncate/terminate it */
        } else if ((pEnd - last_dot) > 4)
            last_dot[4] = '\0';                /* too many chars in extension */
        /* else filename is fine as is:  no change */
    }
} /* end function map2fat() */
#endif





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
#ifdef MSC
    int attrs;                /* work around MSC stat() bug */
#endif

#   define FN_MASK   7
#   define FUNCTION  (flag & FN_MASK)



/*---------------------------------------------------------------------------
    APPEND_DIR:  append the path component to the path being built and check
    for its existence.  If doesn't exist and we are creating directories, do
    so for this one; else signal success or error as appropriate.
  ---------------------------------------------------------------------------*/

    if (FUNCTION == APPEND_DIR) {
        int too_long = FALSE;

        Trace((stderr, "appending dir segment [%s]\n", pathcomp));
        while ((*end = *pathcomp++) != '\0')
            ++end;

        /* GRR:  could do better check, see if overrunning buffer as we go:
         * check end-buildpath after each append, set warning variable if
         * within 20 of FILNAMSIZ; then if var set, do careful check when
         * appending.  Clear variable when begin new path. */

        if ((end-buildpath) > FILNAMSIZ-3)  /* need '/', one-char name, '\0' */
            too_long = TRUE;                /* check if extracting directory? */
#ifdef MSC /* MSC 6.00 bug:  stat(non-existent-dir) == 0 [exists!] */
        if (_dos_getfileattr(buildpath, &attrs) || stat(buildpath, &G.statbuf))
#else
        if (stat(buildpath, &G.statbuf))    /* path doesn't exist */
#endif
        {
            if (!G.create_dirs) { /* told not to create (freshening) */
                free(buildpath);
                return 2;         /* path doesn't exist:  nothing to do */
            }
            if (too_long) {
                Info(slide, 1, ((char *)slide, LoadFarString(PathTooLong),
                  buildpath));
                free(buildpath);
                return 4;         /* no room for filenames:  fatal */
            }
            if (MKDIR(buildpath, 0777) == -1) {   /* create the directory */
                Info(slide, 1, ((char *)slide, LoadFarString(CantCreateDir),
                  buildpath, G.filename));
                free(buildpath);
                return 3;      /* path didn't exist, tried to create, failed */
            }
            created_dir = TRUE;
        } else if (!S_ISDIR(G.statbuf.st_mode)) {
            Info(slide, 1, ((char *)slide, LoadFarString(DirIsntDirectory),
              buildpath, G.filename));
            free(buildpath);
            return 3;          /* path existed but wasn't dir */
        }
        if (too_long) {
            Info(slide, 1, ((char *)slide, LoadFarString(PathTooLong),
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
#ifdef NOVELL_BUG_WORKAROUND
        if (end == buildpath) {
            /* work-around for Novell's "overwriting executables" bug:
               prepend "./" to name when no path component is specified */
            *end++ = '.';
            *end++ = '/';
        }
#endif /* NOVELL_BUG_WORKAROUND */
        Trace((stderr, "appending filename [%s]\n", pathcomp));
        while ((*end = *pathcomp++) != '\0') {
            ++end;
            if ((end-buildpath) >= FILNAMSIZ) {
                *--end = '\0';
                Info(slide, 1, ((char *)slide, LoadFarString(PathTooLongTrunc),
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

    if (FUNCTION == INIT) {
        Trace((stderr, "initializing buildpath to "));
        /* allocate space for full filename, root path, and maybe "./" */
        if ((buildpath = (char *)malloc(strlen(G.filename)+rootlen+3)) ==
            (char *)NULL)
            return 10;
        if (G.pInfo->vollabel) {
/* GRR:  for network drives, do strchr() and return IZ_VOL_LABEL if not [1] */
            if (renamed_fullpath && pathcomp[1] == ':')
                *buildpath = ToLower(*pathcomp);
            else if (!renamed_fullpath && rootlen > 1 && rootpath[1] == ':')
                *buildpath = ToLower(*rootpath);
            else {
                GETDRIVE(nLabelDrive);   /* assumed that a == 1, b == 2, etc. */
                *buildpath = (char)(nLabelDrive - 1 + 'a');
            }
            nLabelDrive = *buildpath - 'a' + 1;        /* save for mapname() */
            if (G.volflag == 0 || *buildpath < 'a' ||  /* no label/bogus disk */
               (G.volflag == 1 && !isfloppy(nLabelDrive)))  /* -$:  no fixed */
            {
                free(buildpath);
                return IZ_VOL_LABEL;     /* skipping with message */
            }
            *buildpath = '\0';
            end = buildpath;
        } else if (renamed_fullpath) {   /* pathcomp = valid data */
            end = buildpath;
            while ((*end = *pathcomp++) != '\0')
                ++end;
        } else if (rootlen > 0) {
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
    command line.  Note that under OS/2 and MS-DOS, if a candidate extract-to
    directory specification includes a drive letter (leading "x:"), it is
    treated just as if it had a trailing '/'--that is, one directory level
    will be created if the path doesn't exist, unless this is otherwise pro-
    hibited (e.g., freshening).
  ---------------------------------------------------------------------------*/

#if (!defined(SFX) || defined(SFX_EXDIR))
    if (FUNCTION == ROOT) {
        Trace((stderr, "initializing root path to [%s]\n", pathcomp));
        if (pathcomp == (char *)NULL) {
            rootlen = 0;
            return 0;
        }
        if ((rootlen = strlen(pathcomp)) > 0) {
            int had_trailing_pathsep=FALSE, has_drive=FALSE, xtra=2;

            if (isalpha(pathcomp[0]) && pathcomp[1] == ':')
                has_drive = TRUE;   /* drive designator */
            if (pathcomp[rootlen-1] == '/') {
                pathcomp[--rootlen] = '\0';
                had_trailing_pathsep = TRUE;
            }
            if (has_drive && (rootlen == 2)) {
                if (!had_trailing_pathsep)   /* i.e., original wasn't "x:/" */
                    xtra = 3;      /* room for '.' + '/' + 0 at end of "x:" */
            } else if (rootlen > 0) {     /* need not check "x:." and "x:/" */
#ifdef MSC
                /* MSC 6.00 bug:  stat(non-existent-dir) == 0 [exists!] */
                if (_dos_getfileattr(pathcomp, &attrs) ||
                    SSTAT(pathcomp,&G.statbuf) || !S_ISDIR(G.statbuf.st_mode))
#else
                if (SSTAT(pathcomp,&G.statbuf) || !S_ISDIR(G.statbuf.st_mode))
#endif
                {
                    /* path does not exist */
                    if (!G.create_dirs /* || iswild(pathcomp) */ ) {
                        rootlen = 0;
                        return 2;   /* treat as stored file */
                    }
/* GRR:  scan for wildcard characters?  OS-dependent...  if find any, return 2:
 * treat as stored file(s) */
                    /* create directory (could add loop here to scan pathcomp
                     * and create more than one level, but really necessary?) */
                    if (MKDIR(pathcomp, 0777) == -1) {
                        Info(slide, 1, ((char *)slide,
                          LoadFarString(CantCreateExtractDir), pathcomp));
                        rootlen = 0;   /* path didn't exist, tried to create, */
                        return 3;  /* failed:  file exists, or need 2+ levels */
                    }
                }
            }
            if ((rootpath = (char *)malloc(rootlen+xtra)) == (char *)NULL) {
                rootlen = 0;
                return 10;
            }
            strcpy(rootpath, pathcomp);
            if (xtra == 3)                  /* had just "x:", make "x:." */
                rootpath[rootlen++] = '.';
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






/***********************/
/* Function isfloppy() */
/***********************/

static int isfloppy(nDrive)  /* more precisely, is it removable? */
    int nDrive;
{
    union REGS regs;

    regs.h.ah = 0x44;
    regs.h.al = 0x08;
    regs.h.bl = (uch)nDrive;
#ifdef __EMX__
    _int86(0x21, &regs, &regs);
    if (WREGS(regs,flags) & 1)
#else
    int86(0x21, &regs, &regs);
    if (WREGS(regs,cflag))        /* error:  do default a/b check instead */
#endif
    {
        Trace((stderr,
          "error in DOS function 0x44 (AX = 0x%04x):  guessing instead...\n",
          WREGS(regs,ax)));
        return (nDrive == 1 || nDrive == 2)? TRUE : FALSE;
    } else
        return WREGS(regs,ax)? FALSE : TRUE;
}




#if (!defined(__GO32__) && !defined(__EMX__))

typedef struct dosfcb {
    uch  flag;        /* ff to indicate extended FCB */
    char res[5];      /* reserved */
    uch  vattr;       /* attribute */
    uch  drive;       /* drive (1=A, 2=B, ...) */
    uch  vn[11];      /* file or volume name */
    char dmmy[5];
    uch  nn[11];      /* holds new name if renaming (else reserved) */
    char dmmy2[9];
} dos_fcb;

/**************************/
/* Function volumelabel() */
/**************************/

static int volumelabel(newlabel)
    char *newlabel;
{
#ifdef DEBUG
    char *p;
#endif
    int len = strlen(newlabel);
    dos_fcb  fcb, dta, far *pfcb=&fcb, far *pdta=&dta;
    struct SREGS sregs;
    union REGS regs;


/*---------------------------------------------------------------------------
    Label the diskette specified by nLabelDrive using FCB calls.  (Old ver-
    sions of MS-DOS and OS/2 DOS boxes can't use DOS function 3Ch to create
    labels.)  Must use far pointers for MSC FP_* macros to work; must pad
    FCB filenames with spaces; and cannot include dot in 8th position.  May
    or may not need to zero out FCBs before using; do so just in case.
  ---------------------------------------------------------------------------*/

    memset((char *)&dta, 0, sizeof(dos_fcb));
    memset((char *)&fcb, 0, sizeof(dos_fcb));

#ifdef DEBUG
    for (p = (char *)&dta; (p - (char *)&dta) < sizeof(dos_fcb); ++p)
        if (*p)
            fprintf(stderr, "error:  dta[%d] = %x\n", (p - (char *)&dta), *p);
    for (p = (char *)&fcb; (p - (char *)&fcb) < sizeof(dos_fcb); ++p)
        if (*p)
            fprintf(stderr, "error:  fcb[%d] = %x\n", (p - (char *)&fcb), *p);
    printf("testing pointer macros:\n");
    segread(&sregs);
    printf("cs = %x, ds = %x, es = %x, ss = %x\n", sregs.cs, sregs.ds, sregs.es,
      sregs.ss);
#endif /* DEBUG */

#if 0
#ifdef __TURBOC__
    bdosptr(0x1a, dta, DO_NOT_CARE);
#else
    (intdosx method below)
#endif
#endif /* 0 */

    /* set the disk transfer address for subsequent FCB calls */
    sregs.ds = FP_SEG(pdta);
    WREGS(regs,dx) = FP_OFF(pdta);
    Trace((stderr, "segment:offset of pdta = %x:%x\n", sregs.ds, WREGS(regs,dx)));
    Trace((stderr, "&dta = %lx, pdta = %lx\n", (ulg)&dta, (ulg)pdta));
    regs.h.ah = 0x1a;
    int86x(0x21, &regs, &regs, &sregs);

    /* fill in the FCB */
    sregs.ds = FP_SEG(pfcb);
    WREGS(regs,dx) = FP_OFF(pfcb);
    pfcb->flag = 0xff;          /* extended FCB */
    pfcb->vattr = 0x08;         /* attribute:  disk volume label */
    pfcb->drive = (uch)nLabelDrive;

#ifdef DEBUG
    Trace((stderr, "segment:offset of pfcb = %x:%x\n", sregs.ds, WREGS(regs,dx)));
    Trace((stderr, "&fcb = %lx, pfcb = %lx\n", (ulg)&fcb, (ulg)pfcb));
    Trace((stderr, "(2nd check:  labelling drive %c:)\n", pfcb->drive-1+'A'));
    if (pfcb->flag != fcb.flag)
        fprintf(stderr, "error:  pfcb->flag = %d, fcb.flag = %d\n",
          pfcb->flag, fcb.flag);
    if (pfcb->drive != fcb.drive)
        fprintf(stderr, "error:  pfcb->drive = %d, fcb.drive = %d\n",
          pfcb->drive, fcb.drive);
    if (pfcb->vattr != fcb.vattr)
        fprintf(stderr, "error:  pfcb->vattr = %d, fcb.vattr = %d\n",
          pfcb->vattr, fcb.vattr);
#endif /* DEBUG */

    /* check for existing label */
    Trace((stderr, "searching for existing label via FCBs\n"));
    regs.h.ah = 0x11;      /* FCB find first */
#if 0  /* THIS STRNCPY FAILS (MSC bug?): */
    strncpy(pfcb->vn, "???????????", 11);   /* i.e., "*.*" */
    Trace((stderr, "pfcb->vn = %lx\n", (ulg)pfcb->vn));
    Trace((stderr, "flag = %x, drive = %d, vattr = %x, vn = %s = %s.\n",
      fcb.flag, fcb.drive, fcb.vattr, fcb.vn, pfcb->vn));
#endif
    strncpy((char *)fcb.vn, "???????????", 11);   /* i.e., "*.*" */
    Trace((stderr, "fcb.vn = %lx\n", (ulg)fcb.vn));
    Trace((stderr, "regs.h.ah = %x, regs.x.dx = %04x, sregs.ds = %04x\n",
      regs.h.ah, WREGS(regs,dx), sregs.ds));
    Trace((stderr, "flag = %x, drive = %d, vattr = %x, vn = %s = %s.\n",
      fcb.flag, fcb.drive, fcb.vattr, fcb.vn, pfcb->vn));
    int86x(0x21, &regs, &regs, &sregs);

/*---------------------------------------------------------------------------
    If not previously labelled, write a new label.  Otherwise just rename,
    since MS-DOS 2.x has a bug which damages the FAT when the old label is
    deleted.
  ---------------------------------------------------------------------------*/

    if (regs.h.al) {
        Trace((stderr, "no label found\n\n"));
        regs.h.ah = 0x16;                 /* FCB create file */
        strncpy((char *)fcb.vn, newlabel, len);
        if (len < 11)   /* fill with spaces */
            strncpy((char *)(fcb.vn+len), "           ", 11-len);
        Trace((stderr, "fcb.vn = %lx  pfcb->vn = %lx\n", (ulg)fcb.vn,
          (ulg)pfcb->vn));
        Trace((stderr, "flag = %x, drive = %d, vattr = %x\n", fcb.flag,
          fcb.drive, fcb.vattr));
        Trace((stderr, "vn = %s = %s.\n", fcb.vn, pfcb->vn));
        int86x(0x21, &regs, &regs, &sregs);
        regs.h.ah = 0x10;                 /* FCB close file */
        if (regs.h.al) {
            Trace((stderr, "unable to write volume name (AL = %x)\n",
              regs.h.al));
            int86x(0x21, &regs, &regs, &sregs);
            return 1;
        } else {
            int86x(0x21, &regs, &regs, &sregs);
            Trace((stderr, "new volume label [%s] written\n", newlabel));
            return 0;
        }
    } else {
        Trace((stderr, "found old label [%s]\n\n", dta.vn));  /* not term. */
        regs.h.ah = 0x17;                 /* FCB rename */
        strncpy((char *)fcb.vn, (char *)dta.vn, 11);
        strncpy((char *)fcb.nn, newlabel, len);
        if (len < 11)                     /* fill with spaces */
            strncpy((char *)(fcb.nn+len), "           ", 11-len);
        Trace((stderr, "fcb.vn = %lx  pfcb->vn = %lx\n", (ulg)fcb.vn,
          (ulg)pfcb->vn));
        Trace((stderr, "fcb.nn = %lx  pfcb->nn = %lx\n", (ulg)fcb.nn,
          (ulg)pfcb->nn));
        Trace((stderr, "flag = %x, drive = %d, vattr = %x\n", fcb.flag,
          fcb.drive, fcb.vattr));
        Trace((stderr, "vn = %s = %s.\n", fcb.vn, pfcb->vn));
        Trace((stderr, "nn = %s = %s.\n", fcb.nn, pfcb->nn));
        int86x(0x21, &regs, &regs, &sregs);
        if (regs.h.al) {
            Trace((stderr, "Unable to change volume name (AL = %x)\n",
              regs.h.al));
            return 1;
        } else {
            Trace((stderr, "volume label changed to [%s]\n", newlabel));
            return 0;
        }
    }
} /* end function volumelabel() */

#endif /* !__GO32__ && !__EMX__ */





/****************************/
/* Function close_outfile() */
/****************************/

void close_outfile(__G)
    __GDEF
 /*
  * MS-DOS VERSION
  *
  * Set the output file date/time stamp according to information from the
  * zipfile directory record for this member, then close the file and set
  * its permissions (archive, hidden, read-only, system).  Aside from closing
  * the file, this routine is optional (but most compilers support it).
  */
{
#ifdef USE_EF_UX_TIME
    ztimbuf z_utime;

    /* The following DOS date/time structure is machine dependent as it
     * assumes "little endian" byte order. For MSDOS specific code, which
     * is run on x86 CPUs (or emulators), this assumption is valid; but
     * care should be taken when using this code as template for other ports.
     */
    union {
        ulg z_dostime;
# ifdef __TURBOC__
        struct ftime ft;        /* system file time record */
# endif
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
#else /* !USE_EF_UX_TIME */
# ifdef __TURBOC__
    union {
        struct ftime ft;        /* system file time record */
        struct {
            ush ztime;          /* date and time words */
            ush zdate;          /* .. same format as in .ZIP file */
        } zt;
    } dos_dt;
# endif
#endif /* ?USE_EF_UX_TIME */


/*---------------------------------------------------------------------------
    Copy and/or convert time and date variables, if necessary; then set the
    file time/date.  WEIRD BORLAND "BUG":  if output is buffered, and if run
    under at least some versions of DOS (e.g., 6.0), and if files are smaller
    than DOS physical block size (i.e., 512 bytes) (?), then files MAY NOT
    get timestamped correctly--apparently setftime() occurs before any data
    are written to the file, and when file is closed and buffers are flushed,
    timestamp is overwritten with current time.  Even with a 32K buffer, this
    does not seem to occur with larger files.  UnZip output is now unbuffered,
    but if it were not, could still avoid problem by adding "fflush(outfile)"
    just before setftime() call.  Weird, huh?
  ---------------------------------------------------------------------------*/

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
# ifdef __TURBOC__
    setftime(fileno(G.outfile), &dos_dt.ft);
# else
    _dos_setftime(fileno(G.outfile), dos_dt.zt._d.zdate, dos_dt.zt._t.ztime);
# endif
#else /* !USE_EF_UX_TIME */
# ifdef __TURBOC__
    dos_dt.zt.ztime = G.lrec.last_mod_file_time;
    dos_dt.zt.zdate = G.lrec.last_mod_file_date;
    setftime(fileno(G.outfile), &dos_dt.ft);
# else
    _dos_setftime(fileno(G.outfile), G.lrec.last_mod_file_date,
                                     G.lrec.last_mod_file_time);
# endif
#endif /* ?USE_EF_UX_TIME */

/*---------------------------------------------------------------------------
    And finally we can close the file...at least everybody agrees on how to
    do *this*.  I think...  Also change the mode according to the stored file
    attributes, since we didn't do that when we opened the dude.
  ---------------------------------------------------------------------------*/

    fclose(G.outfile);

#ifdef __TURBOC__
#   if (defined(__BORLANDC__) && (__BORLANDC__ >= 0x0452))
#     define Chmod  _rtl_chmod
#   else
#     define Chmod  _chmod
#   endif
    if (Chmod(G.filename, 1, G.pInfo->file_attr) != G.pInfo->file_attr)
        Info(slide, 1, ((char *)slide, LoadFarString(AttribsMayBeWrong)));
#else /* !__TURBOC__ */
    _dos_setfileattr(G.filename, G.pInfo->file_attr);
#endif /* ?__TURBOC__ */

} /* end function close_outfile() */





#ifndef SFX

/*************************/
/* Function dateformat() */
/*************************/

int dateformat()
{

/*---------------------------------------------------------------------------
    For those operating systems which support it, this function returns a
    value which tells how national convention says that numeric dates are
    displayed.  Return values are DF_YMD, DF_DMY and DF_MDY (the meanings
    should be fairly obvious).
  ---------------------------------------------------------------------------*/

#ifndef MSWIN
    unsigned short CountryInfo[18];
#if (!defined(__GO32__) && !defined(__EMX__))
    unsigned short far *_CountryInfo = CountryInfo;
    struct SREGS sregs;
    union REGS regs;

#ifdef __WATCOMC__
//    segread(&sregs);
#endif
    sregs.ds  = FP_SEG(_CountryInfo);
    WREGS(regs,dx) = FP_OFF(_CountryInfo);
    WREGS(regs,ax) = 0x3800;
    int86x(0x21, &regs, &regs, &sregs);

#else /* __GO32__ || __EMX__ */

    _dos_getcountryinfo(CountryInfo);
#endif

    switch(CountryInfo[0]) {
        case 0:
            return DF_MDY;
        case 1:
            return DF_DMY;
        case 2:
            return DF_YMD;
    }
#endif /* !MSWIN */

    return DF_MDY;   /* default for systems without locale info */

} /* end function dateformat() */





/************************/
/*  Function version()  */
/************************/

void version(__G)
    __GDEF
{
    int len;
#if defined(__DJGPP__) || defined(__WATCOMC__) || \
    (defined(_MSC_VER) && (_MSC_VER != 800))
    char buf[80];
#endif

    len = sprintf((char *)slide, LoadFarString(CompiledWith),

#ifdef __GNUC__
#  ifdef __DJGPP__
      (sprintf(buf, "djgpp v%d / gcc ", __DJGPP__), buf),
#  elif __GO32__                  /* __GO32__ is defined as "1" only (sigh) */
      "djgpp v1.x / gcc ",
#  elif defined(__EMX__)          /* ...so is __EMX__ (double sigh) */
      "emx+gcc ",
#  else
      "gcc ",
#  endif
      __VERSION__,
#elif defined(__WATCOMC__)
#  if (__WATCOMC__ % 10 != 0)
      "Watcom C/C++", (sprintf(buf, " %d.%02d", __WATCOMC__ / 100,
                               __WATCOMC__ % 100), buf),
#  else
      "Watcom C/C++", (sprintf(buf, " %d.%d", __WATCOMC__ / 100,
                               (__WATCOMC__ % 100) / 10), buf),
#  endif
#elif defined(__TURBOC__)
#  ifdef __BORLANDC__
      "Borland C++",
#    if (__BORLANDC__ < 0x0200)
        " 1.0",
#    elif (__BORLANDC__ == 0x0200)   /* James:  __TURBOC__ = 0x0297 */
        " 2.0",
#    elif (__BORLANDC__ == 0x0400)
        " 3.0",
#    elif (__BORLANDC__ == 0x0410)   /* __BCPLUSPLUS__ = 0x0310 */
        " 3.1",
#    elif (__BORLANDC__ == 0x0452)   /* __BCPLUSPLUS__ = 0x0320 */
        " 4.0 or 4.02",
#    elif (__BORLANDC__ == 0x0460)   /* __BCPLUSPLUS__ = 0x0340 */
        " 4.5",
#    elif (__BORLANDC__ == 0x0500)
        " 5.0",
#    else
        " later than 5.0",
#    endif
#  else
      "Turbo C",
#    if (__TURBOC__ > 0x0500)
        "++ later than 5.0",
#    elif (__TURBOC__ == 0x0500)     /* Mike W:  5.0 -> 0x0500 */
        "++ 5.0",
#    elif (__TURBOC__ >= 0x0400)     /* Kevin:  3.0 -> 0x0401 */
        "++ 3.0 or 4.x",
#    elif (__TURBOC__ == 0x0295)     /* [661] vfy'd by Kevin */
        "++ 1.0",
#    elif ((__TURBOC__ >= 0x018d) && (__TURBOC__ <= 0x0200)) /* James: 0x0200 */
        " 2.0",
#    elif (__TURBOC__ > 0x0100)
        " 1.5",                      /* James:  0x0105? */
#    else
        " 1.0",                      /* James:  0x0100 */
#    endif
#  endif
#elif defined(MSC)
      "Microsoft C ",
#  ifdef _MSC_VER
#    if (_MSC_VER == 800)
        "8.0/8.0c (Visual C++ 1.0/1.5)",
#    else
        (sprintf(buf, "%d.%02d", _MSC_VER/100, _MSC_VER%100), buf),
#    endif
#  else
      "5.1 or earlier",
#  endif
#else
      "unknown compiler", "",
#endif /* ?compilers */

      "MS-DOS",

#if (defined(__GNUC__) || defined(WATCOMC_386))
      " (32-bit)",
#else
#  if defined(M_I86HM) || defined(__HUGE__)
      " (16-bit, huge)",
#  else
#  if defined(M_I86LM) || defined(__LARGE__)
      " (16-bit, large)",
#  else
#  if defined(M_I86MM) || defined(__MEDIUM__)
      " (16-bit, medium)",
#  else
#  if defined(M_I86CM) || defined(__COMPACT__)
      " (16-bit, compact)",
#  else
#  if defined(M_I86SM) || defined(__SMALL__)
      " (16-bit, small)",
#  else
#  if defined(M_I86TM) || defined(__TINY__)
      " (16-bit, tiny)",
#  else
      " (16-bit)",
#  endif
#  endif
#  endif
#  endif
#  endif
#  endif
#endif

#ifdef __DATE__
      " on ", __DATE__
#else
      "", ""
#endif
    );

    (*G.message)((zvoid *)&G, slide, (ulg)len, 0);
                                /* MSC can't handle huge macro expansion */

    /* temporary debugging code for Borland compilers only */
#ifdef __TURBOC__
    Info(slide, 0, ((char *)slide, "\tdebug(__TURBOC__ = 0x%04x = %d)\n",
      __TURBOC__, __TURBOC__));
#ifdef __BORLANDC__
    Info(slide, 0, ((char *)slide, "\tdebug(__BORLANDC__ = 0x%04x)\n",
      __BORLANDC__));
#else
    Info(slide, 0, ((char *)slide, "\tdebug(__BORLANDC__ not defined)\n"));
#endif
#ifdef __TCPLUSPLUS__
    Info(slide, 0, ((char *)slide, "\tdebug(__TCPLUSPLUS__ = 0x%04x)\n",
      __TCPLUSPLUS__));
#else
    Info(slide, 0, ((char *)slide, "\tdebug(__TCPLUSPLUS__ not defined)\n"));
#endif
#ifdef __BCPLUSPLUS__
    Info(slide, 0, ((char *)slide, "\tdebug(__BCPLUSPLUS__ = 0x%04x)\n\n",
      __BCPLUSPLUS__));
#else
    Info(slide, 0, ((char *)slide, "\tdebug(__BCPLUSPLUS__ not defined)\n\n"));
#endif
#endif

} /* end function version() */

#endif /* !SFX */





#if (defined(__GO32__) || defined(__EMX__))

int volatile _doserrno;

unsigned _dos_getcountryinfo(void *countrybuffer)
{
    asm("movl %0, %%edx": : "g" (countrybuffer));
    asm("movl $0x3800, %eax");
    asm("int $0x21": : : "%eax", "%ebx", "%ecx", "%edx", "%esi", "%edi");
    _doserrno = 0;
    asm("jnc 1f");
    asm("movl %%eax, %0": "=m" (_doserrno));
    asm("1:");
    return _doserrno;
}


#if (!defined(__DJGPP__) || (__DJGPP__ < 2))

void _dos_setftime(int fd, ush dosdate, ush dostime)
{
    asm("movl %0, %%ebx": : "g" (fd));
    asm("movl %0, %%ecx": : "g" (dostime));
    asm("movl %0, %%edx": : "g" (dosdate));
    asm("movl $0x5701, %eax");
    asm("int $0x21": : : "%eax", "%ebx", "%ecx", "%edx", "%esi", "%edi");
}

void _dos_setfileattr(char *name, int attr)
{
    asm("movl %0, %%edx": : "g" (name));
    asm("movl %0, %%ecx": : "g" (attr));
    asm("movl $0x4301, %eax");
    asm("int $0x21": : : "%eax", "%ebx", "%ecx", "%edx", "%esi", "%edi");
}

void _dos_getdrive(unsigned *d)
{
    asm("movl $0x1900, %eax");
    asm("int $0x21": : : "%eax", "%ebx", "%ecx", "%edx", "%esi", "%edi");
    asm("xorb %ah, %ah");
    asm("incb %al");
    asm("movl %%eax, %0": "=a" (*d));
}

unsigned _dos_creat(char *path, unsigned attr, int *fd)
{
    asm("movl $0x3c00, %eax");
    asm("movl %0, %%edx": :"g" (path));
    asm("movl %0, %%ecx": :"g" (attr));
    asm("int $0x21": : : "%eax", "%ebx", "%ecx", "%edx", "%esi", "%edi");
    asm("movl %%eax, %0": "=a" (*fd));
    _doserrno = 0;
    asm("jnc 1f");
    _doserrno = *fd;
    switch (_doserrno) {
    case 3:
           errno = ENOENT;
           break;
    case 4:
           errno = EMFILE;
           break;
    case 5:
           errno = EACCES;
           break;
    }
    asm("1:");
    return _doserrno;
}

unsigned _dos_close(int fd)
{
    asm("movl %0, %%ebx": : "g" (fd));
    asm("movl $0x3e00, %eax");
    asm("int $0x21": : : "%eax", "%ebx", "%ecx", "%edx", "%esi", "%edi");
    _doserrno = 0;
    asm("jnc 1f");
    asm ("movl %%eax, %0": "=m" (_doserrno));
    if (_doserrno == 6) {
          errno = EBADF;
    }
    asm("1:");
    return _doserrno;
}

#endif /* !__DJGPP__ || (__DJGPP__ < 2) */


static int volumelabel(char *name)
{
    int fd;

    return _dos_creat(name, FA_LABEL, &fd) ? fd : _dos_close(fd);
}


#if (defined(__DJGPP__) && (__DJGPP__ > 1))

/* Prevent globbing of filenames.  This gives the same functionality as
 * "stubedit <program> globbing=no" did with DJGPP v1.
 */
int __crt0_glob_function(void)
{
    return 0;
}

/* Reduce the size of the executable and remove the functionality to read
 * the program's environment from whatever $DJGPP points to.
 */
void __crt_load_environment_file(void)
{
}

#endif /* __DJGPP__ > 1 */
#endif /* __GO32__ || __EMX__ */
