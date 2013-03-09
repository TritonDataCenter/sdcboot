/*---------------------------------------------------------------------------

  fileio.c

  This file contains routines for doing direct but relatively generic input/
  output, file-related sorts of things, plus some miscellaneous stuff.  Most
  of the stuff has to do with opening, closing, reading and/or writing files.

  Contains:  open_input_file()
             open_outfile()           (non-VMS, non-AOS/VS, non-CMS_MVS)
             undefer_input()
             defer_leftover_input()
             readbuf()
             readbyte()
             fillinbuf()
             flush()                  (non-VMS)
             disk_error()             (non-VMS)
             UzpMessagePrnt()
             UzpMessageNull()         (DLL only)
             UzpInput()
             UzpMorePause()
             handler()
             dos_to_unix_time()       (non-VMS, non-OS/2, non-VM/CMS, non-MVS)
             check_for_newer()        (non-VMS, non-OS/2, non-VM/CMS, non-MVS)
             do_string()
             makeword()
             makelong()
             memset()                 (ZMEM only)
             memcpy()                 (ZMEM only)
             zstrnicmp()
             zstat()                  (REGULUS only)
             fLoadFarString()         (SMALL_MEM only)
             fLoadFarStringSmall()    (SMALL_MEM only)
             fLoadFarStringSmall2()   (SMALL_MEM only)
             zfstrcpy()               (SMALL_MEM only)

  ---------------------------------------------------------------------------*/


#define FILEIO_C
#define UNZIP_INTERNAL
#include "unzip.h"
#include "crypt.h"
#include "ttyio.h"
#include "ebcdic.h"   /* definition/initialization of ebcdic[] */
#undef isatty
#define isatty(x) 0
#undef DLL


/*
   Note: Under Windows, the maximum size of the buffer that can be used
   with any of the *printf calls is 16,384, so win_fprintf was used to
   feed the fprintf clone no more than 16K chunks at a time. This should
   be valid for anything up to 64K (and probably beyond, assuming your
   buffers are that big.)
*/
#ifdef MSWIN
#  define WriteError(buf,len,strm) \
   ((extent)win_fprintf(strm, (extent)len, (char *)buf) != (extent)(len))
#else
#  ifdef USE_FWRITE
#    define WriteError(buf,len,strm) \
     ((extent)fwrite((char *)(buf),1,(extent)(len),strm) != (extent)(len))
#  else
#    define WriteError(buf,len,strm) \
     ((extent)write(fileno(strm),(char *)(buf),(extent)(len)) != (extent)(len))
#  endif
#endif /* ?MSWIN */

static int disk_error OF((__GPRO));

#ifdef MSWIN
   extern int win_fprintf(FILE *file, unsigned int, char *);
#endif



/****************************/
/* Strings used in fileio.c */
/****************************/

#ifdef UNIX
   static char Far CannotDeleteOldFile[] = "error:  cannot delete old %s\n";
#endif

static char Far CantOpenZipfile[] = "";
#if (!defined(VMS) && !defined(AOS_VS) && !defined(CMS_MVS))
   static char Far CannotCreateFile[] = "";
#endif
#ifdef NOVELL_BUG_FAILSAFE
   static char Far NovellBug[] =
     "";
#endif
static char Far ReadError[] = "";
static char Far DiskFullQuery[] =
  "";
//static char Far ZipfileCorrupt[] = "";
static char Far FilenameTooLongTrunc[] =
  "";
static char Far ExtraFieldTooLong[] =
  "";

#ifndef MSWIN
#  ifdef MORE
     static char Far MorePrompt[] = "";
#  endif
   static char Far QuitPrompt[] =
     "";
//   static char Far HidePrompt[] = /* "\r                       \r"; */
//     "\r                                                         \r";
#endif /* !MSWIN */





/******************************/
/* Function open_input_file() */
/******************************/

int open_input_file(__G)    /* return 1 if open failed */
    __GDEF
{
    /*
     *  open the zipfile for reading and in BINARY mode to prevent cr/lf
     *  translation, which would corrupt the bitstreams
     */

#if (defined(UNIX) || defined(TOPS20) || defined(AOS_VS))
    G.zipfd = open(G.zipfn, O_RDONLY);
#else /* !(UNIX || TOPS20 || AOS_VS) */
#ifdef VMS
    G.zipfd = open(G.zipfn, O_RDONLY, 0, "ctx=stm");
#else /* !VMS */
#ifdef MACOS
    G.zipfd = open(G.zipfn, 0);
#else /* !MACOS */
#ifdef RISCOS
    G.zipfd = fopen(G.zipfn, "rb");
#else /* !RISCOS */
#ifdef CMS_MVS
    G.zipfd = vmmvs_open_infile(__G);
#else /* !CMS_MVS */
    G.zipfd = open(G.zipfn, O_RDONLY | O_BINARY);
#endif /* ?CMS_MVS */
#endif /* ?RISCOS */
#endif /* ?MACOS */
#endif /* ?VMS */
#endif /* ?(UNIX || TOPS20 || AOS_VS) */

#ifdef USE_STRM_INPUT
    if (G.zipfd == NULL)
#else
    if (G.zipfd < 0)
#endif
    {
        Info(slide, 0x401, ((char *)slide, LoadFarString(CantOpenZipfile),
          G.zipfn));
        return 1;
    }
    return 0;

} /* end function open_input_file() */




#if (!defined(VMS) && !defined(AOS_VS) && !defined(CMS_MVS))

/***************************/
/* Function open_outfile() */
/***************************/

int open_outfile(__G)         /* return 1 if fail */
    __GDEF
{
#ifdef DLL
    if (G.redirect_data)
        return redirect_outfile(__G)==FALSE;
#endif
#ifdef DOS_OS2_W32
    if (stat(G.filename, &G.statbuf) == 0) {
        Trace((stderr, "open_outfile:  stat(%s) returns 0 (file exists)\n",
          G.filename));
        if (!(G.statbuf.st_mode & S_IWRITE)) {
            Trace((stderr, "open_outfile:  existing file %s is read-only\n",
              G.filename));
            chmod(G.filename, S_IREAD | S_IWRITE);
            Trace((stderr, "open_outfile:  %s now writable\n", G.filename));
        }
        if (unlink(G.filename) != 0)
            return 1;
        Trace((stderr, "open_outfile:  %s now deleted\n", G.filename));
    }
#endif /* DOS_OS2_W32 */
#ifdef UNIX
    if (stat(G.filename, &G.statbuf) == 0 && unlink(G.filename) < 0) {
        Info(slide, 0x401, ((char *)slide, LoadFarString(CannotDeleteOldFile),
          G.filename));
        return 1;
    }
#endif /* UNIX */
#ifdef RISCOS
    if (SWI_OS_File_7(G.filename,0xDEADDEAD,0xDEADDEAD,G.lrec.ucsize)!=NULL) {
        Info(slide, 1, ((char *)slide, LoadFarString(CannotCreateFile),
          G.filename));
        return 1;
    }
#endif /* RISCOS */
#ifdef TOPS20
    char *tfilnam;

    if ((tfilnam = (char *)malloc(2*strlen(G.filename)+1)) == (char *)NULL)
        return 1;
    strcpy(tfilnam, G.filename);
    upper(tfilnam);
    enquote(tfilnam);
    if ((G.outfile = fopen(tfilnam, FOPW)) == (FILE *)NULL) {
        Info(slide, 1, ((char *)slide, LoadFarString(CannotCreateFile),
          tfilnam));
        free(tfilnam);
        return 1;
    }
    free(tfilnam);
#else /* !TOPS20 */
#ifdef MTS
    if (G.aflag)
        G.outfile = fopen(G.filename, FOPWT);
    else
        G.outfile = fopen(G.filename, FOPW);
    if (G.outfile == (FILE *)NULL) {
        Info(slide, 1, ((char *)slide, LoadFarString(CannotCreateFile),
          G.filename));
        return 1;
    }
#else /* !MTS */
#ifdef DEBUG
    Info(slide, 1, ((char *)slide,
      "open_outfile:  doing fopen(%s) for reading\n", G.filename));
    if ((G.outfile = fopen(G.filename, FOPR)) == (FILE *)NULL)
        Info(slide, 1, ((char *)slide,
          "open_outfile:  fopen(%s) for reading failed:  does not exist\n",
          G.filename));
    else {
        Info(slide, 1, ((char *)slide,
          "open_outfile:  fopen(%s) for reading succeeded:  file exists\n",
          G.filename));
        fclose(G.outfile);
    }
#endif /* DEBUG */
#ifdef NOVELL_BUG_FAILSAFE
    if (G.dne && ((G.outfile = fopen(G.filename, FOPR)) != (FILE *)NULL)) {
        Info(slide, 0x401, ((char *)slide, LoadFarString(NovellBug),
          G.filename));
        fclose(G.outfile);
        return 1;   /* with "./" fix in checkdir(), should never reach here */
    }
#endif /* NOVELL_BUG_FAILSAFE */
    Trace((stderr, "open_outfile:  doing fopen(%s) for writing\n", G.filename));
    if ((G.outfile = fopen(G.filename, FOPW)) == (FILE *)NULL) {
        Info(slide, 0x401, ((char *)slide, LoadFarString(CannotCreateFile),
          G.filename));
        return 1;
    }
    Trace((stderr, "open_outfile:  fopen(%s) for writing succeeded\n",
      G.filename));
#endif /* !MTS */
#endif /* !TOPS20 */

#if 0      /* this SUCKS!  on Ultrix, it must be writing a byte at a time... */
    setbuf(G.outfile, (char *)NULL);   /* make output unbuffered */
#endif

#ifdef USE_FWRITE
#ifdef DOS_OS2_W32
    /* 16-bit MSC: buffer size must be strictly LESS than 32K (WSIZE):  bogus */
    setbuf(G.outfile, (char *)NULL);   /* make output unbuffered */
#else /* !DOS_OS2_W32 */
#ifndef RISCOS
#ifdef _IOFBF  /* make output fully buffered (works just about like write()) */
    setvbuf(G.outfile, (char *)slide, _IOFBF, WSIZE);
#else
    setbuf(G.outfile, (char *)slide);
#endif
#endif /* !RISCOS */
#endif /* ?DOS_OS2_W32 */
#endif /* USE_FWRITE */
    return 0;

} /* end function open_outfile() */

#endif /* !VMS && !AOS_VS && !CMS_MVS */





/*
 * These functions allow NEXTBYTE to function without needing two bounds
 * checks.  Call defer_leftover_input() if you ever have filled G.inbuf
 * by some means other than readbyte(), and you then want to start using
 * NEXTBYTE.  When going back to processing bytes without NEXTBYTE, call
 * undefer_input().  For example, extract_or_test_member brackets its
 * central section that does the decompression with these two functions.
 * If you need to check the number of bytes remaining in the current
 * file while using NEXTBYTE, check (G.csize + G.incnt), not G.csize.
 */

/****************************/
/* function undefer_input() */
/****************************/

void undefer_input(__G)
    __GDEF
{
    if (G.incnt > 0)
        G.csize += G.incnt;
    if (G.incnt_leftover > 0) {
        /* We know that "(G.csize < MAXINT)" so we can cast G.csize to int:
         * This condition was checked when G.incnt_leftover was set > 0 in
         * defer_leftover_input(), and it is NOT allowed to touch G.csize
         * before calling undefer_input() when (G.incnt_leftover > 0)
         * (single exception: see read_byte()'s  "G.csize <= 0" handling) !!
         */
        G.incnt = G.incnt_leftover + (int)G.csize;
        G.inptr = G.inptr_leftover - (int)G.csize;
        G.incnt_leftover = 0;
    } else if (G.incnt < 0)
        G.incnt = 0;
} /* end function undefer_input() */





/***********************************/
/* function defer_leftover_input() */
/***********************************/

void defer_leftover_input(__G)
    __GDEF
{
    if ((long)G.incnt > G.csize) {
        /* (G.csize < MAXINT), we can safely cast it to int !! */
        if (G.csize < 0L)
            G.csize = 0L;
        G.inptr_leftover = G.inptr + (int)G.csize;
        G.incnt_leftover = G.incnt - (int)G.csize;
        G.incnt = (int)G.csize;
    } else
        G.incnt_leftover = 0;
    G.csize -= G.incnt;
} /* end function defer_leftover_input() */





/**********************/
/* Function readbuf() */
/**********************/

unsigned readbuf(__G__ buf, size)   /* return number of bytes read into buf */
    __GDEF
    char *buf;
    register unsigned size;
{
    register unsigned count;
    unsigned n;

    n = size;
    while (size) {
        if (G.incnt <= 0) {
            if ((G.incnt = read(G.zipfd, (char *)G.inbuf, INBUFSIZ)) == 0)
                return (n-size);
            else if (G.incnt < 0) {
                /* another hack, but no real harm copying same thing twice */
                (*G.message)((zvoid *)&G,
                  (uch *)LoadFarString(ReadError),  /* CANNOT use slide */
                  (ulg)strlen(LoadFarString(ReadError)), 0x401);
                return 0;  /* discarding some data; better than lock-up */
            }
            /* buffer ALWAYS starts on a block boundary:  */
            G.cur_zipfile_bufstart += INBUFSIZ;
            G.inptr = G.inbuf;
        }
        count = MIN(size, (unsigned)G.incnt);
        memcpy(buf, G.inptr, count);
        buf += count;
        G.inptr += count;
        G.incnt -= count;
        size -= count;
    }
    return n;

} /* end function readbuf() */





/***********************/
/* Function readbyte() */
/***********************/

int readbyte(__G)   /* refill inbuf and return a byte if available, else EOF */
    __GDEF
{
    if (G.mem_mode)
        return EOF;
    if (G.csize <= 0) {
        G.csize--;             /* for tests done after exploding */
        G.incnt = 0;
        return EOF;
    }
    if (G.incnt <= 0) {
        if ((G.incnt = read(G.zipfd, (char *)G.inbuf, INBUFSIZ)) == 0) {
            G.incnt = 0;       /* do not allow negative value to affect stuff */
            return EOF;
        } else if (G.incnt < 0) {  /* "fail" (abort, retry, ...) returns this */
            /* another hack, but no real harm copying same thing twice */
            (*G.message)((zvoid *)&G,
              (uch *)LoadFarString(ReadError),
              (ulg)strlen(LoadFarString(ReadError)), 0x401);
            echon();
            DESTROYGLOBALS()
            exit(PK_BADERR);  /* totally bailing; better than lock-up */
        }
        G.cur_zipfile_bufstart += INBUFSIZ; /* always starts on block bndry */
        G.inptr = G.inbuf;
        defer_leftover_input(__G);           /* decrements G.csize */
    }

#ifdef CRYPT
    if (G.pInfo->encrypted) {
        uch *p;
        int n;

        /* This was previously set to decrypt one byte beyond G.csize, when
         * incnt reached that far.  GRR said, "but it's required:  why?"  This
         * was a bug in fillinbuf() -- was it also a bug here?
         */
        for (n = G.incnt, p = G.inptr;  n--;  p++)
            zdecode(*p);
    }
#endif /* CRYPT */

    --G.incnt;
    return *G.inptr++;

} /* end function readbyte() */





#ifdef USE_ZLIB

/************************/
/* Function fillinbuf() */
/************************/

int fillinbuf(__G) /* like readbyte() except returns number of bytes in inbuf */
    __GDEF
{
    if (G.mem_mode ||
                  (G.incnt = read(G.zipfd, (char *)G.inbuf, INBUFSIZ)) <= 0)
        return 0;
    G.cur_zipfile_bufstart += INBUFSIZ;   /* always starts on a block boundary */
    G.inptr = G.inbuf;
    defer_leftover_input(__G);           /* decrements G.csize */

#ifdef CRYPT
    if (G.pInfo->encrypted) {
        uch *p;
        int n;

        for (n = G.incnt, p = G.inptr;  n--;  p++)
            zdecode(*p);
    }
#endif /* CRYPT */

    return G.incnt;

} /* end function fillinbuf() */

#endif /* USE_ZLIB */





#ifndef VMS                 /* for VMS use code in vms.c */

/********************/
/* Function flush() */
/********************/

int flush(__G__ rawbuf, size, unshrink)   /* cflag => always 0; 50 if write error */
    __GDEF
    uch *rawbuf;
    ulg size;
    int unshrink;
{
    register uch *p, *q;
    uch *transbuf;
#if (defined(SMALL_MEM) || defined(MED_MEM))
    ulg transbufsiz;
#endif
    /* static int didCRlast = FALSE;    moved to globals.h */


/*---------------------------------------------------------------------------
    Compute the CRC first; if testing or if disk is full, that's it.
  ---------------------------------------------------------------------------*/

    G.crc32val = crc32(G.crc32val, rawbuf, (extent)size);

    if (G.tflag || size == 0L)   /* testing or nothing to write:  all done */
        return 0;

    if (G.disk_full)
        return 50;            /* disk already full:  ignore rest of file */

/*---------------------------------------------------------------------------
    Write the bytes rawbuf[0..size-1] to the output device, first converting
    end-of-lines and ASCII/EBCDIC as needed.  If SMALL_MEM or MED_MEM are NOT
    defined, outbuf is assumed to be at least as large as rawbuf and is not
    necessarily checked for overflow.
  ---------------------------------------------------------------------------*/

    if (!G.pInfo->textmode) {   /* write raw binary data */
        /* GRR:  note that for standard MS-DOS compilers, size argument to
         * fwrite() can never be more than 65534, so WriteError macro will
         * have to be rewritten if size can ever be that large.  For now,
         * never more than 32K.  Also note that write() returns an int, which
         * doesn't necessarily limit size to 32767 bytes if write() is used
         * on 16-bit systems but does make it more of a pain; however, because
         * at least MSC 5.1 has a lousy implementation of fwrite() (as does
         * DEC Ultrix cc), write() is used anyway.
         */
#ifdef DLL
        if (G.redirect_data)
            writeToMemory(__G__ rawbuf, (unsigned)(q-transbuf));
        else
#endif
        if (!G.cflag && WriteError(rawbuf, size, G.outfile))
            return disk_error(__G);
        else if (G.cflag && (*G.message)((zvoid *)&G, rawbuf, size, 0))
            return 0;
    } else {
        if (unshrink) {
            /* rawbuf = outbuf */
            transbuf = G.outbuf2;
#if (defined(SMALL_MEM) || defined(MED_MEM))
            transbufsiz = TRANSBUFSIZ;
#endif
        } else {
            /* rawbuf = slide */
            transbuf = G.outbuf;
#if (defined(SMALL_MEM) || defined(MED_MEM))
            transbufsiz = OUTBUFSIZ;
            Trace((stderr, "\ntransbufsiz = OUTBUFSIZ = %u\n", OUTBUFSIZ));
#endif
        }
        if (G.newfile) {
            G.didCRlast = FALSE;   /* no previous buffers written */
            G.newfile = FALSE;
        }
        p = rawbuf;
        if (*p == LF && G.didCRlast)
            ++p;

    /*-----------------------------------------------------------------------
        Algorithm:  CR/LF => native; lone CR => native; lone LF => native.
        This routine is only for non-raw-VMS, non-raw-VM/CMS files (i.e.,
        stream-oriented files, not record-oriented).
      -----------------------------------------------------------------------*/

        G.didCRlast = FALSE;
        for (q = transbuf;  p < rawbuf+(unsigned)size;  ++p) {
            if (*p == CR) {              /* lone CR or CR/LF: EOL either way */
                PutNativeEOL
                if (p == rawbuf+(unsigned)size-1)  /* last char in buffer */
                    G.didCRlast = TRUE;
                else if (p[1] == LF)     /* get rid of accompanying LF */
                    ++p;
            } else if (*p == LF)         /* lone LF */
                PutNativeEOL
            else
#ifndef DOS_OS2_W32
            if (*p != CTRLZ)             /* lose all ^Z's */
#endif
                *q++ = native(*p);

#if (defined(SMALL_MEM) || defined(MED_MEM))
# if (lenEOL == 1)   /* don't check unshrink:  both buffers small but equal */
            if (!unshrink)
# endif
                /* check for danger of buffer overflow and flush */
                if (q > transbuf+(unsigned)transbufsiz-lenEOL) {
                    Trace((stderr,
                      "p - rawbuf = %u   q-transbuf = %u   size = %lu\n",
                      (unsigned)(p-rawbuf), (unsigned)(q-transbuf), size));
                    if (!G.cflag &&
                        WriteError(transbuf, (unsigned)(q-transbuf),G.outfile))
                        return disk_error(__G);
                    else if (G.cflag && (*G.message)((zvoid *)&G, transbuf,
                             (ulg)(q-transbuf), 0))
                        return 0;
                    q = transbuf;
                    continue;
                }
#endif /* SMALL_MEM || MED_MEM */
        }

    /*-----------------------------------------------------------------------
        Done translating:  write whatever we've got to file (or screen).
      -----------------------------------------------------------------------*/

        Trace((stderr, "p - rawbuf = %u   q-transbuf = %u   size = %lu\n",
          (unsigned)(p-rawbuf), (unsigned)(q-transbuf), size));
        if (q > transbuf)
#ifdef DLL
            if (G.redirect_data)
                writeToMemory(__G__ transbuf, (unsigned)(q-transbuf));
            else
#endif
            if (!G.cflag && WriteError(transbuf, (unsigned)(q-transbuf),
                G.outfile))
                return disk_error(__G);
            else if (G.cflag && (*G.message)((zvoid *)&G, transbuf,
                (ulg)(q-transbuf), 0))
                return 0;
    }

    return 0;

} /* end function flush() */





/*************************/
/* Function disk_error() */
/*************************/

static int disk_error(__G)
    __GDEF
{
    /* OK to use slide[] here because this file is finished regardless */
    Info(slide, 0x481, ((char *)slide, LoadFarString(DiskFullQuery), G.filename));

#ifndef MSWIN
    fgets(G.answerbuf, 9, stdin);
    if (*G.answerbuf == 'y')   /* stop writing to this file */
        G.disk_full = 1;       /*  (outfile bad?), but new OK */
    else
#endif
        G.disk_full = 2;       /* no:  exit program */

    return PK_DISK;

} /* end function disk_error() */

#endif /* !VMS */





/*****************************/
/* Function UzpMessagePrnt() */
/*****************************/
#undef MORE
#undef DLL
int UzpMessagePrnt(pG, buf, size, flag)
    zvoid *pG;      /* globals struct:  always passed */
    uch *buf;       /* preformatted string to be printed */
    ulg size;       /* length of string (may include nulls) */
    int flag;       /* flag bits */
{
return 1;
#if 0
    /* Important NOTE:
     * The name of the first parameter of UzpMessagePrnt(), which passes
     * the struct Globals address, >>> MUST <<< be identical to the string
     * expansion of the __G__ macro in the REENTRANT case (see globals.h).
     * This name identity is mandatory for the LoadFarString() macro
     * (in the SMALL_MEM case) !!!
     */
    int error;
    uch *q=buf, *endbuf=buf+(unsigned)size;
#ifdef MORE
    uch *p=buf-1;
#endif
    FILE *outfp;


/*---------------------------------------------------------------------------
    These tests are here to allow fine-tuning of UnZip's output messages,
    but none of them will do anything without setting the appropriate bit
    in the flag argument of every Info() statement which is to be turned
    *off*.  That is, all messages are currently turned on for all ports.
    To turn off *all* messages, use the UzpMessageNull() function instead
    of this one.
  ---------------------------------------------------------------------------*/

#if (defined(OS2) && defined(DLL))
    if (MSG_NO_DLL2(flag))  /* if OS/2 DLL bit is set, do NOT print this msg */
        return 0;
#endif
#ifdef WINDLL
    if (MSG_NO_WDLL(flag))
        return 0;
#endif
#ifdef MSWIN
    if (MSG_NO_WGUI(flag))
        return 0;
#endif
/*
#ifdef ACORN_GUI
    if (MSG_NO_AGUI(flag))
        return 0;
#endif
 */
#ifdef DLL                    /* don't display message if data is redirected */
    if (((struct Globals *)pG)->redirect_data &&
        !((struct Globals *)pG)->redirect_text)
        return 0;
#endif

    if (MSG_STDERR(flag) && !((struct Globals *)pG)->tflag)
        outfp = (FILE *)stderr;
    else
        outfp = (FILE *)stdout;

#ifdef MPW
    if (MSG_MNEWLN(flag)) {   /* assumes writable buffer (e.g., slide[]) */
        *endbuf++ = '\n';     /*  with room for one more char at end of buf */
        ++size;               /*  (safe assumption:  only used for four */
    }                         /*  short queries in extract.c and fileio.c) */
#endif

    if (MSG_TNEWLN(flag)) {   /* again assumes writable buffer:  fragile... */
        if ((!size && !((struct Globals *)pG)->sol) ||
            (size && (endbuf[-1] != '\n')))
        {
            *endbuf++ = '\n';
            ++size;
        }
    }

    if (MSG_LNEWLN(flag) && !((struct Globals *)pG)->sol) {
        /* not at start of line:  want newline */
#ifdef DLL
        if (!((struct Globals *)pG)->redirect_text) {
#endif
            putc('\n', outfp);          /* "more" will be off by one...oops */
            fflush(outfp);
            if (MSG_STDERR(flag) && ((struct Globals *)pG)->tflag &&
                !isatty(1) && isatty(2))
            {
                /* error output from testing redirected:  also send to stderr */
                putc('\n', stderr);
                fflush(stderr);
            }
#ifdef DLL
        } else
           REDIRECTC('\n');
#endif
        ((struct Globals *)pG)->sol = TRUE;
    }

    /* put zipfile name, filename and/or error/warning keywords here */

#ifdef MORE
    /* room for --More-- and one line of overlap: */
    ((struct Globals *)pG)->height = SCREENLINES - 2;

    if (((struct Globals *)pG)->M_flag
#ifdef DLL
         && !((struct Globals *)pG)->redirect_text
#endif
                                                 )
    {
        while (++p < endbuf) {
            if (*p == '\n') {
                ++((struct Globals *)pG)->numlines;
                if (((struct Globals *)pG)->numlines %
                    ((struct Globals *)pG)->height == 0L)    /* GRR: fix */
                {
                    if ((error = WriteError(q, p-q+1, outfp)) != 0)
                        return error;
                    fflush(outfp);
                    ((struct Globals *)pG)->sol = TRUE;
                    q = p + 1;
                    (*((struct Globals *)pG)->mpause)((zvoid *)pG,
                      LoadFarString(MorePrompt), 1);
                }
            }
        } /* end while */
        size = (ulg)(p - q);   /* remaining text */
    }
#endif /* MORE */

    if (size) {
#ifdef DLL
        if (!((struct Globals *)pG)->redirect_text) {
#endif
            if ((error = WriteError(q, size, outfp)) != 0)
                return error;
            fflush(outfp);
            if (MSG_STDERR(flag) && ((struct Globals *)pG)->tflag &&
                !isatty(1) && isatty(2))
            {
                /* error output from testing redirected:  also send to stderr */
                if ((error = WriteError(q, size, stderr)) != 0)
                    return error;
                fflush(stderr);
            }
#ifdef DLL
        } else {
            REDIRECTPRINT(q, size);   /* GRR:  this is ugly:  hide with macro */
        }
#endif
        ((struct Globals *)pG)->sol = (endbuf[-1] == '\n');
    }
    return 0;
#endif
} /* end function UzpMessagePrnt() */




#ifdef DLL
#if 0
/*****************************/
/* Function UzpMessageNull() */  /* convenience routine for no output at all */
/*****************************/

int UzpMessageNull(pG, buf, size, flag)
    zvoid *pG;    /* globals struct:  always passed */
    uch *buf;     /* preformatted string to be printed */
    ulg size;     /* length of string (may include nulls) */
    int flag;     /* flag bits */
{
    return 0;

} /* end function UzpMessageNull() */

#endif
#endif /* DLL */





/***********************/
/* Function UzpInput() */   /* GRR:  this is a placeholder for now */
/***********************/

int UzpInput(pG, buf, size, flag)
    zvoid *pG;    /* globals struct:  always passed */
    uch *buf;     /* preformatted string to be printed */
    int *size;    /* (address of) size of buf and of returned string */
    int flag;     /* flag bits (bit 0: no echo) */
{
    *size = 0;
    return 0;

} /* end function UzpInput() */





#if (!defined(MSWIN) && !defined(MACOS))

/***************************/
/* Function UzpMorePause() */
/***************************/
#if 0
void UzpMorePause(pG, prompt, flag)
    zvoid *pG;            /* globals struct:  always passed */
    const char *prompt;   /* "--More--" prompt */
    int flag;             /* 0 = any char OK; 1 = accept only '\n', ' ', q */
{
    uch c;

/*---------------------------------------------------------------------------
    Print a prompt and wait for the user to press a key, then erase prompt
    if possible.
  ---------------------------------------------------------------------------*/

    if (!((struct Globals *)pG)->sol)
        fprintf(stderr, "\n");
    /* numlines may or may not be used: */
    fprintf(stderr, prompt, ((struct Globals *)pG)->numlines);
    fflush(stderr);
    if (flag & 1) {
        do {
            c = (uch)FGETCH(0);
        } while (c != '\r' && c != '\n' && c != ' ' && c != 'q' && c != 'Q');
    } else
        c = (uch)FGETCH(0);

    /* newline was not echoed, so cover up prompt line */
    fprintf(stderr, LoadFarString(HidePrompt));
    fflush(stderr);

    if (ToLower(c) == 'q') {
        DESTROYGLOBALS()
        EXIT(PK_COOL);
    }

    ((struct Globals *)pG)->sol = TRUE;

} /* end function UzpMorePause() */
#endif
#endif /* !MSWIN && !MACOS */





/**********************/
/* Function handler() */
/**********************/

#if 0
void handler(signal)   /* upon interrupt, turn on echo and exit cleanly */
    int signal;
{
    GETGLOBALS();

#if !(defined(SIGBUS) || defined(SIGSEGV))      /* add a newline if not at */
    (*G.message)((zvoid *)&G, slide, 0L, 0x41); /*  start of line (to stderr; */
#endif                                          /*  slide[] should be safe) */

    echon();

#ifdef SIGBUS
    if (signal == SIGBUS) {
        Info(slide, 0x421, ((char *)slide, LoadFarString(ZipfileCorrupt),
          "bus error"));
        DESTROYGLOBALS()
        EXIT(PK_BADERR);
    }
#endif /* SIGBUS */

#ifdef SIGSEGV
    if (signal == SIGSEGV) {
        Info(slide, 0x421, ((char *)slide, LoadFarString(ZipfileCorrupt),
          "segmentation violation"));
        DESTROYGLOBALS()
        EXIT(PK_BADERR);
    }
#endif /* SIGSEGV */

    /* probably ctrl-C */
    DESTROYGLOBALS()
    EXIT(PK_ERR);         /* was EXIT(0) */
}
#endif





#if (!defined(VMS) && !defined(OS2) && !defined(CMS_MVS))
#ifdef MSWIN
#  ifndef timezone
#    define timezone _timezone
#  endif
#endif

/*******************************/
/* Function dos_to_unix_time() */   /* only used for freshening/updating */
/*******************************/

time_t dos_to_unix_time(ddate, dtime)
    unsigned ddate, dtime;
{
    time_t m_time;
    int yr, mo, dy, hh, mm, ss;
#ifdef TOPS20
#   define YRBASE  1900
    struct tmx *tmx;
    char temp[20];
#else /* !TOPS20 */
#   define YRBASE  1970
    static short yday[]={0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334};
    int leap;
    unsigned days;
#if (!defined(MACOS) && !defined(RISCOS))
#ifndef BSD4_4   /* GRR:  change to !defined(MODERN) ? */
#if (defined(BSD) || defined(MTS) || defined(__GO32__))
    struct timeb tbp;
#else /* !(BSD || MTS || __GO32__) */
#ifdef DECLARE_TIMEZONE
    extern time_t timezone;
#endif
#endif /* ?(BSD || MTS || __GO32__) */
#endif /* !BSD4_4 */
#endif /* !MACOS && !RISCOS */
#endif /* ?TOPS20 */


    /* dissect date */
    yr = ((ddate >> 9) & 0x7f) + (1980 - YRBASE);
    mo = ((ddate >> 5) & 0x0f) - 1;
    dy = (ddate & 0x1f) - 1;

    /* dissect time */
    hh = (dtime >> 11) & 0x1f;
    mm = (dtime >> 5) & 0x3f;
    ss = (dtime & 0x1f) * 2;

#ifdef TOPS20
    tmx = (struct tmx *)malloc(sizeof(struct tmx));
    sprintf (temp, "%02d/%02d/%02d %02d:%02d:%02d", mo+1, dy+1, yr, hh, mm, ss);
    time_parse(temp, tmx, (char *)0);
    m_time = time_make(tmx);
    free(tmx);

#else /* !TOPS20 */
    /* leap = # of leap yrs from YRBASE up to but not including current year */
    leap = ((yr + YRBASE - 1) / 4);   /* leap year base factor */

    /* calculate days from BASE to this year and add expired days this year */
    days = (yr * 365) + (leap - 492) + yday[mo];

    /* if year is a leap year and month is after February, add another day */
    if ((mo > 1) && ((yr+YRBASE)%4 == 0) && ((yr+YRBASE) != 2100))
        ++days;                 /* OK through 2199 */

    /* convert date & time to seconds relative to 00:00:00, 01/01/YRBASE */
    m_time = (time_t)((long)(days + dy) * 86400L + (long)hh * 3600L +
                      (long)(mm * 60 + ss));
      /* - 1;   MS-DOS times always rounded up to nearest even second */
    TTrace((stderr, "dos_to_unix_time:\n"));
    TTrace((stderr, "  m_time before timezone = %ld\n", (long)m_time));

#if (!defined(MACOS) && !defined(RISCOS))
    /* adjust for local standard timezone offset */
#if (defined(BSD) || defined(MTS) || defined(__GO32__))
#ifdef BSD4_4
    m_time -= localtime(&m_time)->tm_gmtoff;    /* sec. EAST of GMT: subtr. */
#else /* !BSD4_4 */
    ftime(&tbp);                                /* get `timezone' */
    m_time += tbp.timezone * 60L;               /* seconds WEST of GMT:  add */
#endif /* ?BSD4_4 */
#else /* !(BSD || MTS || __GO32__) */
#ifdef WIN32
    {
        TIME_ZONE_INFORMATION tzinfo;
        DWORD res;

        /* account for timezone differences */
        res = GetTimeZoneInformation(&tzinfo);
        if (res == TIME_ZONE_ID_STANDARD)
            m_time += 60*(tzinfo.Bias + tzinfo.StandardBias);
        else if (res == TIME_ZONE_ID_DAYLIGHT)
            m_time += 60*(tzinfo.Bias + tzinfo.DaylightBias);
        /* GRR:  are other return-values possible? */
    }
#else /* !WIN32 */
    tzset();                    /* set `timezone' variable */
    m_time += timezone;         /* seconds WEST of GMT:  add */
#endif /* ?WIN32 */
#endif /* ?(BSD || MTS || __GO32__) */

    TTrace((stderr, "  m_time after timezone =  %ld\n", (long)m_time));
#endif /* !MACOS && !RISCOS */

#ifndef WIN32
#ifndef BSD4_4  /* (DST already added to tm_gmtoff, so skip tm_isdst) */
    /* adjust for local daylight savings (summer) time */
    if (localtime((time_t *)&m_time)->tm_isdst)
        m_time -= 60L * 60L;    /* adjust for daylight savings time */

    TTrace((stderr, "  m_time after DST =       %ld\n", (long)m_time));
#endif /* !BSD4_4 */
#endif /* !WIN32 */

#endif /* ?TOPS20 */

    return m_time;

} /* end function dos_to_unix_time() */





/******************************/
/* Function check_for_newer() */  /* used for overwriting/freshening/updating */
/******************************/

int check_for_newer(__G__ filename)  /* return 1 if existing file is newer */
    __GDEF                           /*  or equal; 0 if older; -1 if doesn't */
    char *filename;                  /*  exist yet */
{
    time_t existing, archive;
#ifdef USE_EF_UX_TIME
    ztimbuf z_utime;
#endif
#ifdef AOS_VS
    long    dmm, ddd, dyy, dhh, dmin, dss;


    dmm = (lrec.last_mod_file_date >> 5) & 0x0f;
    ddd = lrec.last_mod_file_date & 0x1f;
    dyy = (lrec.last_mod_file_date >> 9) + 1980;
    dhh = (lrec.last_mod_file_time >> 11) & 0x1f;
    dmin = (lrec.last_mod_file_time >> 5) & 0x3f;
    dss = (lrec.last_mod_file_time & 0x1f) * 2;

    /* under AOS/VS, file times can only be set at creation time,
     * with the info in a special DG format.  Make sure we can create
     * it here - we delete it later & re-create it, whether or not
     * it exists now.
     */
    if (!zvs_create(filename, (((ulg)dgdate(dmm, ddd, dyy)) << 16) |
        (dhh*1800L + dmin*30L + dss/2L), -1L, -1L, (char *) -1, -1, -1, -1))
        return DOES_NOT_EXIST;
#endif /* AOS_VS */

    Trace((stderr, "check_for_newer:  doing stat(%s)\n", filename));
    if (stat(filename, &G.statbuf)) {
        Trace((stderr,
          "check_for_newer:  stat(%s) returns %d:  file does not exist\n",
          filename, stat(filename, &G.statbuf)));
        return DOES_NOT_EXIST;
    }
    Trace((stderr, "check_for_newer:  stat(%s) returns 0:  file exists\n",
      filename));

#ifdef USE_EF_UX_TIME
    /* The `Unix extra field mtime' should be used for comparison with the
     * time stamp of the existing file >>>ONLY<<< when the EF info is also
     * used to set the modification time of the extracted file.
     * Currently, this is only the case for systems that incorporate
     * the OS specific code from the "unix/" subdirectory.
     */
    if (G.extra_field &&
        ef_scan_for_izux(G.extra_field, G.lrec.extra_field_length,
                         &z_utime, NULL) > 0) {
        TTrace((stderr, "check_for_newer:  using Unix extra field mtime\n"));
        existing = G.statbuf.st_mtime;
        archive  = z_utime.modtime;
    } else {
        /* round up existing filetime to nearest 2 seconds for comparison */
        existing = G.statbuf.st_mtime & 1 ? G.statbuf.st_mtime + 1
                                          : G.statbuf.st_mtime;
        archive  = dos_to_unix_time(G.lrec.last_mod_file_date,
                                    G.lrec.last_mod_file_time);
    }
#else /* !USE_EF_UX_TIME */
    /* round up existing filetime to nearest 2 seconds for comparison */
    existing = G.statbuf.st_mtime & 1 ? G.statbuf.st_mtime + 1
                                      : G.statbuf.st_mtime;
    archive  = dos_to_unix_time(G.lrec.last_mod_file_date,
                                G.lrec.last_mod_file_time);
#endif /* ?USE_EF_UX_TIME */

    TTrace((stderr, "check_for_newer:  existing %ld, archive %ld, e-a %ld\n",
      existing, archive, existing-archive));

    return (existing >= archive);

} /* end function check_for_newer() */

#endif /* !VMS && !OS2 && !CMS_MVS */





/************************/
/* Function do_string() */
/************************/

int do_string(__G__ len, option)      /* return PK-type error code */
    __GDEF
    unsigned int len;           /* without prototype, ush converted to this */
    int option;
{
    long comment_bytes_left, block_length;
    int error=PK_OK;
    ush extra_len;
#ifdef AMIGA
    char tmp_filenote[2 * AMIGA_FILENOTELEN]; /* extra room for squozen chars */
#endif


/*---------------------------------------------------------------------------
    This function processes arbitrary-length (well, usually) strings.  Four
    options are allowed:  SKIP, wherein the string is skipped (pretty logical,
    eh?); DISPLAY, wherein the string is printed to standard output after un-
    dergoing any necessary or unnecessary character conversions; DS_FN,
    wherein the string is put into the filename[] array after undergoing ap-
    propriate conversions (including case-conversion, if that is indicated:
    see the global variable pInfo->lcflag); and EXTRA_FIELD, wherein the
    `string' is assumed to be an extra field and is copied to the (freshly
    malloced) buffer G.extra_field.  The third option should be OK since
    filename is dimensioned at 1025, but we check anyway.

    The string, by the way, is assumed to start at the current file-pointer
    position; its length is given by len.  So start off by checking length
    of string:  if zero, we're already done.
  ---------------------------------------------------------------------------*/

    if (!len)
        return PK_COOL;

    switch (option) {

    /*
     * First case:  print string on standard output.  First set loop vari-
     * ables, then loop through the comment in chunks of OUTBUFSIZ bytes,
     * converting formats and printing as we go.  The second half of the
     * loop conditional was added because the file might be truncated, in
     * which case comment_bytes_left will remain at some non-zero value for
     * all time.  outbuf and slide are used as scratch buffers because they
     * are available (we should be either before or in between any file pro-
     * cessing).
     */

    case DISPLAY:
        comment_bytes_left = len;
        block_length = OUTBUFSIZ;    /* for the while statement, first time */
        while (comment_bytes_left > 0 && block_length > 0) {
            register uch *p = G.outbuf;
            register uch *q = G.outbuf;

            if ((block_length = readbuf(__G__ (char *)G.outbuf,
                   (unsigned) MIN((long)OUTBUFSIZ, comment_bytes_left))) == 0)
                return PK_EOF;
            comment_bytes_left -= block_length;

            /* this is why we allocated an extra byte for outbuf:  terminate
             *  with zero (ASCIIZ) */
            G.outbuf[(unsigned)block_length] = '\0';

            /* remove all ASCII carriage returns from comment before printing
             * (since used before A_TO_N(), check for CR instead of '\r')
             */
            while (*p) {
                while (*p == CR)
                    ++p;
                *q++ = *p++;
            }
            /* could check whether (p - outbuf) == block_length here */
            *q = '\0';

            A_TO_N(G.outbuf);   /* translate string to native */

#ifdef MSWIN
            /* ran out of local mem -- had to cheat */
            win_fprintf(stdout, len, (char *)G.outbuf);
            win_fprintf(stdout, 2, (char *)"\n\n");
#else /* !MSWIN */
#ifdef NATIVE          /* GRR:  can ANSI be used with EBCDIC? */
            (*G.message)((zvoid *)&G, G.outbuf, (ulg)(q-G.outbuf), 0);
#else /* ASCII */
            p = G.outbuf - 1;
            q = slide;
            while (*++p) {
                int pause = FALSE;

                if (*p == 0x1B) {          /* ASCII escape char */
                    *q++ = '^';
                    *q++ = '[';
                } else if (*p == 0x13) {   /* ASCII ^S (pause) */
                    pause = TRUE;
                    if (p[1] == 0x0a)      /* ASCII LF */
                        *q++ = *++p;
                    else if (p[1] == 0x0d && p[2] == 0x0a) {  /* ASCII CR LF */
                        *q++ = *++p;
                        *q++ = *++p;
                    }
                } else
                    *q++ = *p;
                if ((unsigned)(q-slide) > WSIZE-3 || pause) {   /* flush */
                    (*G.message)((zvoid *)&G, slide, (ulg)(q-slide), 0);
                    q = slide;
                    if (pause)
                        (*G.mpause)((zvoid *)&G, LoadFarString(QuitPrompt), 0);
                }
            }
            (*G.message)((zvoid *)&G, slide, (ulg)(q-slide), 0);
#endif /* ?NATIVE */
#endif /* ?MSWIN */
        }
        /* add '\n' if not at start of line */
        (*G.message)((zvoid *)&G, slide, 0L, 0x40);
        break;

    /*
     * Second case:  read string into filename[] array.  The filename should
     * never ever be longer than FILNAMSIZ-1 (1024), but for now we'll check,
     * just to be sure.
     */

    case DS_FN:
        extra_len = 0;
        if (len >= FILNAMSIZ) {
            Info(slide, 0x401, ((char *)slide,
              LoadFarString(FilenameTooLongTrunc)));
            error = PK_WARN;
            extra_len = (ush)(len - FILNAMSIZ + 1);
            len = FILNAMSIZ - 1;
        }
        if (readbuf(__G__  G.filename, len) == 0)
            return PK_EOF;
        G.filename[len] = '\0';   /* terminate w/zero:  ASCIIZ */

        A_TO_N(G.filename);       /* translate string to native */

        if (G.pInfo->lcflag)      /* replace with lowercase filename */
            TOLOWER(G.filename, G.filename);

        if (G.pInfo->vollabel && len > 8 && G.filename[8] == '.') {
            char *p = G.filename+8;
            while (*p++)
                p[-1] = *p;  /* disk label, and 8th char is dot:  remove dot */
        }

        if (!extra_len)         /* we're done here */
            break;

        /*
         * We truncated the filename, so print what's left and then fall
         * through to the SKIP routine.
         */
        Info(slide, 0x401, ((char *)slide, "[ %s ]\n", G.filename));
        len = extra_len;
        /*  FALL THROUGH...  */

    /*
     * Third case:  skip string, adjusting readbuf's internal variables
     * as necessary (and possibly skipping to and reading a new block of
     * data).
     */

    case SKIP:
        /* cur_zipfile_bufstart already takes account of extra_bytes, so don't
         * correct for it twice: */
        ZLSEEK(G.cur_zipfile_bufstart - G.extra_bytes +
               (G.inptr-G.inbuf) + len)
        break;

    /*
     * Fourth case:  assume we're at the start of an "extra field"; malloc
     * storage for it and read data into the allocated space.
     */

    case EXTRA_FIELD:
        if (G.extra_field != (uch *)NULL)
            free(G.extra_field);
        if ((G.extra_field = (uch *)malloc(len)) == (uch *)NULL) {
            Info(slide, 0x401, ((char *)slide, LoadFarString(ExtraFieldTooLong),
              len));
            /* cur_zipfile_bufstart already takes account of extra_bytes,
             * so don't correct for it twice: */
            ZLSEEK(G.cur_zipfile_bufstart - G.extra_bytes +
                   (G.inptr-G.inbuf) + len)
        } else
            if (readbuf(__G__  (char *)G.extra_field, len) == 0)
                return PK_EOF;
        break;

#ifdef AMIGA
    /*
     * Fifth case, for the Amiga only:  take the comment that would ordinarily
     * be skipped over, and turn it into a 79 character string that will be
     * attached to the file as a "filenote" after it is extracted.
     */

    case FILENOTE:
        if ((extra_len = readbuf(__G__ tmp_filenote, (unsigned)
                                 MIN(len, 2 * AMIGA_FILENOTELEN - 1))) == 0)
            return PK_EOF;
        if ((len -= extra_len) > 0)     /* treat remainder as in case SKIP: */
            ZLSEEK(G.cur_zipfile_bufstart - G.extra_bytes
                   + (G.inptr - G.inbuf) + len)
        /* convert multi-line text into single line with no ctl-chars: */
        tmp_filenote[extra_len] = '\0';
        while ((int) --extra_len >= 0)
            if ((unsigned) tmp_filenote[extra_len] < ' ')
                if (tmp_filenote[extra_len+1] == ' ')     /* no excess */
                    strcpy(tmp_filenote+extra_len, tmp_filenote+extra_len+1);
                else
                    tmp_filenote[extra_len] = ' ';
        tmp_filenote[AMIGA_FILENOTELEN - 1] = '\0';
        if (G.filenotes[G.filenote_slot])
            free(G.filenotes[G.filenote_slot]);     /* should not happen */
        G.filenotes[G.filenote_slot] = NULL;
        if (tmp_filenote[0]) {
            if (!(G.filenotes[G.filenote_slot] = malloc(strlen(tmp_filenote)+1)))
                return PK_MEM;
            strcpy(G.filenotes[G.filenote_slot], tmp_filenote);
        }
        break;
#endif /* AMIGA */

    } /* end switch (option) */

    return error;

} /* end function do_string() */





/***********************/
/* Function makeword() */
/***********************/

ush makeword(b)
    uch *b;
{
    /*
     * Convert Intel style 'short' integer to non-Intel non-16-bit
     * host format.  This routine also takes care of byte-ordering.
     */
    return (ush)((b[1] << 8) | b[0]);
}





/***********************/
/* Function makelong() */
/***********************/

ulg makelong(sig)
    uch *sig;
{
    /*
     * Convert intel style 'long' variable to non-Intel non-16-bit
     * host format.  This routine also takes care of byte-ordering.
     */
    return (((ulg)sig[3]) << 24)
        + (((ulg)sig[2]) << 16)
        + (((ulg)sig[1]) << 8)
        + ((ulg)sig[0]);
}





#ifdef ZMEM  /* memset/memcpy for systems without either them or bzero/bcopy */
             /* (no known systems as of 960211) */

/*********************/
/* Function memset() */
/*********************/

zvoid *memset(buf, init, len)
    register zvoid *buf;        /* buffer location */
    register int init;          /* initializer character */
    register unsigned int len;  /* length of the buffer */
{
    zvoid *start;

    start = buf;
    while (len--)
        *((char *)buf++) = (char)init;
    return start;
}





/*********************/
/* Function memcpy() */
/*********************/

zvoid *memcpy(dst, src, len)
    register zvoid *dst;
    register const zvoid *src;
    register unsigned int len;
{
    zvoid *start;

    start = dst;
    while (len-- > 0)
        *((char *)dst)++ = *((char *)src)++;
    return start;
}

#endif /* ZMEM */





/************************/
/* Function zstrnicmp() */
/************************/

int zstrnicmp(s1, s2, n)
    register const char *s1, *s2;
    register unsigned n;
{
    for (; n > 0;  --n, ++s1, ++s2) {

        if (ToLower(*s1) != ToLower(*s2))
            /* test includes early termination of one string */
            return (ToLower(*s1) < ToLower(*s2))? -1 : 1;

        if (*s1 == '\0')   /* both strings terminate early */
            return 0;
    }
    return 0;
}





#ifdef REGULUS  /* returns the inode number on success(!)...argh argh argh */
#  undef stat

/********************/
/* Function zstat() */
/********************/

int zstat(p, s)
    char *p;
    struct stat *s;
{
    return (stat(p,s) >= 0? 0 : (-1));
}

#endif /* REGULUS */





#ifdef SMALL_MEM

/*******************************/
/*  Function fLoadFarString()  */   /* (and friends...) */
/*******************************/

char *fLoadFarString(__GPRO__ const char Far *sz)
{
    (void)zfstrcpy(G.rgchBigBuffer, sz);
    return G.rgchBigBuffer;
}

char *fLoadFarStringSmall(__GPRO__ const char Far *sz)
{
    (void)zfstrcpy(G.rgchSmallBuffer, sz);
    return G.rgchSmallBuffer;
}

char *fLoadFarStringSmall2(__GPRO__ const char Far *sz)
{
    (void)zfstrcpy(G.rgchSmallBuffer2, sz);
    return G.rgchSmallBuffer2;
}




#if (!defined(_MSC_VER) || (_MSC_VER < 600))
/*************************/
/*  Function zfstrcpy()  */   /* portable clone of _fstrcpy() */
/*************************/

char Far * Far zfstrcpy(char Far *s1, const char Far *s2)
{
    char Far *p = s1;

    while ((*s1++ = *s2++) != '\0');
    return p;
}
#endif /* !_MSC_VER || (_MSC_VER < 600) */

#endif /* SMALL_MEM */
