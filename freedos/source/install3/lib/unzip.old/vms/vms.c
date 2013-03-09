/*---------------------------------------------------------------------------

  vms.c                                        Igor Mandrichenko and others

  This file contains routines to extract VMS file attributes from a zipfile
  extra field and create a file with these attributes.  The code was almost
  entirely written by Igor, with a couple of routines by CN and lots of
  modifications by Christian Spieler.

  Contains:  check_format()
             open_outfile()
             find_vms_attrs()
             flush()
             close_outfile()
             do_wild()
             mapattr()
             mapname()
             checkdir()
             check_for_newer()
             return_VMS
             screenlines()
             version()

  ---------------------------------------------------------------------------

     Copyright (C) 1992-93 Igor Mandrichenko.
     Permission is granted to any individual or institution to use, copy,
     or redistribute this software so long as all of the original files
     are included unmodified and that this copyright notice is retained.

  Revision history:

     1.x   [moved to History.510 for brevity]
     2.x   [moved to History.513 for brevity]
     3.0   Cave Newt           4 Sep 94
            relocated version history; renamed secinf to X_flag

  ---------------------------------------------------------------------------*/

#ifdef VMS                      /* VMS only! */

#define UNZIP_INTERNAL
#include "unzip.h"
#include "vms.h"
#include "vmsdefs.h"
#include <lib$routines.h>
#include <unixlib.h>

#define BUFS512 8192*2          /* Must be a multiple of 512 */

#define OK(s)   ((s)&1)         /* VMS success or warning status */
#define STRICMP(s1,s2)  STRNICMP(s1,s2,2147483647)

/*
*   Local static storage
*/
static struct FAB       fileblk;
static struct XABDAT    dattim;
static struct XABRDT    rdt;
static struct RAB       rab;
static struct NAM       nam;

static struct FAB *outfab = NULL;
static struct RAB *outrab = NULL;
static struct XABFHC *xabfhc = NULL;
static struct XABDAT *xabdat = NULL;
static struct XABRDT *xabrdt = NULL;
static struct XABPRO *xabpro = NULL;
static struct XABKEY *xabkey = NULL;
static struct XABALL *xaball = NULL;
static struct XAB *first_xab = NULL, *last_xab = NULL;

static char query = 0;
static int  text_output = 0,
            bin_fixed = 0;
#ifdef USE_ORIG_DOS
static int  hostnum;
#endif

static uch rfm;

static uch locbuf[BUFS512];
static int loccnt = 0;
static uch *locptr;
static char got_eol = 0;

static int  _flush_blocks(__GPRO__ uch *rawbuf, unsigned size, int final_flag),
            _flush_stream(__GPRO__ uch *rawbuf, unsigned size, int final_flag),
            _flush_varlen(__GPRO__ uch *rawbuf, unsigned size, int final_flag),
            _flush_qio(__GPRO__ uch *rawbuf, unsigned size, int final_flag),
            _close_rms(__GPRO),
            _close_qio(__GPRO),
            WriteBuffer(__GPRO__ uch *buf, int len),
            WriteRecord(__GPRO__ uch *rec, int len),
            find_eol(uch *p, int n, int *l);

static int  (*_flush_routine)(__GPRO__ uch *rawbuf, unsigned size,
                              int final_flag),
            (*_close_routine)(__GPRO);

static void init_buf_ring(void);
static void set_default_datetime_XABs(__GPRO);
static int  replace(__GPRO);
static void free_up(void);
#ifdef CHECK_VERSIONS
static int  get_vms_version(char *verbuf, int len);
#endif /* CHECK_VERSIONS */
static uch  *extract_block(__GPRO__ struct IZ_block *p, int *retlen,
                           uch *init, int needlen);
static void decompress_bits(uch *outptr, int needlen, uch *bitptr);
static void vms_msg(__GPRO__ char *string, int status);

struct bufdsc
{
    struct bufdsc *next;
    uch *buf;
    int bufcnt;
};

static struct bufdsc b1, b2, *curbuf;
static uch buf1[BUFS512];

int check_format(__G)
    __GDEF
{
    int rtype;
    struct FAB fab;

    fab = cc$rms_fab;
    fab.fab$l_fna = G.zipfn;
    fab.fab$b_fns = strlen(G.zipfn);

    if ((sys$open(&fab) & 1) == 0)
    {
        Info(slide, 1, ((char *)slide, "\n\
     error:  cannot open zipfile [ %s ] (access denied?).\n\n",
          G.zipfn));
        return PK_ERR;
    }
    rtype = fab.fab$b_rfm;
    sys$close(&fab);

    if (rtype == FAB$C_VAR || rtype == FAB$C_VFC)
    {
        Info(slide, 1, ((char *)slide, "\n\
     Error:  zipfile is in variable-length record format.  Please\n\
     run \"bilf l %s\" to convert the zipfile to stream-LF\n\
     record format.  (BILF is available at various VMS archives.)\n\n",
          G.zipfn));
        return PK_ERR;
    }

    return PK_COOL;
}



#define PRINTABLE_FORMAT(x)      ( (x) == FAB$C_VAR     \
                                || (x) == FAB$C_STMLF   \
                                || (x) == FAB$C_STMCR   \
                                || (x) == FAB$C_STM     )

/* VMS extra field types */
#define VAT_NONE    0
#define VAT_IZ      1   /* old INFO-ZIP format */
#define VAT_PK      2   /* PKWARE format */

static int  vet;

static int  create_default_output(__GPRO),
            create_rms_output(__GPRO),
            create_qio_output(__GPRO);

/*
 *  open_outfile() assignments:
 *
 *  VMS attributes ?        create_xxx      _flush_xxx
 *  ----------------        ----------      ----------
 *  not found               'default'       text mode ?
 *                                          yes -> 'stream'
 *                                          no  -> 'block'
 *
 *  yes, in IZ format       'rms'           G.cflag ?
 *                                          yes -> switch(fab.rfm)
 *                                              VAR  -> 'varlen'
 *                                              STM* -> 'stream'
 *                                              default -> 'block'
 *                                          no -> 'block'
 *
 *  yes, in PK format       'qio'           G.cflag ?
 *                                          yes -> switch(pka_rattr)
 *                                              VAR  -> 'varlen'
 *                                              STM* -> 'stream'
 *                                              default -> 'block'
 *                                          no -> 'qio'
 *
 *  "text mode" == G.pInfo->textmode || G.cflag
 */

int open_outfile(__G)           /* return 1 (PK_WARN) if fail */
    __GDEF
{
    switch(vet = find_vms_attrs(__G))
    {
        case VAT_NONE:
        default:
            return  create_default_output(__G);
        case VAT_IZ:
            return  create_rms_output(__G);
        case VAT_PK:
            return  create_qio_output(__G);
    }
}

static void init_buf_ring()
{
    locptr = &locbuf[0];
    loccnt = 0;

    b1.buf = &locbuf[0];
    b1.bufcnt = 0;
    b1.next = &b2;
    b2.buf = &buf1[0];
    b2.bufcnt = 0;
    b2.next = &b1;
    curbuf = &b1;
}


/* Static data storage for time conversion: */

/*   string constants for month names */
static const char *month[] =
            {"JAN", "FEB", "MAR", "APR", "MAY", "JUN",
             "JUL", "AUG", "SEP", "OCT", "NOV", "DEC"};

/*   buffer for time string */
static char timbuf[24];         /* length = first entry in "stupid" + 1 */

/*   fixed-length string descriptor for timbuf: */
static const struct dsc$descriptor stupid =
            {sizeof(timbuf)-1, DSC$K_DTYPE_T, DSC$K_CLASS_S, timbuf};


static void set_default_datetime_XABs(__GPRO)
{
    unsigned yr, mo, dy, hh, mm, ss;
#ifdef USE_EF_UX_TIME
    ztimbuf z_utime;

    if (G.extra_field &&
        ef_scan_for_izux(G.extra_field, G.crec.extra_field_length,
                         &z_utime, NULL) > 0)
    {
        struct tm *t = localtime(&(z_utime.modtime));

        yr = t->tm_year + 1900;
        mo = t->tm_mon;
        dy = t->tm_mday;
        hh = t->tm_hour;
        mm = t->tm_min;
        ss = t->tm_sec;
    }
    else
    {
        yr = ((G.lrec.last_mod_file_date >> 9) & 0x7f) + 1980;
        mo = ((G.lrec.last_mod_file_date >> 5) & 0x0f) - 1;
        dy = (G.lrec.last_mod_file_date & 0x1f);
        hh = (G.lrec.last_mod_file_time >> 11) & 0x1f;
        mm = (G.lrec.last_mod_file_time >> 5) & 0x3f;
        ss = (G.lrec.last_mod_file_time & 0x1f) * 2;
    }
#else /* !USE_EF_UX_TIME */

    yr = ((G.lrec.last_mod_file_date >> 9) & 0x7f) + 1980;
    mo = ((G.lrec.last_mod_file_date >> 5) & 0x0f) - 1;
    dy = (G.lrec.last_mod_file_date & 0x1f);
    hh = (G.lrec.last_mod_file_time >> 11) & 0x1f;
    mm = (G.lrec.last_mod_file_time >> 5) & 0x3f;
    ss = (G.lrec.last_mod_file_time & 0x1f) * 2;
#endif /* ?USE_EF_UX_TIME */

    dattim = cc$rms_xabdat;     /* fill XABs with default values */
    rdt = cc$rms_xabrdt;
    sprintf(timbuf, "%02u-%3s-%04u %02u:%02u:%02u.00", dy, month[mo],
            yr, hh, mm, ss);
    sys$bintim(&stupid, &dattim.xab$q_cdt);
    memcpy(&rdt.xab$q_rdt, &dattim.xab$q_cdt, sizeof(rdt.xab$q_rdt));
}


static int create_default_output(__GPRO)        /* return 1 (PK_WARN) if fail */
{
    int ierr;

    text_output = G.pInfo->textmode ||
                  G.cflag;      /* extract the file in text
                                 * (variable-length) format */
    bin_fixed = text_output || (G.bflag == 0)
                ? 0
                : ((G.bflag - 1) ? 1 : !G.pInfo->textfile);
#ifdef USE_ORIG_DOS
    hostnum = G.pInfo -> hostnum;
#endif

    rfm = FAB$C_STMLF;  /* Default, stream-LF format from VMS or UNIX */

    if (!G.cflag)               /* Redirect output */
    {
        rab = cc$rms_rab;       /* fill RAB with default values */
        fileblk = cc$rms_fab;   /* fill FAB with default values */

        outfab = &fileblk;
        outfab->fab$l_xab = NULL;

        if (text_output)
        {   /* Default format for output `real' text file */

            outfab->fab$b_rfm = FAB$C_VAR;      /* variable length records */
            outfab->fab$b_rat = FAB$M_CR;       /* implied (CR) carriage ctrl */
        }
        else if (bin_fixed)
        {   /* Default format for output `real' binary file */

            outfab->fab$b_rfm = FAB$C_FIX;      /* fixed length record format */
            outfab->fab$w_mrs = 512;            /* record size 512 bytes */
            outfab->fab$b_rat = 0;              /* no carriage ctrl */
        }
        else
        {   /* Default format for output misc (bin or text) file */

            outfab->fab$b_rfm = FAB$C_STMLF;    /* stream-LF record format */
            outfab->fab$b_rat = FAB$M_CR;       /* implied (CR) carriage ctrl */
        }

        outfab->fab$l_fna = G.filename;
        outfab->fab$b_fns = strlen(outfab->fab$l_fna);

        {
            set_default_datetime_XABs(__G);

            dattim.xab$l_nxt = outfab->fab$l_xab;
            outfab->fab$l_xab = (void *) &dattim;
        }

        outfab->fab$w_ifi = 0;  /* Clear IFI. It may be nonzero after ZIP */

        ierr = sys$create(outfab);
        if (ierr == RMS$_FEX)
            ierr = replace(__G);

        if (ierr == 0)          /* Canceled */
            return (free_up(), PK_WARN);

        if (ERR(ierr))
        {
            char buf[256];

            sprintf(buf, "[ Cannot create output file %s ]\n", G.filename);
            vms_msg(__G__ buf, ierr);
            vms_msg(__G__ "", outfab->fab$l_stv);
            free_up();
            return PK_WARN;
        }

        if (!text_output)    /* Do not reopen text files and stdout
                              *  Just open them in right mode         */
        {
            /*
             *      Reopen file for Block I/O with no XABs.
             */
            if ((ierr = sys$close(outfab)) != RMS$_NORMAL)
            {
#ifdef DEBUG
                vms_msg(__G__ "[ create_default_output: sys$close failed ]\n",
                        ierr);
                vms_msg(__G__ "", outfab->fab$l_stv);
#endif
                Info(slide, 1, ((char *)slide,
                     "Can't create output file:  %s\n", G.filename));
                free_up();
                return PK_WARN;
            }


            outfab->fab$b_fac = FAB$M_BIO | FAB$M_PUT;  /* Get ready for block
                                                         * output */
            outfab->fab$l_xab = NULL;   /* Unlink all XABs */

            if ((ierr = sys$open(outfab)) != RMS$_NORMAL)
            {
                char buf[256];

                sprintf(buf, "[ Cannot open output file %s ]\n", G.filename);
                vms_msg(__G__ buf, ierr);
                vms_msg(__G__ "", outfab->fab$l_stv);
                free_up();
                return PK_WARN;
            }
        }

        outrab = &rab;
        rab.rab$l_fab = outfab;
        if (!text_output)
        {
            rab.rab$l_rop |= RAB$M_BIO;
            rab.rab$l_rop |= RAB$M_ASY;
        }
        rab.rab$b_rac = RAB$C_SEQ;

        if ((ierr = sys$connect(outrab)) != RMS$_NORMAL)
        {
#ifdef DEBUG
            vms_msg(__G__ "create_default_output: sys$connect failed.\n", ierr);
            vms_msg(__G__ "", outfab->fab$l_stv);
#endif
            Info(slide, 1, ((char *)slide,
                 "Can't create output file:  %s\n", G.filename));
            free_up();
            return PK_WARN;
        }
    }                   /* end if (!G.cflag) */

    init_buf_ring();

    _flush_routine = text_output? got_eol=0,_flush_stream : _flush_blocks;
    _close_routine = _close_rms;
    return PK_COOL;
}



static int create_rms_output(__GPRO)           /* return 1 (PK_WARN) if fail */
{
    int ierr;

    text_output = G.cflag;      /* extract the file in text
                                 * (variable-length) format;
                                 * we ignore "-a" when attributes saved */
#ifdef USE_ORIG_DOS
    hostnum = G.pInfo -> hostnum;
#endif

    rfm = outfab->fab$b_rfm;    /* Use record format from VMS extra field */

    if (G.cflag)
    {
        if (!PRINTABLE_FORMAT(rfm))
        {
            Info(slide, 1, ((char *)slide,
               "[ File %s has illegal record format to put to screen ]\n",
               G.filename));
            free_up();
            return PK_WARN;
        }
    }
    else                        /* Redirect output */
    {
        rab = cc$rms_rab;       /* fill RAB with default values */

        /* The output FAB has already been initialized with the values
         * found in the Zip file's "VMS attributes" extra field */

        outfab->fab$l_fna = G.filename;
        outfab->fab$b_fns = strlen(outfab->fab$l_fna);

        if (!(xabdat && xabrdt))        /* Use date/time info
                                         *  from zipfile if
                                         *  no attributes given
                                         */
        {
            set_default_datetime_XABs(__G);

            if (xabdat == NULL)
            {
                dattim.xab$l_nxt = outfab->fab$l_xab;
                outfab->fab$l_xab = (void *) &dattim;
            }
        }

        outfab->fab$w_ifi = 0;  /* Clear IFI. It may be nonzero after ZIP */

        ierr = sys$create(outfab);
        if (ierr == RMS$_FEX)
            ierr = replace(__G);

        if (ierr == 0)          /* Canceled */
            return (free_up(), PK_WARN);

        if (ERR(ierr))
        {
            char buf[256];

            sprintf(buf, "[ Cannot create output file %s ]\n", G.filename);
            vms_msg(__G__ buf, ierr);
            vms_msg(__G__ "", outfab->fab$l_stv);
            free_up();
            return PK_WARN;
        }

        if (!text_output)    /* Do not reopen text files and stdout
                              *  Just open them in right mode         */
        {
            /*
             *      Reopen file for Block I/O with no XABs.
             */
            if ((ierr = sys$close(outfab)) != RMS$_NORMAL)
            {
#ifdef DEBUG
                vms_msg(__G__ "[ create_rms_output: sys$close failed ]\n",
                        ierr);
                vms_msg(__G__ "", outfab->fab$l_stv);
#endif
                Info(slide, 1, ((char *)slide,
                     "Can't create output file:  %s\n", G.filename));
                free_up();
                return PK_WARN;
            }


            outfab->fab$b_fac = FAB$M_BIO | FAB$M_PUT;  /* Get ready for block
                                                         * output */
            outfab->fab$l_xab = NULL;   /* Unlink all XABs */

            if ((ierr = sys$open(outfab)) != RMS$_NORMAL)
            {
                char buf[256];

                sprintf(buf, "[ Cannot open output file %s ]\n", G.filename);
                vms_msg(__G__ buf, ierr);
                vms_msg(__G__ "", outfab->fab$l_stv);
                free_up();
                return PK_WARN;
            }
        }

        outrab = &rab;
        rab.rab$l_fab = outfab;
        if (!text_output)
        {
            rab.rab$l_rop |= RAB$M_BIO;
            rab.rab$l_rop |= RAB$M_ASY;
        }
        rab.rab$b_rac = RAB$C_SEQ;

        if ((ierr = sys$connect(outrab)) != RMS$_NORMAL)
        {
#ifdef DEBUG
            vms_msg(__G__ "create_rms_output: sys$connect failed.\n", ierr);
            vms_msg(__G__ "", outfab->fab$l_stv);
#endif
            Info(slide, 1, ((char *)slide,
                 "Can't create output file:  %s\n", G.filename));
            free_up();
            return PK_WARN;
        }
    }                   /* end if (!G.cflag) */

    init_buf_ring();

    if ( text_output )
        switch(rfm)
        {
            case FAB$C_VAR:
                _flush_routine = _flush_varlen;
                break;
            case FAB$C_STM:
            case FAB$C_STMCR:
            case FAB$C_STMLF:
                _flush_routine = _flush_stream;
                got_eol = 0;
                break;
            default:
                _flush_routine = _flush_blocks;
                break;
        }
    else
        _flush_routine = _flush_blocks;
    _close_routine = _close_rms;
    return PK_COOL;
}



static  int pka_devchn;
static  int pka_vbn;

static struct
{
    short   status;
    long    count;
    short   dummy;
} pka_io_sb;

static struct
{
    short   status;
    short   dummy;
    void    *addr;
} pka_acp_sb;

static struct fibdef    pka_fib;
static struct atrdef    pka_atr[VMS_MAX_ATRCNT];
static int              pka_idx;
static ulg              pka_uchar;
static struct fatdef    pka_rattr;

static struct dsc$descriptor    pka_fibdsc =
{   sizeof(pka_fib), DSC$K_DTYPE_Z, DSC$K_CLASS_S, (void *) &pka_fib  };

static struct dsc$descriptor_s  pka_devdsc =
{   0, DSC$K_DTYPE_T, DSC$K_CLASS_S, &nam.nam$t_dvi[1]  };

static struct dsc$descriptor_s  pka_fnam =
{   0, DSC$K_DTYPE_T, DSC$K_CLASS_S, NULL  };

#define PK_PRINTABLE_RECTYP(x)   ( (x) == FAT$C_VARIABLE \
                                || (x) == FAT$C_STREAMLF \
                                || (x) == FAT$C_STREAMCR \
                                || (x) == FAT$C_STREAM   )


static int create_qio_output(__GPRO)            /* return 1 (PK_WARN) if fail */
{
    int status;
    static char exp_nam[NAM$C_MAXRSS];
    static char res_nam[NAM$C_MAXRSS];
    int i;

    if ( G.cflag )
    {
        int rtype = pka_rattr.fat$v_rtype;
        if (!PK_PRINTABLE_RECTYP(rtype))
        {
            Info(slide, 1, ((char *)slide,
               "[ File %s has illegal record format to put to screen ]\n",
               G.filename));
            return PK_WARN;
        }

        init_buf_ring();

        switch(rtype)
        {
            case FAT$C_VARIABLE:
                _flush_routine = _flush_varlen;
                break;
            case FAT$C_STREAM:
            case FAT$C_STREAMCR:
            case FAT$C_STREAMLF:
                _flush_routine = _flush_stream;
                got_eol = 0;
                break;
            default:
                _flush_routine = _flush_blocks;
                break;
        }
        _close_routine = _close_rms;
    }
    else                        /* !(G.cflag) : redirect output */
    {

        fileblk = cc$rms_fab;
        fileblk.fab$l_fna = G.filename;
        fileblk.fab$b_fns = strlen(G.filename);

        nam = cc$rms_nam;
        fileblk.fab$l_nam = &nam;
        nam.nam$l_esa = exp_nam;
        nam.nam$b_ess = sizeof(exp_nam);
        nam.nam$l_rsa = res_nam;
        nam.nam$b_rss = sizeof(res_nam);

        if ( ERR(status = sys$parse(&fileblk)) )
        {
            vms_msg(__G__ "create_qio_output: sys$parse failed.\n", status);
            return PK_WARN;
        }

        pka_devdsc.dsc$w_length = (unsigned short)nam.nam$t_dvi[0];

        if ( ERR(status = sys$assign(&pka_devdsc,&pka_devchn,0,0)) )
        {
            vms_msg(__G__ "sys$assign failed.\n",status);
            return PK_WARN;
        }

        pka_fnam.dsc$a_pointer = nam.nam$l_name;
        pka_fnam.dsc$w_length  = nam.nam$b_name + nam.nam$b_type;
        if ( G.V_flag /* keep versions */ )
            pka_fnam.dsc$w_length += nam.nam$b_ver;

        for (i=0;i<3;i++)
        {
            pka_fib.FIB$W_DID[i]=nam.nam$w_did[i];
            pka_fib.FIB$W_FID[i]=0;
        }

        pka_fib.FIB$L_ACCTL = FIB$M_WRITE;
        /* Allocate space for the file */
        pka_fib.FIB$W_EXCTL = FIB$M_EXTEND;
        if ( pka_uchar & FCH$M_CONTIG )
            pka_fib.FIB$W_EXCTL |= FIB$M_ALCON | FIB$M_FILCON;
        if ( pka_uchar & FCH$M_CONTIGB )
            pka_fib.FIB$W_EXCTL |= FIB$M_ALCONB;

#define SWAPW(x)        ( (((x)>>16)&0xFFFF) + ((x)<<16) )

        pka_fib.fib$l_exsz = SWAPW(pka_rattr.fat$l_hiblk);

        status = sys$qiow(0, pka_devchn, IO$_CREATE|IO$M_CREATE|IO$M_ACCESS,
                          &pka_acp_sb, 0, 0,
                          &pka_fibdsc, &pka_fnam, 0, 0, &pka_atr, 0);

        if ( !ERR(status) )
            status = pka_acp_sb.status;

        if ( ERR(status) )
        {
            vms_msg(__G__ "[ Create file QIO failed.\n",status);
            return PK_WARN;
            sys$dassgn(pka_devchn);
        }

        locptr = locbuf;
        loccnt = 0;
        pka_vbn = 1;
        _flush_routine = _flush_qio;
        _close_routine = _close_qio;
    }                   /* end if (!G.cflag) */
    return PK_COOL;
}



static int replace(__GPRO)
{                       /*
                         *      File exists. Inquire user about further action.
                         */
    char answ[10];
    struct NAM nam;
    int ierr;

    if (query == 0)
    {
        do
        {
            Info(slide, 1, ((char *)slide,
                 "%s exists:  [o]verwrite, new [v]ersion or [n]o extract?\n\
  (uppercase response [O,V,N] = do same for all files): ",
                 G.filename));
            fflush(stderr);
        } while (fgets(answ, 9, stderr) == NULL && !isalpha(answ[0])
                 && tolower(answ[0]) != 'o'
                 && tolower(answ[0]) != 'v'
                 && tolower(answ[0]) != 'n');

        if (isupper(answ[0]))
            query = answ[0] = tolower(answ[0]);
    }
    else
        answ[0] = query;

    switch (answ[0])
    {
        case 'n':
            ierr = 0;
            break;
        case 'v':
            nam = cc$rms_nam;
            nam.nam$l_rsa = G.filename;
            nam.nam$b_rss = FILNAMSIZ - 1;

            outfab->fab$l_fop |= FAB$M_MXV;
            outfab->fab$l_nam = &nam;

            ierr = sys$create(outfab);
            if (!ERR(ierr))
            {
                outfab->fab$l_nam = NULL;
                G.filename[outfab->fab$b_fns = nam.nam$b_rsl] = '\0';
            }
            break;
        case 'o':
            outfab->fab$l_fop |= FAB$M_SUP;
            ierr = sys$create(outfab);
            break;
    }
    return ierr;
}



#define W(p)    (*(unsigned short*)(p))
#define L(p)    (*(unsigned long*)(p))
#define EQL_L(a,b)      ( L(a) == L(b) )
#define EQL_W(a,b)      ( W(a) == W(b) )

/****************************************************************
 * Function find_vms_attrs scans ZIP entry extra field if any   *
 * and looks for VMS attribute records. Returns 0 if either no  *
 * attributes found or no fab given.                            *
 ****************************************************************/
int find_vms_attrs(__G)
    __GDEF
{
    uch *scan = G.extra_field;
    struct  EB_header *hdr;
    int len;
    int type=VAT_NONE;

    outfab = NULL;
    xabfhc = NULL;
    xabdat = NULL;
    xabrdt = NULL;
    xabpro = NULL;
    first_xab = last_xab = NULL;

    if (scan == NULL)
        return VAT_NONE;
    len = G.lrec.extra_field_length;

#define LINK(p) {/* Link xaballs and xabkeys into chain */      \
                if ( first_xab == NULL )                \
                        first_xab = (void *) p;         \
                if ( last_xab != NULL )                 \
                        last_xab -> xab$l_nxt = (void *) p;     \
                last_xab = (void *) p;                  \
                p -> xab$l_nxt = NULL;                  \
        }
    /* End of macro LINK */

    while (len > 0)
    {
        hdr = (struct EB_header *) scan;
        if (EQL_W(&hdr->tag, IZ_SIGNATURE))
        {
            /*
             *  INFO-ZIP style extra block decoding
             */
            struct IZ_block *blk;
            uch *block_id;

            type = VAT_IZ;

            blk = (struct IZ_block *)hdr;
            block_id = (uch *) &blk->bid;
            if (EQL_L(block_id, FABSIG))
            {
                outfab = (struct FAB *) extract_block(__G__ blk, 0,
                                                (uch *)&cc$rms_fab, FABL);
            }
            else if (EQL_L(block_id, XALLSIG))
            {
                xaball = (struct XABALL *) extract_block(__G__ blk, 0,
                                                (uch *)&cc$rms_xaball, XALLL);
                LINK(xaball);
            }
            else if (EQL_L(block_id, XKEYSIG))
            {
                xabkey = (struct XABKEY *) extract_block(__G__ blk, 0,
                                                (uch *)&cc$rms_xabkey, XKEYL);
                LINK(xabkey);
            }
            else if (EQL_L(block_id, XFHCSIG))
            {
                xabfhc = (struct XABFHC *) extract_block(__G__ blk, 0,
                                                (uch *)&cc$rms_xabfhc, XFHCL);
            }
            else if (EQL_L(block_id, XDATSIG))
            {
                xabdat = (struct XABDAT *) extract_block(__G__ blk, 0,
                                                (uch *)&cc$rms_xabdat, XDATL);
            }
            else if (EQL_L(block_id, XRDTSIG))
            {
                xabrdt = (struct XABRDT *) extract_block(__G__ blk, 0,
                                                (uch *)&cc$rms_xabrdt, XRDTL);
            }
            else if (EQL_L(block_id, XPROSIG))
            {
                xabpro = (struct XABPRO *) extract_block(__G__ blk, 0,
                                                (uch *)&cc$rms_xabpro, XPROL);
            }
            else if (EQL_L(block_id, VERSIG))
            {
#ifdef CHECK_VERSIONS
                char verbuf[80];
                int verlen = 0;
                uch *vers;
                char *m;

                get_vms_version(verbuf, sizeof(verbuf));
                vers = extract_block(__G__ blk, &verlen, 0, 0);
                if ((m = strrchr((char *) vers, '-')) != NULL)
                    *m = '\0';  /* Cut out release number */
                if (strcmp(verbuf, (char *) vers) && G.qflag < 2)
                {
                    Info(slide, 0, ((char *)slide,
                         "[ Warning: VMS version mismatch."));

                    Info(slide, 0, ((char *)slide,
                         "   This version %s --", verbuf));
                    strncpy(verbuf, (char *) vers, verlen);
                    verbuf[verlen] = '\0';
                    Info(slide, 0, ((char *)slide,
                         " version made by %s ]\n", verbuf));
                }
                free(vers);
#endif /* CHECK_VERSIONS */
            }
            else
                Info(slide, 1, ((char *)slide,
                     "[ Warning: Unknown block signature %s ]\n",
                     block_id));
        }
        else if (hdr->tag == PK_SIGNATURE)
        {
            /*
             *  PKWARE style extra block decoding
             */
            struct  PK_header   *blk;
            register byte   *scn;
            register int    len;

            type = VAT_PK;

            blk = (struct PK_header *)hdr;
            len = blk->size - (PK_HEADER_SIZE - EB_HEADSIZE);
            scn = (byte *)(&blk->data);
            pka_idx = 0;

            if (blk->crc32 != crc32(CRCVAL_INITIAL, scn, (extent)len))
            {
                Info(slide, 1, ((char *)slide,
                     "[ Warning: CRC error, discarting PKware extra field]\n"
                     ));
                len = 0;
                type = VAT_NONE;
            }

            while (len > PK_FLDHDR_SIZE)
            {
                register struct  PK_field  *fld;
                int skip=0;

                fld = (struct PK_field *)scn;
                switch(fld->tag)
                {
                    case ATR$C_UCHAR:
                        pka_uchar = L(&fld->value);
                        break;
                    case ATR$C_RECATTR:
                        pka_rattr = *(struct fatdef *)(&fld->value);
                        break;
                    case ATR$C_UIC:
                    case ATR$C_ADDACLENT:
                        skip = !G.X_flag;
                        break;
                }

                if ( !skip )
                {
                    pka_atr[pka_idx].atr$w_size = fld->size;
                    pka_atr[pka_idx].atr$w_type = fld->tag;
                    pka_atr[pka_idx].atr$l_addr = &fld->value;
                    ++pka_idx;
                }
                len -= fld->size + PK_FLDHDR_SIZE;
                scn += fld->size + PK_FLDHDR_SIZE;
            }
            pka_atr[pka_idx].atr$w_size = 0;    /* End of list */
            pka_atr[pka_idx].atr$w_type = 0;
            pka_atr[pka_idx].atr$l_addr = 0; /* NULL when DECC VAX gets fixed */
        }
        len -= hdr->size + EB_HEADSIZE;
        scan += hdr->size + EB_HEADSIZE;
    }


    if ( type == VAT_IZ )
    {
        if (outfab != NULL)
        {   /* Do not link XABPRO,XABRDT now. Leave them for sys$close() */

            outfab->fab$l_xab = NULL;
            if (xabfhc != NULL)
            {
                xabfhc->xab$l_nxt = outfab->fab$l_xab;
                outfab->fab$l_xab = (void *) xabfhc;
            }
            if (xabdat != NULL)
            {
                xabdat->xab$l_nxt = outfab->fab$l_xab;
                outfab->fab$l_xab = (void *) xabdat;
            }
            if (first_xab != NULL)      /* Link xaball,xabkey subchain */
            {
                last_xab->xab$l_nxt = outfab->fab$l_xab;
                outfab->fab$l_xab = (void *) first_xab;
            }
        }
        else
            type = VAT_NONE;
    }
    return type;
}



static void free_up()
{
                                /*
                                 *      Free up all allocated xabs
                                 */
    if (xabdat != NULL) free(xabdat);
    if (xabpro != NULL) free(xabpro);
    if (xabrdt != NULL) free(xabrdt);
    if (xabfhc != NULL) free(xabfhc);
    while (first_xab != NULL)
    {
        struct XAB *x;

        x = (struct XAB *) first_xab->xab$l_nxt;
        free(first_xab);
        first_xab = x;
    }
    if (outfab != NULL && outfab != &fileblk)
        free(outfab);
}



#ifdef CHECK_VERSIONS

static int get_vms_version(verbuf, len)
    char *verbuf;
    int len;
{
    int i = SYI$_VERSION;
    int verlen = 0;
    struct dsc$descriptor version;
    char *m;

    version.dsc$a_pointer = verbuf;
    version.dsc$w_length  = len - 1;
    version.dsc$b_dtype   = DSC$K_DTYPE_B;
    version.dsc$b_class   = DSC$K_CLASS_S;

    if (ERR(lib$getsyi(&i, 0, &version, &verlen, 0, 0)) || verlen == 0)
        return 0;

    /* Cut out trailing spaces "V5.4-3   " -> "V5.4-3" */
    for (m = verbuf + verlen, i = verlen - 1; i > 0 && verbuf[i] == ' '; --i)
        --m;
    *m = '\0';

    /* Cut out release number "V5.4-3" -> "V5.4" */
    if ((m = strrchr(verbuf, '-')) != NULL)
        *m = '\0';
    return strlen(verbuf) + 1;  /* Transmit ending '\0' too */
}

#endif /* CHECK_VERSIONS */



/*
 * Extracts block from p. If resulting length is less then needed, fill
 * extra space with corresponding bytes from 'init'.
 * Currently understands 3 formats of block compression:
 * - Simple storing
 * - Compression of zero bytes to zero bits
 * - Deflation (see memextract() in extract.c)
 */
static uch *extract_block(__G__ p, retlen, init, needlen)
    __GDEF
    struct IZ_block *p;
    int *retlen;
    uch *init;
    int needlen;
{
    uch *block;         /* Pointer to block allocated */
    int cmptype;
    int usiz, csiz, max;

    cmptype = p->flags & BC_MASK;
    csiz = p->size - EXTBSL - RESL;
    usiz = (cmptype == BC_STORED ? csiz : p->length);

    if (needlen == 0)
        needlen = usiz;

    if (retlen)
        *retlen = usiz;

#ifndef MAX
# define MAX(a,b)       ( (a) > (b) ? (a) : (b) )
#endif

    if ((block = (uch *) malloc(MAX(needlen, usiz))) == NULL)
        return NULL;

    if (init && (usiz < needlen))
        memcpy(block, init, needlen);

    switch (cmptype)
    {
        case BC_STORED: /* The simplest case */
            memcpy(block, &(p->body[0]), usiz);
            break;
        case BC_00:
            decompress_bits(block, usiz, &(p->body[0]));
            break;
        case BC_DEFL:
            memextract(__G__ block, usiz, &(p->body[0]), csiz);
            break;
        default:
            free(block);
            block = NULL;
    }
    return block;
}



/*
 *  Simple uncompression routine. The compression uses bit stream.
 *  Compression scheme:
 *
 *  if (byte!=0)
 *      putbit(1),putbyte(byte)
 *  else
 *      putbit(0)
 */
static void decompress_bits(outptr, needlen, bitptr)
    uch *outptr;        /* Pointer into output block */
    int needlen;        /* Size of uncompressed block */
    uch *bitptr;        /* Pointer into compressed data */
{
    ulg bitbuf = 0;
    int bitcnt = 0;

#define _FILL   if (bitcnt+8 <= 32)                     \
                {       bitbuf |= (*bitptr++) << bitcnt;\
                        bitcnt += 8;                    \
                }

    while (needlen--)
    {
        if (bitcnt <= 0)
            _FILL;

        if (bitbuf & 1)
        {
            bitbuf >>= 1;
            if ((bitcnt -= 1) < 8)
                _FILL;
            *outptr++ = (uch) bitbuf;
            bitcnt -= 8;
            bitbuf >>= 8;
        }
        else
        {
            *outptr++ = '\0';
            bitcnt -= 1;
            bitbuf >>= 1;
        }
    }
}



/* flush contents of output buffer */
int flush(__G__ rawbuf, size, unshrink)    /* return PK-type error code */
    __GDEF
    uch *rawbuf;
    ulg size;
    int unshrink;
{
    G.crc32val = crc32(G.crc32val, rawbuf, (extent)size);
    if (G.tflag)
        return PK_COOL; /* Do not output. Update CRC only */
    else
        return (*_flush_routine)(__G__ rawbuf, size, 0);
}



static int _flush_blocks(__G__ rawbuf, size, final_flag)
                                                /* Asynchronous version */
    __GDEF
    uch *rawbuf;
    unsigned size;
    int final_flag;   /* 1 if this is the final flushout */
{
    int round;
    int rest;
    int off = 0;
    int status;

    while (size > 0)
    {
        if (curbuf->bufcnt < BUFS512)
        {
            int ncpy;

            ncpy = size > (BUFS512 - curbuf->bufcnt) ?
                            BUFS512 - curbuf->bufcnt :
                            size;
            memcpy(curbuf->buf + curbuf->bufcnt, rawbuf + off, ncpy);
            size -= ncpy;
            curbuf->bufcnt += ncpy;
            off += ncpy;
        }
        if (curbuf->bufcnt == BUFS512)
        {
            status = WriteBuffer(__G__ curbuf->buf, curbuf->bufcnt);
            if (status)
                return status;
            curbuf = curbuf->next;
            curbuf->bufcnt = 0;
        }
    }

    return (final_flag && (curbuf->bufcnt > 0)) ?
        WriteBuffer(__G__ curbuf->buf, curbuf->bufcnt) :
        PK_COOL;
}



static int _flush_qio(__G__ rawbuf, size, final_flag)
    __GDEF
    uch *rawbuf;
    unsigned size;
    int final_flag;   /* 1 if this is the final flushout */
{
    int status;
    uch *out_ptr=rawbuf;

    if ( final_flag )
    {
        if ( loccnt > 0 )
        {
            status = sys$qiow(0, pka_devchn, IO$_WRITEVBLK,
                              &pka_io_sb, 0, 0,
                              locbuf,
                              (loccnt+1)&(~1), /* Round up to even byte count */
                              pka_vbn,
                              0, 0, 0);
            if (!ERR(status))
                status = pka_io_sb.status;
            if (ERR(status))
            {
                vms_msg(__G__ "[ Write QIO failed ]\n",status);
                return PK_DISK;
            }
        }
        return PK_COOL;
    }

    if ( loccnt > 0 )
    {
        /*
         *   Fill local buffer upto 512 bytes then put it out
         */
        int ncpy;

        ncpy = 512-loccnt;
        if ( ncpy > size )
            ncpy = size;

        memcpy(locptr,rawbuf,ncpy);
        locptr += ncpy;
        loccnt += ncpy;
        size -= ncpy;
        out_ptr += ncpy;
        if ( loccnt == 512 )
        {
            status = sys$qiow(0, pka_devchn, IO$_WRITEVBLK,
                              &pka_io_sb, 0, 0,
                              locbuf, loccnt, pka_vbn,
                              0, 0, 0);
            if (!ERR(status))
                status = pka_io_sb.status;
            if (ERR(status))
            {
                vms_msg(__G__ "[ Write QIO failed ]\n",status);
                return PK_DISK;
            }

            pka_vbn++;
            loccnt = 0;
            locptr = locbuf;
        }
    }

    if ( size >= 512 )
    {
        int nblk,put_cnt;

        /*
         *   Put rest of buffer as a single VB
         */
        put_cnt = (nblk = size>>9)<<9;
        status = sys$qiow(0, pka_devchn, IO$_WRITEVBLK,
                          &pka_io_sb, 0, 0,
                          out_ptr, put_cnt, pka_vbn,
                          0, 0, 0);
        if (!ERR(status))
            status = pka_io_sb.status;
        if (ERR(status))
        {
            vms_msg(__G__ "[ Write QIO failed ]\n",status);
            return PK_DISK;
        }

        pka_vbn += nblk;
        out_ptr += put_cnt;
        size -= put_cnt;
    }

    if ( size > 0 )
    {
        memcpy(locptr,out_ptr,size);
        loccnt += size;
        locptr += size;
    }

    return PK_COOL;
}



static int _flush_varlen(__G__ rawbuf, size, final_flag)
    __GDEF
    uch *rawbuf;
    unsigned size;
    int final_flag;
{
    ush nneed;
    ush reclen;
    uch *inptr=rawbuf;

    /*
     * Flush local buffer
     */

    if ( loccnt > 0 )
    {
        reclen = *(ush*)locbuf;
        if ( (nneed = reclen + 2 - loccnt) > 0 )
        {
            if ( nneed > size )
            {
                if ( size+loccnt > BUFS512 )
                {
                    char buf[80];
                    Info(buf, 1, (buf,
                         "[ Record too long (%d bytes) ]\n",reclen ));
                    return PK_DISK;
                }
                memcpy(locbuf+loccnt,rawbuf,size);
                loccnt += size;
                size = 0;
            }
            else
            {
                memcpy(locbuf+loccnt,rawbuf,nneed);
                loccnt += nneed;
                size -= nneed;
                inptr += nneed;
                if ( reclen & 1 )
                {
                    size--;
                    inptr++;
                }
                if ( WriteRecord(__G__ locbuf+2,reclen) )
                    return PK_DISK;
                loccnt = 0;
            }
        }
        else
        {
            if (WriteRecord(__G__ locbuf+2,reclen))
                return PK_DISK;
            loccnt -= reclen+2;
        }
    }
    /*
     * Flush incoming records
     */
    while (size > 0)
    {
        reclen = *(ush*)inptr;
        if ( reclen+2 <= size )
        {
            if (WriteRecord(__G__ inptr+2,reclen))
                return PK_DISK;
            size -= 2+reclen;
            inptr += 2+reclen;
            if ( reclen & 1)
            {
                --size;
                ++inptr;
            }
        }
        else
        {
            memcpy(locbuf,inptr,size);
            loccnt = size;
            size = 0;
        }

    }
    /*
     * Final flush rest of local buffer
     */
    if ( final_flag && loccnt > 0 )
    {
        char buf[80];

        Info(buf, 1, (buf,
            "[ Warning, incomplete record of length %d ]\n",
            *(ush*)locbuf));
        if ( WriteRecord(__G__ locbuf+2,loccnt-2) )
            return PK_DISK;
    }
    return PK_COOL;
}



/*
*   Routine _flush_stream breaks decompressed stream into records
*   depending on format of the stream (fab->rfm, G.pInfo->textmode, etc.)
*   and puts out these records. It also handles CR LF sequences.
*   Should be used when extracting *text* files.
*/

#define VT      0x0B
#define FF      0x0C

/* The file is from MSDOS/OS2/NT -> handle CRLF as record end, throw out ^Z */

/* GRR NOTES:  cannot depend on hostnum!  May have "flip'd" file or re-zipped
 * a Unix file, etc. */

#ifdef USE_ORIG_DOS
# define ORG_DOS   (hostnum==FS_FAT_ || hostnum==FS_HPFS_ || hostnum==FS_NTFS_)
#else
# define ORG_DOS    1
#endif

/* Record delimiters */
#ifdef undef
#define RECORD_END(c,f)                                                 \
(    ( ORG_DOS || G.pInfo->textmode ) && c==CTRLZ                      \
  || ( f == FAB$C_STMLF && c==LF )                                      \
  || ( f == FAB$C_STMCR || ORG_DOS || G.pInfo->textmode ) && c==CR     \
  || ( f == FAB$C_STM && (c==CR || c==LF || c==FF || c==VT) )           \
)
#else
#   define  RECORD_END(c,f)   ((c) == LF || (c) == (CR))
#endif

static int  find_eol(p,n,l)
/*
 *  Find first CR,LF,CR-LF or LF-CR in string 'p' of length 'n'.
 *  Return offset of the sequence found or 'n' if not found.
 *  If found, return in '*l' length of the sequence (1 or 2) or
 *  zero if sequence end not seen, i.e. CR or LF is last char
 *  in the buffer.
 */
    uch *p;
    int n;
    int *l;
{
    int off = n;
    uch *q;

    *l = 0;

    for (q=p ; n > 0 ; --n,++q)
        if ( RECORD_END(*q,rfm) )
        {
            off = q-p;
            break;
        }

    if ( n > 1 )
    {
        *l = 1;
        if ( ( q[0] == CR && q[1] == LF ) || ( q[0] == LF && q[1] == CR ) )
            *l = 2;
    }

    return off;
}

/* Record delimiters that must be put out */
#define PRINT_SPEC(c)   ( (c)==FF || (c)==VT )



static int _flush_stream(__G__ rawbuf, size, final_flag)
    __GDEF
    uch *rawbuf;
    unsigned size;
    int final_flag; /* 1 if this is the final flushout */
{
    int rest;
    int end = 0, start = 0;
    int off = 0;

    if (size == 0 && loccnt == 0)
        return PK_COOL;         /* Nothing to do ... */

    if ( final_flag )
    {
        int recsize;

        /*
         * This is flush only call. size must be zero now.
         * Just eject everything we have in locbuf.
         */
        recsize = loccnt - (got_eol ? 1:0);
        /*
         *  If the last char of file was ^Z ( end-of-file in MSDOS ),
         *  we will see it now.
         */
        if ( recsize==1 && locbuf[0] == CTRLZ )
            return PK_COOL;

        return WriteRecord(__G__ locbuf, recsize) ? PK_DISK : PK_COOL;
    }


    if ( loccnt > 0 )
    {
        /* Find end of record partially saved in locbuf */

        int recsize;
        int complete=0;

        if ( got_eol )
        {
            recsize = loccnt - 1;
            complete = 1;

            if ( (got_eol == CR && rawbuf[0] == LF) ||
                 (got_eol == LF && rawbuf[0] == CR) )
                end = 1;

            got_eol = 0;
        }
        else
        {
            int eol_len;
            int eol_off;

            eol_off = find_eol(rawbuf,size,&eol_len);

            if ( loccnt+eol_off > BUFS512 )
            {
                /*
                 *  No room in locbuf. Dump it and clear
                 */
                char buf[80];           /* CANNOT use slide for Info() */

                recsize = loccnt;
                start = 0;
                Info(buf, 1, (buf,
                     "[ Warning: Record too long (%d) ]\n", loccnt+eol_off));
                complete = 1;
                end = 0;
            }
            else
            {
                if ( eol_off >= size )
                {
                    end = size;
                    complete = 0;
                }
                else if ( eol_len == 0 )
                {
                    got_eol = rawbuf[eol_off];
                    end = size;
                    complete = 0;
                }
                else
                {
                    memcpy(locptr, rawbuf, eol_off);
                    recsize = loccnt + eol_off;
                    locptr += eol_off;
                    loccnt += eol_off;
                    end = eol_off + eol_len;
                    complete = 1;
                }
            }
        }

        if ( complete )
        {
            if (WriteRecord(__G__ locbuf, recsize))
                return PK_DISK;
            loccnt = 0;
            locptr = locbuf;
        }
    }                           /* end if ( loccnt ) */

    for (start = end; start < size && end < size; )
    {
        int eol_off,eol_len;

        got_eol = 0;

#ifdef undef
        if (G.cflag)
            /* skip CR's at the beginning of record */
            while (start < size && rawbuf[start] == CR)
                ++start;
#endif

        if ( start >= size )
            continue;

        /* Find record end */
        end = start+(eol_off = find_eol(rawbuf+start, size-start, &eol_len));

        if ( end >= size )
            continue;

        if ( eol_len > 0 )
        {
            if ( WriteRecord(__G__ rawbuf+start, end-start) )
                return PK_DISK;
            start = end + eol_len;
        }
        else
        {
            got_eol = rawbuf[end];
            end = size;
            continue;
        }
    }

    rest = size - start;

    if (rest > 0)
    {
        if ( rest > BUFS512 )
        {
            int recsize;
            char buf[80];               /* CANNOT use slide for Info() */

            recsize = rest - (got_eol ? 1:0 );
            Info(buf, 1, (buf,
                 "[ Warning: Record too long (%d) ]\n", recsize));
            got_eol = 0;
            return WriteRecord(__G__ rawbuf+start,recsize) ? PK_DISK : PK_COOL;
        }
        else
        {
            memcpy(locptr, rawbuf + start, rest);
            locptr += rest;
            loccnt += rest;
        }
    }
    return PK_COOL;
}



static int WriteBuffer(__G__ buf, len)
    __GDEF
    uch *buf;
    int len;
{
    int status;

    status = sys$wait(outrab);
    if (ERR(status))
    {
        vms_msg(__G__ "[ WriteBuffer failed ]\n", status);
        vms_msg(__G__ "", outrab->rab$l_stv);
    }
    outrab->rab$w_rsz = len;
    outrab->rab$l_rbf = (char *) buf;

    if (ERR(status = sys$write(outrab)))
    {
        vms_msg(__G__ "[ WriteBuffer failed ]\n", status);
        vms_msg(__G__ "", outrab->rab$l_stv);
        return PK_DISK;
    }
    return PK_COOL;
}



static int WriteRecord(__G__ rec, len)
    __GDEF
    uch *rec;
    int len;
{
    int status;

    if (G.cflag)
    {
        (void)(*G.message)((zvoid *)&G, rec, len, 0);
        (void)(*G.message)((zvoid *)&G, (uch *) ("\n"), 1, 0);
    }
    else
    {
        if (ERR(status = sys$wait(outrab)))
        {
            vms_msg(__G__ "[ WriteRecord failed ]\n", status);
            vms_msg(__G__ "", outrab->rab$l_stv);
        }
        outrab->rab$w_rsz = len;
        outrab->rab$l_rbf = (char *) rec;

        if (ERR(status = sys$put(outrab)))
        {
            vms_msg(__G__ "[ WriteRecord failed ]\n", status);
            vms_msg(__G__ "", outrab->rab$l_stv);
            return PK_DISK;
        }
    }
    return PK_COOL;
}



void close_outfile(__G)
    __GDEF
{
    int status;

    status = (*_flush_routine)(__G__ NULL, 0, 1);
    if (status)
        return /* PK_DISK */;
    if (G.cflag)
        return /* PK_COOL */;   /* Don't close stdout */
    /* return */ (*_close_routine)(__G);
}



static int _close_rms(__GPRO)
{
    int status;
    struct XABPRO pro;

    /* Link XABRDT,XABDAT and optionaly XABPRO */
    if (xabrdt != NULL)
    {
        xabrdt->xab$l_nxt = NULL;
        outfab->fab$l_xab = (void *) xabrdt;
    }
    else
    {
        rdt.xab$l_nxt = NULL;
        outfab->fab$l_xab = (void *) &rdt;
    }
    if (xabdat != NULL)
    {
        xabdat->xab$l_nxt = outfab->fab$l_xab;
        outfab->fab$l_xab = (void *)xabdat;
    }

    if (xabpro != NULL)
    {
        if ( !G.X_flag )
            xabpro->xab$l_uic = 0;    /* Use default (user's) uic */
        xabpro->xab$l_nxt = outfab->fab$l_xab;
        outfab->fab$l_xab = (void *) xabpro;
    }
    else
    {
        pro = cc$rms_xabpro;
        pro.xab$w_pro = G.pInfo->file_attr;
        pro.xab$l_nxt = outfab->fab$l_xab;
        outfab->fab$l_xab = (void *) &pro;
    }

    sys$wait(outrab);

    status = sys$close(outfab);
#ifdef DEBUG
    if (ERR(status))
    {
        vms_msg(__G__
          "\r[ Warning: cannot set owner/protection/time attributes ]\n",
          status);
        vms_msg(__G__ "", outfab->fab$l_stv);
    }
#endif
    free_up();
    return PK_COOL;
}



static int _close_qio(__GPRO)
{
    int status;

    pka_fib.FIB$L_ACCTL =
        FIB$M_WRITE | FIB$M_NOTRUNC ;
    pka_fib.FIB$W_EXCTL = 0;

    pka_fib.FIB$W_FID[0] =
    pka_fib.FIB$W_FID[1] =
    pka_fib.FIB$W_FID[2] =
    pka_fib.FIB$W_DID[0] =
    pka_fib.FIB$W_DID[1] =
    pka_fib.FIB$W_DID[2] = 0;

    status = sys$qiow(0, pka_devchn, IO$_DEACCESS, &pka_acp_sb,
                      0, 0,
                      &pka_fibdsc, 0, 0, 0,
                      &pka_atr, 0);

    sys$dassgn(pka_devchn);
    if ( !ERR(status) )
        status = pka_acp_sb.status;
    if ( ERR(status) )
    {
        vms_msg(__G__ "[ Deaccess QIO failed ]\n",status);
        return PK_DISK;
    }
    return PK_COOL;
}



#ifdef DEBUG
#if 0   /* currently not used anywhere ! */
void dump_rms_block(p)
    unsigned char *p;
{
    unsigned char bid, len;
    int err;
    char *type;
    char buf[132];
    int i;

    err = 0;
    bid = p[0];
    len = p[1];
    switch (bid)
    {
        case FAB$C_BID:
            type = "FAB";
            break;
        case XAB$C_ALL:
            type = "xabALL";
            break;
        case XAB$C_KEY:
            type = "xabKEY";
            break;
        case XAB$C_DAT:
            type = "xabDAT";
            break;
        case XAB$C_RDT:
            type = "xabRDT";
            break;
        case XAB$C_FHC:
            type = "xabFHC";
            break;
        case XAB$C_PRO:
            type = "xabPRO";
            break;
        default:
            type = "Unknown";
            err = 1;
            break;
    }
    printf("Block @%08X of type %s (%d).", p, type, bid);
    if (err)
    {
        printf("\n");
        return;
    }
    printf(" Size = %d\n", len);
    printf(" Offset - Hex - Dec\n");
    for (i = 0; i < len; i += 8)
    {
        int j;

        printf("%3d - ", i);
        for (j = 0; j < 8; j++)
            if (i + j < len)
                printf("%02X ", p[i + j]);
            else
                printf("   ");
        printf(" - ");
        for (j = 0; j < 8; j++)
            if (i + j < len)
                printf("%03d ", p[i + j]);
            else
                printf("    ");
        printf("\n");
    }
}

#endif                          /* never */
#endif                          /* DEBUG */



static void vms_msg(__GPRO__ char *string, int status)
{
    static char msgbuf[256];

    $DESCRIPTOR(msgd, msgbuf);
    int msglen = 0;

    if (ERR(lib$sys_getmsg(&status, &msglen, &msgd, 0, 0)))
        Info(slide, 1, ((char *)slide,
             "%s[ VMS status = %d ]\n", string, status));
    else
    {
        msgbuf[msglen] = '\0';
        Info(slide, 1, ((char *)slide, "%s[ %s ]\n", string, msgbuf));
    }
}



#ifndef SFX

char *do_wild( __G__ wld )
    __GDEF
    char *wld;
{
    int status;

    static char filenam[256];
    static char efn[256];
    static char last_wild[256];
    static struct FAB fab;
    static struct NAM nam;
    static int first_call=1;
    static const char deflt[] = "[]*.zip";

    if ( first_call || strcmp(wld, last_wild) )
    {   /* (Re)Initialize everything */

        strcpy( last_wild, wld );
        first_call = 1;            /* New wild spec */

        fab = cc$rms_fab;
        fab.fab$l_fna = last_wild;
        fab.fab$b_fns = strlen(last_wild);
        fab.fab$l_dna = (char *) deflt;
        fab.fab$b_dns = sizeof(deflt)-1;
        fab.fab$l_nam = &nam;
        nam = cc$rms_nam;
        nam.nam$l_esa = efn;
        nam.nam$b_ess = sizeof(efn)-1;
        nam.nam$l_rsa = filenam;
        nam.nam$b_rss = sizeof(filenam)-1;

        if ( !OK(sys$parse(&fab)) )
            return (char *)NULL;     /* Initialization failed */
        first_call = 0;
        if ( !OK(sys$search(&fab)) )
        {
            strcpy( filenam, wld );
            return filenam;
        }
    }
    else
    {
        if ( !OK(sys$search(&fab)) )
        {
            first_call = 1;        /* Reinitialize next time */
            return (char *)NULL;
        }
    }
    filenam[nam.nam$b_rsl] = 0;
    return filenam;

} /* end function do_wild() */

#endif /* !SFX */



static ulg unix_to_vms[8]={ /* Map from UNIX rwx to VMS rwed */
                            /* Note that unix w bit is mapped to VMS wd bits */
                                                              /* no access */
    XAB$M_NOREAD | XAB$M_NOWRITE | XAB$M_NODEL | XAB$M_NOEXE,    /* --- */
    XAB$M_NOREAD | XAB$M_NOWRITE | XAB$M_NODEL,                  /* --x */
    XAB$M_NOREAD |                               XAB$M_NOEXE,    /* -w- */
    XAB$M_NOREAD,                                                /* -wx */
                   XAB$M_NOWRITE | XAB$M_NODEL | XAB$M_NOEXE,    /* r-- */
                   XAB$M_NOWRITE | XAB$M_NODEL,                  /* r-x */
                                                 XAB$M_NOEXE,    /* rw- */
    0                                                            /* rwx */
                                                              /* full access */
};

#define SETDFPROT   /* We are using undocumented VMS System Service     */
                    /* SYS$SETDFPROT here. If your version of VMS does  */
                    /* not have that service, undef SETDFPROT.          */
                    /* IM: Maybe it's better to put this to Makefile    */
                    /* and DESCRIP.MMS */

#ifdef SETDFPROT
extern int SYS$SETDFPROT();
#endif


int mapattr(__G)
    __GDEF
{
    ulg  tmp=G.crec.external_file_attributes, theprot;
    static ulg  defprot = -1L,
                sysdef,owndef,grpdef,wlddef;  /* Default protection fields */


    /* IM: The only field of XABPRO we need to set here is */
    /*     file protection, so we need not to change type */
    /*     of G.pInfo->file_attr. WORD is quite enough. */

    if ( defprot == -1L )
    {
        /*
         * First time here -- Get user default settings
         */

#ifdef SETDFPROT    /* Undef this if linker cat't resolve SYS$SETDFPROT */
        defprot = 0L;
        if ( !ERR(SYS$SETDFPROT(0,&defprot)) )
        {
            sysdef = defprot & ( (1L<<XAB$S_SYS)-1 ) << XAB$V_SYS;
            owndef = defprot & ( (1L<<XAB$S_OWN)-1 ) << XAB$V_OWN;
            grpdef = defprot & ( (1L<<XAB$S_GRP)-1 ) << XAB$V_GRP;
            wlddef = defprot & ( (1L<<XAB$S_WLD)-1 ) << XAB$V_WLD;
        }
        else
        {
#endif /* ?SETDFPROT */
            umask(defprot = umask(0));
            defprot = ~defprot;
            wlddef = unix_to_vms[defprot & 07] << XAB$V_WLD;
            grpdef = unix_to_vms[(defprot>>3) & 07] << XAB$V_GRP;
            owndef = unix_to_vms[(defprot>>6) & 07] << XAB$V_OWN;
            sysdef = owndef << (XAB$V_SYS - XAB$V_OWN);
            defprot = sysdef | owndef | grpdef | wlddef;
#ifdef SETDFPROT
        }
#endif /* ?SETDFPROT */
    }

    switch (G.pInfo->hostnum) {
        case UNIX_:
        case VMS_:  /*IM: ??? Does VMS Zip store protection in UNIX format ?*/
                    /* GRR:  Yup.  Bad decision on my part... */
            tmp = (unsigned)(tmp >> 16);  /* drwxrwxrwx */
            theprot  = (unix_to_vms[tmp & 07] << XAB$V_WLD)
                     | (unix_to_vms[(tmp>>3) & 07] << XAB$V_GRP)
                     | (unix_to_vms[(tmp>>6) & 07] << XAB$V_OWN);

            if ( tmp & 0x4000 )
                /* Directory -- set D bits */
                theprot |= (XAB$M_NODEL << XAB$V_SYS)
                        | (XAB$M_NODEL << XAB$V_OWN)
                        | (XAB$M_NODEL << XAB$V_GRP)
                        | (XAB$M_NODEL << XAB$V_WLD);
            G.pInfo->file_attr = theprot;
            break;

        case AMIGA_:
            tmp = (unsigned)(tmp>>16 & 0x0f);   /* Amiga RWED bits */
            G.pInfo->file_attr =  (tmp << XAB$V_OWN) |
                                   grpdef | sysdef | wlddef;
            break;

        /* all remaining cases:  expand MSDOS read-only bit into write perms */
        case FS_FAT_:
        case FS_HPFS_:
        case FS_NTFS_:
        case MAC_:
        case ATARI_:             /* (used to set = 0666) */
        case TOPS20_:
        default:
            theprot = defprot;
            if ( tmp & 1 )   /* Test read-only bit */
            {   /* Bit is set -- set bits in all fields */
                tmp = XAB$M_NOWRITE | XAB$M_NODEL;
                theprot |= (tmp << XAB$V_SYS) | (tmp << XAB$V_OWN) |
                           (tmp << XAB$V_GRP) | (tmp << XAB$V_WLD);
            }
            G.pInfo->file_attr = theprot;
            break;
    } /* end switch (host-OS-created-by) */

    return 0;

} /* end function mapattr() */



#ifndef EEXIST
#  include <errno.h>    /* For mkdir() status codes */
#endif

#include <fscndef.h> /* for filescan */

#   define FN_MASK   7
#   define USE_DEFAULT  (FN_MASK+1)

/*
 * Checkdir function codes:
 *      ROOT        -   set root path from unzip qq d:[dir]
 *      INIT        -   get ready for "filename"
 *      APPEND_DIR  -   append pathcomp
 *      APPEND_NAME -   append filename
 *      APPEND_NAME | USE_DEFAULT   -    expand filename using collected path
 *      GETPATH     -   return resulting filespec
 *      END         -   free dynamically allocated space prior to program exit
 */

static  int created_dir;

int mapname(__G__ renamed)
            /* returns: */
            /* 0 (PK_COOL) if no error, */
            /* 1 (PK_WARN) if caution (filename trunc), */
            /* 2 (PK_ERR)  if warning (skip file because dir doesn't exist), */
            /* 3 (PK_BADERR) if error (skip file), */
            /* 77 (IZ_CREATED_DIR) if has created directory, */
            /* 10 if no memory (skip file) */
    __GDEF
    int renamed;
{
    char pathcomp[FILNAMSIZ];   /* path-component buffer */
    char *pp, *cp=NULL;         /* character pointers */
    char *lastsemi = NULL;      /* pointer to last semi-colon in pathcomp */
    char *last_dot = NULL;      /* last dot not converted to underscore */
    int quote = FALSE;          /* flag:  next char is literal */
    int dotname = FALSE;        /* flag:  path component begins with dot */
    int error = 0;
    register unsigned workch;   /* hold the character being tested */

    if ( renamed )
    {
        if ( !(error = checkdir(__G__ pathcomp, APPEND_NAME | USE_DEFAULT)) )
            strcpy(G.filename, pathcomp);
        return error;
    }

/*---------------------------------------------------------------------------
    Initialize various pointers and counters and stuff.
  ---------------------------------------------------------------------------*/

    /* can create path as long as not just freshening, or if user told us */
    G.create_dirs = !G.fflag;

    created_dir = FALSE;        /* not yet */

/* GRR:  for VMS, convert to internal format now or later? or never? */
    if (checkdir(__G__ pathcomp, INIT) == 10)
        return 10;              /* initialize path buffer, unless no memory */

    *pathcomp = '\0';           /* initialize translation buffer */
    pp = pathcomp;              /* point to translation buffer */
    if (G.jflag)                /* junking directories */
/* GRR:  watch out for VMS version... */
        cp = (char *)strrchr(G.filename, '/');
    if (cp == NULL)             /* no '/' or not junking dirs */
        cp = G.filename;        /* point to internal zipfile-member pathname */
    else
        ++cp;                   /* point to start of last component of path */

/*---------------------------------------------------------------------------
    Begin main loop through characters in G.filename.
  ---------------------------------------------------------------------------*/

    while ((workch = (uch)*cp++) != 0) {

        if (quote) {              /* if character quoted, */
            *pp++ = (char)workch; /*  include it literally */
            quote = FALSE;
        } else
            switch (workch) {
            case '/':             /* can assume -j flag not given */
                *pp = '\0';
                if ((error = checkdir(__G__ pathcomp, APPEND_DIR)) > 1)
                    return error;
                pp = pathcomp;    /* reset conversion buffer for next piece */
                last_dot = NULL;  /* directory names must not contain dots */
                lastsemi = NULL;  /* leave directory semi-colons alone */
                break;

            case ':':
                *pp++ = '_';      /* drive names not stored in zipfile, */
                break;            /*  so no colons allowed */

            case '.':
                if (pp == pathcomp) {     /* nothing appended yet... */
                    if (*cp == '/') {     /* don't bother appending a "./" */
                        ++cp;             /*  component to the path:  skip */
                                          /*  to next char after the '/' */
                    } else if (*cp == '.' && cp[1] == '/') {   /* "../" */
                        *pp++ = '.';      /* add first dot, unchanged... */
                        *pp++ = '.';      /* add second dot, unchanged... */
                        ++cp;             /* skip second dot */
                    }                     /* next char is  the '/' */
                    break;
                }
                last_dot = pp;    /* point at last dot so far... */
                *pp++ = '_';      /* convert dot to underscore for now */
                break;

            case ';':             /* start of VMS version? */
                if (lastsemi)
                    *lastsemi = '_';   /* convert previous one to underscore */
                lastsemi = pp;
                *pp++ = ';';      /* keep for now; remove VMS vers. later */
                break;

            case ' ':
                *pp++ = '_';
                break;

            default:
                if ( isalpha(workch) || isdigit(workch) ||
                    workch=='$' || workch=='-' )
                    *pp++ = (char)workch;
                else
                    *pp++ = '_';  /* convert everything else to underscore */
                break;
            } /* end switch */

    } /* end while loop */

    *pp = '\0';                   /* done with pathcomp:  terminate it */

    /* if not saving them, remove VMS version numbers (appended "###") */
    if (lastsemi) {
        pp = lastsemi + 1;        /* expect all digits after semi-colon */
        while (isdigit((uch)(*pp)))
            ++pp;
        if (*pp)                  /* not version number:  convert ';' to '_' */
            *lastsemi = '_';
        else if (!G.V_flag)       /* only digits between ';' and end:  nuke */
            *lastsemi = '\0';
        /* else only digits and we're saving version number:  do nothing */
    }

    if (last_dot != NULL)         /* one dot is OK:  put it back in */
        *last_dot = '.';

/*---------------------------------------------------------------------------
    Report if directory was created (and no file to create:  filename ended
    in '/'), check name to be sure it exists, and combine path and name be-
    fore exiting.
  ---------------------------------------------------------------------------*/

    if (G.filename[strlen(G.filename) - 1] == '/') {
        checkdir(__G__ "", APPEND_NAME);   /* create directory, if not found */
        checkdir(__G__ G.filename, GETPATH);
        if (created_dir && QCOND2) {
            Info(slide, 0, ((char *)slide, "   creating: %s\n", G.filename));
            return IZ_CREATED_DIR;   /* set dir time (note trailing '/') */
        }
        return 2;   /* dir existed already; don't look for data to extract */
    }

    if (*pathcomp == '\0') {
        Info(slide, 1, ((char *)slide,
             "mapname:  conversion of %s failed\n", G.filename));
        return 3;
    }

    checkdir(__G__ pathcomp, APPEND_NAME); /* returns 1 if truncated:  care? */
    checkdir(__G__ G.filename, GETPATH);

    return error;

} /* end function mapname() */



int checkdir(__G__ pathcomp, fcn)
/*
 * returns:  1 - (on APPEND_NAME) truncated filename
 *           2 - path doesn't exist, not allowed to create
 *           3 - path doesn't exist, tried to create and failed; or
 *               path exists and is not a directory, but is supposed to be
 *           4 - path is too long
 *          10 - can't allocate memory for filename buffers
 */
    __GDEF
    char *pathcomp;
    int fcn;
{
    int function=fcn & FN_MASK;
    static char pathbuf[FILNAMSIZ];
    static char lastdir[FILNAMSIZ]="\t"; /* directory created last time */
                                         /* initially - impossible dir. spec. */
    static char *pathptr=pathbuf;        /* For debugger */
    static char *devptr, *dirptr, *namptr;
    static int  devlen, dirlen, namlen;
    static int  root_dirlen;
    static char *end;
    static int  first_comp,root_has_dir;
    static int  rootlen=0;
    static char *rootend;
    static int  mkdir_failed=0;
    int status;

/************
 *** ROOT ***
 ************/

#if (!defined(SFX) || defined(SFX_EXDIR))
    if (function==ROOT)
    {        /*  Assume VMS root spec */
        char  *p = pathcomp;
        char  *q;

        struct
        {
            short  len;
            short  code;
            char   *addr;
        } itl [4] =
        {
            {  0,  FSCN$_DEVICE,    NULL  },
            {  0,  FSCN$_ROOT,      NULL  },
            {  0,  FSCN$_DIRECTORY, NULL  },
            {  0,  0,               NULL  }   /* End of itemlist */
        };
        int fields = 0;
        struct dsc$descriptor  pthcmp;

        /*
         *  Initialize everything
         */
        end = devptr = dirptr = rootend = pathbuf;
        devlen = dirlen = rootlen = 0;

        pthcmp.dsc$a_pointer = pathcomp;
        if ( (pthcmp.dsc$w_length = strlen(pathcomp)) > 255 )
            return 4;

        status = sys$filescan(&pthcmp, itl, &fields);
        if ( !OK(status) )
            return 3;

        if ( fields & FSCN$M_DEVICE )
        {
            strncpy(devptr = end, itl[0].addr, itl[0].len);
            dirptr = (end += (devlen = itl[0].len));
        }

        root_has_dir = 0;

        if ( fields & FSCN$M_ROOT )
        {
            int   len;

            strncpy(dirptr = end, itl[1].addr,
                len = itl[1].len - 1);        /* Cut out trailing ']' */
            end += len;
            root_has_dir = 1;
        }

        if ( fields & FSCN$M_DIRECTORY )
        {
            char  *ptr;
            int   len;

            len = itl[2].len-1;
            ptr = itl[2].addr;

            if ( root_has_dir /* i.e. root specified */ )
            {
                --len;                            /* Cut out leading dot */
                ++ptr;                            /* ??? [a.b.c.][.d.e] */
            }

            strncpy(dirptr=end, ptr, len);  /* Replace trailing ']' */
            *(end+=len) = '.';                    /* ... with dot */
            ++end;
            root_has_dir = 1;
        }

        /* When user specified "[a.b.c.]" or "[qq...]", we have too many
        *  trailing dots. Let's cut them out. Now we surely have at least
        *  one trailing dot and "end" points just behind it. */

        dirlen = end - dirptr;
        while ( dirlen > 1 && end[-2] == '.' )
            --dirlen,--end;

        first_comp = !root_has_dir;
        root_dirlen = end - dirptr;
        *(rootend = end) = '\0';
        rootlen = rootend - devptr;
        return 0;
    }
#endif /* !SFX || SFX_EXDIR */


/************
 *** INIT ***
 ************/

    if ( function == INIT )
    {
        if ( strlen(G.filename) + rootlen + 13 > 255 )
            return 4;

        if ( rootlen == 0 )     /* No root given, reset everything. */
        {
            devptr = dirptr = rootend = pathbuf;
            devlen = dirlen = 0;
        }
        end = rootend;
        first_comp = !root_has_dir;
        if ( dirlen = root_dirlen )
            end[-1] = '.';
        *end = '\0';
        return 0;
    }


/******************
 *** APPEND_DIR ***
 ******************/
    if ( function == APPEND_DIR )
    {
        int cmplen;

        cmplen = strlen(pathcomp);

        if ( first_comp )
        {
            *end++ = '[';
            if ( cmplen )
                *end++ = '.';   /*       "dir/..." --> "[.dir...]"    */
            /*                     else  "/dir..." --> "[dir...]"     */
            first_comp = 0;
        }

        if ( cmplen == 1 && *pathcomp == '.' )
            ; /* "..././..." -- ignore */

        else if ( cmplen == 2 && pathcomp[0] == '.' && pathcomp[1] == '.' )
        {   /* ".../../..." -- convert to "...-..." */
            *end++ = '-';
            *end++ = '.';
        }

        else if ( cmplen + (end-pathptr) > 255 )
            return 4;

        else
        {
            strcpy(end, pathcomp);
            *(end+=cmplen) = '.';
            ++end;
        }
        dirlen = end - dirptr;
        *end = '\0';
        return 0;
    }


/*******************
 *** APPEND_NAME ***
 *******************/
    if ( function == APPEND_NAME )
    {
        if ( fcn & USE_DEFAULT )
        {   /* Expand renamed filename using collected path, return
             *  at pathcomp */
            struct        FAB fab;
            struct        NAM nam;

            fab = cc$rms_fab;
            fab.fab$l_fna = G.filename;
            fab.fab$b_fns = strlen(G.filename);
            fab.fab$l_dna = pathptr;
            fab.fab$b_dns = end-pathptr;

            fab.fab$l_nam = &nam;
            nam = cc$rms_nam;
            nam.nam$l_esa = pathcomp;
            nam.nam$b_ess = 255;            /* Assume large enaugh */

            if (!OK(status = sys$parse(&fab)) && status == RMS$_DNF )
                                         /* Directory not found: */
            {                            /* ... try to create it */
                char    save;
                char    *dirend;
                int     mkdir_failed;

                dirend = (char*)nam.nam$l_dir + nam.nam$b_dir;
                save = *dirend;
                *dirend = '\0';
                if ( (mkdir_failed = mkdir(nam.nam$l_dev, 0)) &&
                     errno == EEXIST )
                    mkdir_failed = 0;
                *dirend = save;
                if ( mkdir_failed )
                    return 3;
                created_dir = TRUE;
            }                                /* if (sys$parse... */
            pathcomp[nam.nam$b_esl] = '\0';
            return 0;
        }                                /* if (USE_DEFAULT) */
        else
        {
            *end = '\0';
            if ( dirlen )
            {
                dirptr[dirlen-1] = ']'; /* Close directory */

                /*
                 *  Try to create the target directory.
                 *  Don't waste time creating directory that was created
                 *  last time.
                 */
                if ( STRICMP(lastdir,pathbuf) )
                {
                    mkdir_failed = 0;
                    if ( mkdir(pathbuf,0) )
                    {
                        if ( errno != EEXIST )
                            mkdir_failed = 1;   /* Mine for GETPATH */
                    }
                    else
                        created_dir = TRUE;
                    strcpy(lastdir,pathbuf);
                }
            }
            else
            {   /*
                 * Target directory unspecified.
                 * Try to create "sys$disk:[]"
                 */
                if ( strcmp(lastdir,"sys$disk:[]") )
                {
                    strcpy(lastdir,"sys$disk:[]");
                    mkdir_failed = 0;
                    if ( mkdir(lastdir,0) && errno != EEXIST )
                        mkdir_failed = 1;   /* Mine for GETPATH */
                }
            }
            if ( strlen(pathcomp) + (end-pathbuf) > 255 )
                return 1;
            strcpy(end, pathcomp);
            end += strlen(pathcomp);
            return 0;
        }
    }


/***************
 *** GETPATH ***
 ***************/
    if ( function == GETPATH )
    {
        if ( mkdir_failed )
            return 3;
        *end = '\0';                    /* To be safe */
        strcpy( pathcomp, pathbuf );
        return 0;
    }


/***********
 *** END ***
 ***********/
    if ( function == END )
    {
        Trace((stderr, "checkdir(): nothing to free...\n"));
        return 0;
    }

    return 99;  /* should never reach */

}



int check_for_newer(__G__ filenam)   /* return 1 if existing file newer or */
    __GDEF                           /*  equal; 0 if older; -1 if doesn't */
    char *filenam;                   /*  exist yet */
{
#ifdef USE_EF_UX_TIME
    ztimbuf z_utime;
#endif
    unsigned short timbuf[7];
    unsigned dy, mo, yr, hh, mm, ss, dy2, mo2, yr2, hh2, mm2, ss2;
    struct FAB fab;
    struct XABDAT xdat;


    if (stat(filenam, &G.statbuf))
        return DOES_NOT_EXIST;

    fab  = cc$rms_fab;
    xdat = cc$rms_xabdat;

    fab.fab$l_xab = (char *) &xdat;
    fab.fab$l_fna = filenam;
    fab.fab$b_fns = strlen(filenam);
    fab.fab$l_fop = FAB$M_GET | FAB$M_UFO;

    if ((sys$open(&fab) & 1) == 0)       /* open failure:  report exists and */
        return EXISTS_AND_OLDER;         /*  older so new copy will be made  */
    sys$numtim(&timbuf,&xdat.xab$q_cdt);
    fab.fab$l_xab = NULL;

    sys$dassgn(fab.fab$l_stv);
    sys$close(&fab);   /* be sure file is closed and RMS knows about it */

#ifdef USE_EF_UX_TIME
    if (G.extra_field &&
        ef_scan_for_izux(G.extra_field, G.lrec.extra_field_length,
                         &z_utime, NULL) > 0) {
        struct tm *t = localtime(&(z_utime.modtime));

        yr2 = (unsigned)(t->tm_year) + 1900;
        mo2 = (unsigned)(t->tm_mon) + 1;
        dy2 = (unsigned)(t->tm_mday);
        hh2 = (unsigned)(t->tm_hour);
        mm2 = (unsigned)(t->tm_min);
        ss2 = (unsigned)(t->tm_sec);

        /* round to nearest sec--may become 60,
           but doesn't matter for compare */
        ss = (unsigned)((float)timbuf[5] + (float)timbuf[6]*.01 + 0.5);
        TTrace((stderr, "check_for_newer:  using Unix extra field mtime\n"));
    } else {
        yr2 = ((G.lrec.last_mod_file_date >> 9) & 0x7f) + 1980;
        mo2 = ((G.lrec.last_mod_file_date >> 5) & 0x0f);
        dy2 = (G.lrec.last_mod_file_date & 0x1f);
        hh2 = (G.lrec.last_mod_file_time >> 11) & 0x1f;
        mm2 = (G.lrec.last_mod_file_time >> 5) & 0x3f;
        ss2 = (G.lrec.last_mod_file_time & 0x1f) * 2;

        /* round to nearest 2 secs--may become 60,
           but doesn't matter for compare */
        ss = (unsigned)((float)timbuf[5] + (float)timbuf[6]*.01 + 1.) & (~1);
    }
#else /* !USE_EF_UX_TIME */
    yr2 = ((G.lrec.last_mod_file_date >> 9) & 0x7f) + 1980;
    mo2 = ((G.lrec.last_mod_file_date >> 5) & 0x0f);
    dy2 = (G.lrec.last_mod_file_date & 0x1f);
    hh2 = (G.lrec.last_mod_file_time >> 11) & 0x1f;
    mm2 = (G.lrec.last_mod_file_time >> 5) & 0x3f;
    ss2 = (G.lrec.last_mod_file_time & 0x1f) * 2;

    /* round to nearest 2 secs--may become 60, but doesn't matter for compare */
    ss = (unsigned)((float)timbuf[5] + (float)timbuf[6]*.01 + 1.) & (~1);
#endif /* ?USE_EF_UX_TIME */
    yr = timbuf[0];
    mo = timbuf[1];
    dy = timbuf[2];
    hh = timbuf[3];
    mm = timbuf[4];

    if (yr > yr2)
        return EXISTS_AND_NEWER;
    else if (yr < yr2)
        return EXISTS_AND_OLDER;

    if (mo > mo2)
        return EXISTS_AND_NEWER;
    else if (mo < mo2)
        return EXISTS_AND_OLDER;

    if (dy > dy2)
        return EXISTS_AND_NEWER;
    else if (dy < dy2)
        return EXISTS_AND_OLDER;

    if (hh > hh2)
        return EXISTS_AND_NEWER;
    else if (hh < hh2)
        return EXISTS_AND_OLDER;

    if (mm > mm2)
        return EXISTS_AND_NEWER;
    else if (mm < mm2)
        return EXISTS_AND_OLDER;

    if (ss >= ss2)
        return EXISTS_AND_NEWER;

    return EXISTS_AND_OLDER;
}



#ifdef RETURN_CODES
void return_VMS(__G__ ziperr)
    __GDEF
#else
void return_VMS(ziperr)
#endif
    int ziperr;
{
    int severity = (ziperr == 2 || (ziperr >= 9 && ziperr <= 11))? 2 : 4;

#ifdef RETURN_CODES
/*---------------------------------------------------------------------------
    Do our own, explicit processing of error codes and print message, since
    VMS misinterprets return codes as rather obnoxious system errors ("access
    violation," for example).
  ---------------------------------------------------------------------------*/

    switch (ziperr) {
        case PK_COOL:
            break;   /* life is fine... */
        case PK_WARN:
            Info(slide, 1, ((char *)slide, "\n\
[return-code %d:  warning error \
(e.g., failed CRC or unknown compression method)]\n", ziperr));
            break;
        case PK_ERR:
        case PK_BADERR:
            Info(slide, 1, ((char *)slide, "\n\
[return-code %d:  error in zipfile \
(e.g., can't find local file header sig)]\n", ziperr));
            break;
        case PK_MEM:
        case PK_MEM2:
        case PK_MEM3:
        case PK_MEM4:
        case PK_MEM5:
            Info(slide, 1, ((char *)slide,
              "\n[return-code %d:  insufficient memory]\n", ziperr));
            break;
        case PK_NOZIP:
            Info(slide, 1, ((char *)slide,
              "\n[return-code %d:  zipfile not found]\n", ziperr));
            break;
        case PK_PARAM:   /* exit(PK_PARAM); gives "access violation" */
            Info(slide, 1, ((char *)slide, "\n\
[return-code %d:  bad or illegal parameters specified on command line]\n",
              ziperr));
            break;
        case PK_FIND:
            Info(slide, 1, ((char *)slide,
              "\n[return-code %d:  no files found to extract/view/etc.]\n",
              ziperr));
            break;
        case PK_DISK:
            Info(slide, 1, ((char *)slide,
              "\n[return-code %d:  disk full or other I/O error]\n", ziperr));
            break;
        case PK_EOF:
            Info(slide, 1, ((char *)slide, "\n\
[return-code %d:  unexpected EOF in zipfile (i.e., truncated)]\n", ziperr));
            break;
        default:
            Info(slide, 1, ((char *)slide,
              "\n[return-code %d:  unknown return-code (screw-up)]\n", ziperr));
            break;
    }
#endif /* RETURN_CODES */

/*---------------------------------------------------------------------------
    Return an intelligent status/severity level:

        $STATUS          $SEVERITY = $STATUS & 7
        31 .. 16 15 .. 3   2 1 0
                           -----
        VMS                0 0 0  0    Warning
        FACILITY           0 0 1  1    Success
        Number             0 1 0  2    Error
                 MESSAGE   0 1 1  3    Information
                 Number    1 0 0  4    Severe (fatal) error

    0x7FFF0000 was chosen (by experimentation) to be outside the range of
    VMS FACILITYs that have dedicated message numbers.  Hopefully this will
    always result in silent exits--it does on VMS 5.4.  Note that the C li-
    brary translates exit arguments of zero to a $STATUS value of 1 (i.e.,
    exit is both silent and has a $SEVERITY of "success").
  ---------------------------------------------------------------------------*/

    exit(                                          /* $SEVERITY:        */
         (ziperr == PK_COOL) ? 1 :                 /*   success         */
         (ziperr == PK_WARN) ? 0x7FFF0000 :        /*   warning         */
         (0x7FFF0000 | (ziperr << 4) | severity)   /*   error or fatal  */
        );

} /* end function return_VMS() */


#ifdef MORE
int screenlines(void)
{
    /*
     * For VMS v5.x:
     *   IO$_SENSEMODE/SETMODE info:  Programming, Vol. 7A, System Programming,
     *     I/O User's: Part I, sec. 8.4.1.1, 8.4.3, 8.4.5, 8.6
     *   sys$assign(), sys$qio() info:  Programming, Vol. 4B, System Services,
     *     System Services Reference Manual, pp. sys-23, sys-379
     *   fixed-length descriptor info:  Programming, Vol. 3, System Services,
     *     Intro to System Routines, sec. 2.9.2
     * GRR, 15 Aug 91 / SPC, 07 Aug 1995
     */

#ifndef OUTDEVICE_NAME
#define OUTDEVICE_NAME  "SYS$OUTPUT"
#endif

    static int scrnlines = -1;

    static const struct dsc$descriptor_s OutDevDesc =
        {(sizeof(OUTDEVICE_NAME) - 1), DSC$K_DTYPE_T, DSC$K_CLASS_S,
         OUTDEVICE_NAME};
     /* {dsc$w_length, dsc$b_dtype, dsc$b_class, dsc$a_pointer}; */

    short  OutDevChan, iosb[4];
    long   status;
    struct tt_characts
    {
        uch class, type;
        ush pagewidth;
        uch ttcharsbits[3];
        uch pagelength;
    }      ttmode;              /* total length = 8 bytes */


    if (scrnlines < 0)
    {
        /* assign a channel to standard output */
        status = sys$assign(&OutDevDesc, &OutDevChan, 0, 0);
        if (status & 1)
        {
            /* use sys$qiow and the IO$_SENSEMODE function to determine
             * the current tty status.
             */
            status = sys$qiow(0, OutDevChan, IO$_SENSEMODE, &iosb, 0, 0,
                              &ttmode, sizeof(ttmode), 0, 0, 0, 0);
            /* deassign the output channel by way of clean-up */
            (void) sys$dassgn(OutDevChan);
        }

        scrnlines = ( ( (status & 1) &&
                        ((status = iosb[0]) & 1) &&
                        (ttmode.pagelength >= 5)
                      )
                     ? (int) (ttmode.pagelength)        /* TT device value */
                     : (24) );                          /* VT 100 default  */
    }

    return (scrnlines);
}
#endif /* MORE */


#ifndef SFX

/************************/
/*  Function version()  */
/************************/

void version(__G)
    __GDEF
{
    int len;
#ifdef VMS_VERSION
    char buf[40];
#endif
#ifdef __DECC_VER
    char buf2[40];
    int  vtyp;
#endif

/*  DEC C in ANSI mode does not like "#ifdef MACRO" inside another
    macro when MACRO is equated to a value (by "#define MACRO 1").   */

    len = sprintf((char *)slide, LoadFarString(CompiledWith),

#ifdef __GNUC__
      "gcc ", __VERSION__,
#else
#  if defined(DECC) || defined(__DECC) || defined (__DECC__)
      "DEC C",
#    ifdef __DECC_VER
      (sprintf(buf2, " %c%d.%d-%03d",
               ((vtyp = (__DECC_VER / 10000) % 10) == 6 ? 'T' :
                (vtyp == 8 ? 'S' : 'V')),
               __DECC_VER / 10000000,
               (__DECC_VER % 10000000) / 100000, __DECC_VER % 1000), buf2),
#    else
      "",
#    endif
#  else
#  ifdef VAXC
      "VAX C", "",
#  else
      "unknown compiler", "",
#  endif
#  endif
#endif

#ifdef VMS_VERSION
#  if defined(__alpha)
      "OpenVMS",   /* version has trailing spaces ("V6.1   "), so truncate: */
      (sprintf(buf, " (%.4s for Alpha)", VMS_VERSION), buf),
#  else /* VAX */
      (VMS_VERSION[1] >= '6')? "OpenVMS" : "VMS",
      (sprintf(buf, " (%.4s for VAX)", VMS_VERSION), buf),
#  endif
#else
      "VMS",
      "",
#endif /* ?VMS_VERSION */

#ifdef __DATE__
      " on ", __DATE__
#else
      "", ""
#endif
    );

    (*G.message)((zvoid *)&G, slide, (ulg)len, 0);

} /* end function version() */

#endif /* !SFX */
#endif /* VMS */
