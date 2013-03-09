/*---------------------------------------------------------------------------

  mac.c

  Macintosh-specific routines for use with Info-ZIP's UnZip 5.1 and later.

  This source file incorporates the contents of what was formerly macfile.c,
  which supported commands (such as mkdir()) not available directly on the
  Mac, and which also determined whether HFS (Hierarchical File System) or
  MFS (Macintosh File System) was in use.

  Contains:  do_wild()
             mapattr()
             mapname()
             checkdir()
             close_outfile()
             version()
             IsHFSDisk()
             MacFSTest()
             macmkdir()
             ResolveMacVol()
             macopen()
             macfopen()
             maccreat()
             macread()
             macwrite()
             macclose()
             maclseek()

  ---------------------------------------------------------------------------*/



#define UNZIP_INTERNAL
#include "unzip.h"

#ifdef MACOS
#ifndef FSFCBLen
#  define FSFCBLen  (*(short *)0x3F6)
#endif

#define read_only   file_attr   /* for readability only */

static short wAppVRefNum;
static long lAppDirID;
/* int HFSFlag;            /* set if disk has hierarchical file system */

static int created_dir;        /* used in mapname(), checkdir() */
static int renamed_fullpath;   /* ditto */

#define MKDIR(path)     macmkdir(path, G.gnVRefNum, G.glDirID)



#ifndef SFX

/**********************/
/* Function do_wild() */   /* for porting:  dir separator; match(ignore_case) */
/**********************/

char *do_wild(__G__ wildspec)
    __GDEF
    char *wildspec;         /* only used first time on a given dir */
{
    static DIR *dir = (DIR *)NULL;
    static char *dirname, *wildname, matchname[FILNAMSIZ];
    static int firstcall=TRUE, have_dirname, dirnamelen;
    struct direct *file;


    /* Even when we're just returning wildspec, we *always* do so in
     * matchname[]--calling routine is allowed to append four characters
     * to the returned string, and wildspec may be a pointer to argv[].
     */
    if (firstcall) {        /* first call:  must initialize everything */
        firstcall = FALSE;

        /* break the wildspec into a directory part and a wildcard filename */
        if ((wildname = strrchr(wildspec, ':')) == (char *)NULL) {
            dirname = ":";
            dirnamelen = 1;
            have_dirname = FALSE;
            wildname = wildspec;
        } else {
            ++wildname;     /* point at character after ':' */
            dirnamelen = wildname - wildspec;
            if ((dirname = (char *)malloc(dirnamelen+1)) == (char *)NULL) {
                Info(slide, 0x201, ((char *)slide,
                  "warning:  can't allocate wildcard buffers\n"));
                strcpy(matchname, wildspec);
                return matchname;   /* but maybe filespec was not a wildcard */
            }
            strncpy(dirname, wildspec, dirnamelen);
            dirname[dirnamelen] = '\0';   /* terminate for strcpy below */
            have_dirname = TRUE;
        }

        if ((dir = opendir(dirname)) != (DIR *)NULL) {
            while ((file = readdir(dir)) != (struct direct *)NULL) {
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
            dir = (DIR *)NULL;
        }

        /* return the raw wildspec in case that works (e.g., directory not
         * searchable, but filespec was not wild and file is readable) */
        strcpy(matchname, wildspec);
        return matchname;
    }

    /* last time through, might have failed opendir but returned raw wildspec */
    if (dir == (DIR *)NULL) {
        firstcall = TRUE;  /* nothing left to try--reset for new wildspec */
        if (have_dirname)
            free(dirname);
        return (char *)NULL;
    }

#ifndef THINK_C            /* Think C only matches one at most (for now) */
    /* If we've gotten this far, we've read and matched at least one entry
     * successfully (in a previous call), so dirname has been copied into
     * matchname already.
     */
    while ((file = readdir(dir)) != (struct direct *)NULL)
        if (match(file->d_name, wildname, 0)) {   /* 0 == don't ignore case */
            if (have_dirname) {
                /* strcpy(matchname, dirname); */
                strcpy(matchname+dirnamelen, file->d_name);
            } else
                strcpy(matchname, file->d_name);
            return matchname;
        }
#endif

    closedir(dir);     /* have read at least one dir entry; nothing left */
    dir = (DIR *)NULL;
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
    /* only care about read-only bit, so just look at MS-DOS side of attrs */
    G.pInfo->read_only = (unsigned)(G.crec.external_file_attributes & 1);
    return 0;

} /* end function mapattr() */





/************************/
/*  Function mapname()  */
/************************/

int mapname(__G__ renamed)   /* return 0 if no error, 1 if caution (filename */
    __GDEF                   /* truncated), 2 if warning (skip file because  */
    int renamed;             /* dir doesn't exist), 3 if error (skip file),  */
{                            /* 10 if no memory (skip file) */
    char pathcomp[FILNAMSIZ];    /* path-component buffer */
    char *pp, *cp=(char *)NULL;  /* character pointers */
    char *lastsemi=(char *)NULL; /* pointer to last semi-colon in pathcomp */
    int quote = FALSE;           /* flags */
    int error = 0;
    register unsigned workch;    /* hold the character being tested */


/*---------------------------------------------------------------------------
    Initialize various pointers and counters and stuff.
  ---------------------------------------------------------------------------*/

    /* can create path as long as not just freshening, or if user told us */
    G.create_dirs = (!G.fflag || renamed);

    created_dir = FALSE;        /* not yet */

    /* user gave full pathname:  don't prepend rootpath */
    renamed_fullpath = (renamed && (*G.filename == '/'));

    if (checkdir(__G__ (char *)NULL, INIT) == 10)
        return 10;              /* initialize path buffer, unless no memory */

    pp = pathcomp;              /* point to translation buffer */
    if (!(renamed_fullpath || G.jflag))
        *pp++ = ':';
    *pp = '\0';

    if (G.jflag)                /* junking directories */
        cp = (char *)strrchr(G.filename, '/');
    if (cp == (char *)NULL)     /* no '/' or not junking dirs */
        cp = G.filename;        /* point to internal zipfile-member pathname */
    else
        ++cp;                   /* point to start of last component of path */

/*---------------------------------------------------------------------------
    Begin main loop through characters in filename.
  ---------------------------------------------------------------------------*/

    while ((workch = (uch)*cp++) != 0) {

        if (quote) {                 /* if character quoted, */
            *pp++ = (char)workch;    /*  include it literally */
            quote = FALSE;
        } else
            switch (workch) {
            case '/':             /* can assume -j flag not given */
                *pp = '\0';
                if ((error = checkdir(__G__ pathcomp, APPEND_DIR)) > 1)
                    return error;
                pp = pathcomp;    /* reset conversion buffer for next piece */
                lastsemi = (char *)NULL; /* leave directory semi-colons alone */
                break;

            case ';':             /* VMS version (or DEC-20 attrib?) */
                lastsemi = pp;         /* keep for now; remove VMS ";##" */
                *pp++ = (char)workch;  /*  later, if requested */
                break;

            case '\026':          /* control-V quote for special chars */
                quote = TRUE;     /* set flag for next character */
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
            Info(slide, 0, ((char *)slide, "   creating: %s\n", G.filename));
            return IZ_CREATED_DIR;   /* set dir time (note trailing '/') */
        }
        return 2;   /* dir existed already; don't look for data to extract */
    }

    if (*pathcomp == '\0') {
        Info(slide, 1, ((char *)slide, "mapname:  conversion of %s failed\n",
          G.filename));
        return 3;
    }

    checkdir(__G__ pathcomp, APPEND_NAME);   /* returns 1 if truncated: care? */
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
#ifdef SHORT_NAMES
        char *old_end = end;
#endif

        Trace((stderr, "appending dir segment [%s]\n", pathcomp));
        while ((*end = *pathcomp++) != '\0')
            ++end;
#ifdef SHORT_NAMES   /* path components restricted to 14 chars, typically */
        if ((end-old_end) > FILENAME_MAX)  /* GRR:  proper constant? */
            *(end = old_end + FILENAME_MAX) = '\0';
#endif

        /* GRR:  could do better check, see if overrunning buffer as we go:
         * check end-buildpath after each append, set warning variable if
         * within 20 of FILNAMSIZ; then if var set, do careful check when
         * appending.  Clear variable when begin new path. */

        if ((end-buildpath) > FILNAMSIZ-3)  /* need ':', one-char name, '\0' */
            too_long = TRUE;                /* check if extracting directory? */
        if (stat(buildpath, &G.statbuf)) {  /* path doesn't exist */
            if (!G.create_dirs) { /* told not to create (freshening) */
                free(buildpath);
                return 2;         /* path doesn't exist:  nothing to do */
            }
            if (too_long) {
                Info(slide, 1, ((char *)slide,
                  "checkdir error:  path too long: %s\n", buildpath));
                free(buildpath);
                return 4;         /* no room for filenames:  fatal */
            }
            if (MKDIR(buildpath) == -1) {   /* create the directory */
                Info(slide, 1, ((char *)slide,
                  "checkdir error:  can't create %s\n\
                 unable to process %s.\n", buildpath, G.filename));
                free(buildpath);
                return 3;      /* path didn't exist, tried to create, failed */
            }
            created_dir = TRUE;
        } else if (!S_ISDIR(G.statbuf.st_mode)) {
            Info(slide, 1, ((char *)slide,
              "checkdir error:  %s exists but is not directory\n\
                 unable to process %s.\n", buildpath, G.filename));
            free(buildpath);
            return 3;          /* path existed but wasn't dir */
        }
        if (too_long) {
            Info(slide, 1, ((char *)slide,
              "checkdir error:  path too long: %s\n", buildpath));
            free(buildpath);
            return 4;         /* no room for filenames:  fatal */
        }
        *end++ = ':';
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
#ifdef SHORT_NAMES
        char *old_end = end;
#endif

        Trace((stderr, "appending filename [%s]\n", pathcomp));
        while ((*end = *pathcomp++) != '\0') {
            ++end;
#ifdef SHORT_NAMES  /* truncate name at 14 characters, typically */
            if ((end-old_end) > FILENAME_MAX)      /* GRR:  proper constant? */
                *(end = old_end + FILENAME_MAX) = '\0';
#endif
            if ((end-buildpath) >= FILNAMSIZ) {
                *--end = '\0';
                Info(slide, 0x201, ((char *)slide,
                  "checkdir warning:  path too long; truncating\n\
                   %s\n                -> %s\n", G.filename, buildpath));
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
        if ((buildpath = (char *)malloc(strlen(G.filename)+rootlen+2)) ==
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
            if (pathcomp[rootlen-1] == ':') {
                pathcomp[--rootlen] = '\0';
            }
            if (rootlen > 0 && (stat(pathcomp, &G.statbuf) ||
                !S_ISDIR(G.statbuf.st_mode)))       /* path does not exist */
            {
                if (!G.create_dirs /* || iswild(pathcomp) */ ) {
                    rootlen = 0;
                    return 2;   /* skip (or treat as stored file) */
                }
                /* create the directory (could add loop here to scan pathcomp
                 * and create more than one level, but why really necessary?) */
                if (MKDIR(pathcomp) == -1) {
                    Info(slide, 1, ((char *)slide,
                      "checkdir:  can't create extraction directory: %s\n",
                      pathcomp));
                    rootlen = 0;   /* path didn't exist, tried to create, and */
                    return 3;  /* failed:  file exists, or 2+ levels required */
                }
            }
            if ((rootpath = (char *)malloc(rootlen+2)) == (char *)NULL) {
                rootlen = 0;
                return 10;
            }
            strcpy(rootpath, pathcomp);
            rootpath[rootlen++] = ':';
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

void close_outfile(__G)    /* GRR: change to return PK-style warning level */
    __GDEF
{
#ifdef USE_EF_UX_TIME
    ztimbuf z_utime;
#endif
    long m_time;
    DateTimeRec dtr;
    ParamBlockRec pbr;
    HParamBlockRec hpbr;
    OSErr err;


    if (fileno(G.outfile) == 1)  /* don't attempt to close or set time on stdout */
        return;

    fclose(G.outfile);

    /*
     * Macintosh bases all file modification times on the number of seconds
     * elapsed since Jan 1, 1904, 00:00:00.  Therefore, to maintain
     * compatibility with MS-DOS archives, which date from Jan 1, 1980,
     * with NO relation to GMT, the following conversions must be made:
     *      the Year (yr) must be incremented by 1980;
     *      and converted to seconds using the Mac routine Date2Secs(),
     *      almost similar in complexity to the Unix version :-)
     *                                     J. Lee
     */

#ifdef USE_EF_UX_TIME
    if (G.extra_field &&
        ef_scan_for_izux(G.extra_field, G.lrec.extra_field_length,
                         &z_utime, NULL) > 0) {
        struct tm *t;

        TTrace((stderr, "close_outfile:  Unix e.f. modif. time = %ld\n",
          z_utime.modtime));
        t = localtime(&(z_utime.modtime));

        dtr.year = t->tm_year;
        dtr.month = t->tm_mon + 1;
        dtr.day = t->tm_mday;

        dtr.hour = t->tm_hour;
        dtr.minute = t->tm_min;
        dtr.second = t->tm_sec;
    } else {
        dtr.year = (((G.lrec.last_mod_file_date >> 9) & 0x7f) + 1980);
        dtr.month = ((G.lrec.last_mod_file_date >> 5) & 0x0f);
        dtr.day = (G.lrec.last_mod_file_date & 0x1f);

        dtr.hour = ((G.lrec.last_mod_file_time >> 11) & 0x1f);
        dtr.minute = ((G.lrec.last_mod_file_time >> 5) & 0x3f);
        dtr.second = ((G.lrec.last_mod_file_time & 0x1f) * 2);
    }
#else /* !USE_EF_UX_TIME */
    dtr.year = (((G.lrec.last_mod_file_date >> 9) & 0x7f) + 1980);
    dtr.month = ((G.lrec.last_mod_file_date >> 5) & 0x0f);
    dtr.day = (G.lrec.last_mod_file_date & 0x1f);

    dtr.hour = ((G.lrec.last_mod_file_time >> 11) & 0x1f);
    dtr.minute = ((G.lrec.last_mod_file_time >> 5) & 0x3f);
    dtr.second = ((G.lrec.last_mod_file_time & 0x1f) * 2);
#endif /* ?USE_EF_UX_TIME */

    Date2Secs(&dtr, (unsigned long *)&m_time);
    c2pstr(G.filename);
    if (G.HFSFlag) {
        hpbr.fileParam.ioNamePtr = (StringPtr)G.filename;
        hpbr.fileParam.ioVRefNum = G.gnVRefNum;
        hpbr.fileParam.ioDirID = G.glDirID;
        hpbr.fileParam.ioFDirIndex = 0;
        err = PBHGetFInfo(&hpbr, 0L);
        hpbr.fileParam.ioFlMdDat = m_time;
        if ( !G.fMacZipped )
            hpbr.fileParam.ioFlCrDat = m_time;
        hpbr.fileParam.ioDirID = G.glDirID;
        if (err == noErr)
            err = PBHSetFInfo(&hpbr, 0L);
        if (err != noErr)
            printf("error:  can't set the time for %s\n", G.filename);
    } else {
        pbr.fileParam.ioNamePtr = (StringPtr)G.filename;
        pbr.fileParam.ioVRefNum = pbr.fileParam.ioFVersNum =
          pbr.fileParam.ioFDirIndex = 0;
        err = PBGetFInfo(&pbr, 0L);
        pbr.fileParam.ioFlMdDat = pbr.fileParam.ioFlCrDat = m_time;
        if (err == noErr)
            err = PBSetFInfo(&pbr, 0L);
        if (err != noErr)
            printf("error:  can't set the time for %s\n", G.filename);
    }

    /* set read-only perms if needed */
    if ((err == noErr) && G.pInfo->read_only) {
        if (G.HFSFlag) {
            hpbr.fileParam.ioNamePtr = (StringPtr)G.filename;
            hpbr.fileParam.ioVRefNum = G.gnVRefNum;
            hpbr.fileParam.ioDirID = G.glDirID;
            err = PBHSetFLock(&hpbr, 0);
        } else
            err = SetFLock((ConstStr255Param)G.filename, 0);
    }
    p2cstr(G.filename);

} /* end function close_outfile() */





#ifndef SFX

/************************/
/*  Function version()  */
/************************/

void version(__G)
    __GDEF
{
#if 0
    char buf[40];
#endif

    sprintf((char *)slide, LoadFarString(CompiledWith),

#ifdef __GNUC__
      "gcc ", __VERSION__,
#else
#  if 0
      "cc ", (sprintf(buf, " version %d", _RELEASE), buf),
#  else
#  ifdef THINK_C
      "Think C", "",
#  else
#  ifdef MPW
      "MPW C", "",
#  else
      "unknown compiler", "",
#  endif
#  endif
#  endif
#endif

      "MacOS",

#if defined(foobar) || defined(FOOBAR)
      " (Foo BAR)",    /* hardware or OS version */
#else
      "",
#endif /* Foo BAR */

#ifdef __DATE__
      " on ", __DATE__
#else
      "", ""
#endif
    );

    (*G.message)((zvoid *)&G, slide, (ulg)strlen((char *)slide), 0);

} /* end function version() */

#endif /* !SFX */





/************************/
/* Function IsHFSDisk() */
/************************/

static int IsHFSDisk(short wRefNum)
{
    /* get info about the specified volume */
    if (G.HFSFlag == true) {
        HParamBlockRec    hpbr;
        Str255 temp;
        short wErr;

        hpbr.volumeParam.ioCompletion = 0;
        hpbr.volumeParam.ioNamePtr = temp;
        hpbr.volumeParam.ioVRefNum = wRefNum;
        hpbr.volumeParam.ioVolIndex = 0;
        wErr = PBHGetVInfo(&hpbr, 0);

        if (wErr == noErr && hpbr.volumeParam.ioVFSID == 0
            && hpbr.volumeParam.ioVSigWord == 0x4244) {
                return true;
        }
    }

    return false;
} /* IsHFSDisk */





/************************/
/* Function MacFSTest() */
/************************/

void MacFSTest(int vRefNum)
{
    Str255 st;

    /* is this machine running HFS file system? */
    if (FSFCBLen <= 0) {
        G.HFSFlag = false;
    }
    else
    {
        G.HFSFlag = true;
    }

    /* get the file's volume reference number and directory ID */
    if (G.HFSFlag == true) {
        WDPBRec    wdpb;
        OSErr err = noErr;

        if (vRefNum != 0) {
            wdpb.ioCompletion = false;
            wdpb.ioNamePtr = st;
            wdpb.ioWDIndex = 0;
            wdpb.ioVRefNum = vRefNum;
            err = PBHGetVol(&wdpb, false);

            if (err == noErr) {
                wAppVRefNum = wdpb.ioWDVRefNum;
                lAppDirID = wdpb.ioWDDirID;
            }
        }

        /* is the disk we're using formatted for HFS? */
        G.HFSFlag = IsHFSDisk(wAppVRefNum);
    }

    return;
} /* mactest */





/***********************/
/* Function macmkdir() */
/***********************/

int macmkdir(char *path, short nVRefNum, long lDirID)
{
    OSErr    err = -1;

    if (path != 0 && strlen(path)<256 && G.HFSFlag == true) {
        HParamBlockRec    hpbr;
        Str255    st;

        c2pstr(path);
        if ((nVRefNum == 0) && (lDirID == 0))
        {
            hpbr.fileParam.ioNamePtr = st;
            hpbr.fileParam.ioCompletion = NULL;
            err = PBHGetVol((WDPBPtr)&hpbr, false);
            nVRefNum = hpbr.wdParam.ioWDVRefNum;
            lDirID = hpbr.wdParam.ioWDDirID;
        }
        else
        {
            err = noErr;
        }
        if (err == noErr) {
            hpbr.fileParam.ioCompletion = NULL;
            hpbr.fileParam.ioVRefNum = nVRefNum;
            hpbr.fileParam.ioDirID = lDirID;
            hpbr.fileParam.ioNamePtr = (StringPtr)path;
            err = PBDirCreate(&hpbr, false);
        }
        p2cstr(path);
    }

    return (int)err;
} /* macmkdir */





/****************************/
/* Function ResolveMacVol() */
/****************************/

void ResolveMacVol(short nVRefNum, short *pnVRefNum, long *plDirID, StringPtr pst)
{
    if (G.HFSFlag)
    {
        WDPBRec  wdpbr;
        Str255   st;
        OSErr    err;

        wdpbr.ioCompletion = (ProcPtr)NULL;
        wdpbr.ioNamePtr = st;
        wdpbr.ioVRefNum = nVRefNum;
        wdpbr.ioWDIndex = 0;
        wdpbr.ioWDProcID = 0;
        wdpbr.ioWDVRefNum = 0;
        err = PBGetWDInfo( &wdpbr, false );
        if ( err == noErr )
        {
            if (pnVRefNum)
                *pnVRefNum = wdpbr.ioWDVRefNum;
            if (plDirID)
                *plDirID = wdpbr.ioWDDirID;
            if (pst)
                BlockMove( st, pst, st[0]+1 );
        }
    }
    else
    {
        if (pnVRefNum)
            *pnVRefNum = nVRefNum;
        if (plDirID)
            *plDirID = 0;
        if (pst)
            *pst = 0;
    }
}





/**********************/
/* Function macopen() */
/**********************/

short macopen(char *sz, short nFlags, short nVRefNum, long lDirID)
{
    OSErr   err;
    Str255  st;
    char    chPerms = (!nFlags) ? fsRdPerm : fsRdWrPerm;
    short   nFRefNum;

    c2pstr( sz );
    BlockMove( sz, st, sz[0]+1 );
    p2cstr( sz );
    if (G.HFSFlag)
    {
        if (nFlags > 1)
            err = HOpenRF( nVRefNum, lDirID, st, chPerms, &nFRefNum);
        else
            err = HOpen( nVRefNum, lDirID, st, chPerms, &nFRefNum);
    }
    else
    {
        /*
         * Have to use PBxxx style calls since the high level
         * versions don't support specifying permissions
         */
        ParamBlockRec    pbr;

        pbr.ioParam.ioNamePtr = st;
        pbr.ioParam.ioVRefNum = G.gnVRefNum;
        pbr.ioParam.ioVersNum = 0;
        pbr.ioParam.ioPermssn = chPerms;
        pbr.ioParam.ioMisc = 0;
        if (nFlags >1)
            err = PBOpenRF( &pbr, false );
        else
            err = PBOpen( &pbr, false );
        nFRefNum = pbr.ioParam.ioRefNum;
    }
    if ( err || (nFRefNum == 1) )
        return -1;
    else {
        if ( nFlags )
            SetEOF( nFRefNum, 0 );
        return nFRefNum;
    }
}





/***********************/
/* Function macfopen() */
/***********************/

FILE *macfopen(char *filename, char *mode, short nVRefNum, long lDirID)
    {
        short outfd, fDataFork=TRUE;
        MACINFO mi;
        OSErr err;

        G.fMacZipped = FALSE;
        c2pstr(G.filename);
        if (G.extra_field &&
            (G.lrec.extra_field_length > sizeof(MACINFOMIN)) &&
            (G.lrec.extra_field_length <= sizeof(MACINFO))) {
            BlockMove(G.extra_field, &mi, G.lrec.extra_field_length);
            if ((makeword((uch *)&mi.header) == 1992) &&
                (makeword((uch *)&mi.data) ==
                    G.lrec.extra_field_length-sizeof(ZIP_EXTRA_HEADER)) &&
                (mi.signature == 'JLEE')) {
                G.gostCreator = mi.finfo.fdCreator;
                G.gostType = mi.finfo.fdType;
                fDataFork = (mi.flags & 1) ? TRUE : FALSE;
                G.fMacZipped = true;
                /* If it was Zipped w/Mac version, the filename has either */
                /* a 'd' or 'r' appended.  Remove the d/r when unzipping */
                G.filename[0]-=1;
            }
        }
        if (!G.fMacZipped) {
            if (!G.aflag)
                G.gostType = G.gostCreator = '\?\?\?\?';
            else {
                G.gostCreator = CREATOR;
                G.gostType = 'TEXT';
            }
        }
        p2cstr(G.filename);

        if ((outfd = creat(G.filename, 0)) != -1) {
            if (G.fMacZipped) {
                c2pstr(G.filename);
                if (G.HFSFlag) {
                    HParamBlockRec   hpbr;

                    hpbr.fileParam.ioNamePtr = (StringPtr)G.filename;
                    hpbr.fileParam.ioVRefNum = G.gnVRefNum;
                    hpbr.fileParam.ioDirID = G.glDirID;
                    hpbr.fileParam.ioFlFndrInfo = mi.finfo;
                    hpbr.fileParam.ioFlCrDat = mi.lCrDat;
                    hpbr.fileParam.ioFlMdDat = mi.lMdDat;
                    err = PBHSetFInfo(&hpbr, 0);
                } else {
                    err = SetFInfo((StringPtr)G.filename , 0, &mi.finfo);
                }
                p2cstr(G.filename);
            }
            outfd = open(G.filename, (fDataFork) ? 1 : 2);
        }

        if (outfd == -1)
            return NULL;
        else
            return (FILE *)outfd;
    }





/***********************/
/* Function maccreat() */
/***********************/

short maccreat(char *sz, short nVRefNum, long lDirID, OSType ostCreator, OSType ostType)
{
    OSErr   err;
    Str255  st;
    FInfo   fi;

    c2pstr( sz );
    BlockMove( sz, st, sz[0]+1 );
    p2cstr( sz );
    if (G.HFSFlag)
    {
        err = HGetFInfo( nVRefNum, lDirID, st, &fi );
        if (err == fnfErr)
            err = HCreate( nVRefNum, lDirID, st, ostCreator, ostType );
        else if (err == noErr)
        {
            err = HRstFLock( nVRefNum, lDirID, st );
            if ( err == noErr ) {
                fi.fdCreator = ostCreator;
                fi.fdType = ostType;
                err = HSetFInfo( nVRefNum, lDirID, st, &fi );
            }
        }
    }
    else
    {
        err = GetFInfo( st, nVRefNum, &fi );
        if (err == fnfErr)
            err = Create( st, nVRefNum, ostCreator, ostType );
        else if (err == noErr)
        {
            err = RstFLock( st, nVRefNum );
            if ( err == noErr ) {
                fi.fdCreator = ostCreator;
                fi.fdType = ostType;
                err = SetFInfo( st, nVRefNum, &fi );
            }
        }
    }
    if (err == noErr)
        return noErr;
    else
        return -1;
}





/**********************/
/* Function macread() */
/**********************/

short macread(short nFRefNum, char *pb, unsigned cb)
{
    long    lcb = cb;

    (void)FSRead( nFRefNum, &lcb, pb );

    return (short)lcb;
}





/***********************/
/* Function macwrite() */
/***********************/

long macwrite(short nFRefNum, char *pb, unsigned cb)
{
    long    lcb = cb;

#ifdef THINK_C
    if ( (nFRefNum == 1) || (nFRefNum == 2) )
        screenDump( pb, lcb );
    else
#endif
        (void)FSWrite( nFRefNum, &lcb, pb );

    return (long)lcb;
}





/***********************/
/* Function macclose() */
/***********************/

short macclose(short nFRefNum)
{
    return FSClose( nFRefNum );
}





/***********************/
/* Function maclseek() */
/***********************/

long maclseek(short nFRefNum, long lib, short nMode)
{
    ParamBlockRec   pbr;

    if (nMode == SEEK_SET)
        nMode = fsFromStart;
    else if (nMode == SEEK_CUR)
        nMode = fsFromMark;
    else if (nMode == SEEK_END)
        nMode = fsFromLEOF;
    pbr.ioParam.ioRefNum = nFRefNum;
    pbr.ioParam.ioPosMode = nMode;
    pbr.ioParam.ioPosOffset = lib;
    (void)PBSetFPos(&pbr, 0);
    return pbr.ioParam.ioPosOffset;
}




/***********************/
/* Function macgetch() */
/***********************/

int macgetch(void)
{
    WindowPtr whichWindow;
    EventRecord theEvent;
    char c;                     /* one-byte buffer for read() to use */

    do {
        SystemTask();
        if (!GetNextEvent(everyEvent, &theEvent))
            theEvent.what = nullEvent;
        else {
            switch (theEvent.what) {
            case keyDown:
                c = theEvent.message & charCodeMask;
                break;
            case mouseDown:
                if (FindWindow(theEvent.where, &whichWindow) ==
                    inSysWindow)
                    SystemClick(&theEvent, whichWindow);
                break;
            case updateEvt:
                screenUpdate((WindowPtr)theEvent.message);
                break;
            }
        }
    } while (theEvent.what != keyDown);

    return (int)c;
}

#endif /* MACOS */
