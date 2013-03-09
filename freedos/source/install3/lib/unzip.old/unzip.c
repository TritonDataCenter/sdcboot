/*---------------------------------------------------------------------------

  unzip.c

  UnZip - a zipfile extraction utility.  See below for make instructions, or
  read the comments in Makefile and the various Contents files for more de-
  tailed explanations.  To report a bug, send a *complete* description to
  Zip-Bugs@wkuvx1.wku.edu; include machine type, operating system and ver-
  sion, compiler and version, and reasonably detailed error messages or prob-
  lem report.  To join Info-ZIP, see the instructions in README.

  UnZip 5.x is a greatly expanded and partially rewritten successor to 4.x,
  which in turn was almost a complete rewrite of version 3.x.  For a detailed
  revision history, see UnzpHist.zip at quest.jpl.nasa.gov.  For a list of
  the many (near infinite) contributors, see "CONTRIBS" in the UnZip source
  distribution.

  ---------------------------------------------------------------------------

  [from original zipinfo.c]

  This program reads great gobs of totally nifty information, including the
  central directory stuff, from ZIP archives ("zipfiles" for short).  It
  started as just a testbed for fooling with zipfiles, but at this point it
  is actually a useful utility.  It also became the basis for the rewrite of
  UnZip (3.16 -> 4.0), using the central directory for processing rather than
  the individual (local) file headers.

  As of ZipInfo v2.0 and UnZip v5.1, the two programs are combined into one.
  If the executable is named "unzip" (or "unzip.exe", depending), it behaves
  like UnZip by default; if it is named "zipinfo" or "ii", it behaves like
  ZipInfo.  The ZipInfo behavior may also be triggered by use of unzip's -Z
  option; for example, "unzip -Z [zipinfo_options] archive.zip".

  Another dandy product from your buddies at Newtware!

  Author:  Greg Roelofs, newt@uchicago.edu, 23 August 1990 -> December 1995

  ---------------------------------------------------------------------------

  Version:  unzip520.{tar.Z | zip | zoo} for Unix, VMS, OS/2, MS-DOS, Amiga,
              Atari, Windows 3.x/95/NT, Macintosh, Human68K, Acorn RISC OS,
              VM/CMS, MVS, AOS/VS and TOPS-20.  Decryption requires sources
              in zcrypt26.zip.  See the accompanying "Where" file in the main
              source distribution for ftp, uucp, BBS and mail-server sites.

  Copyrights:  see accompanying file "COPYING" in UnZip source distribution.
               (This software is free but NOT IN THE PUBLIC DOMAIN.  There
               are some restrictions on commercial use.)

  ---------------------------------------------------------------------------*/



#define UNZIP_INTERNAL
#include "unzip.h"        /* includes, typedefs, macros, prototypes, etc. */
#include "crypt.h"
#include "version.h"
#ifdef USE_ZLIB
#  include "zlib.h"
#endif



/*************/
/* Constants */
/*************/

#include "consts.h"  /* all constant global variables are in here */
                     /* (non-constant globals were moved to globals.c) */

#undef isatty
#define isatty(x)  0

/* constant local variables: */

#ifndef SFX
   static char Far EnvUnZip[] = ENV_UNZIP;
   static char Far EnvUnZip2[] = ENV_UNZIP2;
   static char Far EnvZipInfo[] = ENV_ZIPINFO;
   static char Far EnvZipInfo2[] = ENV_ZIPINFO2;
#endif

#if (!defined(SFX) || defined(SFX_EXDIR))
   static char Far NotExtracting[] = "";
   static char Far MustGiveExdir[] =
     "";
   static char Far OnlyOneExdir[] =
     "";
#endif

static char Far InvalidOptionsMsg[] = "";
static char Far IgnoreOOptionMsg[] =
  "";

/* usage() strings */
#ifndef VMSCLI
#ifndef SFX
#ifdef VMS
   static char Far Example3[] = "vms.c";
   static char Far Example2[] = "  unzip\
 \"-V\" foo \"Bar\" => must quote uppercase options and filenames in VMS\n";
#else /* !VMS */
   static char Far Example3[] = "";
#ifdef RISCOS
   static char Far Example2[] =
"  unzip foo -d RAM:$   => extract all files from foo into RAMDisc\n";
#else /* !RISCOS */
#if defined(OS2) || (defined(DOS_OS2_W32) && defined(MORE))
   static char Far Example2[] = "";   /* no room:  too many local3[] items */
#else /* !OS2 */
   static char Far Example2[] = "";
#endif /* ?OS2 */
#endif /* ?RISCOS */
#endif /* ?VMS */

/* local1[]:  command options */
#if (defined(DLL) && defined(API_DOC))
   static char Far local1[] = "";
#else /* !(DLL && API_DOC) */
   static char Far local1[] = "";
#endif /* ?(DLL && API_DOC) */

/* local2[] and local3[]:  modifier options */
#ifdef DOS_OS2_W32
   static char Far local2[] = "";
#ifdef OS2
#ifdef MORE
   static char Far local3[] = "";
#else
   static char Far local3[] = "";
#endif
#else /* !OS2 */
#ifdef MORE
   static char Far local3[] = "";
#else
   static char Far local3[] = "";
#endif
#endif /* ?OS2 */
#else /* !DOS_OS2_W32 */
#ifdef VMS
   static char Far local2[] = "\"-X\" restore owner/protection info";
#ifdef MORE
   static char Far local3[] = "  \
                                          \"-M\" pipe through \"more\" pager\n";
#else
   static char Far local3[] = "\n";
#endif
#else /* !VMS */
#ifdef UNIX
   static char Far local2[] = " -X  restore UID/GID info";
#ifdef MORE
   static char Far local3[] = "\
                                             -M  pipe through \"more\" pager\n";
#else
   static char Far local3[] = "\n";
#endif
#else /* !UNIX */
#ifdef AMIGA
    static char Far local2[] = " -N  restore comments as filenotes";
#ifdef MORE
    static char Far local3[] = "\
                                             -M  pipe through \"more\" pager\n";
#else
    static char Far local3[] = "\n";
#endif
#else /* !AMIGA */
#ifdef MORE
    static char Far local2[] = "";
    static char Far local3[] = "";
#else
    static char Far local2[] = "";   /* Atari, Mac, etc. */
    static char Far local3[] = "";
#endif
#endif /* ?AMIGA */
#endif /* ?UNIX */
#endif /* ?VMS */
#endif /* ?DOS_OS2_W32 */
#endif /* !SFX */

#ifndef NO_ZIPINFO
#ifdef VMS
   static char Far ZipInfoExample[] = "* or % (e.g., \"*font-%.zip\")";
#else
   static char Far ZipInfoExample[] = "";
#endif

static char Far ZipInfoUsageLine1[] = "";

static char Far ZipInfoUsageLine2[] = "";

static char Far ZipInfoUsageLine3[] = "";
#ifdef MORE
#ifdef VMS
   static char Far ZipInfoUsageLine4[] =
     " \"-M\" page output through built-in \"more\"\n";
#else
   static char Far ZipInfoUsageLine4[] =
     "";
#endif
#else /* !MORE */
   static char Far ZipInfoUsageLine4[] = "";
#endif /* ?MORE */
#endif /* !NO_ZIPINFO */
#endif /* !VMSCLI */

#ifdef BETA
   static char Far BetaVersion[] = "%s\
      THIS IS STILL A BETA VERSION OF UNZIP%s -- DO NOT DISTRIBUTE.\n\n";
#endif

#ifdef SFX
   static char Far UnzipSFXBanner[] =
     "UnZipSFX %d.%d%d%s of %s, by Info-ZIP (Zip-Bugs@wkuvx1.wku.edu).\n";
#  if (defined(SFX_EXDIR) && !defined(VMS))
     static char Far UnzipSFXOpts[] =
    "Valid options are -tfupcz and -d <exdir>; modifiers are -abjnoqCL%sV%s.\n";
#  else
     static char Far UnzipSFXOpts[] =
       "Valid options are -tfupcz; modifiers are -abjnoqCL%sV%s.\n";
#  endif
#else /* !SFX */
   static char Far CompileOptions[] = "";
   static char Far CompileOptFormat[] = "";
   static char Far EnvOptions[] = "";
   static char Far EnvOptFormat[] = "";
   static char Far None[] = "";
#  ifdef ASM_CRC
     static char Far AsmCRC[] = "";
#  endif
#  ifdef ASM_INFLATECODES
     static char Far AsmInflateCodes[] = "";
#  endif
#  ifdef CHECK_VERSIONS
     static char Far Check_Versions[] = "";
#  endif
#  ifdef COPYRIGHT_CLEAN
     static char Far Copyright_Clean[] =
     "";
#  endif
#  ifdef DEBUG
     static char Far Debug[] = "DEBUG";
#  endif
#  ifdef DEBUG_TIME
     static char Far DebugTime[] = "DEBUG_TIME";
#  endif
#  ifdef DLL
     static char Far Dll[] = "";
#  endif
#  ifdef DOSWILD
     static char Far DosWild[] = "";
#  endif
#  ifndef MORE
     static char Far No_More[] = "";
#  endif
#  ifdef NO_ZIPINFO
     static char Far No_ZipInfo[] = "";
#  endif
#  ifdef OS2_EAS
     static char Far OS2ExtAttrib[] = "OS2_EAS";
#  endif
#  ifdef REENTRANT
     static char Far Reentrant[] = "";
#  endif
#  ifdef REGARGS
     static char Far RegArgs[] = "";
#  endif
#  ifdef RETURN_CODES
     static char Far Return_Codes[] = "";
#  endif
#  ifdef TIMESTAMP
     static char Far TimeStamp[] = "";
#  endif
#  ifndef COPYRIGHT_CLEAN
     static char Far Use_Smith_Code[] =
     "";
#  endif
#ifdef USE_EF_UX_TIME
     static char Far Use_EF_UX_time[] = "";
#endif
#  ifdef USE_ZLIB
     static char Far UseZlib[] =
     "USE_ZLIB (compiled with version %s; using version %s)";
#  endif
#  ifdef VMSCLI
     static char Far VmsCLI[] = "VMSCLI";
#  endif
#  ifdef VMSWILD
     static char Far VmsWild[] = "VMSWILD";
#  endif
#  ifdef CRYPT
#    ifdef PASSWD_FROM_STDIN
       static char Far PasswdStdin[] = "";
#    endif
     static char Far Decryption[] = "";
     static char Far CryptDate[] = CR_VERSION_DATE;
#  endif
#  ifdef __EMX__
     static char Far EnvEMX[] = "EMX";
     static char Far EnvEMXOPT[] = "EMXOPT";
#  endif
#  ifdef __GO32__
     static char Far EnvGO32[] = "GO32";
     static char Far EnvGO32TMP[] = "GO32TMP";
#  endif

/* UnzipUsageLine1[] is also used in vms/cmdline.c:  do not make it static */
#ifdef COPYRIGHT_CLEAN
   char Far UnzipUsageLine1[] = "";
#else
   char Far UnzipUsageLine1[] = "";
#endif /* ?COPYRIGHT_CLEAN */

static char Far UnzipUsageLine2a[] = "";

#ifndef VMSCLI
static char Far UnzipUsageLine2[] = "";

#ifdef NO_ZIPINFO
#  define ZIPINFO_MODE_OPTION  ""
   static char Far ZipInfoMode[] =
     "(ZipInfo mode is disabled in this version.)";
#else
#  define ZIPINFO_MODE_OPTION  "[-Z] "
#  ifdef VMS
     static char Far ZipInfoMode[] =
       "";
#  else
     static char Far ZipInfoMode[] =
       "";
#  endif
#endif /* ?NO_ZIPINFO */

static char Far UnzipUsageLine3[] = "";

static char Far UnzipUsageLine4[] = "";

static char Far UnzipUsageLine5[] = "";
#endif /* !VMSCLI */
#endif /* ?SFX */





/*****************************/
/*  main() / UzpMain() stub  */
/*****************************/

int MAIN(argc, argv)   /* return PK-type error code (except under VMS) */
    int argc;
    char *argv[];
{
    int r;

    CONSTRUCTGLOBALS();
    r = unzip(__G__ argc, argv);
    DESTROYGLOBALS()
    RETURN(r);
}




/*******************************/
/*  Primary UnZip entry point  */
/*******************************/

int unzip(__G__ argc, argv)
    __GDEF
    int argc;
    char *argv[];
{
#ifndef NO_ZIPINFO
    char *p;
#endif
    int i, retcode, error=FALSE;

/*---------------------------------------------------------------------------
    Macintosh initialization code.
  ---------------------------------------------------------------------------*/

#ifdef MACOS
    int a;

    for (a = 0;  a < 4;  ++a)
        G.rghCursor[a] = GetCursor(a+128);
    G.giCursor = 0;
    error = FALSE;
#endif

#if defined(__IBMC__) && defined(__DEBUG_ALLOC__)
    extern void DebugMalloc(void);

    atexit(DebugMalloc);
#endif

#ifdef MALLOC_WORK
    G.area.Slide =(uch *)calloc(8193, sizeof(short)+sizeof(char)+sizeof(char));
    G.area.shrink.Parent = (shrint *)G.area.Slide;
    G.area.shrink.value = G.area.Slide + (sizeof(shrint)*(HSIZE+1));
    G.area.shrink.Stack = G.area.Slide +
                           (sizeof(shrint) + sizeof(uch))*(HSIZE+1);
#endif

/*---------------------------------------------------------------------------
    Human68K initialization code.
  ---------------------------------------------------------------------------*/

#ifdef __human68k__
    InitTwentyOne();
#endif

/*---------------------------------------------------------------------------
    Acorn RISC OS initialization code.
  ---------------------------------------------------------------------------*/

#ifdef RISCOS
    set_prefix();
#endif

/*---------------------------------------------------------------------------
    Set signal handler for restoring echo, warn of zipfile corruption, etc.
  ---------------------------------------------------------------------------*/

#ifndef CMS_MVS
//    signal(SIGINT, handler);
#ifdef SIGTERM                 /* some systems really have no SIGTERM */
//   signal(SIGTERM, handler);
#endif
#ifdef SIGBUS
//    signal(SIGBUS, handler);
#endif
#ifdef SIGSEGV
//    signal(SIGSEGV, handler);
#endif
#endif /* !CMS_MVS */

/*---------------------------------------------------------------------------
    First figure out if we're running in UnZip mode or ZipInfo mode, and put
    the appropriate environment-variable options into the queue.  Then rip
    through any command-line options lurking about...
  ---------------------------------------------------------------------------*/

#ifdef SFX
    G.argv0 = argv[0];
#if defined(OS2) || defined(WIN32)
    G.zipfn = GetLoadPath(__G);/* non-MSC NT puts path into G.filename[] */
#else
    G.zipfn = G.argv0;
#endif
    G.zipinfo_mode = FALSE;
    if ((error = uz_opts(__G__ &argc, &argv)) != 0)
        return(error);

#else /* !SFX */

#ifdef RISCOS
    /* get the extensions to swap from environment */
    getRISCOSexts("Unzip$Exts");
#endif

#ifdef MSDOS
    /* extract MKS extended argument list from environment (before envargs!) */
    mksargs(&argc, &argv);
#endif

#ifdef VMSCLI
    {
        ulg status = vms_unzip_cmdline(&argc, &argv);
        if (!(status & 1))
            return status;
    }
#endif /* VMSCLI */

#ifndef NO_ZIPINFO
    if ((p = strrchr(argv[0], DIR_END)) == (char *)NULL)
        p = argv[0];
    else
        ++p;

    if (STRNICMP(p, LoadFarStringSmall(Zipnfo), 7) == 0 ||
        STRNICMP(p, "ii", 2) == 0 ||
        (argc > 1 && strncmp(argv[1], "-Z", 2) == 0))
    {
        G.zipinfo_mode = TRUE;
        envargs(__G__ &argc, &argv, LoadFarStringSmall(EnvZipInfo),
          LoadFarStringSmall2(EnvZipInfo2));
        error = zi_opts(__G__ &argc, &argv);
    } else
#endif /* NO_ZIPINFO */
    {
        G.zipinfo_mode = FALSE;
        envargs(__G__ &argc, &argv, LoadFarStringSmall(EnvUnZip),
          LoadFarStringSmall2(EnvUnZip2));
        error = uz_opts(__G__ &argc, &argv);
    }
    if ((argc < 0) || error)
        return(error);

#endif /* ?SFX */

/*---------------------------------------------------------------------------
    Now get the zipfile name from the command line and then process any re-
    maining options and file specifications.
  ---------------------------------------------------------------------------*/

#ifdef DOS_H68_OS2_W32
    /* convert MSDOS-style directory separators to Unix-style ones for
     * user's convenience (include zipfile name itself)
     */
    for (G.pfnames = argv, i = argc+1;  i > 0;  --i) {
        char *q;

        for (q = *G.pfnames;  *q;  ++q)
            if (*q == '\\')
                *q = '/';
        ++G.pfnames;
    }
#if 0                          /* old version; depends on argv[last] == NULL */
    G.pfnames = argv;
    while (*G.pfnames != (char *)NULL) {
        char *q;

        for (q = *G.pfnames;  *q;  ++q)
            if (*q == '\\')
                *q = '/';
        ++G.pfnames;
    }
#endif /* 0 */
#endif /* DOS_H68_OS2_W32 */

#ifndef SFX
    G.wildzipfn = *argv++;
#endif

#if (defined(SFX) && !defined(SFX_EXDIR)) /* only check for -x */

    G.filespecs = argc;
    G.xfilespecs = 0;

    if (argc > 0) {
        char **pp = argv-1;

        G.pfnames = argv;
        while (*++pp)
            if (strcmp(*pp, "-x") == 0) {
                if (pp > argv) {
                    *pp = 0;              /* terminate G.pfnames */
                    G.filespecs = pp - G.pfnames;
                } else {
                    G.pfnames = fnames;  /* defaults */
                    G.filespecs = 0;
                }
                G.pxnames = pp + 1;      /* excluded-names ptr: _after_ -x */
                G.xfilespecs = argc - G.filespecs - 1;
                break;                    /* skip rest of args */
            }
        G.process_all_files = FALSE;
    } else
        G.process_all_files = TRUE;      /* for speed */

#else /* !SFX || SFX_EXDIR */             /* check for -x or -d */

    G.filespecs = argc;
    G.xfilespecs = 0;

    if (argc > 0) {
        int in_files=FALSE, in_xfiles=FALSE;
        char **pp = argv-1;

        G.process_all_files = FALSE;
        G.pfnames = argv;
        while (*++pp) {
            Trace((stderr, "pp - argv = %d\n", pp-argv));
#ifdef CMS_MVS
            if (!G.dflag && STRNICMP(*pp, "-d", 2) == 0) {
#else
            if (!G.dflag && strncmp(*pp, "-d", 2) == 0) {
#endif
                char *q = *pp;
                int firstarg = (pp == argv);

                G.dflag = TRUE;
                if (in_files) {      /* ... zipfile ... -d exdir ... */
                    *pp = (char *)NULL;         /* terminate G.pfnames */
                    G.filespecs = pp - G.pfnames;
                    in_files = FALSE;
                } else if (in_xfiles) {
                    *pp = (char *)NULL;         /* terminate G.pxnames */
                    G.xfilespecs = pp - G.pxnames;
                    /* "... -x xlist -d exdir":  nothing left */
                }
                /* first check for "-dexdir", then for "-d exdir" */
                if (q[2])
                    q += 2;
                else if (*++pp)
                    q = *pp;
                else {
                    Info(slide, 0x401, ((char *)slide,
                      LoadFarString(MustGiveExdir)));
                    return(PK_PARAM);  /* don't extract here by accident */
                }
                if (G.extract_flag) {
                    G.create_dirs = !G.fflag;
                    if ((error = checkdir(__G__ q, ROOT)) > 2) {
                        return(error);  /* out of memory, or file in way */
                    }
                } else
                    Info(slide, 0x401, ((char *)slide,
                      LoadFarString(NotExtracting)));
                if (firstarg)   /* ... zipfile -d exdir ... */
                    if (pp[1]) {
                        G.pfnames = pp + 1;  /* argv+2 */
                        G.filespecs = argc - (G.pfnames-argv);  /* for now... */
                    } else {
                        G.process_all_files = TRUE;
                        G.pfnames = fnames;  /* GRR: necessary? */
                        G.filespecs = 0;     /* GRR: necessary? */
                        break;
                    }
            } else if (!in_xfiles) {
                if (strcmp(*pp, "-x") == 0) {
                    in_xfiles = TRUE;
                    if (pp == G.pfnames) {
                        G.pfnames = fnames;  /* defaults */
                        G.filespecs = 0;
                    } else if (in_files) {
                        *pp = 0;                   /* terminate G.pfnames */
                        G.filespecs = pp - G.pfnames;  /* adjust count */
                        in_files = FALSE;
                    }
                    G.pxnames = pp + 1;  /* excluded-names ptr starts after -x */
                    G.xfilespecs = argc - (G.pxnames-argv);  /* anything left... */
                } else
                    in_files = TRUE;
            }
        }
    } else
        G.process_all_files = TRUE;      /* for speed */

#endif /* ?(SFX && !SFX_EXDIR) */

/*---------------------------------------------------------------------------
    Okey dokey, we have everything we need to get started.  Let's roll.
  ---------------------------------------------------------------------------*/

    retcode = process_zipfiles(__G);
    return(retcode);

} /* end main()/unzip() */





/**********************/
/* Function uz_opts() */
/**********************/

int uz_opts(__G__ pargc, pargv)
    int *pargc;
    char ***pargv;
    __GDEF
{
    char **argv, *s;
#if (!defined(SFX) || defined(SFX_EXDIR))
    char *exdir = NULL;         /* initialized to shut up false GCC warning */
#endif /* !SFX || SFX_EXDIR */
    int argc, c, error=FALSE, negative=0;


    argc = *pargc;
    argv = *pargv;

    while (--argc > 0 && (*++argv)[0] == '-') {
        s = argv[0] + 1;
        while ((c = *s++) != 0) {    /* "!= 0":  prevent Turbo C warning */
#ifdef CMS_MVS
            switch (tolower(c)) {
#else
            switch (c) {
#endif
                case ('-'):
                    ++negative;
                    break;
                case ('a'):
                    if (negative) {
                        G.aflag = MAX(G.aflag-negative,0);
                        negative = 0;
                    } else
                        ++G.aflag;
                    break;
                case ('b'):
                    if (negative) {
#ifdef VMS
                        G.bflag = MAX(G.bflag-negative,0);
#endif
                        negative = 0;   /* do nothing:  "-b" is default */
                    } else {
#ifdef VMS
                        if (G.aflag == 0)
                           ++G.bflag;
#endif
                        G.aflag = 0;
                    }
                    break;
                case ('c'):
                    if (negative) {
                        G.cflag = FALSE, negative = 0;
#ifdef NATIVE
                        G.aflag = 0;
#endif
                    } else {
                        G.cflag = TRUE;
#ifdef NATIVE
                        G.aflag = 2;  /* so you can read it on the screen */
#endif
#ifdef DLL
                        if (G.redirect_text)
                            G.redirect_data = 2;
#endif
                    }
                    break;
#ifndef CMS_MVS
                case ('C'):    /* -C:  match filenames case-insensitively */
                    if (negative)
                        G.C_flag = FALSE, negative = 0;
                    else
                        G.C_flag = TRUE;
                    break;
#endif /* !CMS_MVS */
#if (!defined(SFX) || defined(SFX_EXDIR))
                case ('d'):
                    if (negative) {   /* negative not allowed with -d exdir */
                        Info(slide, 0x401, ((char *)slide,
                          LoadFarString(MustGiveExdir)));
                        return(PK_PARAM);  /* don't extract here by accident */
                    }
                    if (G.dflag) {
                        Info(slide, 0x401, ((char *)slide,
                          LoadFarString(OnlyOneExdir)));
                        return(PK_PARAM);    /* GRR:  stupid restriction? */
                    } else {
                        G.dflag = TRUE;
                        /* first check for "-dexdir", then for "-d exdir" */
                        exdir = s;
                        if (*exdir == 0) {
                            if (argc > 1) {
                                --argc;
                                exdir = *++argv;
                                if (*exdir == '-') {
                                    Info(slide, 0x401, ((char *)slide,
                                      LoadFarString(MustGiveExdir)));
                                    return(PK_PARAM);
                                }
                                /* else exdir points at extraction dir */
                            } else {
                                Info(slide, 0x401, ((char *)slide,
                                  LoadFarString(MustGiveExdir)));
                                return(PK_PARAM);
                            }
                        }
                        /* exdir now points at extraction dir (-dexdir or
                         *  -d exdir); point s at end of exdir to avoid mis-
                         *  interpretation of exdir characters as more options
                         */
                        if (*s != 0)
                            while (*++s != 0)
                                ;
                    }
                    break;
#endif /* !SFX || SFX_EXDIR */
                case ('e'):    /* just ignore -e, -x options (extract) */
                    break;
                case ('f'):    /* "freshen" (extract only newer files) */
                    if (negative)
                        G.fflag = G.uflag = FALSE, negative = 0;
                    else
                        G.fflag = G.uflag = TRUE;
                    break;
                case ('h'):    /* just print help message and quit */
                    *pargc = -1;
                    return usage(__G__ FALSE);
                case ('j'):    /* junk pathnames/directory structure */
                    if (negative)
                        G.jflag = FALSE, negative = 0;
                    else
                        G.jflag = TRUE;
                    break;
#ifndef SFX
                case ('l'):
                    if (negative) {
                        G.vflag = MAX(G.vflag-negative,0);
                        negative = 0;
                    } else
                        ++G.vflag;
                    break;
#endif /* !SFX */
#ifndef CMS_MVS
                case ('L'):    /* convert (some) filenames to lowercase */
                    if (negative)
                        G.L_flag = FALSE, negative = 0;
                    else
                        G.L_flag = TRUE;
                    break;
#endif /* !CMS_MVS */
#ifdef MORE
#ifdef CMS_MVS
                case ('m'):
#endif
                case ('M'):    /* send all screen output through "more" fn. */
/* GRR:  eventually check for numerical argument => height */
                    if (negative)
                        G.M_flag = FALSE, negative = 0;
                    else
                        G.M_flag = TRUE;
                    break;
#endif /* MORE */
                case ('n'):    /* don't overwrite any files */
                    if (negative)
                        G.overwrite_none = FALSE, negative = 0;
                    else
                        G.overwrite_none = TRUE;
                    break;
#ifdef AMIGA
                case ('N'):    /* restore comments as filenotes */
                    if (negative)
                        G.N_flag = FALSE, negative = 0;
                    else
                        G.N_flag = TRUE;
                    break;
#endif /* AMIGA */
                case ('o'):    /* OK to overwrite files without prompting */
                    if (negative) {
                        G.overwrite_all = MAX(G.overwrite_all-negative,0);
                        negative = 0;
                    } else
                        ++G.overwrite_all;
                    break;
                case ('p'):    /* pipes:  extract to stdout, no messages */
                    if (negative) {
                        G.cflag = FALSE;
                        G.qflag = MAX(G.qflag-999,0);
                        negative = 0;
                    } else {
                        G.cflag = TRUE;
                        G.qflag += 999;
                    }
                    break;
                case ('q'):    /* quiet:  fewer comments/messages */
                    if (negative) {
                        G.qflag = MAX(G.qflag-negative,0);
                        negative = 0;
                    } else
                        ++G.qflag;
                    break;
#if (defined(DLL) && defined(API_DOC))
                case ('A'):    /* extended help for API */
                    APIhelp(__G__ argc,argv);
                    return 0;
#endif
#ifdef DOS_OS2_W32
                case ('s'):    /* spaces in filenames:  allow by default */
                    if (negative)
                        G.sflag = FALSE, negative = 0;
                    else
                        G.sflag = TRUE;
                    break;
#endif /* DOS_OS2_W32 */
                case ('t'):
                    if (negative)
                        G.tflag = FALSE, negative = 0;
                    else
                        G.tflag = TRUE;
                    break;
#ifdef TIMESTAMP
                case ('T'):
                    if (negative)
                        G.T_flag = FALSE, negative = 0;
                    else
                        G.T_flag = TRUE;
                    break;
#endif
                case ('u'):    /* update (extract only new and newer files) */
                    if (negative)
                        G.uflag = FALSE, negative = 0;
                    else
                        G.uflag = TRUE;
                    break;
#ifndef CMS_MVS
                case ('U'):    /* obsolete; to be removed in version 6.0 */
                    if (negative)
                        G.L_flag = TRUE, negative = 0;
                    else
                        G.L_flag = FALSE;
                    break;
#endif /* !CMS_MVS */
#ifndef SFX
                case ('v'):    /* verbose */
                    if (negative) {
                        G.vflag = MAX(G.vflag-negative,0);
                        negative = 0;
                    } else if (G.vflag)
                        ++G.vflag;
                    else
                        G.vflag = 2;
                    break;
#endif /* !SFX */
#ifndef CMS_MVS
                case ('V'):    /* Version (retain VMS/DEC-20 file versions) */
                    if (negative)
                        G.V_flag = FALSE, negative = 0;
                    else
                        G.V_flag = TRUE;
                    break;
#endif /* !CMS_MVS */
                case ('x'):    /* extract:  default */
#ifdef SFX
                    /* When 'x' is the only option in this argument, and the
                     * next arg is not an option, assume this initiates an
                     * exclusion list (-x xlist): Then, terminate option
                     * scanning; leave uz_opts with still pointing to "-x";
                     * the xlist is processed later.
                     */
                    if (s - argv[0] == 2 && *s == 0 &&
                        argc > 1 && argv[1][0] != '-') {
                        /* break out of nested loops without "++argv;--argc" */
                        goto opts_done;
                    }
#endif
                    break;
#if defined(VMS) || defined(UNIX) || defined(OS2)
                case ('X'):   /* restore owner/protection info (need privs?) */
                    if (negative)
                        G.X_flag = FALSE, negative = 0;
                    else
                        G.X_flag = TRUE;
                    break;
#endif /* VMS || UNIX || OS2 */
                case ('z'):    /* display only the archive comment */
                    if (negative) {
                        G.zflag -= negative;
                        negative = 0;
                    } else
                        ++G.zflag;
                    break;
#ifdef DOS_OS2_W32
                case ('$'):
                    if (negative) {
                        G.volflag = MAX(G.volflag-negative,0);
                        negative = 0;
                    } else
                        ++G.volflag;
                    break;
#endif /* DOS_OS2_W32 */
                default:
                    error = TRUE;
                    break;

            } /* end switch */
        } /* end while (not end of argument string) */
    } /* end while (not done with switches) */

/*---------------------------------------------------------------------------
    Check for nonsensical combinations of options.
  ---------------------------------------------------------------------------*/

#ifdef SFX
opts_done:  /* yes, very ugly...but only used by UnZipSFX with -x xlist */
#endif

    if ((G.cflag && G.tflag) || (G.cflag && G.uflag) ||
        (G.tflag && G.uflag) || (G.fflag && G.overwrite_none))
    {
        Info(slide, 0x401, ((char *)slide, LoadFarString(InvalidOptionsMsg)));
        error = TRUE;
    }
    if (G.aflag > 2)
        G.aflag = 2;
#ifdef VMS
    if (G.bflag > 2)
        G.bflag = 2;
#endif
    if (G.overwrite_all && G.overwrite_none) {
        Info(slide, 0x401, ((char *)slide, LoadFarString(IgnoreOOptionMsg)));
        G.overwrite_all = FALSE;
    }
#ifdef MORE
    if (G.M_flag && !isatty(1))  /* stdout redirected: "more" func useless */
        G.M_flag = 0;
#endif

#ifdef SFX
    if (error)
#else
    if ((argc-- == 0) || error)
#endif
    {
        *pargc = argc;
        *pargv = argv;
#ifndef SFX
        if (G.vflag >= 2 && argc == -1) {              /* "unzip -v" */
            if (G.qflag > 3)                           /* "unzip -vqqqq" */
                Info(slide, 0, ((char *)slide, "%d\n",
                  (UZ_MAJORVER*100 + UZ_MINORVER*10 + PATCHLEVEL)));
            else {
                char *envptr, *getenv();
                int numopts = 0;

                Info(slide, 0, ((char *)slide, LoadFarString(UnzipUsageLine1),
                  UZ_MAJORVER, UZ_MINORVER, PATCHLEVEL, BETALEVEL,
                  LoadFarStringSmall(VersionDate)));
                Info(slide, 0, ((char *)slide,
                  LoadFarString(UnzipUsageLine2a)));
                version(__G);
                Info(slide, 0, ((char *)slide, LoadFarString(CompileOptions)));
#ifdef ASM_CRC
                Info(slide, 0, ((char *)slide, LoadFarString(CompileOptFormat),
                  LoadFarStringSmall(AsmCRC)));
                ++numopts;
#endif
#ifdef ASM_INFLATECODES
                Info(slide, 0, ((char *)slide, LoadFarString(CompileOptFormat),
                  LoadFarStringSmall(AsmInflateCodes)));
                ++numopts;
#endif
#ifdef CHECK_VERSIONS
                Info(slide, 0, ((char *)slide, LoadFarString(CompileOptFormat),
                  LoadFarStringSmall(Check_Versions)));
                ++numopts;
#endif
#ifdef COPYRIGHT_CLEAN
                Info(slide, 0, ((char *)slide, LoadFarString(CompileOptFormat),
                  LoadFarStringSmall(Copyright_Clean)));
                ++numopts;
#endif
#ifdef DEBUG
                Info(slide, 0, ((char *)slide, LoadFarString(CompileOptFormat),
                  LoadFarStringSmall(Debug)));
                ++numopts;
#endif
#ifdef DEBUG_TIME
                Info(slide, 0, ((char *)slide, LoadFarString(CompileOptFormat),
                  LoadFarStringSmall(DebugTime)));
                ++numopts;
#endif
#ifdef DLL
                Info(slide, 0, ((char *)slide, LoadFarString(CompileOptFormat),
                  LoadFarStringSmall(Dll)));
                ++numopts;
#endif
#ifdef DOSWILD
                Info(slide, 0, ((char *)slide, LoadFarString(CompileOptFormat),
                  LoadFarStringSmall(DosWild)));
                ++numopts;
#endif
#ifndef MORE
                Info(slide, 0, ((char *)slide, LoadFarString(CompileOptFormat),
                  LoadFarStringSmall(No_More)));
                ++numopts;
#endif
#ifdef NO_ZIPINFO
                Info(slide, 0, ((char *)slide, LoadFarString(CompileOptFormat),
                  LoadFarStringSmall(No_ZipInfo)));
                ++numopts;
#endif
#ifdef OS2_EAS
                Info(slide, 0, ((char *)slide, LoadFarString(CompileOptFormat),
                  LoadFarStringSmall(OS2ExtAttrib)));
                ++numopts;
#endif
#ifdef REENTRANT
                Info(slide, 0, ((char *)slide, LoadFarString(CompileOptFormat),
                  LoadFarStringSmall(Reentrant)));
                ++numopts;
#endif
#ifdef REGARGS
                Info(slide, 0, ((char *)slide, LoadFarString(CompileOptFormat),
                  LoadFarStringSmall(RegArgs)));
                ++numopts;
#endif
#ifdef RETURN_CODES
                Info(slide, 0, ((char *)slide, LoadFarString(CompileOptFormat),
                  LoadFarStringSmall(Return_Codes)));
                ++numopts;
#endif
#ifdef TIMESTAMP
                Info(slide, 0, ((char *)slide, LoadFarString(CompileOptFormat),
                  LoadFarStringSmall(TimeStamp)));
                ++numopts;
#endif
#ifndef COPYRIGHT_CLEAN
                Info(slide, 0, ((char *)slide, LoadFarString(CompileOptFormat),
                  LoadFarStringSmall(Use_Smith_Code)));
                ++numopts;
#endif
#ifdef USE_EF_UX_TIME
                Info(slide, 0, ((char *)slide, LoadFarString(CompileOptFormat),
                  LoadFarStringSmall(Use_EF_UX_time)));
                ++numopts;
#endif
#ifdef USE_ZLIB
                sprintf((char *)(slide+256), LoadFarStringSmall(UseZlib),
                  ZLIB_VERSION, zlib_version);
                Info(slide, 0, ((char *)slide, LoadFarString(CompileOptFormat),
                  (char *)(slide+256)));
                ++numopts;
#endif
#ifdef VMSCLI
                Info(slide, 0, ((char *)slide, LoadFarString(CompileOptFormat),
                  LoadFarStringSmall(VmsCLI)));
                ++numopts;
#endif
#ifdef VMSWILD
                Info(slide, 0, ((char *)slide, LoadFarString(CompileOptFormat),
                  LoadFarStringSmall(VmsWild)));
                ++numopts;
#endif
#ifdef CRYPT
# ifdef PASSWD_FROM_STDIN
                Info(slide, 0, ((char *)slide, LoadFarString(CompileOptFormat),
                  LoadFarStringSmall(PasswdStdin)));
# endif
                Info(slide, 0, ((char *)slide, LoadFarString(Decryption),
                  CR_MAJORVER, CR_MINORVER, CR_BETA_VER,
                  LoadFarStringSmall(CryptDate)));
                ++numopts;
#endif /* CRYPT */
                if (numopts == 0)
                    Info(slide, 0, ((char *)slide,
                      LoadFarString(CompileOptFormat),
                      LoadFarStringSmall(None)));

                Info(slide, 0, ((char *)slide, LoadFarString(EnvOptions)));
                envptr = getenv(LoadFarStringSmall(EnvUnZip));
                Info(slide, 0, ((char *)slide, LoadFarString(EnvOptFormat),
                  LoadFarStringSmall(EnvUnZip),
                  (envptr == (char *)NULL || *envptr == 0)?
                  LoadFarStringSmall2(None) : envptr));
                envptr = getenv(LoadFarStringSmall(EnvUnZip2));
                Info(slide, 0, ((char *)slide, LoadFarString(EnvOptFormat),
                  LoadFarStringSmall(EnvUnZip2),
                  (envptr == (char *)NULL || *envptr == 0)?
                  LoadFarStringSmall2(None) : envptr));
                envptr = getenv(LoadFarStringSmall(EnvZipInfo));
                Info(slide, 0, ((char *)slide, LoadFarString(EnvOptFormat),
                  LoadFarStringSmall(EnvZipInfo),
                  (envptr == (char *)NULL || *envptr == 0)?
                  LoadFarStringSmall2(None) : envptr));
                envptr = getenv(LoadFarStringSmall(EnvZipInfo2));
                Info(slide, 0, ((char *)slide, LoadFarString(EnvOptFormat),
                  LoadFarStringSmall(EnvZipInfo2),
                  (envptr == (char *)NULL || *envptr == 0)?
                  LoadFarStringSmall2(None) : envptr));
#ifdef __EMX__
                envptr = getenv(LoadFarStringSmall(EnvEMX));
                Info(slide, 0, ((char *)slide, LoadFarString(EnvOptFormat),
                  LoadFarStringSmall(EnvEMX),
                  (envptr == (char *)NULL || *envptr == 0)?
                  LoadFarStringSmall2(None) : envptr));
                envptr = getenv(LoadFarStringSmall(EnvEMXOPT));
                Info(slide, 0, ((char *)slide, LoadFarString(EnvOptFormat),
                  LoadFarStringSmall(EnvEMXOPT),
                  (envptr == (char *)NULL || *envptr == 0)?
                  LoadFarStringSmall2(None) : envptr));
#endif /* __EMX__ */
#ifdef __GO32__
                envptr = getenv(LoadFarStringSmall(EnvGO32));
                Info(slide, 0, ((char *)slide, LoadFarString(EnvOptFormat),
                  LoadFarStringSmall(EnvGO32),
                  (envptr == (char *)NULL || *envptr == 0)?
                  LoadFarStringSmall2(None) : envptr));
                envptr = getenv(LoadFarStringSmall(EnvGO32TMP));
                Info(slide, 0, ((char *)slide, LoadFarString(EnvOptFormat),
                  LoadFarStringSmall(EnvGO32TMP),
                  (envptr == (char *)NULL || *envptr == 0)?
                  LoadFarStringSmall2(None) : envptr));
#endif /* __GO32__ */
            }
            return 0;
        } else
#endif /* !SFX */
            return usage(__G__ error);
    }

#ifdef SFX
    /* print our banner unless we're being fairly quiet */
    if (G.qflag < 2)
        Info(slide, error? 1 : 0, ((char *)slide, LoadFarString(UnzipSFXBanner),
          UZ_MAJORVER, UZ_MINORVER, PATCHLEVEL, BETALEVEL,
          LoadFarStringSmall(VersionDate)));
#ifdef BETA
    /* always print the beta warning:  no unauthorized distribution!! */
    Info(slide, error? 1 : 0, ((char *)slide, LoadFarString(BetaVersion), "\n",
      "SFX"));
#endif
#endif /* SFX */

    if (G.cflag || G.tflag || G.vflag)
        G.extract_flag = FALSE;
    else
        G.extract_flag = TRUE;

#if (!defined(SFX) || defined(SFX_EXDIR))
    if (G.dflag) {
        if (G.extract_flag) {
            G.create_dirs = !G.fflag;
            if ((error = checkdir(__G__ exdir, ROOT)) > 2) {
                return(error);  /* out of memory, or file in way */
            }
        } else
            Info(slide, 0x401, ((char *)slide,
              LoadFarString(NotExtracting)));  /* -d ignored */
    }
#endif /* !SFX || SFX_EXDIR */

    *pargc = argc;
    *pargv = argv;
    return 0;

} /* end function uz_opts() */





/********************/
/* Function usage() */
/********************/

#ifdef SFX
#  ifdef VMS
#    define LOCAL "X.  Quote uppercase options"
#  endif
#  ifdef UNIX
#    define LOCAL "X"
#  endif
#  ifdef DOS_OS2_W32
#    define LOCAL "s$"
#  endif
#  ifdef AMIGA
#    define LOCAL "N"
#  endif
   /* Default for all other systems: */
#  ifndef LOCAL
#    define LOCAL ""
#  endif

#  ifdef MORE
#    define SFXOPT1 "M"
#  else
#    define SFXOPT1 ""
#  endif

int usage(__G__ error)   /* return PK-type error code */
    int error;
     __GDEF
{
    Info(slide, error? 1 : 0, ((char *)slide, LoadFarString(UnzipSFXBanner),
      UZ_MAJORVER, UZ_MINORVER, PATCHLEVEL, BETALEVEL,
      LoadFarStringSmall(VersionDate)));
    Info(slide, error? 1 : 0, ((char *)slide, LoadFarString(UnzipSFXOpts),
      SFXOPT1, LOCAL));
#ifdef BETA
    Info(slide, error? 1 : 0, ((char *)slide, LoadFarString(BetaVersion), "\n",
      "SFX"));
#endif

    if (error)
        return PK_PARAM;
    else
        return PK_COOL;     /* just wanted usage screen: no error */

} /* end function usage() */





#else /* !SFX */
#ifndef VMSCLI
#  ifdef VMS
#    define QUOT '\"'
#    define QUOTS "\""
#  else
#    define QUOT ' '
#    define QUOTS ""
#  endif

int usage(__G__ error)   /* return PK-type error code */
    int error;
    __GDEF
{
    int flag = (error? 1 : 0);


/*---------------------------------------------------------------------------
    Print either ZipInfo usage or UnZip usage, depending on incantation.
    (Strings must be no longer than 512 bytes for Turbo C, apparently.)
  ---------------------------------------------------------------------------*/

    if (G.zipinfo_mode) {

#ifndef NO_ZIPINFO

        Info(slide, flag, ((char *)slide, LoadFarString(ZipInfoUsageLine1),
          ZI_MAJORVER, ZI_MINORVER, PATCHLEVEL, BETALEVEL,
          LoadFarStringSmall(VersionDate),
          LoadFarStringSmall2(ZipInfoExample), QUOTS,QUOTS));
        Info(slide, flag, ((char *)slide, LoadFarString(ZipInfoUsageLine2)));
        Info(slide, flag, ((char *)slide, LoadFarString(ZipInfoUsageLine3),
          QUOT,QUOT, LoadFarStringSmall(ZipInfoUsageLine4)));
#ifdef VMS
        Info(slide, flag, ((char *)slide, "\nRemember that non-lowercase\
 filespecs must be quoted in VMS (e.g., \"Makefile\").\n"));
#endif

#endif /* !NO_ZIPINFO */

    } else {   /* UnZip mode */

        Info(slide, flag, ((char *)slide, LoadFarString(UnzipUsageLine1),
          UZ_MAJORVER, UZ_MINORVER, PATCHLEVEL, BETALEVEL,
          LoadFarStringSmall(VersionDate)));
#ifdef BETA
        Info(slide, flag, ((char *)slide, LoadFarString(BetaVersion), "", ""));
#endif
        Info(slide, flag, ((char *)slide, LoadFarString(UnzipUsageLine2),
          ZIPINFO_MODE_OPTION, LoadFarStringSmall(ZipInfoMode)));

        Info(slide, flag, ((char *)slide, LoadFarString(UnzipUsageLine3),
          LoadFarStringSmall(local1)));

        Info(slide, flag, ((char *)slide, LoadFarString(UnzipUsageLine4),
          QUOT,QUOT, QUOT,QUOT, LoadFarStringSmall(local2), QUOT,QUOT,
          LoadFarStringSmall2(local3)));

        /* This is extra work for SMALL_MEM, but it will work since
         * LoadFarStringSmall2 uses the same buffer.  Remember, this
         * is a hack. */
        Info(slide, flag, ((char *)slide, LoadFarString(UnzipUsageLine5),
          LoadFarStringSmall(Example2), LoadFarStringSmall2(Example3),
          LoadFarStringSmall2(Example3)));

    } /* end if (G.zipinfo_mode) */

    if (error)
        return PK_PARAM;
    else
        return PK_COOL;     /* just wanted usage screen: no error */

} /* end function usage() */

#endif /* !VMSCLI */
#endif /* ?SFX */
