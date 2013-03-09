/*---------------------------------------------------------------------------

  zipinfo.c                                                    Greg Roelofs

  This file contains all of the ZipInfo-specific listing routines for UnZip.

  Contains:  zi_opts()
             zi_end_central()
             zipinfo()
             zi_long()
             zi_short()
             zi_time()

  ---------------------------------------------------------------------------*/


#define UNZIP_INTERNAL
#include "unzip.h"


#ifndef NO_ZIPINFO  /* strings use up too much space in small-memory systems */

/* Define OS-specific attributes for use on ALL platforms--the S_xxxx
 * versions of these are defined differently (or not defined) by different
 * compilers and operating systems. */

#define UNX_IFMT       0170000     /* Unix file type mask */
#define UNX_IFDIR      0040000     /* Unix directory */
#define UNX_IFREG      0100000     /* Unix regular file */
#define UNX_IFSOCK     0140000     /* Unix socket (BSD, not SysV or Amiga) */
#define UNX_IFLNK      0120000     /* Unix symbolic link (not SysV, Amiga) */
#define UNX_IFBLK      0060000     /* Unix block special       (not Amiga) */
#define UNX_IFCHR      0020000     /* Unix character special   (not Amiga) */
#define UNX_IFIFO      0010000     /* Unix fifo    (BCC, not MSC or Amiga) */
#define UNX_ISUID      04000       /* Unix set user id on execution */
#define UNX_ISGID      02000       /* Unix set group id on execution */
#define UNX_ISVTX      01000       /* Unix directory permissions control */
#define UNX_ENFMT      UNX_ISGID   /* Unix record locking enforcement flag */
#define UNX_IRWXU      00700       /* Unix read, write, execute: owner */
#define UNX_IRUSR      00400       /* Unix read permission: owner */
#define UNX_IWUSR      00200       /* Unix write permission: owner */
#define UNX_IXUSR      00100       /* Unix execute permission: owner */
#define UNX_IRWXG      00070       /* Unix read, write, execute: group */
#define UNX_IRGRP      00040       /* Unix read permission: group */
#define UNX_IWGRP      00020       /* Unix write permission: group */
#define UNX_IXGRP      00010       /* Unix execute permission: group */
#define UNX_IRWXO      00007       /* Unix read, write, execute: other */
#define UNX_IROTH      00004       /* Unix read permission: other */
#define UNX_IWOTH      00002       /* Unix write permission: other */
#define UNX_IXOTH      00001       /* Unix execute permission: other */

#define VMS_IRUSR      UNX_IRUSR   /* VMS read/owner */
#define VMS_IWUSR      UNX_IWUSR   /* VMS write/owner */
#define VMS_IXUSR      UNX_IXUSR   /* VMS execute/owner */
#define VMS_IRGRP      UNX_IRGRP   /* VMS read/group */
#define VMS_IWGRP      UNX_IWGRP   /* VMS write/group */
#define VMS_IXGRP      UNX_IXGRP   /* VMS execute/group */
#define VMS_IROTH      UNX_IROTH   /* VMS read/other */
#define VMS_IWOTH      UNX_IWOTH   /* VMS write/other */
#define VMS_IXOTH      UNX_IXOTH   /* VMS execute/other */

#define AMI_IFMT       06000       /* Amiga file type mask */
#define AMI_IFDIR      04000       /* Amiga directory */
#define AMI_IFREG      02000       /* Amiga regular file */
#define AMI_IHIDDEN    00200       /* to be supported in AmigaDOS 3.x */
#define AMI_ISCRIPT    00100       /* executable script (text command file) */
#define AMI_IPURE      00040       /* allow loading into resident memory */
#define AMI_IARCHIVE   00020       /* not modified since bit was last set */
#define AMI_IREAD      00010       /* can be opened for reading */
#define AMI_IWRITE     00004       /* can be opened for writing */
#define AMI_IEXECUTE   00002       /* executable image, a loadable runfile */
#define AMI_IDELETE    00001       /* can be deleted */

#define LFLAG  3   /* short "ls -l" type listing */

static int   zi_long   OF((__GPRO__ ulg endprev));
static int   zi_short  OF((__GPRO));
static char *zi_time   OF((__GPRO__ const ush *datez, const ush *timez,
                           const time_t *modtimez, char *d_t_str));


/**********************************************/
/*  Strings used in zipinfo.c (ZipInfo half)  */
/**********************************************/

static char Far LongHeader[] = "";
static char Far ShortHeader[] = "";
static char Far EndCentDirRec[] = "";
static char Far LineSeparators[] = "";
static char Far ActOffsetCentDir[] = "";
static char Far SinglePartArchive1[] = "";
static char Far SinglePartArchive2[] = "";
static char Far MultiPartArchive1[] = "";
static char Far MultiPartArchive2[] = "";
static char Far MultiPartArchive3[] = "";
static char Far NoZipfileComment[] = "";
static char Far ZipfileCommentDesc[] =
  "";
static char Far ZipfileCommBegin[] =
 "";
static char Far ZipfileCommEnd[] =
 "";
static char Far ZipfileCommTrunc2[] = "";
static char Far ZipfileCommTruncMsg[] =
  "";

static char Far CentralDirEntry[] =
  "";
static char Far ZipfileStats[] =
  "";

/* zi_long() strings */
static char Far OS_FAT[] = "";
static char Far OS_Amiga[] = "";
static char Far OS_VMS[] = "";
static char Far OS_Unix[] = "";
static char Far OS_VMCMS[] = "";
static char Far OS_AtariST[] = "";
static char Far OS_HPFS[] = "";
static char Far OS_Macintosh[] = "";
static char Far OS_ZSystem[] = "";
static char Far OS_CPM[] = "";
static char Far OS_TOPS20[] = "";
static char Far OS_NTFS[] = "";
static char Far OS_QDOS[] = "";
static char Far OS_Acorn[] = "";
static char Far OS_MVS[] = "";
static char Far OS_VFAT[] = "";
static char Far OS_BeBox[] = "";

static char Far MthdNone[] = "";
static char Far MthdShrunk[] = "";
static char Far MthdRedF1[] = "";
static char Far MthdRedF2[] = "";
static char Far MthdRedF3[] = "";
static char Far MthdRedF4[] = "";
static char Far MthdImplode[] = "";
static char Far MthdToken[] = "";
static char Far MthdDeflate[] = "";

static char Far DeflNorm[] = "";
static char Far DeflMax[] = "";
static char Far DeflFast[] = "";
static char Far DeflSFast[] = "";

static char Far ExtraBytesPreceding[] =
  "";

static char Far Unknown[] = "";

static char Far HostOS[] =
  "";
static char Far EncodeSWVer[] =
  "";
static char Far MinOSCompReq[] =
  "";
static char Far MinSWVerReq[] =
  "";
static char Far CompressMethod[] =
  "";
static char Far SlideWindowSizeImplode[] =
  "";
static char Far ShannonFanoTrees[] =
  "";
static char Far CompressSubtype[] =
  "";
static char Far FileSecurity[] =
  "";
static char Far ExtendedLocalHdr[] =
  "";
static char Far FileModDate[] =
  "";
#ifdef USE_EF_UX_TIME
  static char Far UX_FileModDate[] =
    "";
  static char Far LocalTime[] = "";
#ifndef NO_GMTIME
  static char Far GMTime[] = "";
#endif
#endif /* USE_EF_UX_TIME */
static char Far CRC32Value[] =
  "";
static char Far CompressedFileSize[] =
  "";
static char Far UncompressedFileSize[] =
  "";
static char Far FilenameLength[] =
  "";
static char Far ExtraFieldLength[] =
  "";
static char Far FileCommentLength[] =
  "";
static char Far FileDiskNum[] =
  "";
static char Far ApparentFileType[] =
  "";
static char Far VMSFileAttributes[] =
  "";
static char Far AmigaFileAttributes[] =
  "";
static char Far UnixFileAttributes[] =
  "";
static char Far NonMSDOSFileAttributes[] =
  "";
static char Far MSDOSFileAttributes[] =
  "";
static char Far MSDOSFileAttributesRO[] =
  "";
static char Far MSDOSFileAttributesAlpha[] =
  "";
static char Far LocalHeaderOffset[] =
  "";

static char Far ExtraFieldTrunc[] = "";
static char Far ExtraFields[] = "";
static char Far ExtraFieldType[] = "";
static char Far OS2EAs[] = "";
static char Far ACLData[] = "";
#ifdef CMS_MVS
   static char Far VmMvsExtraField[] = ".\n\
  The stored file open mode (FLDATA TYPE) is \"%s\".";
   static char Far VmMvsInvalid[] = "[invalid]";
#endif /* CMS_MVS */
static char Far First16[] = "";
static char Far ColonIndent[] = "";

static char Far efFormat[] = "";
static char Far efAV[] = "";
static char Far efOS2[] = "";
static char Far efACL[] = "";
static char Far efPKVMS[] = "";
static char Far efASiUnix[] = "";
static char Far efIZVMS[] = "";
static char Far efIZUnix[] = "";
static char Far efVMCMS[] = "";
static char Far efMVS[] = "";
static char Far efSpark[] = "";
static char Far efMD5[] = "";
static char Far efUnknown[] = "";

static char Far efIZuid[] = "";
static char Far efIZnouid[] = "";

static char Far lExtraFieldType[] = "";

static char Far NoFileComment[] = "";
static char Far FileCommBegin[] = "";
static char Far FileCommEnd[] = "";

/* zi_time() strings */
static char Far BogusFmt[] = "";
static char Far DMYHMTime[] = "";
static char Far YMDHMSTime[] = "";
static char Far DecimalTime[] = "";





/************************/
/*  Function zi_opts()  */
/************************/

int zi_opts(__G__ pargc, pargv)
    int *pargc;
    char ***pargv;
     __GDEF
{
    char   **argv, *s;
    int    argc, c, error=FALSE, negative=0;
    int    hflag_slmv=TRUE, hflag_2=FALSE;  /* diff options => diff defaults */
    int    tflag_slm=TRUE, tflag_2v=FALSE;
    int    explicit_h=FALSE, explicit_t=FALSE;


#ifdef MACOS
    G.lflag = LFLAG;   /* reset default on each call */
#endif
    argc = *pargc;
    argv = *pargv;

    while (--argc > 0 && (*++argv)[0] == '-') {
        s = argv[0] + 1;
        while ((c = *s++) != 0) {    /* "!= 0":  prevent Turbo C warning */
            switch (c) {
                case '-':
                    ++negative;
                    break;
                case '1':      /* shortest listing:  JUST filenames */
                    if (negative)
                        G.lflag = -2, negative = 0;
                    else
                        G.lflag = 1;
                    break;
                case '2':      /* just filenames, plus headers if specified */
                    if (negative)
                        G.lflag = -2, negative = 0;
                    else
                        G.lflag = 2;
                    break;
                case 'h':      /* header line */
                    if (negative)
                        hflag_2 = hflag_slmv = FALSE, negative = 0;
                    else {
                        hflag_2 = hflag_slmv = explicit_h = TRUE;
                        if (G.lflag == -1)
                            G.lflag = 0;
                    }
                    break;
                case 'l':      /* longer form of "ls -l" type listing */
                    if (negative)
                        G.lflag = -2, negative = 0;
                    else
                        G.lflag = 5;
                    break;
                case 'm':      /* medium form of "ls -l" type listing */
                    if (negative)
                        G.lflag = -2, negative = 0;
                    else
                        G.lflag = 4;
                    break;
#ifdef MORE
                case 'M':      /* send output through built-in "more" */
                    if (negative)
                        G.M_flag = FALSE, negative = 0;
                    else
                        G.M_flag = TRUE;
                    break;
#endif
                case 's':      /* default:  shorter "ls -l" type listing */
                    if (negative)
                        G.lflag = -2, negative = 0;
                    else
                        G.lflag = 3;
                    break;
                case 't':      /* totals line */
                    if (negative)
                        tflag_2v = tflag_slm = FALSE, negative = 0;
                    else {
                        tflag_2v = tflag_slm = explicit_t = TRUE;
                        if (G.lflag == -1)
                            G.lflag = 0;
                    }
                    break;
                case ('T'):    /* use (sortable) decimal time format */
                    if (negative)
                        G.T_flag = FALSE, negative = 0;
                    else
                        G.T_flag = TRUE;
                    break;
                case 'v':      /* turbo-verbose listing */
                    if (negative)
                        G.lflag = -2, negative = 0;
                    else
                        G.lflag = 10;
                    break;
                case 'z':      /* print zipfile comment */
                    if (negative)
                        G.zflag = negative = 0;
                    else
                        G.zflag = 1;
                    break;
                case 'Z':      /* ZipInfo mode:  ignore */
                    break;
                default:
                    error = TRUE;
                    break;
            }
        }
    }
    if ((argc-- == 0) || error) {
        *pargc = argc;
        *pargv = argv;
#if (!defined(MSWIN) && !defined(CMS_MVS))
        return usage(__G__ error);
#else
        return error;
#endif
    }

    /* if no listing options given (or all negated), or if only -h/-t given
     * with individual files specified, use default listing format */
    if ((G.lflag < 0) || ((argc > 0) && (G.lflag == 0)))
        G.lflag = LFLAG;

    /* set header and totals flags to default or specified values */
    switch (G.lflag) {
        case 0:   /* 0:  can only occur if either -t or -h explicitly given; */
        case 2:   /*  therefore set both flags equal to normally false value */
            G.hflag = hflag_2;
            G.tflag = tflag_2v;
            break;
        case 1:   /* only filenames, *always* */
            G.hflag = FALSE;
            G.tflag = FALSE;
            G.zflag = FALSE;
            break;
        case 3:
        case 4:
        case 5:
            G.hflag = ((argc > 0) && !explicit_h)? FALSE : hflag_slmv;
            G.tflag = ((argc > 0) && !explicit_t)? FALSE : tflag_slm;
            break;
        case 10:
            G.hflag = hflag_slmv;
            G.tflag = tflag_2v;
            break;
    }

    *pargc = argc;
    *pargv = argv;
    return 0;

} /* end function zi_opts() */





/*******************************/
/*  Function zi_end_central()  */
/*******************************/

int zi_end_central(__G)   /* return PK-type error code */
     __GDEF
{
    int  error = PK_COOL;


/*---------------------------------------------------------------------------
    Print out various interesting things about the zipfile.
  ---------------------------------------------------------------------------*/

    /* header fits on one line, for anything up to 10GB and 10000 files: */
    if (G.hflag)
        Info(slide, 0, ((char *)slide, ((int)strlen(G.zipfn) < 39)?
          LoadFarString(LongHeader) : LoadFarString(ShortHeader), G.zipfn,
          (long)G.ziplen, G.ecrec.total_entries_central_dir,
          (G.ecrec.total_entries_central_dir==1)? "" : "s"));

    /* verbose format */
    if (G.lflag > 9) {
        Info(slide, 0, ((char *)slide, LoadFarString(EndCentDirRec)));
        Info(slide, 0, ((char *)slide, LoadFarString(LineSeparators)));

        Info(slide, 0, ((char *)slide, LoadFarString(ActOffsetCentDir),
          (long)G.real_ecrec_offset, (long)G.real_ecrec_offset,
          (long)G.expect_ecrec_offset, (long)G.expect_ecrec_offset));

        if (G.ecrec.number_this_disk == 0) {
            Info(slide, 0, ((char *)slide, LoadFarString(SinglePartArchive1),
              G.ecrec.total_entries_central_dir,
              (G.ecrec.total_entries_central_dir == 1)? "entry" : "entries",
              G.ecrec.size_central_directory,
              G.ecrec.size_central_directory));
            Info(slide, 0, ((char *)slide, LoadFarString(SinglePartArchive2),
              G.ecrec.offset_start_central_directory,
              G.ecrec.offset_start_central_directory));
        } else {
            Info(slide, 0, ((char *)slide, LoadFarString(MultiPartArchive1),
              G.ecrec.number_this_disk,
              G.ecrec.num_disk_start_cdir,
              G.ecrec.num_entries_centrl_dir_ths_disk,
              (G.ecrec.num_entries_centrl_dir_ths_disk == 1)? "is" : "are"));
            Info(slide, 0, ((char *)slide, LoadFarString(MultiPartArchive2),
              G.ecrec.total_entries_central_dir,
              (G.ecrec.total_entries_central_dir == 1) ? "entry" : "entries",
              G.ecrec.size_central_directory,
              G.ecrec.size_central_directory));
            Info(slide, 0, ((char *)slide, LoadFarString(MultiPartArchive3),
              G.ecrec.offset_start_central_directory,
              G.ecrec.offset_start_central_directory));
        }

    /*-----------------------------------------------------------------------
        Get the zipfile comment, if any, and print it out.  (Comment may be
        up to 64KB long.  May the fleas of a thousand camels infest the arm-
        pits of anyone who actually takes advantage of this fact.)
      -----------------------------------------------------------------------*/

        if (!G.ecrec.zipfile_comment_length)
            Info(slide, 0, ((char *)slide, LoadFarString(NoZipfileComment)));
        else {
            Info(slide, 0, ((char *)slide, LoadFarString(ZipfileCommentDesc),
              G.ecrec.zipfile_comment_length));
            Info(slide, 0, ((char *)slide, LoadFarString(ZipfileCommBegin)));
            if (do_string(__G__ G.ecrec.zipfile_comment_length, DISPLAY))
                error = PK_WARN;
            Info(slide, 0, ((char *)slide, LoadFarString(ZipfileCommEnd)));
            if (error)
                Info(slide, 0, ((char *)slide,
                  LoadFarString(ZipfileCommTrunc2)));
        } /* endif (comment exists) */

    /* non-verbose mode:  print zipfile comment only if requested */
    } else if (G.zflag && G.ecrec.zipfile_comment_length) {
        if (do_string(__G__ G.ecrec.zipfile_comment_length,DISPLAY)) {
            Info(slide, 0x401, ((char *)slide,
              LoadFarString(ZipfileCommTruncMsg)));
            error = PK_WARN;
        }
    } /* endif (verbose) */

    return error;

} /* end function zi_end_central() */





/************************/
/*  Function zipinfo()  */
/************************/

int zipinfo(__G)   /* return PK-type error code */
     __GDEF
{
    int   j, do_this_file=FALSE, error, error_in_archive=PK_COOL;
    int   *fn_matched=NULL, *xn_matched=NULL;
    ush   members=0;
    ulg   tot_csize=0L, tot_ucsize=0L;
    ulg   endprev;   /* buffers end of previous entry for zi_long()'s check
                      *  of extra bytes */


/*---------------------------------------------------------------------------
    Malloc space for check on unmatched filespecs (no big deal if one or both
    are NULL).
  ---------------------------------------------------------------------------*/

    if (G.filespecs > 0  &&
        (fn_matched=(int *)malloc(G.filespecs*sizeof(int))) != NULL)
        for (j = 0;  j < G.filespecs;  ++j)
            fn_matched[j] = FALSE;

    if (G.xfilespecs > 0  &&
        (xn_matched=(int *)malloc(G.xfilespecs*sizeof(int))) != NULL)
        for (j = 0;  j < G.xfilespecs;  ++j)
            xn_matched[j] = FALSE;

/*---------------------------------------------------------------------------
    Set file pointer to start of central directory, then loop through cen-
    tral directory entries.  Check that directory-entry signature bytes are
    actually there (just a precaution), then process the entry.  We know
    the entire central directory is on this disk:  we wouldn't have any of
    this information unless the end-of-central-directory record was on this
    disk, and we wouldn't have gotten to this routine unless this is also
    the disk on which the central directory starts.  In practice, this had
    better be the *only* disk in the archive, but maybe someday we'll add
    multi-disk support.
  ---------------------------------------------------------------------------*/

    G.pInfo->lcflag = 0;    /* used in do_string(): never TRUE in zipinfo mode */
    G.pInfo->textmode = 0;  /* so one can read on screen (but is it ever used?) */

    /* reset endprev for new zipfile; account for multi-part archives (?) */
    endprev = (G.crec.relative_offset_local_header == 4L)? 4L : 0L;


    for (j = 0;  j < (int)G.ecrec.total_entries_central_dir;  ++j) {
        if (readbuf(__G__ G.sig, 4) == 0)
            return PK_EOF;
        if (strncmp(G.sig, G.central_hdr_sig, 4)) {  /* just to make sure */
            Info(slide, 0x401, ((char *)slide, LoadFarString(CentSigMsg), j));
            return PK_BADERR;   /* sig not found */
        }
        if ((error = get_cdir_ent(__G)) != PK_COOL)
            return error;       /* only PK_EOF defined */

        /* do Amigas (AMIGA_) also have volume labels? */
        if (IS_VOLID(G.crec.external_file_attributes) &&
            (G.pInfo->hostnum == FS_FAT_ || G.pInfo->hostnum == FS_HPFS_ ||
             G.pInfo->hostnum == FS_NTFS_ || G.pInfo->hostnum == ATARI_))
            G.pInfo->vollabel = TRUE;
        else
            G.pInfo->vollabel = FALSE;

        if ((error = do_string(__G__ G.crec.filename_length, DS_FN)) !=
             PK_COOL)
        {
          error_in_archive = error;   /* might be warning */
          if (error > PK_WARN)        /* fatal */
              return error;
        }

        if (!G.process_all_files) {    /* check if specified on command line */
            char  **pfn = G.pfnames-1;

            do_this_file = FALSE;
            while (*++pfn)
                if (match(G.filename, *pfn, 0)) {
                    do_this_file = TRUE;
                    if (fn_matched)
                        fn_matched[(int)(pfn-G.pfnames)] = TRUE;
                    break;       /* found match, so stop looping */
                }
            if (do_this_file) {  /* check if this is an excluded file */
                char  **pxn = G.pxnames-1;

                while (*++pxn)
                    if (match(G.filename, *pxn, 0)) {
                        do_this_file = FALSE;
                        if (xn_matched)
                            xn_matched[(int)(pxn-G.pxnames)] = TRUE;
                        break;
                    }
            }
        }

    /*-----------------------------------------------------------------------
        If current file was specified on command line, or if no names were
        specified, do the listing for this file.  Otherwise, get rid of the
        file comment and go back for the next file.
      -----------------------------------------------------------------------*/

        if (G.process_all_files || do_this_file) {

            switch (G.lflag) {
                case 1:
                case 2:
                    fnprint(__G);
                    SKIP_(G.crec.extra_field_length)
                    SKIP_(G.crec.file_comment_length)
                    break;

                case 3:
                case 4:
                case 5:
                    if ((error = zi_short(__G)) != PK_COOL) {
                        error_in_archive = error;   /* might be warning */
                        if (error > PK_WARN)        /* fatal */
                            return error;
                    }
                    break;

                case 10:
                    Info(slide, 0, ((char *)slide,
                      LoadFarString(CentralDirEntry), j));
                    if ((error = zi_long(__G__ endprev)) != PK_COOL) {
                        error_in_archive = error;   /* might be warning */
                        if (error > PK_WARN)        /* fatal */
                            return error;
                    }
                    break;

                default:
                    SKIP_(G.crec.extra_field_length)
                    SKIP_(G.crec.file_comment_length)
                    break;

            } /* end switch (lflag) */

            tot_csize += G.crec.csize;
            tot_ucsize += G.crec.ucsize;
            if (G.crec.general_purpose_bit_flag & 1)
                tot_csize -= 12;   /* don't count encryption header */
            ++members;

        } else {   /* not listing */
            SKIP_(G.crec.extra_field_length)
            SKIP_(G.crec.file_comment_length)

        } /* end if (list member?) */

    } /* end for-loop (j: member files) */

/*---------------------------------------------------------------------------
    Double check that we're back at the end-of-central-directory record.
  ---------------------------------------------------------------------------*/

    if (readbuf(__G__ G.sig, 4) == 0)  /* disk error? */
        return PK_EOF;
    if (strncmp(G.sig, G.end_central_sig, 4)) {   /* just to make sure again */
        Info(slide, 0x401, ((char *)slide, LoadFarString(EndSigMsg)));
        error_in_archive = PK_WARN;   /* didn't find sig */
    }

/*---------------------------------------------------------------------------
    Check that we actually found requested files; if so, print totals.
  ---------------------------------------------------------------------------*/

    if (G.tflag) {
        char *sgn = "";
        int cfactor = ratio(tot_ucsize, tot_csize);

        if (cfactor < 0) {
            sgn = "-";
            cfactor = -cfactor;
        }
        Info(slide, 0, ((char *)slide, LoadFarString(ZipfileStats),
          members, (members==1)? "":"s", tot_ucsize, tot_csize, sgn, cfactor/10,
          cfactor%10));
    }

/*---------------------------------------------------------------------------
    Check for unmatched filespecs on command line and print warning if any
    found.
  ---------------------------------------------------------------------------*/

    if (fn_matched) {
        for (j = 0;  j < G.filespecs;  ++j)
            if (!fn_matched[j])
                Info(slide, 0x401, ((char *)slide,
                  LoadFarString(FilenameNotMatched), G.pfnames[j]));
        free((zvoid *)fn_matched);
    }
    if (xn_matched) {
        for (j = 0;  j < G.xfilespecs;  ++j)
            if (!xn_matched[j])
                Info(slide, 0x401, ((char *)slide,
                  LoadFarString(ExclFilenameNotMatched), G.pxnames[j]));
        free((zvoid *)xn_matched);
    }
    if (members == 0 && error_in_archive <= PK_WARN)
        error_in_archive = PK_FIND;

    return error_in_archive;

} /* end function zipinfo() */





/************************/
/*  Function zi_long()  */
/************************/

static int zi_long(__G__ endprev)   /* return PK-type error code */
     __GDEF
    ulg endprev;                    /* for zi_long() check of extra bytes */
{
#ifdef USE_EF_UX_TIME
    ztimbuf z_utime;
#endif
    int  error, error_in_archive=PK_COOL;
    ush  hostnum, hostver, extnum, extver, methnum, xattr;
    char workspace[12], attribs[22];
    char *varmsg_str;
    char unkn[16];
    static char Far *os[NUM_HOSTS] = {
        OS_FAT, OS_Amiga, OS_VMS, OS_Unix, OS_VMCMS, OS_AtariST, OS_HPFS,
        OS_Macintosh, OS_ZSystem, OS_CPM, OS_TOPS20, OS_NTFS, OS_QDOS,
        OS_Acorn, OS_VFAT, OS_MVS, OS_BeBox
    };
    static char Far *method[NUM_METHODS] = {
        MthdNone, MthdShrunk, MthdRedF1, MthdRedF2, MthdRedF3, MthdRedF4,
        MthdImplode, MthdToken, MthdDeflate
    };
    static char Far *dtype[4] = {
        DeflNorm, DeflMax, DeflFast, DeflSFast
    };


/*---------------------------------------------------------------------------
    Check whether there's any extra space inside the zipfile.
  ---------------------------------------------------------------------------*/

    if (G.crec.relative_offset_local_header != endprev)
        Info(slide, 0, ((char *)slide, LoadFarString(ExtraBytesPreceding),
          (long)G.crec.relative_offset_local_header - (long)endprev));

    /* calculate endprev for next time around */
    endprev = G.crec.relative_offset_local_header + 4L + LREC_SIZE +
      G.crec.filename_length + G.crec.extra_field_length + G.crec.file_comment_length
      + G.crec.csize;

/*---------------------------------------------------------------------------
    Read the extra field, if any. It may be used to get UNIX style modtime.
  ---------------------------------------------------------------------------*/

    if ((error = do_string(__G__ G.crec.extra_field_length, EXTRA_FIELD))
        != 0) {
        if (G.extra_field != NULL) {
            free(G.extra_field);
            G.extra_field = NULL;
        }
        error_in_archive = error;
        /* The premature return in case of a "fatal" error (PK_EOF) is
         * delayed until we analyse the extra field contents.
         * This allows to display all the other info that has been
         * successfully read in.
         */
    }

/*---------------------------------------------------------------------------
    Print out various interesting things about the compressed file.
  ---------------------------------------------------------------------------*/

    hostnum = (ush)MIN(G.crec.version_made_by[1], NUM_HOSTS);
    hostver = G.crec.version_made_by[0];
    extnum = (ush)MIN(G.crec.version_needed_to_extract[1], NUM_HOSTS);
    extver = G.crec.version_needed_to_extract[0];
    methnum = (ush)MIN(G.crec.compression_method, NUM_METHODS);

    (*G.message)((zvoid *)&G, (uch *)"  ", 2L, 0);  fnprint(__G);

    if (hostnum >= NUM_HOSTS) {
        sprintf(unkn, LoadFarString(Unknown),
                (int)G.crec.version_made_by[1]);
        varmsg_str = unkn;
    } else {
        varmsg_str = LoadFarStringSmall(os[hostnum]);
    }
    Info(slide, 0, ((char *)slide, LoadFarString(HostOS), varmsg_str));
    Info(slide, 0, ((char *)slide, LoadFarString(EncodeSWVer), hostver/10,
      hostver%10));

    if (extnum >= NUM_HOSTS) {
        sprintf(unkn, LoadFarString(Unknown),
                (int)G.crec.version_needed_to_extract[1]);
        varmsg_str = unkn;
    } else {
        varmsg_str = LoadFarStringSmall(os[extnum]);
    }
    Info(slide, 0, ((char *)slide, LoadFarString(MinOSCompReq), varmsg_str));
    Info(slide, 0, ((char *)slide, LoadFarString(MinSWVerReq), extver/10,
      extver%10));

    if (methnum >= NUM_METHODS) {
        sprintf(unkn, LoadFarString(Unknown), G.crec.compression_method);
        varmsg_str = unkn;
    } else {
        varmsg_str = LoadFarStringSmall(method[methnum]);
    }
    Info(slide, 0, ((char *)slide, LoadFarString(CompressMethod), varmsg_str));
    if (methnum == IMPLODED) {
        Info(slide, 0, ((char *)slide, LoadFarString(SlideWindowSizeImplode),
          (G.crec.general_purpose_bit_flag & 2)? '8' : '4'));
        Info(slide, 0, ((char *)slide, LoadFarString(ShannonFanoTrees),
          (G.crec.general_purpose_bit_flag & 4)? '3' : '2'));
    } else if (methnum == DEFLATED) {
        ush  dnum=(ush)((G.crec.general_purpose_bit_flag>>1) & 3);
        Info(slide, 0, ((char *)slide, LoadFarString(CompressSubtype),
          LoadFarStringSmall(dtype[dnum])));
    }
    Info(slide, 0, ((char *)slide, LoadFarString(FileSecurity),
      (G.crec.general_purpose_bit_flag & 1)? "" : "not "));
    Info(slide, 0, ((char *)slide, LoadFarString(ExtendedLocalHdr),
      (G.crec.general_purpose_bit_flag & 8)? "yes" : "no"));
    /* print upper 3 bits for amusement? */

    /* For printing of date & time, a "char d_t_buf[21]" is required.
     * To save stack space, we reuse the "char attribs[22]" buffer which
     * is not used yet.
     */
#   define d_t_buf attribs
    zi_time(__G__ &G.crec.last_mod_file_date, &G.crec.last_mod_file_time,
      NULL, d_t_buf);
    Info(slide, 0, ((char *)slide, LoadFarString(FileModDate), d_t_buf));
#ifdef USE_EF_UX_TIME
    if (G.extra_field  &&  ef_scan_for_izux(G.extra_field,
        G.crec.extra_field_length, &z_utime, NULL) > 0)
    {
        d_t_buf[0] = (char)0;           /* signal "show local time" */
        zi_time(__G__ &G.crec.last_mod_file_date, &G.crec.last_mod_file_time,
          &(z_utime.modtime), d_t_buf);
        Info(slide, 0, ((char *)slide, LoadFarString(UX_FileModDate),
          d_t_buf, LoadFarStringSmall(LocalTime)));
#ifndef NO_GMTIME
        d_t_buf[0] = (char)1;           /* signal "show UTC (GMT) time" */
        zi_time(__G__ &G.crec.last_mod_file_date, &G.crec.last_mod_file_time,
          &(z_utime.modtime), d_t_buf);
        Info(slide, 0, ((char *)slide, LoadFarString(UX_FileModDate),
          d_t_buf, LoadFarStringSmall(GMTime)));
#endif /* !NO_GMTIME */
    }
#endif /* USE_EF_UX_TIME */

    Info(slide, 0, ((char *)slide, LoadFarString(CRC32Value), G.crec.crc32));
    Info(slide, 0, ((char *)slide, LoadFarString(CompressedFileSize),
      G.crec.csize));
    Info(slide, 0, ((char *)slide, LoadFarString(UncompressedFileSize),
      G.crec.ucsize));
    Info(slide, 0, ((char *)slide, LoadFarString(FilenameLength),
      G.crec.filename_length));
    Info(slide, 0, ((char *)slide, LoadFarString(ExtraFieldLength),
      G.crec.extra_field_length));
    Info(slide, 0, ((char *)slide, LoadFarString(FileCommentLength),
      G.crec.file_comment_length));
    Info(slide, 0, ((char *)slide, LoadFarString(FileDiskNum),
      G.crec.disk_number_start));
    Info(slide, 0, ((char *)slide, LoadFarString(ApparentFileType),
      (G.crec.internal_file_attributes & 1)? "text"
         : (G.crec.internal_file_attributes & 2)? "ebcdic"
              : "binary"));             /* changed to accept EBCDIC */
#ifdef ATARI
    printf("  external file attributes (hex):                   %.8lx\n",
      G.crec.external_file_attributes);
#endif
    xattr = (ush)((G.crec.external_file_attributes >> 16) & 0xFFFF);
    if (hostnum == VMS_) {
        char   *p=attribs, *q=attribs+1;
        int    i, j, k;

        for (k = 0;  k < 12;  ++k)
            workspace[k] = 0;
        if (xattr & VMS_IRUSR)
            workspace[0] = 'R';
        if (xattr & VMS_IWUSR) {
            workspace[1] = 'W';
            workspace[3] = 'D';
        }
        if (xattr & VMS_IXUSR)
            workspace[2] = 'E';
        if (xattr & VMS_IRGRP)
            workspace[4] = 'R';
        if (xattr & VMS_IWGRP) {
            workspace[5] = 'W';
            workspace[7] = 'D';
        }
        if (xattr & VMS_IXGRP)
            workspace[6] = 'E';
        if (xattr & VMS_IROTH)
            workspace[8] = 'R';
        if (xattr & VMS_IWOTH) {
            workspace[9] = 'W';
            workspace[11] = 'D';
        }
        if (xattr & VMS_IXOTH)
            workspace[10] = 'E';

        *p++ = '(';
        for (k = j = 0;  j < 3;  ++j) {    /* loop over groups of permissions */
            for (i = 0;  i < 4;  ++i, ++k)  /* loop over perms within a group */
                if (workspace[k])
                    *p++ = workspace[k];
            *p++ = ',';                       /* group separator */
            if (j == 0)
                while ((*p++ = *q++) != ','); /* system, owner perms are same */
        }
        *p-- = 0;
        *p = ')';   /* overwrite last comma */
        Info(slide, 0, ((char *)slide, LoadFarString(VMSFileAttributes), xattr,
          attribs));

    } else if (hostnum == AMIGA_) {
        switch (xattr & AMI_IFMT) {
            case AMI_IFDIR:  attribs[0] = 'd';  break;
            case AMI_IFREG:  attribs[0] = '-';  break;
            default:         attribs[0] = '?';  break;
        }
        attribs[1] = (xattr & AMI_IHIDDEN)?   'h' : '-';
        attribs[2] = (xattr & AMI_ISCRIPT)?   's' : '-';
        attribs[3] = (xattr & AMI_IPURE)?     'p' : '-';
        attribs[4] = (xattr & AMI_IARCHIVE)?  'a' : '-';
        attribs[5] = (xattr & AMI_IREAD)?     'r' : '-';
        attribs[6] = (xattr & AMI_IWRITE)?    'w' : '-';
        attribs[7] = (xattr & AMI_IEXECUTE)?  'e' : '-';
        attribs[8] = (xattr & AMI_IDELETE)?   'd' : '-';
        attribs[9] = 0;   /* better dlm the string */
        Info(slide, 0, ((char *)slide, LoadFarString(AmigaFileAttributes),
          xattr, attribs));

    } else if ((hostnum != FS_FAT_) && (hostnum != FS_HPFS_) &&
               (hostnum != FS_NTFS_) && (hostnum != FS_VFAT_) &&
               (hostnum != ACORN_) &&   /*  (hostnum != BEBOX_) &&   Be Box? */
               (hostnum != VM_CMS_) && (hostnum != MVS_))
    {                                 /* assume Unix-like */
        switch (xattr & UNX_IFMT) {
            case UNX_IFDIR:   attribs[0] = 'd';  break;
            case UNX_IFREG:   attribs[0] = '-';  break;
            case UNX_IFLNK:   attribs[0] = 'l';  break;
            case UNX_IFBLK:   attribs[0] = 'b';  break;
            case UNX_IFCHR:   attribs[0] = 'c';  break;
            case UNX_IFIFO:   attribs[0] = 'p';  break;
            case UNX_IFSOCK:  attribs[0] = 's';  break;
            default:          attribs[0] = '?';  break;
        }
        attribs[1] = (xattr & UNX_IRUSR)? 'r' : '-';
        attribs[4] = (xattr & UNX_IRGRP)? 'r' : '-';
        attribs[7] = (xattr & UNX_IROTH)? 'r' : '-';

        attribs[2] = (xattr & UNX_IWUSR)? 'w' : '-';
        attribs[5] = (xattr & UNX_IWGRP)? 'w' : '-';
        attribs[8] = (xattr & UNX_IWOTH)? 'w' : '-';

        if (xattr & UNX_IXUSR)
            attribs[3] = (xattr & UNX_ISUID)? 's' : 'x';
        else
            attribs[3] = (xattr & UNX_ISUID)? 'S' : '-';   /* S = undefined */
        if (xattr & UNX_IXGRP)
            attribs[6] = (xattr & UNX_ISGID)? 's' : 'x';   /* == UNX_ENFMT */
        else
            attribs[6] = (xattr & UNX_ISGID)? 'l' : '-';
        if (xattr & UNX_IXOTH)
            attribs[9] = (xattr & UNX_ISVTX)? 't' : 'x';   /* "sticky bit" */
        else
            attribs[9] = (xattr & UNX_ISVTX)? 'T' : '-';   /* T = undefined */
        attribs[10] = 0;

        Info(slide, 0, ((char *)slide, LoadFarString(UnixFileAttributes), xattr,
          attribs));

    } else {
        Info(slide, 0, ((char *)slide, LoadFarString(NonMSDOSFileAttributes),
            G.crec.external_file_attributes >> 8));

    } /* endif (hostnum: external attributes format) */

    if ((xattr=(ush)(G.crec.external_file_attributes & 0xFF)) == 0)
        Info(slide, 0, ((char *)slide, LoadFarString(MSDOSFileAttributes),
          xattr));
    else if (xattr == 1)
        Info(slide, 0, ((char *)slide, LoadFarString(MSDOSFileAttributesRO),
          xattr));
    else
        Info(slide, 0, ((char *)slide, LoadFarString(MSDOSFileAttributesAlpha),
          xattr, (xattr&1)?"rdo ":"", (xattr&2)?"hid ":"", (xattr&4)?"sys ":"",
          (xattr&8)?"lab ":"", (xattr&16)?"dir ":"", (xattr&32)?"arc":""));
    Info(slide, 0, ((char *)slide, LoadFarString(LocalHeaderOffset),
      G.crec.relative_offset_local_header, G.crec.relative_offset_local_header));

/*---------------------------------------------------------------------------
    Analyse the extra field, if any, and print the file comment, if any (the
    filename has already been printed, above).  That finishes up this file
    entry...
  ---------------------------------------------------------------------------*/

    if (G.crec.extra_field_length > 0) {
        uch *ef_ptr = G.extra_field;
        ush ef_len = G.crec.extra_field_length;
        ush ef_id, ef_datalen;

        if (error_in_archive > PK_WARN)   /* fatal:  can't continue */
            /* delayed "fatal error" return from extra field reading */
            return error;
        if (G.extra_field == (uch *)NULL)
            return PK_ERR;   /* not consistent with crec length */

        Info(slide, 0, ((char *)slide, LoadFarString(ExtraFields)));

        while (ef_len >= EB_HEADSIZE) {
            ef_id = makeword(&ef_ptr[EB_ID]);
            ef_datalen = makeword(&ef_ptr[EB_LEN]);

            if (ef_datalen > (ush)(ef_len - EB_HEADSIZE)) {
                Info(slide, 0, ((char *)slide, LoadFarString(ExtraFieldTrunc),
                  ef_datalen, (ef_len - EB_HEADSIZE)));
                ef_datalen = (ef_len - EB_HEADSIZE);
            }

            switch (ef_id) {
                case EF_AV:
                    Info(slide, 0, ((char *)slide, LoadFarString(ExtraFieldType),
                      ef_id, LoadFarStringSmall(efAV), ef_datalen));
                    break;
                case EF_OS2:
                    Info(slide, 0, ((char *)slide, LoadFarString(ExtraFieldType),
                      ef_id, LoadFarStringSmall(efOS2), ef_datalen));
                    if (ef_datalen >= 4)
                        Info(slide, 0, ((char *)slide, LoadFarString(OS2EAs),
                          makelong(&ef_ptr[EB_HEADSIZE])));
                    break;
                case EF_ACL:
                    Info(slide, 0, ((char *)slide, LoadFarString(ExtraFieldType),
                      ef_id, LoadFarStringSmall(efACL), ef_datalen));
                    if (ef_datalen >= 4)
                        Info(slide, 0, ((char *)slide, LoadFarString(ACLData),
                          makelong(&ef_ptr[EB_HEADSIZE])));
                    break;
                case EF_PKVMS:
                    Info(slide, 0, ((char *)slide, LoadFarString(ExtraFieldType),
                      ef_id, LoadFarStringSmall(efPKVMS), ef_datalen));
                    break;
                case EF_ASIUNIX:
                    Info(slide, 0, ((char *)slide, LoadFarString(ExtraFieldType),
                      ef_id, LoadFarStringSmall(efASiUnix), ef_datalen));
                    break;
                case EF_IZVMS:
                    Info(slide, 0, ((char *)slide, LoadFarString(ExtraFieldType),
                      ef_id, LoadFarStringSmall(efIZVMS), ef_datalen));
                    break;
                case EF_IZUNIX:
                    Info(slide, 0, ((char *)slide, LoadFarString(ExtraFieldType),
                      ef_id, LoadFarStringSmall(efIZUnix), ef_datalen));
                    break;
                case EF_VMCMS:
                    Info(slide, 0, ((char *)slide, LoadFarString(ExtraFieldType),
                      ef_id, LoadFarStringSmall(efVMCMS), ef_datalen));
                    break;
                case EF_MVS:
                    Info(slide, 0, ((char *)slide, LoadFarString(ExtraFieldType),
                      ef_id, LoadFarStringSmall(efMVS), ef_datalen));
                    break;
                case EF_SPARK:   /* from RISCOS */
                    Info(slide, 0, ((char *)slide, LoadFarString(ExtraFieldType),
                      ef_id, LoadFarStringSmall(efSpark), ef_datalen));
                    break;
                case EF_MD5:
                    Info(slide, 0, ((char *)slide, LoadFarString(ExtraFieldType),
                      ef_id, LoadFarStringSmall(efMD5), ef_datalen));
                    break;
                default:
                    Info(slide, 0, ((char *)slide, LoadFarString(ExtraFieldType),
                      ef_id, LoadFarStringSmall(efUnknown), ef_datalen));
                    break;
            }
#ifdef CMS_MVS
            if (ef_id == EF_VMCMS || ef_id == EF_MVS) {
               char type[100];

               Info(slide, 0, ((char *)slide, LoadFarString(VmMvsExtraField),
                    (getVMMVSexfield(type, ef_ptr, (unsigned)ef_datalen) > 0)
                    ? type : LoadFarStringSmall(VmMvsInvalid)));
            } else
#endif /* CMS_MVS */
            if (ef_id != EF_OS2 && ef_id != EF_ACL) {
                ush i, n;

                if (ef_datalen <= 16) {
                    Info(slide, 0, ((char *)slide, LoadFarString(ColonIndent)));
                    n = ef_datalen;
                } else {
                    Info(slide, 0, ((char *)slide, LoadFarString(First16),
                      LoadFarStringSmall(ColonIndent)));
                    n = 16;
                }
                for (i = 0;  i < n;  ++i)
                    Info(slide, 0, ((char *)slide, LoadFarString(efFormat),
                      ef_ptr[i+EB_HEADSIZE]));
            }

            ef_ptr += (ef_datalen + EB_HEADSIZE);
            ef_len -= (ef_datalen + EB_HEADSIZE);
        }
        (*G.message)((zvoid *)&G, (uch *)"\n", 1L, 0);
    }

    /* high bit == Unix/OS2/NT GMT times (mtime, atime); next bit == UID/GID */
    if ((xattr = (ush)((G.crec.external_file_attributes & 0xC000) >> 12)) & 8)
        if (hostnum == UNIX_ && xattr == 12)
            Info(slide, 0, ((char *)slide, LoadFarString(lExtraFieldType),
              "is", EF_IZUNIX, LoadFarStringSmall(efIZUnix), xattr, efIZuid));
        else if (hostnum == UNIX_ || hostnum == FS_HPFS_ || hostnum == FS_NTFS_)
            Info(slide, 0, ((char *)slide, LoadFarString(lExtraFieldType),
              "is", EF_IZUNIX, LoadFarStringSmall(efIZUnix), xattr,
              (xattr&4)? efIZuid : efIZnouid));
        else if (hostnum == FS_FAT_ && !(xattr&4))
            Info(slide, 0, ((char *)slide, LoadFarString(lExtraFieldType),
              "may be", EF_IZUNIX, LoadFarStringSmall(efIZUnix), xattr,
              (xattr&4)? efIZuid : efIZnouid));

    if (!G.crec.file_comment_length)
        Info(slide, 0, ((char *)slide, LoadFarString(NoFileComment)));
    else {
        Info(slide, 0, ((char *)slide, LoadFarString(FileCommBegin)));
        if ((error = do_string(__G__ G.crec.file_comment_length, DISPLAY)) != PK_COOL) {
            error_in_archive = error;   /* might be warning */
            if (error > PK_WARN)   /* fatal */
                return error;
        }
        Info(slide, 0, ((char *)slide, LoadFarString(FileCommEnd)));
    }

    return error_in_archive;

} /* end function zi_long() */





/*************************/
/*  Function zi_short()  */
/*************************/

static int zi_short(__G)   /* return PK-type error code */
     __GDEF
{
#ifdef USE_EF_UX_TIME
    ztimbuf     z_utime;
    time_t      *z_modtim;
#endif
    int         k, error, error_in_archive=PK_COOL;
    ush         methnum, hostnum, hostver, xattr;
    char        *p, workspace[12], attribs[16];
    char        methbuf[5];
    static char dtype[5]="NXFS";  /* normal, maximum, fast, superfast */
    static char Far os[NUM_HOSTS+1][4] = {
        "fat", "ami", "vms", "unx", "cms", "atr", "hpf", "mac", "zzz",
        "cpm", "t20", "ntf", "qds", "aco", "vft", "mvs", "be ", "???"
    };
    static char Far method[NUM_METHODS+1][5] = {
        "stor", "shrk", "re:1", "re:2", "re:3", "re:4", "i#:#", "tokn",
        "def#", "u###"
    };


/*---------------------------------------------------------------------------
    Print out various interesting things about the compressed file.
  ---------------------------------------------------------------------------*/

    methnum = (ush)MIN(G.crec.compression_method, NUM_METHODS);
    hostnum = (ush)MIN(G.crec.version_made_by[1], NUM_HOSTS);
    hostver = G.crec.version_made_by[0];
/*
    extnum = MIN(G.crec.version_needed_to_extract[1], NUM_HOSTS);
    extver = G.crec.version_needed_to_extract[0];
 */

    zfstrcpy(methbuf, method[methnum]);
    if (methnum == IMPLODED) {
        methbuf[1] = (char)((G.crec.general_purpose_bit_flag & 2)? '8' : '4');
        methbuf[3] = (char)((G.crec.general_purpose_bit_flag & 4)? '3' : '2');
    } else if (methnum == DEFLATED) {
        ush  dnum=(ush)((G.crec.general_purpose_bit_flag>>1) & 3);
        methbuf[3] = dtype[dnum];
    } else if (methnum >= NUM_METHODS) {   /* unknown */
        sprintf(&methbuf[1], "%03u", G.crec.compression_method);
    }

    for (k = 0;  k < 15;  ++k)
        attribs[k] = ' ';
    attribs[15] = 0;

    xattr = (ush)((G.crec.external_file_attributes >> 16) & 0xFFFF);
    switch (hostnum) {
        case VMS_:
            {   int    i, j;

                for (k = 0;  k < 12;  ++k)
                    workspace[k] = 0;
                if (xattr & VMS_IRUSR)
                    workspace[0] = 'R';
                if (xattr & VMS_IWUSR) {
                    workspace[1] = 'W';
                    workspace[3] = 'D';
                }
                if (xattr & VMS_IXUSR)
                    workspace[2] = 'E';
                if (xattr & VMS_IRGRP)
                    workspace[4] = 'R';
                if (xattr & VMS_IWGRP) {
                    workspace[5] = 'W';
                    workspace[7] = 'D';
                }
                if (xattr & VMS_IXGRP)
                  workspace[6] = 'E';
                if (xattr & VMS_IROTH)
                    workspace[8] = 'R';
                if (xattr & VMS_IWOTH) {
                    workspace[9] = 'W';
                    workspace[11] = 'D';
                }
                if (xattr & VMS_IXOTH)
                    workspace[10] = 'E';

                p = attribs;
                for (k = j = 0;  j < 3;  ++j) {     /* groups of permissions */
                    for (i = 0;  i < 4;  ++i, ++k)  /* perms within a group */
                        if (workspace[k])
                            *p++ = workspace[k];
                    *p++ = ',';                     /* group separator */
                }
                *--p = ' ';   /* overwrite last comma */
                if ((p - attribs) < 12)
                    sprintf(&attribs[12], "%d.%d", hostver/10, hostver%10);
            }
            break;

        case FS_FAT_:
        case FS_HPFS_:
        case FS_NTFS_:
        case VM_CMS_:
        case FS_VFAT_:
        case MVS_:
        case BEBOX_:      /* GRR:  guessing */
        case ACORN_:
            xattr = (ush)(G.crec.external_file_attributes & 0xFF);
            sprintf(attribs, ".r.-...     %d.%d", hostver/10, hostver%10);
            attribs[2] = (xattr & 0x01)? '-' : 'w';
            attribs[5] = (xattr & 0x02)? 'h' : '-';
            attribs[6] = (xattr & 0x04)? 's' : '-';
            attribs[4] = (xattr & 0x20)? 'a' : '-';
            if (xattr & 0x10) {
                attribs[0] = 'd';
                attribs[3] = 'x';
            } else
                attribs[0] = '-';
            if (IS_VOLID(xattr))
                attribs[0] = 'V';
            else if ((p = strrchr(G.filename, '.')) != (char *)NULL) {
                ++p;
                if (STRNICMP(p, "com", 3) == 0 || STRNICMP(p, "exe", 3) == 0 ||
                    STRNICMP(p, "btm", 3) == 0 || STRNICMP(p, "cmd", 3) == 0 ||
                    STRNICMP(p, "bat", 3) == 0)
                    attribs[3] = 'x';
            }
            break;

        case AMIGA_:
            switch (xattr & AMI_IFMT) {
                case AMI_IFDIR:  attribs[0] = 'd';  break;
                case AMI_IFREG:  attribs[0] = '-';  break;
                default:         attribs[0] = '?';  break;
            }
            attribs[1] = (xattr & AMI_IHIDDEN)?   'h' : '-';
            attribs[2] = (xattr & AMI_ISCRIPT)?   's' : '-';
            attribs[3] = (xattr & AMI_IPURE)?     'p' : '-';
            attribs[4] = (xattr & AMI_IARCHIVE)?  'a' : '-';
            attribs[5] = (xattr & AMI_IREAD)?     'r' : '-';
            attribs[6] = (xattr & AMI_IWRITE)?    'w' : '-';
            attribs[7] = (xattr & AMI_IEXECUTE)?  'e' : '-';
            attribs[8] = (xattr & AMI_IDELETE)?   'd' : '-';
            sprintf(&attribs[12], "%d.%d", hostver/10, hostver%10);
            break;

        default:   /* assume Unix-like */
            switch (xattr & UNX_IFMT) {
                case UNX_IFDIR:   attribs[0] = 'd';  break;
                case UNX_IFREG:   attribs[0] = '-';  break;
                case UNX_IFLNK:   attribs[0] = 'l';  break;
                case UNX_IFBLK:   attribs[0] = 'b';  break;
                case UNX_IFCHR:   attribs[0] = 'c';  break;
                case UNX_IFIFO:   attribs[0] = 'p';  break;
                case UNX_IFSOCK:  attribs[0] = 's';  break;
                default:          attribs[0] = '?';  break;
            }
            attribs[1] = (xattr & UNX_IRUSR)? 'r' : '-';
            attribs[4] = (xattr & UNX_IRGRP)? 'r' : '-';
            attribs[7] = (xattr & UNX_IROTH)? 'r' : '-';
            attribs[2] = (xattr & UNX_IWUSR)? 'w' : '-';
            attribs[5] = (xattr & UNX_IWGRP)? 'w' : '-';
            attribs[8] = (xattr & UNX_IWOTH)? 'w' : '-';

            if (xattr & UNX_IXUSR)
                attribs[3] = (xattr & UNX_ISUID)? 's' : 'x';
            else
                attribs[3] = (xattr & UNX_ISUID)? 'S' : '-';  /* S==undefined */
            if (xattr & UNX_IXGRP)
                attribs[6] = (xattr & UNX_ISGID)? 's' : 'x';  /* == UNX_ENFMT */
            else
                /* attribs[6] = (xattr & UNX_ISGID)? 'l' : '-';  real 4.3BSD */
                attribs[6] = (xattr & UNX_ISGID)? 'S' : '-';  /* SunOS 4.1.x */
            if (xattr & UNX_IXOTH)
                attribs[9] = (xattr & UNX_ISVTX)? 't' : 'x';  /* "sticky bit" */
            else
                attribs[9] = (xattr & UNX_ISVTX)? 'T' : '-';  /* T==undefined */

            sprintf(&attribs[12], "%d.%d", hostver/10, hostver%10);
            break;

    } /* end switch (hostnum: external attributes format) */

    Info(slide, 0, ((char *)slide, "%s %s %7lu ", attribs,
      LoadFarStringSmall(os[hostnum]), G.crec.ucsize));
    Info(slide, 0, ((char *)slide, "%c",
      (G.crec.general_purpose_bit_flag & 1)?
      ((G.crec.internal_file_attributes & 1)? 'T' : 'B') :  /* encrypted */
      ((G.crec.internal_file_attributes & 1)? 't' : 'b'))); /* plaintext */
    Info(slide, 0, ((char *)slide, "%c",
      (G.crec.extra_field_length || G.crec.external_file_attributes & 0xC000)?
      ((G.crec.general_purpose_bit_flag & 8)? 'X' : 'x') :  /* extra field */
      ((G.crec.general_purpose_bit_flag & 8)? 'l' : '-'))); /* no extra field */
      /* ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^ extended local header or not */

    if (G.lflag == 4) {
        ulg csiz = G.crec.csize;

        if (G.crec.general_purpose_bit_flag & 1)
            csiz -= 12;    /* if encrypted, don't count encryption header */
        Info(slide, 0, ((char *)slide, "%3d%%",
          (ratio(G.crec.ucsize,csiz)+5)/10));
    } else if (G.lflag == 5)
        Info(slide, 0, ((char *)slide, " %7lu", G.crec.csize));

    /* Read the extra field, if any.  The extra field info may be used
     * in the file modification time section, below.
     */
    if ((error = do_string(__G__ G.crec.extra_field_length, EXTRA_FIELD)) != 0)
    {
        if (G.extra_field != NULL) {
            free(G.extra_field);
            G.extra_field = NULL;
        }
        error_in_archive = error;
        /* We do not return prematurely in case of a "fatal" error (PK_EOF).
         * This does not hurt here, because we do not need to read from the
         * zipfile again before the end of this function.
         */
    }

    /* For printing of date & time, a "char d_t_buf[16]" is required.
     * To save stack space, we reuse the "char attribs[16]" buffer whose
     * content is no longer needed.
     */
#   define d_t_buf attribs
#ifdef USE_EF_UX_TIME
    z_modtim = (G.extra_field && ef_scan_for_izux(G.extra_field,
      G.crec.extra_field_length, &z_utime, NULL) > 0)? &z_utime.modtime : NULL;
    d_t_buf[0] = (char)0;   /* signal "show local time" */
#else
#   define z_modtim NULL
#endif
    Info(slide, 0, ((char *)slide, " %s %s ", methbuf,
      zi_time(__G__ &G.crec.last_mod_file_date, &G.crec.last_mod_file_time,
      z_modtim, d_t_buf)));
    fnprint(__G);

/*---------------------------------------------------------------------------
    Skip the file comment, if any (the filename has already been printed,
    above).  That finishes up this file entry...
  ---------------------------------------------------------------------------*/

    SKIP_(G.crec.file_comment_length)

    return error_in_archive;

} /* end function zi_short() */





/************************/
/*  Function zi_time()  */
/************************/

static char *zi_time(__G__ datez, timez, modtimez, d_t_str)
    __GDEF
    const ush *datez, *timez;
    const time_t *modtimez;
    char *d_t_str;
{
    ush yr, mo, dy, hh, mm, ss;
    char monthbuf[4];
    char *monthstr;
    static char Far month[12][4] = {
        "Jan", "Feb", "Mar", "Apr", "May", "Jun",
        "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
    };



/*---------------------------------------------------------------------------
    Convert the file-modification date and time info to a string of the form
    "1991 Feb 23 17:15:00", "23-Feb-91 17:15" or "19910223.171500", depending
    on values of lflag and T_flag.  If using Unix-time extra fields, convert
    to local time or not, depending on value of first character in d_t_str[].
  ---------------------------------------------------------------------------*/

#ifdef USE_EF_UX_TIME
    if (modtimez != NULL) {
        struct tm *t;

#ifndef NO_GMTIME
        /* check for our secret message from above... */
        t = (d_t_str[0] == (char)1)? gmtime(modtimez) : localtime(modtimez);
#else
        t = localtime(modtimez);
#endif
        mo = (ush)(t->tm_mon + 1);
        dy = (ush)(t->tm_mday);
        yr = (ush)(t->tm_year);

        hh = (ush)(t->tm_hour);
        mm = (ush)(t->tm_min);
        ss = (ush)(t->tm_sec);
    } else
#endif /* USE_EF_UX_TIME */
    {
        yr = (ush)(((*datez >> 9) & 0x7f) + 80);
        mo = (ush)((*datez >> 5) & 0x0f);
        dy = (ush)(*datez & 0x1f);

        hh = (ush)((*timez >> 11) & 0x1f);
        mm = (ush)((*timez >> 5) & 0x3f);
        ss = (ush)((*timez & 0x1f) * 2);
    }

    if (mo == 0 || mo > 12) {
        sprintf(monthbuf, LoadFarString(BogusFmt), mo);
        monthstr = monthbuf;
    } else
        monthstr = LoadFarStringSmall(month[mo-1]);

    if (G.lflag > 9)  /* verbose listing format */
        sprintf(d_t_str, LoadFarString(YMDHMSTime), yr+1900, monthstr, dy, hh,
          mm, ss);
    else if (G.T_flag)
        sprintf(d_t_str, LoadFarString(DecimalTime), yr+1900, mo, dy, hh, mm,
          ss);
    else   /* was:  if ((lflag >= 3) && (lflag <= 5)) */
        sprintf(d_t_str, LoadFarString(DMYHMTime), dy, monthstr, yr%100, hh,
          mm);

    return d_t_str;

} /* end function zi_time() */

#endif /* !NO_ZIPINFO */
