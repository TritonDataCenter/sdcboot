/*---------------------------------------------------------------------------

  extract.c

  This file contains the high-level routines ("driver routines") for extrac-
  ting and testing zipfile members.  It calls the low-level routines in files
  explode.c, inflate.c, unreduce.c and unshrink.c.

  Contains:  extract_or_test_files()
             store_info()
             extract_or_test_member()
             TestExtraField()
             test_OS2()
             memextract()
             memflush()
             fnfilter()

  ---------------------------------------------------------------------------*/


#define UNZIP_INTERNAL
#include "unzip.h"
#include "crypt.h"
#ifdef MSWIN
#  include "wingui\wizunzip.h"
#  include "wingui\replace.h"
#endif

#define GRRDUMP(buf,len) { \
    int i, j; \
 \
    for (j = 0;  j < (len)/16;  ++j) { \
        printf("        "); \
        for (i = 0;  i < 16;  ++i) \
            printf("%02x ", (uch)(buf)[i+(j<<4)]); \
        printf("\n        "); \
        for (i = 0;  i < 16;  ++i) { \
            char c = (char)(buf)[i+(j<<4)]; \
 \
            if (c == '\n') \
                printf("\\n "); \
            else if (c == '\r') \
                printf("\\r "); \
            else \
                printf(" %c ", c); \
        } \
        printf("\n"); \
    } \
    if ((len) % 16) { \
        printf("        "); \
        for (i = j<<4;  i < (len);  ++i) \
            printf("%02x ", (uch)(buf)[i]); \
        printf("\n        "); \
        for (i = j<<4;  i < (len);  ++i) { \
            char c = (char)(buf)[i]; \
 \
            if (c == '\n') \
                printf("\\n "); \
            else if (c == '\r') \
                printf("\\r "); \
            else \
                printf(" %c ", c); \
        } \
        printf("\n"); \
    } \
}

static int store_info OF((__GPRO));
static int extract_or_test_member OF((__GPRO));
static int TestExtraField OF((__GPRO__ uch *ef, unsigned ef_len));
static int test_OS2 OF((__GPRO__ uch *eb, unsigned eb_size));



/*******************************/
/*  Strings used in extract.c  */
/*******************************/

static char Far VersionMsg[] =
  "";
static char Far ComprMsg[] =
  "";
static char Far FilNamMsg[] =
  "";
static char Far ExtFieldMsg[] =
  "";
static char Far OffsetMsg[] =
  "";
static char Far ExtractMsg[] =
  "";
#ifndef SFX
   static char Far LengthMsg[] =
     "";
#endif

static char Far BadFileCommLength[] = "";
static char Far LocalHdrSig[] = "";
static char Far BadLocalHdr[] = "";
static char Far AttemptRecompensate[] = "";
static char Far BackslashPathSep[] =
  "";
static char Far SkipVolumeLabel[] = "";

#ifndef MSWIN
   static char Far ReplaceQuery[] =
     "";
   static char Far AssumeNone[] = "";
   static char Far NewNameQuery[] = "";
   static char Far InvalidResponse[] = "";
#endif /* !MSWIN */

static char Far ErrorInArchive[] = "";
static char Far ZeroFilesTested[] = "";

#ifndef VMS
   static char Far VMSFormatQuery[] =
     "";
#endif

#ifdef CRYPT
   static char Far SkipCantGetPasswd[] =
     "";
   static char Far SkipIncorrectPasswd[] =
     "";
   static char Far FilesSkipBadPasswd[] =
     "";
   static char Far MaybeBadPasswd[] =
     "";
#else
   static char Far SkipEncrypted[] =
     "";
#endif

static char Far NoErrInCompData[] =
  "";
static char Far NoErrInTestedFiles[] =
  "";
static char Far FilesSkipped[] =
  "";

static char Far ErrUnzipFile[] = "";
static char Far ErrUnzipNoFile[] = "";
static char Far NotEnoughMem[] = "";
static char Far InvalidComprData[] = "";
static char Far Inflate[] = "";

#ifndef SFX
   static char Far Explode[] = "";
   static char Far Unshrink[] = "";
#endif

static char Far FileUnknownCompMethod[] = "";
static char Far BadCRC[] = "";

static char Far InconsistEFlength[] =
  "";
      /* TruncEAs[] also used in OS/2 mapname(), close_outfile() */
char Far TruncEAs[] = "";
static char Far InvalidComprDataEAs[] = "";
static char Far BadCRC_EAs[] = "";
static char Far UnknComprMethodEAs[] =
  "";
static char Far NotEnoughMemEAs[] = "";
static char Far UnknErrorEAs[] = "";

static char Far UnsupportedExtraField[] =
  "";
static char Far BadExtraFieldCRC[] =
  "";





/**************************************/
/*  Function extract_or_test_files()  */
/**************************************/

int extract_or_test_files(__G)    /* return PK-type error code */
     __GDEF
{
    uch *cd_inptr;
    int i, j, cd_incnt, renamed, query, filnum=(-1), blknum=0;
    int error, error_in_archive=PK_COOL, *fn_matched=NULL, *xn_matched=NULL;
#ifndef MSWIN
    int len;
#endif
    ush members_remaining, num_skipped=0, num_bad_pwd=0;
    long cd_bufstart, bufstart, inbuf_offset, request;
    LONGINT old_extra_bytes=0L;
    /* static min_info info[DIR_BLKSIZ]; moved to globals.h */


/*---------------------------------------------------------------------------
    The basic idea of this function is as follows.  Since the central di-
    rectory lies at the end of the zipfile and the member files lie at the
    beginning or middle or wherever, it is not very desirable to simply
    read a central directory entry, jump to the member and extract it, and
    then jump back to the central directory.  In the case of a large zipfile
    this would lead to a whole lot of disk-grinding, especially if each mem-
    ber file is small.  Instead, we read from the central directory the per-
    tinent information for a block of files, then go extract/test the whole
    block.  Thus this routine contains two small(er) loops within a very
    large outer loop:  the first of the small ones reads a block of files
    from the central directory; the second extracts or tests each file; and
    the outer one loops over blocks.  There's some file-pointer positioning
    stuff in between, but that's about it.  Btw, it's because of this jump-
    ing around that we can afford to be lenient if an error occurs in one of
    the member files:  we should still be able to go find the other members,
    since we know the offset of each from the beginning of the zipfile.
  ---------------------------------------------------------------------------*/

    G.pInfo = G.info;
    members_remaining = G.ecrec.total_entries_central_dir;
#if (defined(CRYPT) || !defined(NO_ZIPINFO))
    G.newzip = TRUE;
    G.reported_backslash = FALSE;
#endif

    /* malloc space for check on unmatched filespecs (OK if one or both NULL) */
    if (G.filespecs > 0  &&
        (fn_matched=(int *)malloc(G.filespecs*sizeof(int))) != (int *)NULL)
        for (i = 0;  i < G.filespecs;  ++i)
            fn_matched[i] = FALSE;
    if (G.xfilespecs > 0  &&
        (xn_matched=(int *)malloc(G.xfilespecs*sizeof(int))) != (int *)NULL)
        for (i = 0;  i < G.xfilespecs;  ++i)
            xn_matched[i] = FALSE;

/*---------------------------------------------------------------------------
    Begin main loop over blocks of member files.  We know the entire central
    directory is on this disk:  we would not have any of this information un-
    less the end-of-central-directory record was on this disk, and we would
    not have gotten to this routine unless this is also the disk on which
    the central directory starts.  In practice, this had better be the ONLY
    disk in the archive, but we'll add multi-disk support soon.
  ---------------------------------------------------------------------------*/

    while (members_remaining) {
        j = 0;
#ifdef AMIGA
        memzero(G.filenotes, DIR_BLKSIZ * sizeof(char *));
#endif

        /*
         * Loop through files in central directory, storing offsets, file
         * attributes, case-conversion and text-conversion flags until block
         * size is reached.
         */

        while (members_remaining && (j < DIR_BLKSIZ)) {
            --members_remaining;
            G.pInfo = &G.info[j];

            if (readbuf(__G__ G.sig, 4) == 0) {
                error_in_archive = PK_EOF;
                members_remaining = 0;  /* ...so no more left to do */
                break;
            }
            if (strncmp(G.sig, G.central_hdr_sig, 4)) {  /* just to make sure */
                Info(slide, 0x401, ((char *)slide, LoadFarString(CentSigMsg), j));
                Info(slide, 0x401, ((char *)slide, LoadFarString(ReportMsg)));
                error_in_archive = PK_BADERR;
                members_remaining = 0;  /* ...so no more left to do */
                break;
            }
            /* process_cdir_file_hdr() sets pInfo->hostnum, pInfo->lcflag */
            if ((error = process_cdir_file_hdr(__G)) != PK_COOL) {
                error_in_archive = error;   /* only PK_EOF defined */
                members_remaining = 0;  /* ...so no more left to do */
                break;
            }
            if ((error = do_string(__G__ G.crec.filename_length, DS_FN)) !=
                 PK_COOL)
            {
                if (error > error_in_archive)
                    error_in_archive = error;
                if (error > PK_WARN) {  /* fatal:  no more left to do */
                    Info(slide, 0x401, ((char *)slide, LoadFarString(FilNamMsg),
                      fnfilter(G.filename, slide + WSIZE / 2), "central"));
                    members_remaining = 0;
                    break;
                }
            }
            if ((error = do_string(__G__ G.crec.extra_field_length,
                EXTRA_FIELD)) != 0)
            {
                if (error > error_in_archive)
                    error_in_archive = error;
                if (error > PK_WARN) {  /* fatal */
                    Info(slide, 0x401, ((char *)slide,
                      LoadFarString(ExtFieldMsg),
                      fnfilter(G.filename, slide + WSIZE / 2), "central"));
                    members_remaining = 0;
                    break;
                }
            }
#ifdef AMIGA
            G.filenote_slot = j;
            if ((error = do_string(__G__ G.crec.file_comment_length,
                                   G.N_flag ? FILENOTE : SKIP)) != PK_COOL)
#else
            if ((error = do_string(__G__ G.crec.file_comment_length, SKIP))
                != PK_COOL)
#endif
            {
                if (error > error_in_archive)
                    error_in_archive = error;
                if (error > PK_WARN) {  /* fatal */
                    Info(slide, 0x421, ((char *)slide,
                      LoadFarString(BadFileCommLength),
                      fnfilter(G.filename, slide + WSIZE / 2)));
                    members_remaining = 0;
                    break;
                }
            }
            if (G.process_all_files) {
                if (store_info(__G))
                    ++j;  /* file is OK; info[] stored; continue with next */
                else
                    ++num_skipped;
            } else {
                int   do_this_file = FALSE;
                char  **pfn = G.pfnames-1;

                while (*++pfn)
                    if (match(G.filename, *pfn, G.C_flag)) {
                        do_this_file = TRUE;   /* ^-- ignore case or not? */
                        if (fn_matched)
                            fn_matched[(int)(pfn-G.pfnames)] = TRUE;
                        break;       /* found match, so stop looping */
                    }
                if (do_this_file) {  /* check if this is an excluded file */
                    char  **pxn = G.pxnames-1;

                    while (*++pxn)
                        if (match(G.filename, *pxn, G.C_flag)) {
                            do_this_file = FALSE;  /* ^-- ignore case or not? */
                            if (xn_matched)
                                xn_matched[(int)(pxn-G.pxnames)] = TRUE;
                            break;
                        }
                }
                if (do_this_file)
                    if (store_info(__G))
                        ++j;            /* file is OK */
                    else
                        ++num_skipped;  /* unsupp. compression or encryption */
            } /* end if (process_all_files) */


        } /* end while-loop (adding files to current block) */

        /* save position in central directory so can come back later */
        cd_bufstart = G.cur_zipfile_bufstart;
        cd_inptr = G.inptr;
        cd_incnt = G.incnt;

    /*-----------------------------------------------------------------------
        Second loop:  process files in current block, extracting or testing
        each one.
      -----------------------------------------------------------------------*/

        for (i = 0; i < j; ++i) {
            filnum = i + blknum*DIR_BLKSIZ;
            G.pInfo = &G.info[i];
#ifdef NOVELL_BUG_FAILSAFE
            G.dne = FALSE;  /* assume file exists until stat() says otherwise */
#endif

            /* if the target position is not within the current input buffer
             * (either haven't yet read far enough, or (maybe) skipping back-
             * ward), skip to the target position and reset readbuf(). */

            /* ZLSEEK(pInfo->offset):  */
            request = G.pInfo->offset + G.extra_bytes;
            inbuf_offset = request % INBUFSIZ;
            bufstart = request - inbuf_offset;

            Trace((stderr, "\ndebug: request = %ld, inbuf_offset = %ld\n",
              request, inbuf_offset));
            Trace((stderr,
              "debug: bufstart = %ld, cur_zipfile_bufstart = %ld\n",
              bufstart, G.cur_zipfile_bufstart));
            if (request < 0) {
                Info(slide, 0x401, ((char *)slide, LoadFarStringSmall(SeekMsg),
                  G.zipfn, LoadFarString(ReportMsg)));
                error_in_archive = PK_ERR;
                if (filnum == 0 && G.extra_bytes != 0L) {
                    Info(slide, 0x401, ((char *)slide,
                      LoadFarString(AttemptRecompensate)));
                    old_extra_bytes = G.extra_bytes;
                    G.extra_bytes = 0L;
                    request = G.pInfo->offset; /* could also check if this != 0 */
                    inbuf_offset = request % INBUFSIZ;
                    bufstart = request - inbuf_offset;
                    Trace((stderr, "debug: request = %ld, inbuf_offset = %ld\n",
                      request, inbuf_offset));
                    Trace((stderr,
                      "debug: bufstart = %ld, cur_zipfile_bufstart = %ld\n",
                      bufstart, G.cur_zipfile_bufstart));
                } else {
                    error_in_archive = PK_BADERR;
                    continue;  /* this one hosed; try next */
                }
            }
            /* try again */
            if (request < 0) {
                Trace((stderr, "debug: recompensated request still < 0\n"));
                Info(slide, 0x401, ((char *)slide, LoadFarStringSmall(SeekMsg),
                  G.zipfn, LoadFarString(ReportMsg)));
                error_in_archive = PK_BADERR;
                continue;
            } else if (bufstart != G.cur_zipfile_bufstart) {
                Trace((stderr, "debug: bufstart != cur_zipfile_bufstart\n"));
#ifdef USE_STRM_INPUT
                fseek((FILE *)G.zipfd,(LONGINT)bufstart,SEEK_SET);
                G.cur_zipfile_bufstart = ftell((FILE *)G.zipfd);
#else /* !USE_STRM_INPUT */
                G.cur_zipfile_bufstart =
                  lseek(G.zipfd,(LONGINT)bufstart,SEEK_SET);
#endif /* ?USE_STRM_INPUT */
                if ((G.incnt = read(G.zipfd,(char *)G.inbuf,INBUFSIZ)) <= 0)
                {
                    Info(slide, 0x401, ((char *)slide, LoadFarString(OffsetMsg),
                      filnum, "lseek", bufstart));
                    error_in_archive = PK_BADERR;
                    continue;   /* can still do next file */
                }
                G.inptr = G.inbuf + (int)inbuf_offset;
                G.incnt -= (int)inbuf_offset;
            } else {
                G.incnt += (int)(G.inptr-G.inbuf) - (int)inbuf_offset;
                G.inptr = G.inbuf + (int)inbuf_offset;
            }

            /* should be in proper position now, so check for sig */
            if (readbuf(__G__ G.sig, 4) == 0) {  /* bad offset */
                Info(slide, 0x401, ((char *)slide, LoadFarString(OffsetMsg),
                  filnum, "EOF", request));
                error_in_archive = PK_BADERR;
                continue;   /* but can still try next one */
            }
            if (strncmp(G.sig, G.local_hdr_sig, 4)) {
                Info(slide, 0x401, ((char *)slide, LoadFarString(OffsetMsg),
                  filnum, LoadFarStringSmall(LocalHdrSig), request));
                /*
                    GRRDUMP(G.sig, 4)
                    GRRDUMP(G.local_hdr_sig, 4)
                 */
                error_in_archive = PK_ERR;
                if ((filnum == 0 && G.extra_bytes != 0L) ||
                    (G.extra_bytes == 0L && old_extra_bytes != 0L)) {
                    Info(slide, 0x401, ((char *)slide,
                      LoadFarString(AttemptRecompensate)));
                    if (G.extra_bytes) {
                        old_extra_bytes = G.extra_bytes;
                        G.extra_bytes = 0L;
                    } else
                        G.extra_bytes = old_extra_bytes;  /* third attempt */
                    ZLSEEK(G.pInfo->offset)
                    if (readbuf(__G__ G.sig, 4) == 0) {  /* bad offset */
                        Info(slide, 0x401, ((char *)slide,
                          LoadFarString(OffsetMsg), filnum, "EOF", request));
                        error_in_archive = PK_BADERR;
                        continue;   /* but can still try next one */
                    }
                    if (strncmp(G.sig, G.local_hdr_sig, 4)) {
                        Info(slide, 0x401, ((char *)slide,
                          LoadFarString(OffsetMsg), filnum,
                          LoadFarStringSmall(LocalHdrSig), request));
                        error_in_archive = PK_BADERR;
                        continue;
                    }
                } else
                    continue;  /* this one hosed; try next */
            }
            if ((error = process_local_file_hdr(__G)) != PK_COOL) {
                Info(slide, 0x421, ((char *)slide, LoadFarString(BadLocalHdr),
                  filnum));
                error_in_archive = error;   /* only PK_EOF defined */
                continue;   /* can still try next one */
            }
            if ((error = do_string(__G__ G.lrec.filename_length, DS_FN)) !=
                 PK_COOL)
            {
                if (error > error_in_archive)
                    error_in_archive = error;
                if (error > PK_WARN) {
                    Info(slide, 0x401, ((char *)slide, LoadFarString(FilNamMsg),
                      fnfilter(G.filename, slide + WSIZE / 2), "local"));
                    continue;   /* go on to next one */
                }
            }
            if (G.extra_field != (uch *)NULL) {
                free(G.extra_field);
                G.extra_field = (uch *)NULL;
            }
            if ((error =
                 do_string(__G__ G.lrec.extra_field_length,EXTRA_FIELD)) != 0)
            {
                if (error > error_in_archive)
                    error_in_archive = error;
                if (error > PK_WARN) {
                    Info(slide, 0x401, ((char *)slide,
                      LoadFarString(ExtFieldMsg),
                      fnfilter(G.filename, slide + WSIZE / 2), "local"));
                    continue;   /* go on */
                }
            }

            /*
             * just about to extract file:  if extracting to disk, check if
             * already exists, and if so, take appropriate action according to
             * fflag/uflag/overwrite_all/etc. (we couldn't do this in upper
             * loop because we don't store the possibly renamed filename[] in
             * info[])
             */
#ifdef DLL
            if (!G.tflag && !G.cflag && !G.redirect_data)
#else
            if (!G.tflag && !G.cflag)
#endif
            {
                renamed = FALSE;   /* user hasn't renamed output file yet */

startover:
                query = FALSE;
#ifdef MACOS
                G.macflag = (G.pInfo->hostnum == MAC_);
#endif
                /* for files from DOS FAT, check for use of backslash instead
                 *  of slash as directory separator (bug in some zipper(s); so
                 *  far, not a problem in HPFS, NTFS or VFAT systems)
                 */
                if (G.pInfo->hostnum == FS_FAT_ && !strchr(G.filename, '/')) {
                    char *p=G.filename-1;

                    while (*++p) {
                        if (*p == '\\') {
                            if (!G.reported_backslash) {
                                Info(slide, 0x21, ((char *)slide,
                                  LoadFarString(BackslashPathSep), G.zipfn));
                                G.reported_backslash = TRUE;
                                if (!error_in_archive)
                                    error_in_archive = PK_WARN;
                            }
                            *p = '/';
                        }
                    }
                }

                /* mapname can create dirs if not freshening or if renamed */
                if ((error = mapname(__G__ renamed)) > PK_WARN) {
                    if (error == IZ_CREATED_DIR) {

                        /* GRR:  add code to set times/attribs on dirs--
                         * save to list, sort when done (a la zip), set
                         * times/attributes on deepest dirs first */

                    } else if (error == IZ_VOL_LABEL) {
#ifdef DOS_OS2_W32
                        Info(slide, 0x401, ((char *)slide,
                          LoadFarString(SkipVolumeLabel),
                          fnfilter(G.filename, slide + WSIZE / 2),
                          G.volflag? "hard disk " : ""));
#else
                        Info(slide, 1, ((char *)slide,
                          LoadFarString(SkipVolumeLabel),
                          fnfilter(G.filename, slide + WSIZE / 2), ""));
#endif
                    /*  if (!error_in_archive)
                            error_in_archive = PK_WARN;  */
                    } else if (error > PK_ERR  &&  error_in_archive < PK_ERR)
                        error_in_archive = PK_ERR;
                    Trace((stderr, "mapname(%s) returns error = %d\n",
                      G.filename, error));
                    continue;   /* go on to next file */
                }

                switch (check_for_newer(__G__ G.filename)) {
                    case DOES_NOT_EXIST:
#ifdef NOVELL_BUG_FAILSAFE
                        G.dne = TRUE;   /* stat() says file DOES NOT EXIST */
#endif
                        /* if freshening, don't skip if just renamed */
                        if (G.fflag && !renamed)
                            continue;   /* freshen (no new files):  skip */
                        break;
                    case EXISTS_AND_OLDER:
                        if (G.overwrite_none) {
#ifdef MSWIN
                            char szStr[WIZUNZIP_MAX_PATH];
                            if (!lpDCL->PromptToOverwrite) {
                                sprintf(szStr,
                                  "Target file exists.\nSkipping %s\n",
                                  G.filename);
                                win_fprintf(stdout, strlen(szStr), szStr);
                            } else {
                                query = TRUE;
                                break;
                            }
#endif
                            continue;   /* never overwrite:  skip file */
                        }
                        if (!G.overwrite_all)
                            query = TRUE;
                        break;
                    case EXISTS_AND_NEWER:             /* (or equal) */
                        if (G.overwrite_none || (G.uflag && !renamed)) {
#ifdef MSWIN
                            char szStr[WIZUNZIP_MAX_PATH];
                            if (!lpDCL->PromptToOverwrite) {
                                sprintf(szStr,
                                  "Target file newer.\nSkipping %s\n",
                                  G.filename);
                                win_fprintf(stdout, strlen(szStr), szStr);
                            } else {
                                query = TRUE;
                                break;
                            }
#endif
                            continue;  /* skip if update/freshen & orig name */
                        }
                        if (!G.overwrite_all)
                            query = TRUE;
                        break;
                }
                if (query) {
#ifdef MSWIN
                    switch (GetReplaceDlgRetVal()) {
                        case IDM_REPLACE_RENAME:
                            renamed = TRUE;
                            goto startover;
                        case IDM_REPLACE_YES:
                            break;
                        case IDM_REPLACE_ALL:
                            G.overwrite_all = TRUE;
                            G.overwrite_none = FALSE;  /* just to make sure */
                            break;
                        case IDM_REPLACE_NONE:
                            G.overwrite_none = TRUE;
                            G.overwrite_all = FALSE;  /* make sure */
                            /* FALL THROUGH, skip */
                        case IDM_REPLACE_NO:
                            continue;
                    }
#else /* !MSWIN */
reprompt:
                    Info(slide, 0x81, ((char *)slide,
                      LoadFarString(ReplaceQuery), G.filename));
                    if (fgets(G.answerbuf, 9, stdin) == (char *)NULL) {
                        Info(slide, 1, ((char *)slide,
                          LoadFarString(AssumeNone)));
                        *G.answerbuf = 'N';
                        if (!error_in_archive)
                            error_in_archive = 1;  /* not extracted:  warning */
                    }
                    switch (*G.answerbuf) {
                        case 'A':   /* dangerous option:  force caps */
                            G.overwrite_all = TRUE;
                            G.overwrite_none = FALSE;  /* just to make sure */
                            break;
                        case 'r':
                        case 'R':
                            do {
                                Info(slide, 0x81, ((char *)slide,
                                  LoadFarString(NewNameQuery)));
                                fgets(G.filename, FILNAMSIZ, stdin);
                                /* usually get \n here:  better check for it */
                                len = strlen(G.filename);
                                if (G.filename[len-1] == '\n')
                                    G.filename[--len] = 0;
                            } while (len == 0);
                            renamed = TRUE;
                            goto startover;   /* sorry for a goto */
                        case 'y':
                        case 'Y':
                            break;
                        case 'N':
                            G.overwrite_none = TRUE;
                            G.overwrite_all = FALSE;  /* make sure */
                            /* FALL THROUGH, skip */
                        case 'n':
                            continue;   /* skip file */
                        default:
                            Info(slide, 1, ((char *)slide,
                              LoadFarString(InvalidResponse), *G.answerbuf));
                            goto reprompt;   /* yet another goto? */
                    } /* end switch (*answerbuf) */
#endif /* ?MSWIN */
                } /* end if (query) */
            } /* end if (extracting to disk) */

#ifdef CRYPT
            if (G.pInfo->encrypted && (error = decrypt(__G)) != PK_COOL) {
                if (error == PK_MEM2) {
                    if (error > error_in_archive)
                        error_in_archive = error;
                    Info(slide, 0x401, ((char *)slide,
                      LoadFarString(SkipCantGetPasswd),
                      fnfilter(G.filename, slide + WSIZE / 2)));
                } else {  /* (error == PK_WARN) */
                    if (!((G.tflag && G.qflag) || (!G.tflag && !QCOND2)))
                        Info(slide, 0x401, ((char *)slide,
                          LoadFarString(SkipIncorrectPasswd),
                          fnfilter(G.filename, slide + WSIZE / 2)));
                    ++num_bad_pwd;
                }
                continue;   /* go on to next file */
            }
#endif /* CRYPT */
#ifdef MSWIN
            /* play sound during extraction or test, if requested */
            SoundDuring();
#endif
#ifdef AMIGA
            G.filenote_slot = i;
#endif
            G.disk_full = 0;
            if ((error = extract_or_test_member(__G)) != PK_COOL) {
                if (error > error_in_archive)
                    error_in_archive = error;       /* ...and keep going */
                if (G.disk_full > 1) {
#ifdef DYNALLOC_CRCTAB
                    nearfree((zvoid near *)CRC_32_TAB);
#endif /* DYNALLOC_CRCTAB */
                    if (fn_matched)
                        free((zvoid *)fn_matched);
                    if (xn_matched)
                        free((zvoid *)xn_matched);
                    return error_in_archive;        /* (unless disk full) */
                }
            }
        } /* end for-loop (i:  files in current block) */


        /*
         * Jump back to where we were in the central directory, then go and do
         * the next batch of files.
         */

#ifdef USE_STRM_INPUT
        fseek((FILE *)G.zipfd, (LONGINT)cd_bufstart, SEEK_SET);
        G.cur_zipfile_bufstart = ftell((FILE *)G.zipfd);
#else /* !USE_STRM_INPUT */
        G.cur_zipfile_bufstart = lseek(G.zipfd,(LONGINT)cd_bufstart,SEEK_SET);
#endif /* ?USE_STRM_INPUT */
        read(G.zipfd, (char *)G.inbuf, INBUFSIZ);  /* been here before... */
        G.inptr = cd_inptr;
        G.incnt = cd_incnt;
        ++blknum;

#ifdef TEST
        printf("\ncd_bufstart = %ld (%.8lXh)\n", cd_bufstart, cd_bufstart);
        printf("cur_zipfile_bufstart = %ld (%.8lXh)\n", cur_zipfile_bufstart,
          cur_zipfile_bufstart);
        printf("inptr-inbuf = %d\n", G.inptr-G.inbuf);
        printf("incnt = %d\n\n", G.incnt);
#endif

    } /* end while-loop (blocks of files in central directory) */

/*---------------------------------------------------------------------------
    Check for unmatched filespecs on command line and print warning if any
    found.  Free allocated memory.
  ---------------------------------------------------------------------------*/

    if (fn_matched) {
        for (i = 0;  i < G.filespecs;  ++i)
            if (!fn_matched[i]) {
#ifdef DLL
                if (!G.redirect_data && !G.redirect_text)
                    Info(slide, 0x401, ((char *)slide,
                      LoadFarString(FilenameNotMatched), G.pfnames[i]));
                else
                    setFileNotFound(__G);
#else
                Info(slide, 1, ((char *)slide, LoadFarString(FilenameNotMatched),
                  G.pfnames[i]));
#endif
                if (error_in_archive <= PK_WARN)
                    error_in_archive = PK_FIND;   /* some files not found */
            }
        free((zvoid *)fn_matched);
    }
    if (xn_matched) {
        for (i = 0;  i < G.xfilespecs;  ++i)
            if (!xn_matched[i])
                Info(slide, 0x401, ((char *)slide,
                  LoadFarString(ExclFilenameNotMatched), G.pxnames[i]));
        free((zvoid *)xn_matched);
    }

/*---------------------------------------------------------------------------
    Double-check that we're back at the end-of-central-directory record, and
    print quick summary of results, if we were just testing the archive.  We
    send the summary to stdout so that people doing the testing in the back-
    ground and redirecting to a file can just do a "tail" on the output file.
  ---------------------------------------------------------------------------*/

    if (readbuf(__G__ G.sig, 4) == 0)
        error_in_archive = PK_EOF;
    if (strncmp(G.sig, G.end_central_sig, 4)) {         /* just to make sure */
        Info(slide, 0x401, ((char *)slide, LoadFarString(EndSigMsg)));
        Info(slide, 0x401, ((char *)slide, LoadFarString(ReportMsg)));
        if (!error_in_archive)       /* don't overwrite stronger error */
            error_in_archive = PK_WARN;
    }
    ++filnum;  /* initialized to -1, so now zero if no files found */
    if (G.tflag) {
        int num=filnum - num_bad_pwd;

        if (G.qflag < 2) {         /* GRR 930710:  was (G.qflag == 1) */
            if (error_in_archive)
                Info(slide, 0, ((char *)slide, LoadFarString(ErrorInArchive),
                  (error_in_archive == 1)? "warning-" : "", G.zipfn));
            else if (num == 0)
                Info(slide, 0, ((char *)slide, LoadFarString(ZeroFilesTested),
                  G.zipfn));
            else if (G.process_all_files && (num_skipped+num_bad_pwd == 0))
                Info(slide, 0, ((char *)slide, LoadFarString(NoErrInCompData),
                  G.zipfn));
            else
                Info(slide, 0, ((char *)slide, LoadFarString(NoErrInTestedFiles)
                  , G.zipfn, num, (num==1)? "":"s"));
            if (num_skipped > 0)
                Info(slide, 0, ((char *)slide, LoadFarString(FilesSkipped),
                  num_skipped, (num_skipped==1)? "":"s"));
#ifdef CRYPT
            if (num_bad_pwd > 0)
                Info(slide, 0, ((char *)slide, LoadFarString(FilesSkipBadPasswd)
                  , num_bad_pwd, (num_bad_pwd==1)? "":"s"));
#endif /* CRYPT */
        } else if ((G.qflag == 0) && !error_in_archive && (num == 0))
            Info(slide, 0, ((char *)slide, LoadFarString(ZeroFilesTested),
              G.zipfn));
    }

    /* give warning if files not tested or extracted (first condition can still
     * happen if zipfile is empty and no files specified on command line) */
    if ((filnum == 0) && error_in_archive <= PK_WARN)
        error_in_archive = PK_FIND;   /* no files found at all */
    else if ((num_skipped > 0) && !error_in_archive)
        error_in_archive = PK_WARN;
#ifdef CRYPT
    else if ((num_bad_pwd > 0) && !error_in_archive)
        error_in_archive = PK_WARN;
#endif /* CRYPT */

    return error_in_archive;

} /* end function extract_or_test_files() */





/***************************/
/*  Function store_info()  */
/***************************/

static int store_info(__G)   /* return 0 if skipping, 1 if OK */
     __GDEF
{
#ifdef SFX
#  define UNKN_COMPR \
   (G.crec.compression_method!=STORED && G.crec.compression_method!=DEFLATED)
#else
#  ifdef COPYRIGHT_CLEAN  /* no reduced or tokenized files */
#    define UNKN_COMPR  (G.crec.compression_method>SHRUNK && \
     G.crec.compression_method!=IMPLODED && G.crec.compression_method!=DEFLATED)
#  else /* !COPYRIGHT_CLEAN */
#    define UNKN_COMPR \
     (G.crec.compression_method>IMPLODED && G.crec.compression_method!=DEFLATED)
#  endif /* ?COPYRIGHT_CLEAN */
#endif

/*---------------------------------------------------------------------------
    Check central directory info for version/compatibility requirements.
  ---------------------------------------------------------------------------*/

    G.pInfo->encrypted = G.crec.general_purpose_bit_flag & 1;       /* bit field */
    G.pInfo->ExtLocHdr = (G.crec.general_purpose_bit_flag & 8) == 8;/* bit field */
    G.pInfo->textfile = G.crec.internal_file_attributes & 1;        /* bit field */
    G.pInfo->crc = G.crec.crc32;
    G.pInfo->compr_size = G.crec.csize;

    switch (G.aflag) {
        case 0:
            G.pInfo->textmode = FALSE;   /* bit field */
            break;
        case 1:
            G.pInfo->textmode = G.pInfo->textfile;   /* auto-convert mode */
            break;
        default:  /* case 2: */
            G.pInfo->textmode = TRUE;
            break;
    }

    if (G.crec.version_needed_to_extract[1] == VMS_) {
        if (G.crec.version_needed_to_extract[0] > VMS_UNZIP_VERSION) {
            if (!((G.tflag && G.qflag) || (!G.tflag && !QCOND2)))
                Info(slide, 0x401, ((char *)slide, LoadFarString(VersionMsg),
                  fnfilter(G.filename, slide + WSIZE / 2), "VMS",
                  G.crec.version_needed_to_extract[0] / 10,
                  G.crec.version_needed_to_extract[0] % 10,
                  VMS_UNZIP_VERSION / 10, VMS_UNZIP_VERSION % 10));
            return 0;
        }
#ifndef VMS   /* won't be able to use extra field, but still have data */
        else if (!G.tflag && !G.overwrite_all) {   /* if -o, extract regardless */
            Info(slide, 0x481, ((char *)slide, LoadFarString(VMSFormatQuery),
              fnfilter(G.filename, slide + WSIZE / 2)));
            fgets(G.answerbuf, 9, stdin);
            if ((*G.answerbuf != 'y') && (*G.answerbuf != 'Y'))
                return 0;
        }
#endif /* !VMS */
    /* usual file type:  don't need VMS to extract */
    } else if (G.crec.version_needed_to_extract[0] > UNZIP_VERSION) {
        if (!((G.tflag && G.qflag) || (!G.tflag && !QCOND2)))
            Info(slide, 0x401, ((char *)slide, LoadFarString(VersionMsg),
              fnfilter(G.filename, slide + WSIZE / 2), "PK",
              G.crec.version_needed_to_extract[0] / 10,
              G.crec.version_needed_to_extract[0] % 10,
              UNZIP_VERSION / 10, UNZIP_VERSION % 10));
        return 0;
    }

    if UNKN_COMPR {
        if (!((G.tflag && G.qflag) || (!G.tflag && !QCOND2)))
            Info(slide, 0x401, ((char *)slide, LoadFarString(ComprMsg),
              fnfilter(G.filename, slide + WSIZE / 2),
              G.crec.compression_method));
        return 0;
    }
#ifndef CRYPT
    if (G.pInfo->encrypted) {
        if (!((G.tflag && G.qflag) || (!G.tflag && !QCOND2)))
            Info(slide, 0x401, ((char *)slide, LoadFarString(SkipEncrypted),
              fnfilter(G.filename, slide + WSIZE / 2)));
        return 0;
    }
#endif /* !CRYPT */

    /* map whatever file attributes we have into the local format */
    mapattr(__G);   /* GRR:  worry about return value later */

    G.pInfo->offset = (long) G.crec.relative_offset_local_header;
    return 1;

} /* end function store_info() */





/***************************************/
/*  Function extract_or_test_member()  */
/***************************************/

static int extract_or_test_member(__G)    /* return PK-type error code */
     __GDEF
{
    char *nul="[empty] ", *txt="[text]  ", *bin="[binary]";
#ifdef CMS_MVS
    char *ebc="[ebcdic]";
#endif
    register int b;
    int r, error=PK_COOL;
    ulg wsize;


/*---------------------------------------------------------------------------
    Initialize variables, buffers, etc.
  ---------------------------------------------------------------------------*/

    G.bits_left = 0;
    G.bitbuf = 0L;       /* unreduce and unshrink only */
    G.zipeof = 0;
    G.newfile = TRUE;
    G.crc32val = CRCVAL_INITIAL;

#ifdef SYMLINKS
    /* if file came from Unix and is a symbolic link and we are extracting
     * to disk, prepare to restore the link */
    if (S_ISLNK(G.pInfo->file_attr) &&
        (G.pInfo->hostnum == UNIX_ || G.pInfo->hostnum == ATARI_) &&
        !G.tflag && !G.cflag && (G.lrec.ucsize > 0))
        G.symlnk = TRUE;
    else
        G.symlnk = FALSE;
#endif /* SYMLINKS */

    if (G.tflag) {
        if (!G.qflag)
            Info(slide, 0, ((char *)slide, LoadFarString(ExtractMsg), "test",
              fnfilter(G.filename, slide + WSIZE / 2), "", ""));
    } else {
#ifdef DLL
        if (G.cflag && !G.redirect_data)
#else
        if (G.cflag)
#endif
        {
            G.outfile = stdout;
#ifdef DOS_H68_OS2_W32
#ifdef __HIGHC__
            setmode(G.outfile, _BINARY);
#else
            setmode(fileno(G.outfile), O_BINARY);
#endif
#           define NEWLINE "\r\n"
#else /* !DOS_H68_OS2_W32 */
#           define NEWLINE "\n"
#endif /* ?DOS_H68_OS2_W32 */
#ifdef VMS
            if (open_outfile(__G))   /* VMS:  required even for stdout! */
                return PK_DISK;
#endif
        } else if (open_outfile(__G))
            return PK_DISK;
    }

/*---------------------------------------------------------------------------
    Unpack the file.
  ---------------------------------------------------------------------------*/

    defer_leftover_input(__G);    /* so NEXTBYTE bounds check will work */
    switch (G.lrec.compression_method) {
        case STORED:
            if (!G.tflag && QCOND2) {
#ifdef SYMLINKS
                if (G.symlnk)   /* can also be deflated, but rarer... */
                    Info(slide, 0, ((char *)slide, LoadFarString(ExtractMsg),
                      "link", G.filename, "", ""));
                else
#endif /* SYMLINKS */
                Info(slide, 0, ((char *)slide, LoadFarString(ExtractMsg),
                  "extract", G.filename,
                  (G.aflag != 1 /* && pInfo->textfile == pInfo->textmode */ )?
                  "" : (G.lrec.ucsize == 0L? nul : (G.pInfo->textfile? txt :
                  bin)), G.cflag? NEWLINE : ""));
            }
#ifdef DLL
            if (G.redirect_data)
                wsize = G.redirect_size+1, G.outptr = G.redirect_buffer;
            else
#endif
                wsize = WSIZE, G.outptr = slide;
            G.outcnt = 0L;
            while ((b = NEXTBYTE) != EOF && !G.disk_full) {
                *G.outptr++ = (uch)b;
                if (++G.outcnt == wsize) {
                    flush(__G__ slide, G.outcnt, 0);
                    G.outptr = slide;
                    G.outcnt = 0L;
                }
            }
#ifdef DLL
            if (G.outcnt && !G.redirect_data)
#else
            if (G.outcnt)          /* flush final (partial) buffer */
#endif
                flush(__G__ slide, G.outcnt, 0);
            break;

#ifndef SFX
        case SHRUNK:
            if (!G.tflag && QCOND2) {
                Info(slide, 0, ((char *)slide, LoadFarString(ExtractMsg),
                  LoadFarStringSmall(Unshrink), G.filename,
                  (G.aflag != 1 /* && G.pInfo->textfile == G.pInfo->textmode */ )? ""
                  : (G.pInfo->textfile? txt : bin), G.cflag? NEWLINE : ""));
            }
            if ((r = unshrink(__G)) != PK_COOL) {
                if ((G.tflag && G.qflag) || (!G.tflag && !QCOND2))
                    Info(slide, 0x401, ((char *)slide,
                      LoadFarStringSmall(ErrUnzipFile),
                      LoadFarString(NotEnoughMem),
                      LoadFarStringSmall2(Unshrink),
                      G.filename));
                else
                    Info(slide, 0x401, ((char *)slide,
                      LoadFarStringSmall(ErrUnzipNoFile),
                      LoadFarString(NotEnoughMem),
                      LoadFarStringSmall2(Unshrink)));
                error = r;
            }
            break;

#ifndef COPYRIGHT_CLEAN
        case REDUCED1:
        case REDUCED2:
        case REDUCED3:
        case REDUCED4:
            if (!G.tflag && QCOND2) {
                Info(slide, 0, ((char *)slide, LoadFarString(ExtractMsg),
                  "unreduc", G.filename,
                  (G.aflag != 1 /* && G.pInfo->textfile == G.pInfo->textmode */ )? ""
                  : (G.pInfo->textfile? txt : bin), G.cflag? NEWLINE : ""));
            }
            unreduce(__G);
            break;
#endif /* !COPYRIGHT_CLEAN */

        case IMPLODED:
            if (!G.tflag && QCOND2) {
                Info(slide, 0, ((char *)slide, LoadFarString(ExtractMsg),
                  "explod", G.filename,
                  (G.aflag != 1 /* && pInfo->textfile == pInfo->textmode */ )? ""
                  : (G.pInfo->textfile? txt : bin), G.cflag? NEWLINE : ""));
            }
            if (((r = explode(__G)) != 0) && (r != 5)) {   /* treat 5 specially */
                if ((G.tflag && G.qflag) || (!G.tflag && !QCOND2))
                    Info(slide, 0x401, ((char *)slide,
                      LoadFarStringSmall(ErrUnzipFile), r == 3?
                      LoadFarString(NotEnoughMem) :
                      LoadFarString(InvalidComprData),
                      LoadFarStringSmall2(Explode), G.filename));
                else
                    Info(slide, 0x401, ((char *)slide,
                      LoadFarStringSmall(ErrUnzipNoFile), r == 3?
                      LoadFarString(NotEnoughMem) :
                      LoadFarString(InvalidComprData),
                      LoadFarStringSmall2(Explode)));
                error = (r == 3)? PK_MEM3 : PK_ERR;
            }
            if (r == 5) {
                int warning = ((ulg)G.used_csize <= G.lrec.csize);

                if ((G.tflag && G.qflag) || (!G.tflag && !QCOND2))
                    Info(slide, 0x401, ((char *)slide, LoadFarString(LengthMsg), "",
                      warning?  "warning" : "error", G.used_csize, G.lrec.ucsize,
                      warning?  "  " : "", G.lrec.csize, " [", G.filename, "]"));
                else
                    Info(slide, 0x401, ((char *)slide, LoadFarString(LengthMsg),
                      "\n", warning? "warning" : "error", G.used_csize,
                      G.lrec.ucsize, warning? "  ":"", G.lrec.csize, "", "", "."));
                error = warning? PK_WARN : PK_ERR;
            }
            break;
#endif /* !SFX */

        case DEFLATED:
            if (!G.tflag && QCOND2) {
                Info(slide, 0, ((char *)slide, LoadFarString(ExtractMsg),
                  "inflat", G.filename,
                  (G.aflag != 1 /* && pInfo->textfile == pInfo->textmode */ )? ""
                  : (G.pInfo->textfile? txt : bin), G.cflag? NEWLINE : ""));
            }
#ifndef USE_ZLIB  /* zlib's function is called inflate(), too */
#  define UZinflate inflate
#endif
            if ((r = UZinflate(__G)) != 0) {
                if ((G.tflag && G.qflag) || (!G.tflag && !QCOND2))
                    Info(slide, 0x401, ((char *)slide,
                      LoadFarStringSmall(ErrUnzipFile), r == 3?
                      LoadFarString(NotEnoughMem) :
                      LoadFarString(InvalidComprData),
                      LoadFarStringSmall2(Inflate), G.filename));
                else
                    Info(slide, 0x401, ((char *)slide,
                      LoadFarStringSmall(ErrUnzipNoFile), r == 3?
                      LoadFarString(NotEnoughMem) :
                      LoadFarString(InvalidComprData),
                      LoadFarStringSmall2(Inflate)));
                error = (r == 3)? PK_MEM3 : PK_ERR;
            }
            break;

        default:   /* should never get to this point */
            Info(slide, 0x401, ((char *)slide, LoadFarString(FileUnknownCompMethod),
              G.filename));
            /* close and delete file before return? */
            undefer_input(__G);
            return PK_WARN;

    } /* end switch (compression method) */

    if (G.disk_full) {            /* set by flush() */
        if (G.disk_full > 1) {
            undefer_input(__G);
            return PK_DISK;
        }
        error = PK_WARN;
    }

/*---------------------------------------------------------------------------
    Close the file and set its date and time (not necessarily in that order),
    and make sure the CRC checked out OK.  Logical-AND the CRC for 64-bit
    machines (redundant on 32-bit machines).
  ---------------------------------------------------------------------------*/

#ifdef VMS                  /* VMS:  required even for stdout! (final flush) */
    if (!G.tflag)          /* don't close NULL file */
        close_outfile(__G);
#else
#ifdef DLL
    if (!G.tflag && (!G.cflag || G.redirect_data))
        if (G.redirect_data)
            FINISH_REDIRECT();
        else
            close_outfile(__G);
#else
    if (!G.tflag && !G.cflag)   /* don't close NULL file or stdout */
        close_outfile(__G);
#endif
#endif /* VMS */


            /* GRR: CONVERT close_outfile() TO NON-VOID:  CHECK FOR ERRORS! */


    if (error > PK_WARN) {/* don't print redundant CRC error if error already */
        undefer_input(__G);
        return error;
    }
    if (G.crc32val != G.lrec.crc32) {
        /* if quiet enough, we haven't output the filename yet:  do it */
        if ((G.tflag && G.qflag) || (!G.tflag && !QCOND2))
            Info(slide, 0x401, ((char *)slide, "%-22s ", G.filename));
        Info(slide, 0x401, ((char *)slide, LoadFarString(BadCRC), G.crc32val,
          G.lrec.crc32));
#ifdef CRYPT
        if (G.pInfo->encrypted)
            Info(slide, 0x401, ((char *)slide, LoadFarString(MaybeBadPasswd)));
#endif
        error = PK_ERR;
    } else if (G.tflag) {
        if (G.extra_field) {
            if ((r = TestExtraField(__G__ G.extra_field,
                                    G.lrec.extra_field_length)) > error)
                error = r;
        } else if (!G.qflag)
            Info(slide, 0, ((char *)slide, " OK\n"));
    } else {
        if (QCOND2 && !error)   /* GRR:  is stdout reset to text mode yet? */
            Info(slide, 0, ((char *)slide, "\n"));
    }

    undefer_input(__G);
    return error;

} /* end function extract_or_test_member() */





/*******************************/
/*  Function TestExtraField()  */
/*******************************/

static int TestExtraField(__G__ ef, ef_len)
    __GDEF
    uch *ef;
    unsigned ef_len;
{
    ush ebID;
    unsigned ebLen;
    int r;

    /* we know the regular compressed file data tested out OK, or else we
     * wouldn't be here ==> print filename if any extra-field errors found
     */
    while (ef_len >= EB_HEADSIZE) {
        ebID = makeword(ef);
        ebLen = (unsigned)makeword(ef+EB_LEN);

        if (ebLen > (ef_len - EB_HEADSIZE)) {
           /* Discovered some extra field inconsistency! */
            if (G.qflag)
                Info(slide, 1, ((char *)slide, "%-22s ", G.filename));
            Info(slide, 1, ((char *)slide, LoadFarString(InconsistEFlength),
              ebLen, (ef_len - EB_HEADSIZE)));
            return PK_ERR;
        }

        switch (ebID) {
            case EF_OS2:
                if ((r = test_OS2(__G__ ef, ebLen)) != PK_OK) {
                    if (G.qflag)
                        Info(slide, 1, ((char *)slide, "%-22s ", G.filename));
                    switch (r) {
                        case IZ_EF_TRUNC:
                            Info(slide, 1, ((char *)slide,
                              LoadFarString(TruncEAs),
                              makeword(ef+2)-10, "\n"));
                            break;
                        case PK_ERR:
                            Info(slide, 1, ((char *)slide,
                              LoadFarString(InvalidComprDataEAs)));
                            break;
                        case PK_MEM3:
                        case PK_MEM4:
                            Info(slide, 1, ((char *)slide,
                              LoadFarString(NotEnoughMemEAs)));
                            break;
                        default:
                            if ((r & 0xff) != PK_ERR)
                                Info(slide, 1, ((char *)slide,
                                  LoadFarString(UnknErrorEAs)));
                            else {
                                ush m = (ush)(r >> 8);
                                if (m == DEFLATED)            /* GRR KLUDGE! */
                                    Info(slide, 1, ((char *)slide,
                                      LoadFarString(BadCRC_EAs)));
                                else
                                    Info(slide, 1, ((char *)slide,
                                      LoadFarString(UnknComprMethodEAs), m));
                            }
                            break;
                    }
                    return r;
                }
                break;

            case EF_PKVMS:
            case EF_ASIUNIX:
            case EF_IZVMS:
            case EF_IZUNIX:
            case EF_VMCMS:
            case EF_MVS:
            case EF_SPARK:
            case EF_AV:
            default:
                break;
        }
        ef_len -= (ebLen + EB_HEADSIZE);
        ef += (ebLen + EB_HEADSIZE);
    }

    if (!G.qflag)
        Info(slide, 0, ((char *)slide, " OK\n"));

    return PK_COOL;

} /* end function TestExtraField() */





/*************************/
/*  Function test_OS2()  */
/*************************/

static int test_OS2(__G__ eb, eb_size)
    __GDEF
    uch *eb;
    unsigned eb_size;
{
    ulg eb_ucsize = makelong(eb+4);
    uch *eb_uncompressed;
    int r;

    if (eb_ucsize > 0L && eb_size <= 10)
        return IZ_EF_TRUNC;               /* no compressed data! */

    if ((eb_uncompressed = (uch *)malloc((size_t)eb_ucsize)) == (uch *)NULL)
        return PK_MEM4;

    r = memextract(__G__ eb_uncompressed, eb_ucsize, eb+8, (ulg)(eb_size-4));
    free(eb_uncompressed);
    return r;

} /* end function test_OS2() */





/***************************/
/*  Function memextract()  */
/***************************/

int memextract(__G__ tgt, tgtsize, src, srcsize)  /* extract compressed */
    __GDEF                                        /*  extra field block; */
    uch *tgt, *src;                               /*  return PK-type error */
    ulg tgtsize, srcsize;                         /*  level */
{
    uch *old_inptr=G.inptr;
    int  old_incnt=G.incnt, r, error=PK_OK;
    ush  method;
    ulg  extra_field_crc;


    method = makeword(src);
    extra_field_crc = makelong(src+2);

    /* compressed extra field exists completely in memory at this location: */
    G.inptr = src + 2 + 4;      /* method and extra_field_crc */
    G.incnt = (int)(G.csize = (long)(srcsize - (2 + 4)));
    G.mem_mode = TRUE;
    G.outbufptr = tgt;
    G.outsize = tgtsize;

    switch (method) {
        case STORED:
            memcpy((char *)tgt, (char *)G.inptr, (extent)G.incnt);
            G.outcnt = G.csize;   /* for CRC calculation */
            break;
        case DEFLATED:
            G.outcnt = 0L;
            if ((r = UZinflate(__G)) != 0) {
                if (!G.tflag)
                    Info(slide, 0x401, ((char *)slide,
                      LoadFarStringSmall(ErrUnzipNoFile), r == 3?
                      LoadFarString(NotEnoughMem) :
                      LoadFarString(InvalidComprData),
                      LoadFarStringSmall2(Inflate)));
                error = (r == 3)? PK_MEM3 : PK_ERR;
            }
            if (G.outcnt == 0L)   /* inflate's final FLUSH sets outcnt */
                break;
            break;
        default:
            if (G.tflag)
                error = PK_ERR | ((int)method << 8);
            else {
                Info(slide, 0x401, ((char *)slide,
                  LoadFarString(UnsupportedExtraField), method));
                error = PK_ERR;  /* GRR:  should be passed on up via SetEAs() */
            }
            break;
    }

    G.inptr = old_inptr;
    G.incnt = old_incnt;
    G.mem_mode = FALSE;

    if (!error) {
        register ulg crcval = crc32(CRCVAL_INITIAL, tgt, (extent)G.outcnt);

        if (crcval != extra_field_crc) {
            if (G.tflag)
                error = PK_ERR | (DEFLATED << 8);  /* kludge for now */
            else {
                Info(slide, 0x401, ((char *)slide,
                  LoadFarString(BadExtraFieldCRC), G.zipfn, crcval,
                  extra_field_crc));
                error = PK_ERR;
            }
        }
    }
    return error;

} /* end function memextract() */





/*************************/
/*  Function memflush()  */
/*************************/

int memflush(__G__ rawbuf, size)
    __GDEF
    uch *rawbuf;
    ulg size;
{
    if (size > G.outsize)
        return 50;   /* more data than output buffer can hold */

    memcpy((char *)G.outbufptr, (char *)rawbuf, (extent)size);
    G.outbufptr += (unsigned int)size;
    G.outsize -= size;
    G.outcnt += size;

    return 0;

} /* end function memflush() */





/*************************/
/*  Function fnfilter()  */        /* here instead of in list.c for SFX */
/*************************/

char *fnfilter(raw, space)         /* convert name to safely printable form */
    char *raw;
    uch *space;
{
#ifndef NATIVE   /* ASCII:  filter ANSI escape codes, etc. */
    register uch *r=(uch *)raw, *s=space;
    while (*r)
        if (*r < 32)
            *s++ = '^', *s++ = (uch)(64 + *r++);
        else
            *s++ = *r++;
    *s = 0;
    return (char *)space;
#else
    return raw;
#endif
} /* end function fnfilter() */
