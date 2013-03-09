/*---------------------------------------------------------------------------

  unzipapi.c

  This module supplies an UnZip engine for use directly from C/C++
  programs.  The functions are:

    UzpVer *UzpVersion(void);
    int UzpMain(int argc, char *argv[]);
    int UzpAltMain(int argc, char *argv[], UzpInit *init);
    int UzpUnzipToMemory(char *zip, char *file, UzpBuffer *retstr);

  OS/2 only (for now):

    int UzpFileTree(char *name, cbList(callBack), char *cpInclude[],
          char *cpExclude[]);

  You must define `DLL' in order to include the API extensions.

  ---------------------------------------------------------------------------*/


#ifdef OS2
#  define  INCL_DOSMEMMGR
#  include <os2.h>
#endif

#define UNZIP_INTERNAL
#include "unzip.h"
#include "version.h"
#ifdef USE_ZLIB
#  include "zlib.h"
#endif



/*---------------------------------------------------------------------------
    Documented API entry points
  ---------------------------------------------------------------------------*/

#if 0
UzpVer *UzpVersion()   /* should be pointer to const struct */
{
    static UzpVer version;   /* doesn't change between calls */


    version.structlen = UZPVER_LEN;

#ifdef BETA
    version.flag = 1;
#else
    version.flag = 0;
#endif
    version.betalevel = BETALEVEL;
    version.date = VERSION_DATE;

#ifdef ZLIB_VERSION
    version.zlib_version = ZLIB_VERSION;
    version.flag |= 2;
#else
    version.zlib_version = NULL;
#endif

    /* someday each of these may have a separate patchlevel: */
    version.unzip.major = UZ_MAJORVER;
    version.unzip.minor = UZ_MINORVER;
    version.unzip.patchlevel = PATCHLEVEL;

    version.zipinfo.major = ZI_MAJORVER;
    version.zipinfo.minor = ZI_MINORVER;
    version.zipinfo.patchlevel = PATCHLEVEL;

    version.os2dll.major = D2_MAJORVER;
    version.os2dll.minor = D2_MINORVER;
    version.os2dll.patchlevel = PATCHLEVEL;

    version.windll.major = DW_MAJORVER;
    version.windll.minor = DW_MINORVER;
    version.windll.patchlevel = PATCHLEVEL;

    return &version;
}
#endif


#if 0
int UzpAltMain(int argc, char *argv[], UzpInit *init)
{
    int r, (*dummyfn)();


    CONSTRUCTGLOBALS();

    if (init->structlen >= (sizeof(ulg) + sizeof(dummyfn)) && init->msgfn)
        G.message = init->msgfn;

    if (init->structlen >= (sizeof(ulg) + 2*sizeof(dummyfn)) && init->inputfn)
        G.input = init->inputfn;

    if (init->structlen >= (sizeof(ulg) + 3*sizeof(dummyfn)) && init->pausefn)
        G.mpause = init->pausefn;

    if (init->structlen >= (sizeof(ulg) + 4*sizeof(dummyfn)) && init->userfn)
        (*init->userfn)();    /* allow void* arg? */

    r = unzip(__G__ argc, argv);
    DESTROYGLOBALS()
    RETURN(r);
}
#endif


#if 0
int UzpUnzipToMemory(char *zip,char *file,UzpBuffer *retstr)
{
    int r;

    CONSTRUCTGLOBALS();
    G.redirect_data = 1;
    r = unzipToMemory(__G__ zip,file,retstr)==0;
    DESTROYGLOBALS()
    return r;
}
#endif



#ifdef OS2API

int UzpFileTree(char *name, cbList(callBack), char *cpInclude[],
                char *cpExclude[])
{
    int r;

    CONSTRUCTGLOBALS();
    G.qflag = 2;
    G.vflag = 1;
    G.C_flag = 1;
    G.wildzipfn = name;
    G.process_all_files = TRUE;
    if (cpInclude)
        G.pfnames = cpInclude, G.process_all_files = FALSE;
    if (cpExclude)
        G.pxnames = cpExclude, G.process_all_files = FALSE;
  
    G.processExternally = callBack;
    r = process_zipfiles(__G)==0;
    DESTROYGLOBALS()
    return r;
}

#endif /* OS2API */



/*---------------------------------------------------------------------------
    Helper functions
  ---------------------------------------------------------------------------*/


void setFileNotFound(__G)
    __GDEF
{
    G.filenotfound++;
}


#if 0
int unzipToMemory(__GPRO__ char *zip, char *file, UzpBuffer *retstr)
{
    int r;
    char *incname[2];

    G.process_all_files = FALSE;
    G.extract_flag = TRUE;
    G.qflag = 2;
    G.C_flag = 1;
    G.wildzipfn = zip;

    G.pfnames = incname;
    incname[0] = file;
    incname[1] = NULL;
    G.filespecs = 1;

    r = process_zipfiles(__G);
    if (retstr) {
        retstr->strptr = G.redirect_buffer;
        retstr->strlength = G.redirect_size;
    }
    r |= G.filenotfound;
    if (r)
        return r;   /* GRR:  these two lines don't make much sense... */
    return r;
}
#endif



int redirect_outfile(__G)
     __GDEF
{
    G.redirect_size = G.lrec.ucsize;
#ifdef OS2
    DosAllocMem((void **)&G.redirect_buffer, G.redirect_size+1,
      PAG_READ|PAG_WRITE|PAG_COMMIT);
    G.redirect_pointer = G.redirect_buffer;
#else
    G.redirect_pointer = G.redirect_buffer = malloc(G.redirect_size+1);
#endif
    if (!G.redirect_buffer)
        return FALSE;
    G.redirect_pointer[G.redirect_size] = 0;
    return TRUE;
}



int writeToMemory(__GPRO__ uch *rawbuf, ulg size)
{
    if (rawbuf != G.redirect_pointer)
        memcpy(G.redirect_pointer,rawbuf,size);
    G.redirect_pointer += size;
    return 0;
}
