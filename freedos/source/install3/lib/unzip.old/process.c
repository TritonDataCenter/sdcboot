/*---------------------------------------------------------------------------

  process.c

  This file contains the top-level routines for processing multiple zipfiles.

  Contains:  process_zipfiles()
             do_seekable()
             find_ecrec()
             uz_end_central()
             process_cdir_file_hdr()
             get_cdir_ent()
             process_local_file_hdr()
             ef_scan_for_izux()

  ---------------------------------------------------------------------------*/


#define UNZIP_INTERNAL
#include "unzip.h"
#ifdef MSWIN
#  include <windows.h>
#  include "wingui\wizunzip.h"
#endif

static int    do_seekable        OF((__GPRO__ int lastchance));
static int    find_ecrec         OF((__GPRO__ long searchlen));

static char Far CantAllocateBuffers[] =
  "error:  can't allocate unzip buffers\n";

#ifdef SFX
   static char Far CantFindMyself[] =
     "unzipsfx:  can't find myself! [%s]\n";
#else /* !SFX */

   /* process_zipfiles() strings */
   static char Far FilesProcessOK[] = "";
   static char Far ArchiveWarning[] =
     "";
   static char Far ArchiveFatalError[] = "";
   static char Far FileHadNoZipfileDir[] =
     "";
   static char Far ZipfileWasDir[] = "";
   static char Far ManyZipfilesWereDir[] =
     "";
   static char Far NoZipfileFound[] = "";

   /* do_seekable() strings */
# ifdef UNIX
   static char Far CantFindZipfileDirMsg[] =
     "%s:  can't find zipfile directory in one of %s or\n\
        %s%s.zip, and can't find %s, period.\n";
   static char Far CantFindEitherZipfile[] =
     "%s:  can't find %s, %s.zip or %s, so there.\n";
# else /* !UNIX */
   static char Far CantFindWildcardMatch[] =
     "";
   static char Far CantFindZipfileDirMsg[] =
     "";
   static char Far CantFindEitherZipfile[] =
     "";
# endif /* ?UNIX */
   extern char Far Zipnfo[];              /* in unzip.c */
#ifndef MSWIN
   static char Far Unzip[] = "";
#else
   static char Far Unzip[] = "WizUnZip";
#endif
   static char Far MaybeExe[] =
     "";
   static char Far CentDirNotInZipMsg[] = "";
   static char Far EndCentDirBogus[] =
     "";
# ifdef NO_MULTIPART
   static char Far NoMultiDiskArcSupport[] =
     "";
   static char Far MaybePakBug[] = "";
# else
   static char Far MaybePakBug[] = "";
# endif
   static char Far ExtraBytesAtStart[] =
     "";
#endif /* ?SFX */

static char Far MissingBytes[] =
  "";
static char Far NullCentDirOffset[] =
  "";
static char Far ZipfileEmpty[] = "";
static char Far CentDirStartNotFound[] =
  "";
static char Far CentDirTooLong[] =
  "";
static char Far CentDirEndSigNotFound[] = "";
static char Far ZipfileCommTrunc1[] = "";




/*******************************/
/* Function process_zipfiles() */
/*******************************/

int process_zipfiles(__G)    /* return PK-type error code */
     __GDEF
{
#ifndef SFX
    char *lastzipfn = (char *)NULL;
    int NumWinFiles, NumLoseFiles, NumWarnFiles;
    int NumMissDirs, NumMissFiles;
#endif
    int error=0, error_in_archive=0;


/*---------------------------------------------------------------------------
    Start by allocating buffers and (re)constructing the various PK signature
    strings.
  ---------------------------------------------------------------------------*/

    G.inbuf = (uch *)malloc(INBUFSIZ + 4);    /* 4 extra for hold[] (below) */

    /* Normally realbuf and outbuf will be the same.  However, if the data
     * are redirected to a large memory buffer, realbuf will point to the
     * new location while outbuf will remain pointing to the malloc'd
     * memory buffer.  (The extra "1" is for string termination.) */
    G.realbuf = G.outbuf = (uch *)malloc(OUTBUFSIZ + 1);

    if ((G.inbuf == (uch *)NULL) || (G.outbuf == (uch *)NULL)) {
        Info(slide, 0x401, ((char *)slide, LoadFarString(CantAllocateBuffers)));
        RETURN(PK_MEM);
    }
    G.hold = G.inbuf + INBUFSIZ;     /* to check for boundary-spanning sigs */
#ifndef VMS     /* VMS uses its own buffer scheme for textmode flush(). */
#ifdef SMALL_MEM
    G.outbuf2 = G.outbuf+RAWBUFSIZ;  /* never changes */
#endif
#endif /* !VMS */

#if 0 /* CRC_32_TAB has been NULLified by CONSTRUCTGLOBALS !!!! */
    /* allocate the CRC table only later when we know we have a zipfile */
    CRC_32_TAB = (ulg *)NULL;
#endif /* 0 */

    G.local_hdr_sig[0]  /* = extd_local_sig[0] */  = 0x50;   /* ASCII 'P', */
    G.central_hdr_sig[0] = G.end_central_sig[0] = 0x50;     /* not EBCDIC */

    G.local_hdr_sig[1]  /* = extd_local_sig[1] */  = 0x4B;   /* ASCII 'K', */
    G.central_hdr_sig[1] = G.end_central_sig[1] = 0x4B;     /* not EBCDIC */

    /* GRR FIX:  no need for these to be in global struct; OK to "overwrite" */
    strcpy(G.local_hdr_sig+2, LOCAL_HDR_SIG);
    strcpy(G.central_hdr_sig+2, CENTRAL_HDR_SIG);
    strcpy(G.end_central_sig+2, END_CENTRAL_SIG);
/*  strcpy(extd_local_sig+2, EXTD_LOCAL_SIG);   still to be used in multi? */

#if (defined(OS2) && defined(__IBMC__))
    _tzset();   /* initialize timezone info (from TZ) since no other chance */
#endif          /*  (underscore name is accepted by all versions of C Set) */

/*---------------------------------------------------------------------------
    Match (possible) wildcard zipfile specification with existing files and
    attempt to process each.  If no hits, try again after appending ".zip"
    suffix.  If still no luck, give up.
  ---------------------------------------------------------------------------*/

#ifdef SFX
    if ((error = do_seekable(__G__ 0)) == PK_NOZIP) {
#ifdef EXE_EXTENSION
        int len=strlen(G.argv0);

        /* append .exe if appropriate; also .sfx? */
        if ( (G.zipfn = (char *)malloc(len+sizeof(EXE_EXTENSION))) !=
             (char *)NULL ) {
            strcpy(G.zipfn, G.argv0);
            strcpy(G.zipfn+len, EXE_EXTENSION);
            error = do_seekable(__G__ 0);
            free(G.zipfn);
            G.zipfn = G.argv0;  /* for "can't find myself" message only */
        }
#endif /* EXE_EXTENSION */
#ifdef WIN32
        G.zipfn = G.argv0;  /* for "can't find myself" message only */
#endif
    }
    if (error) {
        if (error == IZ_DIR)
            error_in_archive = PK_NOZIP;
        else
            error_in_archive = error;
        if (error == PK_NOZIP)
            Info(slide, 1, ((char *)slide, LoadFarString(CantFindMyself),
              G.zipfn));
    }

#else /* !SFX */
    NumWinFiles = NumLoseFiles = NumWarnFiles = 0;
    NumMissDirs = NumMissFiles = 0;

    while ((G.zipfn = do_wild(__G__ G.wildzipfn)) != (char *)NULL) {
        Trace((stderr, "do_wild( %s ) returns %s\n", G.wildzipfn, G.zipfn));

        lastzipfn = G.zipfn;

        /* print a blank line between the output of different zipfiles */
#ifdef TIMESTAMP
        if (!G.qflag  &&  !G.T_flag  &&  error != PK_NOZIP  &&  error != IZ_DIR
            && (NumWinFiles+NumLoseFiles+NumWarnFiles+NumMissFiles) > 0)
#else
        if (!G.qflag  &&  error != PK_NOZIP  &&  error != IZ_DIR
            && (NumWinFiles+NumLoseFiles+NumWarnFiles+NumMissFiles) > 0)
#endif
            (*G.message)((zvoid *)&G, (uch *)"\n", 1L, 0);

        if ((error = do_seekable(__G__ 0)) == PK_WARN)
            ++NumWarnFiles;
        else if (error == IZ_DIR)
            ++NumMissDirs;
        else if (error == PK_NOZIP)
            ++NumMissFiles;
        else if (error)
            ++NumLoseFiles;
        else
            ++NumWinFiles;

        if (error != IZ_DIR && error > error_in_archive)
            error_in_archive = error;
        Trace((stderr, "do_seekable(0) returns %d\n", error));

    } /* end while-loop (wildcard zipfiles) */

    if ((NumWinFiles + NumWarnFiles + NumLoseFiles) == 0  &&
        (NumMissDirs + NumMissFiles) == 1  &&  lastzipfn != (char *)NULL)
    {
        NumMissDirs = NumMissFiles = 0;
        if (error_in_archive == PK_NOZIP)
            error_in_archive = PK_COOL;

#if (!defined(UNIX) && !defined(AMIGA)) /* filenames with wildcard characters */
        if (iswild(G.wildzipfn))
            Info(slide, 0x401, ((char *)slide,
              LoadFarString(CantFindWildcardMatch), G.zipinfo_mode?
              LoadFarStringSmall(Zipnfo) : LoadFarStringSmall(Unzip),
              G.wildzipfn));
        else
#endif
        {
            char *p = lastzipfn + strlen(lastzipfn);

            G.zipfn = lastzipfn;
            strcpy(p, ZSUFX);

#ifdef UNIX   /* only Unix has case-sensitive filesystems */
            if ((error = do_seekable(__G__ 0)) == PK_NOZIP || error == IZ_DIR) {
                if (error == IZ_DIR)
                    ++NumMissDirs;
                strcpy(p, ".ZIP");
                error = do_seekable(__G__ 1);
            }
#else
            error = do_seekable(__G__ 1);
#endif
            if (error == PK_WARN)   /* GRR: make this a switch/case stmt ... */
                ++NumWarnFiles;
            else if (error == IZ_DIR)
                ++NumMissDirs;
            else if (error == PK_NOZIP)
                /* increment again => bug: "1 file had no zipfile directory." */
                /* ++NumMissFiles */ ;
            else if (error)
                ++NumLoseFiles;
            else
                ++NumWinFiles;

            if (error > error_in_archive)
                error_in_archive = error;
            Trace((stderr, "do_seekable(1) returns %d\n", error));
        }
    }
#endif /* ?SFX */

/*---------------------------------------------------------------------------
    Print summary of all zipfiles, assuming zipfile spec was a wildcard (no
    need for a summary if just one zipfile).
  ---------------------------------------------------------------------------*/

#ifndef SFX
    if (iswild(G.wildzipfn) && G.qflag < 3
#ifdef TIMESTAMP
                                           && !(G.T_flag && G.qflag)
#endif
                                                                    )
    {
        if (NumMissFiles + NumLoseFiles + NumWarnFiles > 0 || NumWinFiles != 1)
            if (!(G.tflag && G.qflag > 1)
#ifdef TIMESTAMP
                                          && !G.T_flag
#endif
                                                      )
                (*G.message)((zvoid *)&G, (uch *)"\n", 1L, 0x401);
        if ((NumWinFiles > 1) || (NumWinFiles == 1 &&
            NumMissDirs + NumMissFiles + NumLoseFiles + NumWarnFiles > 0))
            Info(slide, 0x401, ((char *)slide, LoadFarString(FilesProcessOK),
              NumWinFiles, (NumWinFiles == 1)? " was" : "s were"));
        if (NumWarnFiles > 0)
            Info(slide, 0x401, ((char *)slide, LoadFarString(ArchiveWarning),
              NumWarnFiles, (NumWarnFiles == 1)? "" : "s"));
        if (NumLoseFiles > 0)
            Info(slide, 0x401, ((char *)slide, LoadFarString(ArchiveFatalError),
              NumLoseFiles, (NumLoseFiles == 1)? "" : "s"));
        if (NumMissFiles > 0)
            Info(slide, 0x401, ((char *)slide,
              LoadFarString(FileHadNoZipfileDir), NumMissFiles,
              (NumMissFiles == 1)? "" : "s"));
        if (NumMissDirs == 1)
            Info(slide, 0x401, ((char *)slide, LoadFarString(ZipfileWasDir)));
        else if (NumMissDirs > 0)
            Info(slide, 0x401, ((char *)slide,
              LoadFarString(ManyZipfilesWereDir), NumMissDirs));
        if (NumWinFiles + NumLoseFiles + NumWarnFiles == 0)
            Info(slide, 0x401, ((char *)slide, LoadFarString(NoZipfileFound)));
    }
#endif /* !SFX */

    /* free allocated memory */
    inflate_free(__G);
    checkdir(__G__ (char *)NULL, END);
#ifdef DYNALLOC_CRCTAB
    if (G.extract_flag && CRC_32_TAB)
        nearfree((zvoid near *)CRC_32_TAB);
#endif /* DYNALLOC_CRCTAB */
#ifndef VMS     /* VMS uses its own buffer scheme for textmode flush(). */
#ifndef SMALL_MEM
    if (G.outbuf2)
        free(G.outbuf2);   /* malloc'd ONLY if unshrink and -a */
#endif
#endif /* VMS */
    free(G.outbuf);
    free(G.inbuf);

    return error_in_archive;

} /* end function process_zipfiles() */





/**************************/
/* Function do_seekable() */
/**************************/

static int do_seekable(__G__ lastchance)        /* return PK-type error code */
     __GDEF
    int lastchance;
{
#ifndef SFX
    /* static int no_ecrec = FALSE;  SKM: moved to globals.h */
    int maybe_exe=FALSE;
    int too_weird_to_continue=FALSE;
#endif
    int error=0, error_in_archive;


/*---------------------------------------------------------------------------
    Open the zipfile for reading in BINARY mode to prevent CR/LF translation,
    which would corrupt the bit streams.
  ---------------------------------------------------------------------------*/

    if (SSTAT(G.zipfn, &G.statbuf) ||
        (error = S_ISDIR(G.statbuf.st_mode)) != 0)
    {
#ifndef SFX
        if (lastchance)
#ifdef UNIX
            if (G.no_ecrec)
                Info(slide, 1, ((char *)slide,
                  LoadFarString(CantFindZipfileDirMsg), G.zipinfo_mode?
                  LoadFarStringSmall(Zipnfo) : LoadFarStringSmall(Unzip),
                  G.wildzipfn, G.zipinfo_mode? "  " : "", G.wildzipfn,
                  G.zipfn));
            else
                Info(slide, 1, ((char *)slide,
                  LoadFarString(CantFindEitherZipfile), G.zipinfo_mode?
                  LoadFarStringSmall(Zipnfo) : LoadFarStringSmall(Unzip),
                  G.wildzipfn, G.wildzipfn, G.zipfn));
#else /* !UNIX */
            if (G.no_ecrec)
                Info(slide, 0x401, ((char *)slide,
                  LoadFarString(CantFindZipfileDirMsg), G.zipinfo_mode?
                  LoadFarStringSmall(Zipnfo) : LoadFarStringSmall(Unzip),
                  G.wildzipfn, G.zipinfo_mode? "  " : "", G.zipfn));
            else
                Info(slide, 0x401, ((char *)slide,
                  LoadFarString(CantFindEitherZipfile), G.zipinfo_mode?
                  LoadFarStringSmall(Zipnfo) : LoadFarStringSmall(Unzip),
                  G.wildzipfn, G.zipfn));
#endif /* ?UNIX */
#endif /* !SFX */
        return error? IZ_DIR : PK_NOZIP;
    }
    G.ziplen = G.statbuf.st_size;

#ifndef SFX
#if defined(UNIX) || defined(DOS_OS2_W32)
    if (G.statbuf.st_mode & S_IEXEC)   /* no extension on Unix exes:  might */
        maybe_exe = TRUE;               /*  find unzip, not unzip.zip; etc. */
#endif
#endif /* !SFX */

#ifdef VMS
    if (check_format(__G))              /* check for variable-length format */
        return PK_ERR;
#endif

    if (open_input_file(__G))   /* this should never happen, given */
        return PK_NOZIP;        /*  the stat() test above, but... */

/*---------------------------------------------------------------------------
    Find and process the end-of-central-directory header.  UnZip need only
    check last 65557 bytes of zipfile:  comment may be up to 65535, end-of-
    central-directory record is 18 bytes, and signature itself is 4 bytes;
    add some to allow for appended garbage.  Since ZipInfo is often used as
    a debugging tool, search the whole zipfile if zipinfo_mode is true.
  ---------------------------------------------------------------------------*/

    /* initialize the CRC table pointer (once) */
    if (CRC_32_TAB == (ulg near *)NULL) {
        if ((CRC_32_TAB = (ulg near *)get_crc_table()) == (ulg near *)NULL)
            return PK_MEM2;
    }

    G.cur_zipfile_bufstart = 0;
    G.inptr = G.inbuf;

#if (!defined(MSWIN) && !defined(SFX))
#ifdef TIMESTAMP
    if (!G.qflag && !G.T_flag && !G.zipinfo_mode)
#else
    if (!G.qflag && !G.zipinfo_mode)
#endif
        Info(slide, 0, ((char *)slide, "Archive:  %s\n", G.zipfn));
#endif /* !MSWIN && !SFX */

    if ((
#ifndef NO_ZIPINFO
         G.zipinfo_mode &&
          ((error_in_archive = find_ecrec(__G__ G.ziplen)) != 0 ||
          (error_in_archive = zi_end_central(__G)) > PK_WARN))
        || (!G.zipinfo_mode &&
#endif
          ((error_in_archive = find_ecrec(__G__ MIN(G.ziplen,66000L))) != 0 ||
          (error_in_archive = uz_end_central(__G)) > PK_WARN)))
    {
        close(G.zipfd);
#ifdef SFX
        ++lastchance;   /* avoid picky compiler warnings */
        return error_in_archive;
#else
        if (maybe_exe)
            Info(slide, 0x401, ((char *)slide, LoadFarString(MaybeExe),
            G.zipfn));
        if (lastchance)
            return error_in_archive;
        else {
            G.no_ecrec = TRUE;    /* assume we found wrong file:  e.g., */
            return PK_NOZIP;       /*  unzip instead of unzip.zip */
        }
#endif /* ?SFX */
    }

    if ((G.zflag > 0) && !G.zipinfo_mode) {  /* unzip: zflag = comment ONLY */
        close(G.zipfd);
        return error_in_archive;
    }

/*---------------------------------------------------------------------------
    Test the end-of-central-directory info for incompatibilities (multi-disk
    archives) or inconsistencies (missing or extra bytes in zipfile).
  ---------------------------------------------------------------------------*/

#ifdef NO_MULTIPART
    error = !G.zipinfo_mode && (G.ecrec.number_this_disk == 1) &&
            (G.ecrec.num_disk_start_cdir == 1);
#else
    error = !G.zipinfo_mode && (G.ecrec.number_this_disk != 0);
#endif

#ifndef SFX
    if (G.zipinfo_mode &&
        G.ecrec.number_this_disk != G.ecrec.num_disk_start_cdir)
    {
        if (G.ecrec.number_this_disk > G.ecrec.num_disk_start_cdir) {
            Info(slide, 0x401, ((char *)slide,
              LoadFarString(CentDirNotInZipMsg), G.zipfn,
              G.ecrec.number_this_disk, G.ecrec.num_disk_start_cdir));
            error_in_archive = PK_FIND;
            too_weird_to_continue = TRUE;
        } else {
            Info(slide, 0x401, ((char *)slide,
              LoadFarString(EndCentDirBogus), G.zipfn,
              G.ecrec.number_this_disk, G.ecrec.num_disk_start_cdir));
            error_in_archive = PK_WARN;
        }
#ifdef NO_MULTIPART   /* concatenation of multiple parts works in some cases */
    } else if (!G.zipinfo_mode && !error && G.ecrec.number_this_disk != 0) {
        Info(slide, 0x401, ((char *)slide, LoadFarString(NoMultiDiskArcSupport),
          G.zipfn));
        error_in_archive = PK_FIND;
        too_weird_to_continue = TRUE;
#endif
    }

    if (!too_weird_to_continue) {  /* (relatively) normal zipfile:  go for it */
        if (error) {
            Info(slide, 0x401, ((char *)slide, LoadFarString(MaybePakBug),
              G.zipfn));
            error_in_archive = PK_WARN;
        }
#endif /* !SFX */
        if ((G.extra_bytes = G.real_ecrec_offset-G.expect_ecrec_offset) <
            (LONGINT)0)
        {
            Info(slide, 0x401, ((char *)slide, LoadFarString(MissingBytes),
              G.zipfn, (long)(-G.extra_bytes)));
            error_in_archive = PK_ERR;
        } else if (G.extra_bytes > 0) {
            if ((G.ecrec.offset_start_central_directory == 0) &&
                (G.ecrec.size_central_directory != 0))   /* zip 1.5 -go bug */
            {
                Info(slide, 0x401, ((char *)slide,
                  LoadFarString(NullCentDirOffset), G.zipfn));
                G.ecrec.offset_start_central_directory = G.extra_bytes;
                G.extra_bytes = 0;
                error_in_archive = PK_ERR;
            }
#ifndef SFX
            else {
                Info(slide, 0x401, ((char *)slide,
                  LoadFarString(ExtraBytesAtStart), G.zipfn,
                  (long)G.extra_bytes, (G.extra_bytes == 1)? "":"s"));
                error_in_archive = PK_WARN;
            }
#endif /* !SFX */
        }

    /*-----------------------------------------------------------------------
        Check for empty zipfile and exit now if so.
      -----------------------------------------------------------------------*/

        if (G.expect_ecrec_offset==0L && G.ecrec.size_central_directory==0) {
            if (G.zipinfo_mode)
                Info(slide, 0, ((char *)slide, "%sEmpty zipfile.\n",
                  G.lflag>9? "\n  " : ""));
            else
                Info(slide, 0x401, ((char *)slide, LoadFarString(ZipfileEmpty),
                                    G.zipfn));
            close(G.zipfd);
            return (error_in_archive > PK_WARN)? error_in_archive : PK_WARN;
        }

    /*-----------------------------------------------------------------------
        Compensate for missing or extra bytes, and seek to where the start
        of central directory should be.  If header not found, uncompensate
        and try again (necessary for at least some Atari archives created
        with STZip, as well as archives created by J.H. Holm's ZIPSPLIT 1.1).
      -----------------------------------------------------------------------*/

        ZLSEEK( G.ecrec.offset_start_central_directory )
#ifdef OLD_SEEK_TEST
        if (readbuf(G.sig, 4) == 0) {
            close(G.zipfd);
            return PK_ERR;  /* file may be locked, or possibly disk error(?) */
        }
        if (strncmp(G.sig, G.central_hdr_sig, 4))
#else
        if ((readbuf(__G__ G.sig, 4) == 0) || strncmp(G.sig,
            G.central_hdr_sig, 4))
#endif
        {
            long tmp = G.extra_bytes;

            G.extra_bytes = 0;
            ZLSEEK( G.ecrec.offset_start_central_directory )
            if ((readbuf(__G__ G.sig, 4) == 0) ||
                strncmp(G.sig, G.central_hdr_sig, 4))
            {
                Info(slide, 0x401, ((char *)slide,
                  LoadFarString(CentDirStartNotFound), G.zipfn,
                  LoadFarStringSmall(ReportMsg)));
                close(G.zipfd);
                return PK_BADERR;
            }
            Info(slide, 0x401, ((char *)slide, LoadFarString(CentDirTooLong),
              G.zipfn, -tmp));
            error_in_archive = PK_ERR;
        }

    /*-----------------------------------------------------------------------
        Seek to the start of the central directory one last time, since we
        have just read the first entry's signature bytes; then list, extract
        or test member files as instructed, and close the zipfile.
      -----------------------------------------------------------------------*/

        Trace((stderr, "about to extract/list files (error = %d)\n",
          error_in_archive));

        ZLSEEK( G.ecrec.offset_start_central_directory )

#ifndef NO_ZIPINFO
        if (G.zipinfo_mode) {
            error = zipinfo(__G);                     /* ZIPINFO 'EM */
            if (G.lflag > 9)
                (*G.message)((zvoid *)&G, (uch *)"\n", 1L, 0);
        } else
#endif
#ifndef SFX
#ifdef TIMESTAMP
            if (G.T_flag)
                error = time_stamp(__G);              /* TIME-STAMP 'EM */
            else
#endif
            if (G.vflag && !G.tflag && !G.cflag)
                error = list_files(__G);              /* LIST 'EM */
            else
#endif
                error = extract_or_test_files(__G);   /* EXTRACT OR TEST 'EM */

        Trace((stderr, "done with extract/list files (error = %d)\n", error));

        if (error > error_in_archive)   /* don't overwrite stronger error */
            error_in_archive = error;   /*  with (for example) a warning */
#ifndef SFX
    } /* end if (!too_weird_to_continue) */
#endif

    close(G.zipfd);
    return error_in_archive;

} /* end function do_seekable() */





/*************************/
/* Function find_ecrec() */
/*************************/

static int find_ecrec(__G__ searchlen)          /* return PK-class error */
    long searchlen;
     __GDEF
{
    int i, numblks, found=FALSE;
    LONGINT tail_len;
    ec_byte_rec byterec;


/*---------------------------------------------------------------------------
    Treat case of short zipfile separately.
  ---------------------------------------------------------------------------*/

    if (G.ziplen <= INBUFSIZ) {
        lseek(G.zipfd, 0L, SEEK_SET);
        if ((G.incnt = read(G.zipfd,(char *)G.inbuf,(unsigned int)G.ziplen))
            == (int)G.ziplen)

            /* 'P' must be at least 22 bytes from end of zipfile */
            for (G.inptr = G.inbuf+(int)G.ziplen-22;  G.inptr >= G.inbuf;
                 --G.inptr)
                if ((native(*G.inptr) == 'P')  &&
                     !strncmp((char *)G.inptr, G.end_central_sig, 4)) {
                    G.incnt -= (int)(G.inptr - G.inbuf);
                    found = TRUE;
                    break;
                }

/*---------------------------------------------------------------------------
    Zipfile is longer than INBUFSIZ:  may need to loop.  Start with short
    block at end of zipfile (if not TOO short).
  ---------------------------------------------------------------------------*/

    } else {
        if ((tail_len = G.ziplen % INBUFSIZ) > ECREC_SIZE) {
#ifdef USE_STRM_INPUT
            fseek((FILE *)G.zipfd, G.ziplen-tail_len, SEEK_SET);
            G.cur_zipfile_bufstart = ftell((FILE *)G.zipfd);
#else /* !USE_STRM_INPUT */
            G.cur_zipfile_bufstart = lseek(G.zipfd, G.ziplen-tail_len,
              SEEK_SET);
#endif /* ?USE_STRM_INPUT */
            if ((G.incnt = read(G.zipfd, (char *)G.inbuf,
                (unsigned int)tail_len)) != (int)tail_len)
                goto fail;      /* it's expedient... */

            /* 'P' must be at least 22 bytes from end of zipfile */
            for (G.inptr = G.inbuf+(int)tail_len-22;  G.inptr >= G.inbuf;
                 --G.inptr)
                if ((native(*G.inptr) == 'P')  &&
                     !strncmp((char *)G.inptr, G.end_central_sig, 4)) {
                    G.incnt -= (int)(G.inptr - G.inbuf);
                    found = TRUE;
                    break;
                }
            /* sig may span block boundary: */
            strncpy((char *)G.hold, (char *)G.inbuf, 3);
        } else
            G.cur_zipfile_bufstart = G.ziplen - tail_len;

    /*-----------------------------------------------------------------------
        Loop through blocks of zipfile data, starting at the end and going
        toward the beginning.  In general, need not check whole zipfile for
        signature, but may want to do so if testing.
      -----------------------------------------------------------------------*/

        numblks = (int)((searchlen - tail_len + (INBUFSIZ-1)) / INBUFSIZ);
        /*               ==amount=   ==done==   ==rounding==    =blksiz=  */

        for (i = 1;  !found && (i <= numblks);  ++i) {
            G.cur_zipfile_bufstart -= INBUFSIZ;
            lseek(G.zipfd, G.cur_zipfile_bufstart, SEEK_SET);
            if ((G.incnt = read(G.zipfd,(char *)G.inbuf,INBUFSIZ))
                != INBUFSIZ)
                break;          /* fall through and fail */

            for (G.inptr = G.inbuf+INBUFSIZ-1;  G.inptr >= G.inbuf;
                 --G.inptr)
                if ((native(*G.inptr) == 'P')  &&
                     !strncmp((char *)G.inptr, G.end_central_sig, 4)) {
                    G.incnt -= (int)(G.inptr - G.inbuf);
                    found = TRUE;
                    break;
                }
            /* sig may span block boundary: */
            strncpy((char *)G.hold, (char *)G.inbuf, 3);
        }
    } /* end if (ziplen > INBUFSIZ) */

/*---------------------------------------------------------------------------
    Searched through whole region where signature should be without finding
    it.  Print informational message and die a horrible death.
  ---------------------------------------------------------------------------*/

fail:
    if (!found) {
#ifdef MSWIN
        MessageBeep(1);
#endif
        if (G.qflag || G.zipinfo_mode)
            Info(slide, 0x401, ((char *)slide, "[%s]\n", G.zipfn));
        Info(slide, 0x401, ((char *)slide,
          LoadFarString(CentDirEndSigNotFound)));
        return PK_ERR;   /* failed */
    }

/*---------------------------------------------------------------------------
    Found the signature, so get the end-central data before returning.  Do
    any necessary machine-type conversions (byte ordering, structure padding
    compensation) by reading data into character array and copying to struct.
  ---------------------------------------------------------------------------*/

    G.real_ecrec_offset = G.cur_zipfile_bufstart + (G.inptr-G.inbuf);
#ifdef TEST
    printf("\n  found end-of-central-dir signature at offset %ld (%.8lXh)\n",
      G.real_ecrec_offset, G.real_ecrec_offset);
    printf("    from beginning of file; offset %d (%.4Xh) within block\n",
      G.inptr-G.inbuf, G.inptr-G.inbuf);
#endif

    if (readbuf(__G__ (char *)byterec, ECREC_SIZE+4) == 0)
        return PK_EOF;

    G.ecrec.number_this_disk =
      makeword(&byterec[NUMBER_THIS_DISK]);
    G.ecrec.num_disk_start_cdir =
      makeword(&byterec[NUM_DISK_WITH_START_CENTRAL_DIR]);
    G.ecrec.num_entries_centrl_dir_ths_disk =
      makeword(&byterec[NUM_ENTRIES_CENTRL_DIR_THS_DISK]);
    G.ecrec.total_entries_central_dir =
      makeword(&byterec[TOTAL_ENTRIES_CENTRAL_DIR]);
    G.ecrec.size_central_directory =
      makelong(&byterec[SIZE_CENTRAL_DIRECTORY]);
    G.ecrec.offset_start_central_directory =
      makelong(&byterec[OFFSET_START_CENTRAL_DIRECTORY]);
    G.ecrec.zipfile_comment_length =
      makeword(&byterec[ZIPFILE_COMMENT_LENGTH]);

    G.expect_ecrec_offset = G.ecrec.offset_start_central_directory +
                          G.ecrec.size_central_directory;
    return PK_COOL;

} /* end function find_ecrec() */





/*****************************/
/* Function uz_end_central() */
/*****************************/

int uz_end_central(__G)    /* return PK-type error code */
     __GDEF
{
    int error = PK_COOL;


/*---------------------------------------------------------------------------
    Get the zipfile comment (up to 64KB long), if any, and print it out.
    Then position the file pointer to the beginning of the central directory
    and fill buffer.
  ---------------------------------------------------------------------------*/

#ifdef MSWIN
    cchComment = G.ecrec.zipfile_comment_length; /* save for comment button */
    if (G.ecrec.zipfile_comment_length && (G.zflag > 0))
#else
    if (G.ecrec.zipfile_comment_length && (G.zflag > 0 ||
        (G.zflag == 0 &&
#ifdef TIMESTAMP
                         !G.T_flag &&
#endif
                                      !G.qflag)))
#endif /* ?MSWIN */
    {
        if (do_string(__G__ G.ecrec.zipfile_comment_length,DISPLAY)) {
            Info(slide, 0x401, ((char *)slide,
              LoadFarString(ZipfileCommTrunc1)));
            error = PK_WARN;
        }
    }
    return error;

} /* end function uz_end_central() */





/************************************/
/* Function process_cdir_file_hdr() */
/************************************/

int process_cdir_file_hdr(__G)    /* return PK-type error code */
     __GDEF
{
    int error;


/*---------------------------------------------------------------------------
    Get central directory info, save host and method numbers, and set flag
    for lowercase conversion of filename, depending on the OS from which the
    file is coming.
  ---------------------------------------------------------------------------*/

    if ((error = get_cdir_ent(__G)) != 0)
        return error;

    G.pInfo->hostnum = MIN(G.crec.version_made_by[1], NUM_HOSTS);
/*  extnum = MIN(crec.version_needed_to_extract[1], NUM_HOSTS); */

    G.pInfo->lcflag = 0;
    if (G.L_flag)             /* user specified case-conversion */
        switch (G.pInfo->hostnum) {
            case FS_FAT_:     /* PKZIP and zip -k store in uppercase */
            case CPM_:        /* like MS-DOS, right? */
            case VM_CMS_:     /* all caps? */
            case MVS_:        /* all caps? */
            case TOPS20_:
            case VMS_:        /* our Zip uses lowercase, but ASi's doesn't */
        /*  case Z_SYSTEM_:   ? */
        /*  case QDOS_:       ? */
                G.pInfo->lcflag = 1;   /* convert filename to lowercase */
                break;

            default:     /* AMIGA_, FS_HPFS_, FS_NTFS_, MAC_, UNIX_, ATARI_, */
                break;   /*  FS_VFAT_, BEBOX_ (Z_SYSTEM_):  no conversion */
        }

    /* do Amigas (AMIGA_) also have volume labels? */
    if (IS_VOLID(G.crec.external_file_attributes) &&
        (G.pInfo->hostnum == FS_FAT_ || G.pInfo->hostnum == FS_HPFS_ ||
         G.pInfo->hostnum == FS_NTFS_ || G.pInfo->hostnum == ATARI_))
    {
        G.pInfo->vollabel = TRUE;
        G.pInfo->lcflag = 0;        /* preserve case of volume labels */
    } else
        G.pInfo->vollabel = FALSE;

    return PK_COOL;

} /* end function process_cdir_file_hdr() */





/***************************/
/* Function get_cdir_ent() */
/***************************/

int get_cdir_ent(__G)   /* return PK-type error code */
     __GDEF
{
    cdir_byte_hdr byterec;


/*---------------------------------------------------------------------------
    Read the next central directory entry and do any necessary machine-type
    conversions (byte ordering, structure padding compensation--do so by
    copying the data from the array into which it was read (byterec) to the
    usable struct (crec)).
  ---------------------------------------------------------------------------*/

    if (readbuf(__G__ (char *)byterec, CREC_SIZE) == 0)
        return PK_EOF;

    G.crec.version_made_by[0] = byterec[C_VERSION_MADE_BY_0];
    G.crec.version_made_by[1] = byterec[C_VERSION_MADE_BY_1];
    G.crec.version_needed_to_extract[0] =
      byterec[C_VERSION_NEEDED_TO_EXTRACT_0];
    G.crec.version_needed_to_extract[1] =
      byterec[C_VERSION_NEEDED_TO_EXTRACT_1];

    G.crec.general_purpose_bit_flag =
      makeword(&byterec[C_GENERAL_PURPOSE_BIT_FLAG]);
    G.crec.compression_method =
      makeword(&byterec[C_COMPRESSION_METHOD]);
    G.crec.last_mod_file_time =
      makeword(&byterec[C_LAST_MOD_FILE_TIME]);
    G.crec.last_mod_file_date =
      makeword(&byterec[C_LAST_MOD_FILE_DATE]);
    G.crec.crc32 =
      makelong(&byterec[C_CRC32]);
    G.crec.csize =
      makelong(&byterec[C_COMPRESSED_SIZE]);
    G.crec.ucsize =
      makelong(&byterec[C_UNCOMPRESSED_SIZE]);
    G.crec.filename_length =
      makeword(&byterec[C_FILENAME_LENGTH]);
    G.crec.extra_field_length =
      makeword(&byterec[C_EXTRA_FIELD_LENGTH]);
    G.crec.file_comment_length =
      makeword(&byterec[C_FILE_COMMENT_LENGTH]);
    G.crec.disk_number_start =
      makeword(&byterec[C_DISK_NUMBER_START]);
    G.crec.internal_file_attributes =
      makeword(&byterec[C_INTERNAL_FILE_ATTRIBUTES]);
    G.crec.external_file_attributes =
      makelong(&byterec[C_EXTERNAL_FILE_ATTRIBUTES]);  /* LONG, not word! */
    G.crec.relative_offset_local_header =
      makelong(&byterec[C_RELATIVE_OFFSET_LOCAL_HEADER]);

    return PK_COOL;

} /* end function get_cdir_ent() */





/*************************************/
/* Function process_local_file_hdr() */
/*************************************/

int process_local_file_hdr(__G)    /* return PK-type error code */
     __GDEF
{
    local_byte_hdr byterec;


/*---------------------------------------------------------------------------
    Read the next local file header and do any necessary machine-type con-
    versions (byte ordering, structure padding compensation--do so by copy-
    ing the data from the array into which it was read (byterec) to the
    usable struct (lrec)).
  ---------------------------------------------------------------------------*/

    if (readbuf(__G__ (char *)byterec, LREC_SIZE) == 0)
        return PK_EOF;

    G.lrec.version_needed_to_extract[0] =
      byterec[L_VERSION_NEEDED_TO_EXTRACT_0];
    G.lrec.version_needed_to_extract[1] =
      byterec[L_VERSION_NEEDED_TO_EXTRACT_1];

    G.lrec.general_purpose_bit_flag =
      makeword(&byterec[L_GENERAL_PURPOSE_BIT_FLAG]);
    G.lrec.compression_method = makeword(&byterec[L_COMPRESSION_METHOD]);
    G.lrec.last_mod_file_time = makeword(&byterec[L_LAST_MOD_FILE_TIME]);
    G.lrec.last_mod_file_date = makeword(&byterec[L_LAST_MOD_FILE_DATE]);
    G.lrec.crc32 = makelong(&byterec[L_CRC32]);
    G.lrec.csize = makelong(&byterec[L_COMPRESSED_SIZE]);
    G.lrec.ucsize = makelong(&byterec[L_UNCOMPRESSED_SIZE]);
    G.lrec.filename_length = makeword(&byterec[L_FILENAME_LENGTH]);
    G.lrec.extra_field_length = makeword(&byterec[L_EXTRA_FIELD_LENGTH]);

    G.csize = (long) G.lrec.csize;
    G.ucsize = (long) G.lrec.ucsize;

    if ((G.lrec.general_purpose_bit_flag & 8) != 0) {
        /* can't trust local header, use central directory: */
        G.lrec.crc32 = G.pInfo->crc;
        G.csize = (long)(G.lrec.csize = G.pInfo->compr_size);
    }

    return PK_COOL;

} /* end function process_local_file_hdr() */


#ifdef USE_EF_UX_TIME

/*************************************/
/* Function ef_scan_for_izux() */
/*************************************/

unsigned ef_scan_for_izux(ef_buf, ef_len, z_utim, z_uidgid)
    uch *ef_buf;                /* buffer containing extra field */
    unsigned ef_len;            /* total length of extra field */
    ztimbuf *z_utim;            /* return storage: atime and mtime */
    ush *z_uidgid;              /* return storage: uid and gid */
/* This function scans the extra field for a IZUNIX block containing
 * Unix style time_t (GMT) values for the entry's access and modification
 * time.  If a valid block is found, both time stamps are copied to the
 * ztimebuf structure (provided the z_utim pointer is not NULL).
 * If the IZUNIX block contains UID/GID fields and the z_uidgid array
 * pointer is valid (!= NULL), the owner info is transfered as well.
 * The return value is the size of the IZUNIX block found, or 0 in case
 * of failure.
 */
{
  unsigned r = 0;
  unsigned eb_id;
  unsigned eb_len;

  if (ef_len == 0 || ef_buf == NULL)
    return 0;

  TTrace((stderr,"\nef_scan_ux_: scanning extra field of length %u\n",
          ef_len));
  while (ef_len >= EB_HEADSIZE) {
    eb_id = makeword(EB_ID + ef_buf);
    eb_len = makeword(EB_LEN + ef_buf);

    if (eb_len > (ef_len - EB_HEADSIZE)) {
      /* Discovered some extra field inconsistency! */
      TTrace((stderr,"ef_scan_for_izux: block length %u > rest ef_size %u\n",
              eb_len, ef_len - EB_HEADSIZE));
      break;
    }

    if (eb_id == EF_IZUNIX && eb_len >= EB_UX_MINLEN) {
       TTrace((stderr,"ef_scan_ux_time: Found IZUNIX extra field\n"));
       if (z_utim != NULL) {
         z_utim->actime  = makelong((EB_HEADSIZE+EB_UX_ATIME) + ef_buf);
         z_utim->modtime = makelong((EB_HEADSIZE+EB_UX_MTIME) + ef_buf);
         TTrace((stderr,"  Unix EF access time = %ld\n",z_utim->actime));
         TTrace((stderr,"  Unix EF modif. time = %ld\n",z_utim->modtime));
       }
       if (eb_len >= EB_UX_FULLSIZE && z_uidgid != NULL) {
         z_uidgid[0] = makeword((EB_HEADSIZE+EB_UX_UID) + ef_buf);
         z_uidgid[1] = makeword((EB_HEADSIZE+EB_UX_GID) + ef_buf);
       }
       r = eb_len;              /* signal success */
       break;
    }
    /* Skip this extra field block */
    ef_buf += (eb_len + EB_HEADSIZE);
    ef_len -= (eb_len + EB_HEADSIZE);
  }

  return r;
}

#endif /* USE_EF_UX_TIME */
