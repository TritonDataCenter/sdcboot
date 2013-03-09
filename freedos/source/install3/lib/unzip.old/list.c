/*---------------------------------------------------------------------------

  list.c

  This file contains the non-ZipInfo-specific listing routines for UnZip.

  Contains:  list_files()
             ratio()
             fnprint()

  ---------------------------------------------------------------------------*/


#define UNZIP_INTERNAL
#include "unzip.h"
#ifdef MSWIN
#  include <windows.h>
#  include "wingui\wizunzip.h"
#endif


/* Headers also referenced in UpdateListBox() in updatelb.c (Windows version) */

#ifdef OS2_EAS
   char Far HeadersS[]  = " Length    EAs   ACLs    Date    Time    Name";
   char Far HeadersS1[] = " ------    ---   ----    ----    ----    ----";
#else
   char Far HeadersS[]  = "";
   char Far HeadersS1[] = "";
#endif

char Far HeadersL[]  =
  "";
char Far HeadersL1[] =
  "";

char Far *Headers[][2] = { {HeadersS, HeadersS1}, {HeadersL, HeadersL1} };

static char Far CompFactorStr[] = "";
static char Far CompFactor100[] = "";
#ifdef MSWIN
   static char Far LongHdrStats[] =
     "%7lu  %-7s%7lu %4s  %02u-%02u-%02u  %02u:%02u  %08lx  %c%s";
   static char Far LongFileTrailer[] =
     "%7lu         %7lu %4s                              %u file%s";
   static char Far ShortHdrStats[] = "%7lu  %02u-%02u-%02u  %02u:%02u  %c%s";
   static char Far ShortFileTrailer[] = "%7lu                    %u file%s";
#else /* !MSWIN */
   static char Far CaseConversion[] = "";
   static char Far LongHdrStats[] =
     "";
   static char Far LongFileTrailer[] =
     "";
#ifdef OS2_EAS
   static char Far ShortHdrStats[] = "%7lu %6lu %6lu  %02u-%02u-%02u  %02u:%02u  %c";
   static char Far ShortFileTrailer[] = " ------  -----  -----        \
            -------\n%7lu %6lu %6lu                    %u file%s\n";
   static char Far OS2ExtAttrTrailer[] =
      "%ld file%s %ld bytes of OS/2 extended attributes attached.\n";
   static char Far OS2ACLTrailer[] =
      "%ld file%s %ld bytes of access control lists attached.\n";
#else
   static char Far ShortHdrStats[] = "";
   static char Far ShortFileTrailer[] =
    "";
#endif
#endif /* ?MSWIN */





/*************************/
/* Function list_files() */
/*************************/

int list_files(__G)    /* return PK-type error code */
    __GDEF
{
    char sgn, cfactorstr[10];
    int do_this_file=FALSE, cfactor, error, error_in_archive=PK_COOL;
    int longhdr=(G.vflag>1), date_format;
    unsigned methnum;
#ifdef USE_EF_UX_TIME
    ztimbuf z_utime;
#endif
    ush j, yr, mo, dy, hh, mm, members=0;
    ulg csiz, tot_csize=0L, tot_ucsize=0L;
#ifdef OS2_EAS
    ulg ea_size, tot_easize=0L, tot_eafiles=0L;
    ulg acl_size, tot_aclsize=0L, tot_aclfiles=0L;
#endif
#ifdef MSWIN
    PSTR psLBEntry;  /* list box entry */
#endif
    min_info info;
    char methbuf[8];
    static char dtype[]="NXFS";   /* see zi_short() */
    static char Far method[NUM_METHODS+1][8] =
        {"Stored", "Shrunk", "Reduce1", "Reduce2", "Reduce3", "Reduce4",
         "Implode", "Token", "Defl:#", "Unk:###"};



/*---------------------------------------------------------------------------
    Unlike extract_or_test_files(), this routine confines itself to the cen-
    tral directory.  Thus its structure is somewhat simpler, since we can do
    just a single loop through the entire directory, listing files as we go.

    So to start off, print the heading line and then begin main loop through
    the central directory.  The results will look vaguely like the following:

  Length  Method   Size  Ratio   Date    Time   CRC-32     Name ("^" ==> case
  ------  ------   ----  -----   ----    ----   ------     ----   conversion)
   44004  Implode  13041  71%  11-02-89  19:34  8b4207f7   Makefile.UNIX
    3438  Shrunk    2209  36%  09-15-90  14:07  a2394fd8  ^dos-file.ext
  ---------------------------------------------------------------------------*/

    G.pInfo = &info;
    date_format = DATE_FORMAT;

#ifndef MSWIN
    if (G.qflag < 2)
        if (G.L_flag)
            Info(slide, 0, ((char *)slide, LoadFarString(CaseConversion),
              LoadFarStringSmall(Headers[longhdr][0]),
              LoadFarStringSmall2(Headers[longhdr][1])));
        else
            Info(slide, 0, ((char *)slide, "%s\n%s\n",
               LoadFarString(Headers[longhdr][0]),
               LoadFarStringSmall(Headers[longhdr][1])));
#endif /* !MSWIN */

    for (j = 0; j < G.ecrec.total_entries_central_dir; ++j) {

        if (readbuf(__G__ G.sig, 4) == 0)
            return PK_EOF;
        if (strncmp(G.sig, G.central_hdr_sig, 4)) {  /* just to make sure */
            Info(slide, 0x401, ((char *)slide, LoadFarString(CentSigMsg), j));
            Info(slide, 0x401, ((char *)slide, LoadFarString(ReportMsg)));
            return PK_BADERR;
        }
        /* process_cdir_file_hdr() sets pInfo->lcflag: */
        if ((error = process_cdir_file_hdr(__G)) != PK_COOL)
            return error;       /* only PK_EOF defined */

        /*
         * We could DISPLAY the filename instead of storing (and possibly trun-
         * cating, in the case of a very long name) and printing it, but that
         * has the disadvantage of not allowing case conversion--and it's nice
         * to be able to see in the listing precisely how you have to type each
         * filename in order for unzip to consider it a match.  Speaking of
         * which, if member names were specified on the command line, check in
         * with match() to see if the current file is one of them, and make a
         * note of it if it is.
         */

        if ((error = do_string(__G__ G.crec.filename_length, DS_FN)) !=
             PK_COOL)   /*  ^--(uses pInfo->lcflag) */
        {
            error_in_archive = error;
            if (error > PK_WARN)   /* fatal:  can't continue */
                return error;
        }
        if (G.extra_field != (uch *)NULL) {
            free(G.extra_field);
            G.extra_field = (uch *)NULL;
        }
        if ((error = do_string(__G__ G.crec.extra_field_length, EXTRA_FIELD))
            != 0)
        {
            error_in_archive = error;
            if (error > PK_WARN)      /* fatal */
                return error;
        }
        if (!G.process_all_files) {   /* check if specified on command line */
            char **pfn = G.pfnames-1;

            do_this_file = FALSE;
            while (*++pfn)
                if (match(G.filename, *pfn, G.C_flag)) {
                    do_this_file = TRUE;
                    break;       /* found match, so stop looping */
                }
            if (do_this_file) {  /* check if this is an excluded file */
                char **pxn = G.pxnames-1;

                while (*++pxn)
                    if (match(G.filename, *pxn, G.C_flag)) {
                        do_this_file = FALSE;  /* ^-- ignore case in match */
                        break;
                    }
            }
        }
        /*
         * If current file was specified on command line, or if no names were
         * specified, do the listing for this file.  Otherwise, get rid of the
         * file comment and go back for the next file.
         */

        if (G.process_all_files || do_this_file) {

#ifdef OS2API  /* OS2DLL */
            /* this is used by UzpFileTree() to allow easy processing of lists
             * of zip directory contents */
            if (G.processExternally)
                if ((G.processExternally)(G.filename, &G.crec))
                    break;
                else
                    continue;
#endif
#ifdef OS2_EAS
            {
                uch *ef_ptr = G.extra_field;
                int ef_size, ef_len = G.crec.extra_field_length;
                ea_size = acl_size = 0;

                while (ef_len >= EB_HEADSIZE) {
                    ef_size = makeword(&ef_ptr[EB_LEN]);
                    switch (makeword(&ef_ptr[EB_ID])) {
                        case EF_OS2:
                            ea_size = makelong(&ef_ptr[EB_HEADSIZE]);
                            break;
                        case EF_ACL:
                            acl_size = makelong(&ef_ptr[EB_HEADSIZE]);
                            break;
                    }
                    ef_ptr += (ef_size + EB_HEADSIZE);
                    ef_len -= (ef_size + EB_HEADSIZE);
                }
            }
#endif
#ifdef USE_EF_UX_TIME
            if (G.extra_field &&
                ef_scan_for_izux(G.extra_field, G.crec.extra_field_length,
                                 &z_utime, NULL) > 0) {
                struct tm *t = localtime(&(z_utime.modtime));
                switch (date_format) {
                    case DF_YMD:
                        mo = (ush)(t->tm_year);
                        dy = (ush)(t->tm_mon + 1);
                        yr = (ush)(t->tm_mday);
                        break;
                    case DF_DMY:
                        mo = (ush)(t->tm_mday);
                        dy = (ush)(t->tm_mon + 1);
                        yr = (ush)(t->tm_year);
                        break;
                    default:
                        mo = (ush)(t->tm_mon + 1);
                        dy = (ush)(t->tm_mday);
                        yr = (ush)(t->tm_year);
                }
                hh = (ush)(t->tm_hour);
                mm = (ush)(t->tm_min);
            } else
#endif /* USE_EF_UX_TIME */
            {
                yr = (ush)((((G.crec.last_mod_file_date >> 9) & 0x7f) + 80) %
                           (unsigned)100);
                mo = (ush)((G.crec.last_mod_file_date >> 5) & 0x0f);
                dy = (ush)(G.crec.last_mod_file_date & 0x1f);

                /* permute date so it displays according to nat'l convention */
                switch (date_format) {
                    case DF_YMD:
                        hh = mo; mo = yr; yr = dy; dy = hh;
                        break;
                    case DF_DMY:
                        hh = mo; mo = dy; dy = hh;
                }

                hh = (ush)((G.crec.last_mod_file_time >> 11) & 0x1f);
                mm = (ush)((G.crec.last_mod_file_time >> 5) & 0x3f);
            }

            csiz = G.crec.csize;
            if (G.crec.general_purpose_bit_flag & 1)
                csiz -= 12;   /* if encrypted, don't count encryption header */
            if ((cfactor = ratio(G.crec.ucsize, csiz)) < 0) {
                sgn = '-';
                cfactor = (-cfactor + 5) / 10;
            } else {
                sgn = ' ';
                cfactor = (cfactor + 5) / 10;
            }
            if (cfactor == 100)
                sprintf(cfactorstr, LoadFarString(CompFactor100));
            else
                sprintf(cfactorstr, LoadFarString(CompFactorStr), sgn, cfactor);

            methnum = MIN(G.crec.compression_method, NUM_METHODS);
            zfstrcpy(methbuf, method[methnum]);
            if (methnum == DEFLATED) {
                methbuf[5] = dtype[(G.crec.general_purpose_bit_flag>>1) & 3];
            } else if (methnum >= NUM_METHODS) {
                sprintf(&methbuf[4], "%03u", G.crec.compression_method);
            }

#if 0       /* GRR/Euro:  add this? */
#if defined(DOS_OS2_W32) || defined(UNIX)
            for (p = G.filename;  *p;  ++p)
                if (!isprint(*p))
                    *p = '?';  /* change non-printable chars to '?' */
#endif /* DOS_OS2_W32 || UNIX */
#endif /* 0 */

#ifdef MSWIN
#ifdef NEED_EARLY_REDRAW
            /* turn on listbox redrawing just before adding last line */
            if (j == (G.ecrec.total_entries_central_dir-1))
                (void)SendMessage(hWndList, WM_SETREDRAW, TRUE, 0L);
#endif /* NEED_EARLY_REDRAW */
            psLBEntry =
              (PSTR)GlobalAlloc(GMEM_FIXED, FILNAMSIZ+LONG_FORM_FNAME_INX);
            /* GRR:  does OemToAnsi filter out escape and CR characters? */
            OemToAnsi(G.filename, G.filename);  /* translate to ANSI */
            if (longhdr) {
                wsprintf(psLBEntry, LoadFarString(LongHdrStats),
                  G.crec.ucsize, (LPSTR)methbuf, csiz, cfactorstr,
                  mo, dy, yr, hh, mm, G.crec.crc32,
                  (G.pInfo->lcflag? '^':' '), (LPSTR)G.filename);
                SendMessage(hWndList, LB_ADDSTRING, 0,
                  (LONG)(LPSTR)psLBEntry);
            } else {
                wsprintf(psLBEntry, LoadFarString(ShortHdrStats),
                  G.crec.ucsize, mo, dy, yr, hh, mm,
                  (G.pInfo->lcflag? '^':' '), (LPSTR)G.filename);
                SendMessage(hWndList, LB_ADDSTRING, 0,
                  (LONG)(LPSTR)psLBEntry);
            }
            GlobalFree((HANDLE)psLBEntry);
#else /* !MSWIN */
            if (longhdr)
                Info(slide, 0, ((char *)slide, LoadFarString(LongHdrStats),
                  G.crec.ucsize, methbuf, csiz, cfactorstr, mo, dy,
                  yr, hh, mm, G.crec.crc32, (G.pInfo->lcflag? '^':' ')));
            else
#ifdef OS2_EAS
                Info(slide, 0, ((char *)slide, LoadFarString(ShortHdrStats),
                  G.crec.ucsize, ea_size, acl_size,
                  mo, dy, yr, hh, mm, (G.pInfo->lcflag? '^':' ')));
#else
                Info(slide, 0, ((char *)slide, LoadFarString(ShortHdrStats),
                  G.crec.ucsize,
                  mo, dy, yr, hh, mm, (G.pInfo->lcflag? '^':' ')));
#endif
            fnprint(__G);
#ifdef WINDLL
            /* if this is the Windows 3.1 DLL, fill in our linked list with
             * the filenames (GRR:  also useful for OS/2 and other DLLs?) */
            {
                /* pointer to current ScFileList, used to move down chain;
                 * start at beginning of chain */
                struct ScFileList *pCurFileList = G.WinDLL.pGlobalZipFileList;

                /* move down the chain until we reach the end */
                while (pCurFileList->Next != NULL)
                    pCurFileList = pCurFileList->Next;

                /* if this is not the very first time through... */
                if (!(pCurFileList == G.WinDLL.pGlobalZipFileList &&
                      pCurFileList->Name == NULL))
                {
                    /* now that we are pointing at the last item in the chain,
                     * allocate space for a new one and add it */
                    pCurFileList->Next =
                      (struct ScFileList *)malloc(sizeof(struct ScFileList));
                    /* set the items in the next entry to NULL to mark end */
                    pCurFileList->Next->Name = NULL;
                    pCurFileList->Next->Next = NULL;

                    /* move up to the entry we just created */
                    pCurFileList = pCurFileList->Next;
                }

                /* allocate space for the name and copy the filename into it */
                pCurFileList->Name = (char *)malloc(strlen(G.filename)+1);
                strcpy(pCurFileList->Name, G.filename);
            }
#endif /* WINDLL */
#endif /* ?MSWIN */

            if ((error = do_string(__G__ G.crec.file_comment_length,
                                   QCOND? DISPLAY : SKIP)) != 0)
            {
                error_in_archive = error;  /* might be just warning */
                if (error > PK_WARN)       /* fatal */
                    return error;
            }
            tot_ucsize += G.crec.ucsize;
            tot_csize += csiz;
            ++members;
#ifdef OS2_EAS
            if (ea_size) {
                tot_easize += ea_size;
                ++tot_eafiles;
            }
            if (acl_size) {
                tot_aclsize += acl_size;
                ++tot_aclfiles;
            }
#endif
        } else {        /* not listing this file */
            SKIP_(G.crec.file_comment_length)
        }
    } /* end for-loop (j: files in central directory) */

/*---------------------------------------------------------------------------
    Print footer line and totals (compressed size, uncompressed size, number
    of members in zipfile).
  ---------------------------------------------------------------------------*/

    if (G.qflag < 2) {
        if ((cfactor = ratio(tot_ucsize, tot_csize)) < 0) {
            sgn = '-';
            cfactor = (-cfactor + 5) / 10;
        } else {
            sgn = ' ';
            cfactor = (cfactor + 5) / 10;
        }
        if (cfactor == 100)
            sprintf(cfactorstr, LoadFarString(CompFactor100));
        else
            sprintf(cfactorstr, LoadFarString(CompFactorStr), sgn, cfactor);
#ifdef MSWIN
        /* Display just the totals since the dashed lines get displayed
         * in UpdateListBox(). Get just enough space to display total. */
        if (longhdr)
            wsprintf(lpumb->szTotalsLine,LoadFarString(LongFileTrailer),
              tot_ucsize, tot_csize, cfactorstr, members, members == 1? "":"s");
        else
            wsprintf(lpumb->szTotalsLine, LoadFarString(ShortFileTrailer),
              tot_ucsize, members, members == 1? "" : "s");
#else /* !MSWIN */
        if (longhdr) {
            Info(slide, 0, ((char *)slide, LoadFarString(LongFileTrailer),
              tot_ucsize, tot_csize, cfactorstr, members, members==1? "":"s"));
#ifdef OS2_EAS
            if (tot_easize || tot_aclsize)
                Info(slide, 0, ((char *)slide, "\n"));
            if (tot_eafiles && tot_easize)
                Info(slide, 0, ((char *)slide, LoadFarString(OS2ExtAttrTrailer),
                  tot_eafiles, tot_eafiles == 1? " has" : "s have a total of",
                  tot_easize));
            if (tot_aclfiles && tot_aclsize)
                Info(slide, 0, ((char *)slide, LoadFarString(OS2ACLTrailer),
                  tot_aclfiles, tot_aclfiles == 1? " has" : "s have a total of",
                  tot_aclsize));
#endif /* OS2_EAs */
        } else
#ifdef OS2_EAS
            Info(slide, 0, ((char *)slide, LoadFarString(ShortFileTrailer),
              tot_ucsize, tot_easize, tot_aclsize, members, members == 1?
              "" : "s"));
#else
            Info(slide, 0, ((char *)slide, LoadFarString(ShortFileTrailer),
              tot_ucsize, members, members == 1? "" : "s"));
#endif /* OS2_EAs */
#endif /* ?MSWIN */
    }
/*---------------------------------------------------------------------------
    Double check that we're back at the end-of-central-directory record.
  ---------------------------------------------------------------------------*/

    if (readbuf(__G__ G.sig, 4) == 0)
        return PK_EOF;
    if (strncmp(G.sig, G.end_central_sig, 4)) {     /* just to make sure again */
        Info(slide, 0x401, ((char *)slide, LoadFarString(EndSigMsg)));
        error_in_archive = PK_WARN;
    }
    if (members == 0 && error_in_archive <= PK_WARN)
        error_in_archive = PK_FIND;

    return error_in_archive;

} /* end function list_files() */





#ifdef TIMESTAMP

/*************************/
/* Function time_stamp() */
/*************************/

int time_stamp(__G)    /* return PK-type error code */
    __GDEF
{
    int do_this_file=FALSE, error, error_in_archive=PK_COOL;
#if (defined(USE_EF_UX_TIME) || defined(UNIX))
    ztimbuf z_utime;
#endif
    time_t last_modtime=0L;   /* assuming no zipfile data older than 1970 */
    ush j, members=0;
    min_info info;


/*---------------------------------------------------------------------------
    Unlike extract_or_test_files() but like list_files(), this function works
    on information in the central directory alone.  Thus we have a single,
    large loop through the entire directory, searching for the latest time
    stamp.
  ---------------------------------------------------------------------------*/

    G.pInfo = &info;

    for (j = 0; j < G.ecrec.total_entries_central_dir; ++j) {

        if (readbuf(__G__ G.sig, 4) == 0)
            return PK_EOF;
        if (strncmp(G.sig, G.central_hdr_sig, 4)) {  /* just to make sure */
            Info(slide, 0x401, ((char *)slide, LoadFarString(CentSigMsg), j));
            Info(slide, 0x401, ((char *)slide, LoadFarString(ReportMsg)));
            return PK_BADERR;
        }
        /* process_cdir_file_hdr() sets pInfo->lcflag: */
        if ((error = process_cdir_file_hdr(__G)) != PK_COOL)
            return error;       /* only PK_EOF defined */
        if ((error = do_string(__G__ G.crec.filename_length, DS_FN)) != PK_OK)
        {        /*  ^-- (uses pInfo->lcflag) */
            error_in_archive = error;
            if (error > PK_WARN)   /* fatal:  can't continue */
                return error;
        }
        if (G.extra_field != (uch *)NULL) {
            free(G.extra_field);
            G.extra_field = (uch *)NULL;
        }
        if ((error = do_string(__G__ G.crec.extra_field_length, EXTRA_FIELD))
            != 0)
        {
            error_in_archive = error;
            if (error > PK_WARN)      /* fatal */
                return error;
        }
        if (!G.process_all_files) {   /* check if specified on command line */
            char **pfn = G.pfnames-1;

            do_this_file = FALSE;
            while (*++pfn)
                if (match(G.filename, *pfn, G.C_flag)) {
                    do_this_file = TRUE;
                    break;       /* found match, so stop looping */
                }
            if (do_this_file) {  /* check if this is an excluded file */
                char **pxn = G.pxnames-1;

                while (*++pxn)
                    if (match(G.filename, *pxn, G.C_flag)) {
                        do_this_file = FALSE;  /* ^-- ignore case in match */
                        break;
                    }
            }
        }

        /* If current file was specified on command line, or if no names were
         * specified, check the time for this file.  Either way, get rid of the
         * file comment and go back for the next file.
         */
        if (G.process_all_files || do_this_file) {
#ifdef USE_EF_UX_TIME
            if (G.extra_field && ef_scan_for_izux(G.extra_field,
                G.crec.extra_field_length, &z_utime, NULL) > 0)
            {
                if (last_modtime < z_utime.modtime)
                    last_modtime = z_utime.modtime;
            } else
#endif /* USE_EF_UX_TIME */
            {
                time_t modtime = dos_to_unix_time(G.crec.last_mod_file_date,
                                                  G.crec.last_mod_file_time);

                if (last_modtime < modtime)
                    last_modtime = modtime;
            }
            ++members;
        }
        SKIP_(G.crec.file_comment_length)

    } /* end for-loop (j: files in central directory) */

/*---------------------------------------------------------------------------
    Set the modification (and access) time on the zipfile, assuming we have
    a modification time to set.
  ---------------------------------------------------------------------------*/

    if (members > 0) {
        z_utime.modtime = z_utime.actime = last_modtime;
        if (utime(G.zipfn, &z_utime))
            Info(slide, 0x201, ((char *)slide,
              "warning:  can't set time for %s\n", G.zipfn));
    }

/*---------------------------------------------------------------------------
    Double check that we're back at the end-of-central-directory record.
  ---------------------------------------------------------------------------*/

    if (readbuf(__G__ G.sig, 4) == 0)
        return PK_EOF;
    if (strncmp(G.sig, G.end_central_sig, 4)) {     /* just to make sure again */
        Info(slide, 0x401, ((char *)slide, LoadFarString(EndSigMsg)));
        error_in_archive = PK_WARN;
    }
    if (members == 0 && error_in_archive <= PK_WARN)
        error_in_archive = PK_FIND;

    return error_in_archive;

} /* end function time_stamp() */

#endif /* TIMESTAMP */





/********************/
/* Function ratio() */    /* also used by ZipInfo routines */
/********************/

int ratio(uc, c)
    ulg uc, c;
{
    ulg denom;

    if (uc == 0)
        return 0;
    if (uc > 2000000L) {    /* risk signed overflow if multiply numerator */
        denom = uc / 1000L;
        return ((uc >= c) ?
            (int) ((uc-c + (denom>>1)) / denom) :
          -((int) ((c-uc + (denom>>1)) / denom)));
    } else {             /* ^^^^^^^^ rounding */
        denom = uc;
        return ((uc >= c) ?
            (int) ((1000L*(uc-c) + (denom>>1)) / denom) :
          -((int) ((1000L*(c-uc) + (denom>>1)) / denom)));
    }                            /* ^^^^^^^^ rounding */
}





/************************/
/*  Function fnprint()  */    /* also used by ZipInfo routines */
/************************/

void fnprint(__G)    /* print filename (after filtering) and newline */
    __GDEF
{
    char *name = fnfilter(G.filename, slide);

    (*G.message)((zvoid *)&G, (uch *)name, (ulg)strlen(name), 0);
    (*G.message)((zvoid *)&G, (uch *)"\n", 1L, 0);

} /* end function fnprint() */
