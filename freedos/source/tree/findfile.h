/****************************************************************************

  findfile: Generic FindFirst and FindNext routines for various
            compilers.  Currently: Borland's TC2.01, TC/C++ 1.01,
            TC/C++ 3.0, BC/C++ 3.1, BC/C++ 4.5, BCC5.5.1; 
            (Note: BCC 4.0x and 5.0x should work as-is, but untested)
            Microsoft's Visual C/C++ 5 (Win32), and MS C 6.0 (DOS);
            Dave Dunfield's Micro-C 3.14,3.15,3.21;
            Digital Mars 8 (8.12/8.16) [Symantic C++] DOS,{DOSX not yet}, & Win32 only
            (Note: Symantic C++ & Zortech C++ should work as-is or
             with minor changes (eg defines indicating Zortech), but untested)
            Pacific C Compiler for MS-DOS, v7.51
            generic Win32 (C api) ie most Win32 compilers (USE_WIN32 must be defined)

            With plans to support DJGPP 2, POSIX compliant compilers,
            and openWatcom.

  Written by: Kenneth J. Davis
  Date:       January/February, 2001
  Updated:    June, 2001
  Contact:    jeremyd@computer.org


Copyright released to Public Domain [United States Definition]

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NONINFRINGEMENT. IN NO EVENT SHALL THE COPYRIGHT HOLDERS OR AUTHORS BE
LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT
OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
DEALINGS IN THE SOFTWARE.

****************************************************************************/

/**
 * Several macros or functions are defined to implement various file
 * searching functions with a consistant interface across compilers.
 *
 * One (and only one) of the source files should define the macro
 * FF_INCLUDE_FUNCTION_DEFS to ensure that when functions are required
 * for a given compiler instead of just macros, the function is (only)
 * included once for linking.  If it is not defined at least once
 * before including this file, then on some compilers the linker may
 * report undefined symbols.
 *
 *
 * getCurrentDrive(): returns current drive as integer where A=0,B=1,C=2,...
 * setCurrentDrive(drive): set current drive (input matches getCurrentDrive)
 *         Should be verified by using getCurrentDrive() afterwards.
 * getCurrentDirectory(): returns char * with current directory,
 *         the pointer should be freed by calling free on it when done,
 *         a NULL is returned on error, current drive is assumed
 * getCurrentDirectoryEx(buffer, size): same as getCurrentDirectory(), except
 *         the buffer for the directory (and its size) is specified by
 *         the user, NULL is returned on error (such as size of user
 *         supplied buffer is too small), FF_MAXPATH can be used for max size
 * getCWD(drive): same as getCurrentDirectory, except
 *         the user specifies the drive (0=current, 1=A, 2=B,...)
 *         a NULL is returned on error otherwise the returned pointer
 *         should be freed when it is no longer needed
 *
 * FF_MAXPATH: defines the maximum size of complete path and filename
 *
 * getVolumeLabel(drive, label, maxsize): returns 0 if it was able to get 
 *         the volume label, and nonzero if an error occurred, such as the 
 *         drive is invalid.  The drive may be NULL to use current drive, 
 *         but if specified must refer to root directory, eg c:\ or e:\ 
 *         NOT c: or e:.  label is a char * to a user supplied buffer
 *         of maxsize length.  label should be non NULL and maxsize
 *         should be large enough to hold the volume label.
 *         If successful, but there is no volume label, then an empty
 *         string ("") is returned as the label.  If the actual label
 *         is longer than the buffer supplied, the label is truncated.
 *         This function should be used instead of directly using findfirst 
 *         variants.  It handles cases where LFN entries return false 
 *         values (part of Win9x long filename entries) and cases where 
 *         findfirst will not return entries with label attribute set.
 *
 * int getSerialNumber(char *drive, union media_serial *serial): returns
 *         0 if successful, -1 otherwise.  drive is the same format as
 *         getVolumeLabel, serial points to a union that contains two
 *         words (16bit values), and all but Micro-C a double word
 *         (32bit value).  getSerialNumber should successfully return
 *         the serial number if the disk has one, however there is no
 *         guarentee a disk has a serial #, so this function should Not
 *         be used to verify if a drive is valid.
 *
 * findFirst: performs initial search for file(s) or directory
 * findNext: continues a previous search, finds next file matching criteria
 * findClose: ends search, frees resources if necessary
 *
 * findFirst and findNext return -1 on error, 0 on success, with errno
 *         set to the operating system specific error code.
 *
 * FFDATA: a structure containing filefind information, use macros to access
 *
 * FF_GetFileName(ADDRESSOF(FFDATA ffdata)): returns '\0' terminated 
 *         char * to filename (a C string)
 *
 * FF_MAXDRIVE: defines the max size of drive component, usually 3, "?:\0"
 * FF_MAXFILEDIR: defines the maximum size of directory component
 * FF_MAXFILENAME: defines the maximum size of filename (without path)
 * FF_MAXFILEEXT: defines the maximum size of the filename extension
 *
 * MAXFILENAME: defines the maximum size of a filename and extension,
 *         this is usually either FF_MAXPATH or (FF_MAXFILENAME+FF_MAXFILEEXT)
 *
 * FF_GetAttributes(ADDRESSOF(FFDATA ffdata)): returns integer attributes 
 *         that can can be anded (&) with defines below to determine 
 *         file's attribute
 *
 * FF_GetFileSize(FFDATA * ffdata): defined for all but Micro-C/PC which
 *         does not support long int: returns a FSIZE_T (an unsigned long)
 *         with the size of the file.  The type defined by the compiler
 *         which usually ends up being either long or unsigned long
 *         is available as a _fsize_t
 *
 * The attributes of the matching filename returned
 * FF_A_LABEL: file specifies disk's label; findFirst/findNext may NOT
 *        return any files with volume label attribute set even if
 *        a file exists as label or a file is part of a LFN entry
 *        (any find based on Win32 FindFirstFile/FindNextFile under
 *        Windows NT 2000pro, at least, exhibits this problem);
 *        under DOS, may return multiple files if LFN entries exist.
 * FF_A_DIRECTORY: file is a directory (folder)
 * FF_A_ARCHIVE: file has archive bit set (modified since bit last cleared)
 * FF_A_READONLY: file is read only (can not be modified)
 * FF_A_HIDDEN: file is not normally visible to directory searches
 * FF_A_SYSTEM: file is a system file
 * FF_A_NORMAL: none of the other attributes are set, a normal file
 *
 * Micro-C/PC implements structures different that may be expectd.
 * It will treat struct variables as pointer to struct variables,
 * you should create a FFDATA instance, and use the ADDRESSOF macro when
 * passing the instance to any of the find routines.  If you do not
 * care about supporting Micro-C/PC and its many quirks then you may
 * use the address of operator (&) as normal, do not use the ADDRESSOF
 * macro when you are using pointers to a FFDATA instance.  You should
 * only use the ADDRESSOF macro when you want to pass a reference (address)
 * of a structure [struct/union] to another function, not as a general address
 * of for other types.
 *
 * sample usage:
 *         #include <stdio.h>
 *         #include "findfile.h"
 *         int main(void)
 *         {
 *           FFDATA ffdata;
 *           if (findFirst("*.c", ADDRESSOF(ffdata)))
 *             printf("No files matching *.c in current directory.\n");
 *           else
 *           {
 *             printf("DIR ARC RDO HID SYS Name\n");
 *             do 
 *             {
 *               printf(" %c   %c   %c   %c   %c  %s\n",
 *                 (FF_GetAttributes(ADDRESSOF(ffdata)) & FF_A_DIRECTORY) ? 'Y' : 'N',
 *                 (FF_GetAttributes(ADDRESSOF(ffdata)) & FF_A_ARCHIVE) ? 'Y' : 'N',
 *                 (FF_GetAttributes(ADDRESSOF(ffdata)) & FF_A_READONLY) ? 'Y' : 'N',
 *                 (FF_GetAttributes(ADDRESSOF(ffdata)) & FF_A_HIDDEN) ? 'Y' : 'N',
 *                 (FF_GetAttributes(ADDRESSOF(ffdata)) & FF_A_SYSTEM) ? 'Y' : 'N',
 *                 FF_GetFileName(ADDRESSOF(ffdata))
 *               );
 *             } while (findNext(ADDRESSOF(ffdata)) == 0);
 *             findClose(ADDRESSOF(ffdata));
 *           }
 *           return 0;
 *         }
 */

#ifndef FINDFILE_H
#define FINDFILE_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * MICRO-C/PC does not understand #if defined or #elif, so we are doing:
 * #ifdef MICROC
 *   ... Micro-C/PC specifics
 * #else
 *   ... all other compilers
 *   ... But we first test if we should use generic Win32 API or POSIX api
 *       so compilers that are otherwise supported will use these methods instead
 *   #if defined USE_WIN32
 *   #elif defined USE_POSIX
 *   #elif defined XXX
 *   #elif defined YYY
 *   #elif ...
 *   #else 
 *     #error Unsupported configuration
 *   #endif 
 * #endif
 */
#ifdef MICROC  

/* NOTE: generally Micro-C does not allow its standard include
   files to be included multiple times, so if you leave these
   here, then don't include them in your code!
*/
/* #include <stdlib.h> Doesn't exist */
#include <stdio.h>
#include <file.h>

#define FF_MAXPATH PATH_SIZE     /* 65 under MC3.14, MC3.15, MC3.21 */
#define FF_MAXDRIVE 3            /* drive colon EOS, ?:\0, eg "C:"  */
#define FF_MAXFILEDIR PATH_SIZE  /* should be similar to max path   */
#define FF_MAXFILENAME 9         /* 8.3 convention, 8 + '\0' fname  */
#define FF_MAXFILEEXT 5          /* .3 + '\0' for extension         */

#define MAXFILENAME (FF_MAXFILENAME + FF_MAXFILEEXT)

#define getCurrentDrive() get_drive()
#define setCurrentDrive(drive) set_drive(drive)

#define getCurrentDirectory() getCurrentDirectoryEx(NULL, FF_MAXPATH)

#ifndef FF_INCLUDE_FUNCTION_DEFS
char * getCurrentDirectoryEx(char *buffer, int size);
#else
char * getCurrentDirectoryEx(char *buffer, int size)
{
  char currentDir[FF_MAXPATH];
  register int pathLenNeeded;

  /* get the current drive and prepend it */
  currentDir[0] = 'A' + getCurrentDrive();
  currentDir[1] = ':';
  currentDir[2] = '\\';
  currentDir[3] = '\0';

  /* get current directory, on error return NULL */
  if (getdir(currentDir+3))
    return NULL;  /* error getting cwd */

  /* determine size of path returned, +1 for '\0' */
  pathLenNeeded = strlen(currentDir) + 1;

  /* if user did not give us a buffer allocate one using malloc */
  if (buffer == NULL)
  {
    pathLenNeeded = (pathLenNeeded < size) ? size : pathLenNeeded;
    if ((buffer = (char *)malloc(pathLenNeeded * sizeof(char))) == NULL)
      return NULL;  /* error allocating memory */
  }
  else /* verify buffer given is large enough, if not return NULL */
  {
    if (pathLenNeeded > size) 
      return NULL;  /* error buffer to small */
  }
  
  /* copy contents over */
  strcpy(buffer, currentDir);

  return buffer;
}
#endif

#define FF_LACKS_GETCWD


/* Find File macros */
#define FF_A_LABEL     VOLUME
#define FF_A_DIRECTORY DIRECTORY
#define FF_A_ARCHIVE   ARCHIVE
#define FF_A_READONLY  READONLY
#define FF_A_HIDDEN    HIDDEN
#define FF_A_SYSTEM    SYSTEM
#define FF_A_NORMAL    0x00

#define FF_FINDFILE_ATTR (FF_A_DIRECTORY | FF_A_ARCHIVE | FF_A_READONLY | FF_A_HIDDEN | FF_A_SYSTEM | FF_A_NORMAL)

#define FFDATA struct FF_block

/* work around for Micro-C/PC thinking a struct FF_block is a pointer to a struck FF_BLOCK */
#define ADDRESSOF(x) (x)

/* define errno */
#ifndef FF_INCLUDE_FUNCTION_DEFS
extern int errno;
#else
int errno;
#endif

#define findFirst(filespec, ffdata, attr) (((errno=findfirst(filespec, ffdata, FF_FINDFILE_ATTR)) == 0) ? 0 : -1)
#define findFirstEx(filespec, ffdata, attr) (((errno=findfirst(filespec, ffdata, attr)) == 0) ? 0 : -1)
#define findNext(ffdata) (((errno=findnext(ffdata)) == 0) ? 0 : -1)
#define findClose(ffdata)

#define FF_GetFileName(ffdata) (ffdata)->FF_name
#define FF_GetAttributes(ffdata) (ffdata)->FF_attrib

#define FF_GETVOLLBL_WITH_FINDFIRST

/* #define FF_GETSERIAL_DUMMY */
/* Do NOT use FF_GETSERIAL_DOS and FF_DOS_INTR, it will not work with Micro-C,
   I was never able to get the structure alignment correct, since then I changed it to require long. 
*/

/* no long support, may occupy more than a DWORD, don't use short */
union media_serial
{
  struct {
    unsigned lw, /* low word (on Little Endian machines) */
        hw; /* high word                            */
  } wSerial;
  struct {
    unsigned serial[2];
  } wa;
};

#ifndef FF_INCLUDE_FUNCTION_DEFS
int getSerialNumber(char *drive, union media_serial *serial);
#else
extern int _AX_, _BX_, _CX_, _DX_;
#define FF_CARRYFLAG 1
int getSerialNumber(char *drive, union media_serial *serial)
{
  int driveNum;
  unsigned temp;
  char media[25];

  if (serial == NULL)
    return -1;

  if (drive == NULL || *drive == '\0')
    driveNum = 0;
  else
    driveNum = (*drive & '\xDF') - 'A' + 1; /* Clear BH, drive in BL */

  /* Get Serial Number, only supports drives mapped to letters */
  _AX_ = 0x440D;                  /* Generic IOCTL */
  _BX_ = driveNum;
  _CX_ = 0x0866;                  /* CH=disk drive, CL=Get Serial # */
  _DX_ = media;                   /* DS:DX = &media */
  if (int86(0x21) & FF_CARRYFLAG) /* Actually invoke DOS function */
  {
    serial->wSerial.lw = serial->wSerial.hw = 0;
    return -1;
  }
  else /* successfully got serial # */
  {
    /* Weird hack, because structures are not necessary byte aligned & packed */
    temp = media[3];
    temp = temp << 8;
    temp = temp | media[2];
    serial->wSerial.lw = temp;
    temp = media[5];
    temp = temp << 8;
    temp = temp | media[4];
    serial->wSerial.hw = temp;
    return 0;
  }
}
#endif


#else /* All non Micro-C/PC compilers */

/* work around for Micro-C/PC thinking a struct FF_block is a pointer to a struck FF_BLOCK */
#define ADDRESSOF(x) &(x)


#if defined USE_WIN32  /* Uses the Win32 API for FindFirst/FindNext/FindClose/etc. */

#define WIN32_LEAN_AND_MEAN
#include <windows.h>  /* for GetCurrentDirectory and SetCurrentDirectory */
#include <stdlib.h>   /* for malloc & free */

#define FF_A_LABEL     0x08  /* NOTE: Win32 API should ignore LABEL flag for findfile*/
#define FF_A_DIRECTORY FILE_ATTRIBUTE_DIRECTORY /* 0x10 */
#define FF_A_ARCHIVE   FILE_ATTRIBUTE_ARCHIVE   /* 0x20 */
#define FF_A_READONLY  FILE_ATTRIBUTE_READONLY  /* 0x01 */
#define FF_A_HIDDEN    FILE_ATTRIBUTE_HIDDEN    /* 0x02 */
#define FF_A_SYSTEM    FILE_ATTRIBUTE_SYSTEM    /* 0x04 */
#define FF_A_NORMAL    0x00 /* FILE_ATTRIBUTE_NORMAL 0x80 */

#define FF_FINDFILE_ATTR (FF_A_DIRECTORY | FF_A_ARCHIVE | FF_A_READONLY | FF_A_HIDDEN | FF_A_SYSTEM | FF_A_NORMAL)

#define FF_MAXPATH     260
#define FF_MAXDRIVE      3
#define FF_MAXFILEDIR  256
#define FF_MAXFILENAME 256
#define FF_MAXFILEEXT  256

#define MAXFILENAME FF_MAXPATH

#define FF_LACKS_GETCWD

#ifndef FF_INCLUDE_FUNCTION_DEFS
int getCurrentDrive(void);
void setCurrentDrive(int drive);
char *getCurrentDirectory(void);
char *getCurrentDirectoryEx(char *buffer, int size);
#else

int getCurrentDrive(void)
{
  char buffer[FF_MAXPATH];
  if (GetCurrentDirectory(FF_MAXPATH, buffer) == 0)
    return 0; /* ERROR, failed to get current directory */
  if (buffer[1] == ':')
    return (int)(*buffer - 'A');
  else
    return 0; /* ERROR, not a drive, probably a UNC path */
}

void setCurrentDrive(int drive)
{
  char *driveString = "A:";
  *driveString += drive;
  SetCurrentDirectory(driveString);
}

char *getCurrentDirectory(void)
{
  char *buffer;
  buffer = (char *)malloc(MAXFILENAME*sizeof(char));
  if (buffer == NULL) return NULL;
  if (GetCurrentDirectory(MAXFILENAME, buffer) == 0)
  {
    free(buffer);
    return NULL;
  }
  else
    return buffer;
}

char *getCurrentDirectoryEx(char *buffer, int size)
{
  if (buffer == NULL) return NULL;
  if (GetCurrentDirectory(size, buffer) == 0)
    return NULL;
  else
    return buffer;
}

#endif  /* FF_INCLUDE_FUNCTION_DEFS */


#define FF_GETVOLLBL_WITH_W32GETVOLINFO
#define FF_WIN32_FINDFIRSTFILE
#define FF_GETSERIAL_WIN32

#elif defined USE_POSIX  /* Uses the POSIX defined OpenDir/ReadDir/CloseDir/etc. */

#error POSIX support has not been added yet.

#elif defined __BORLANDC__ || __TURBOC__

#include <stdlib.h>
#include <dir.h>
#include <errno.h>

#ifdef __MSDOS__
#include <dos.h>
#define FF_DOS_INT86X  /* Only define one of these two, use int86x or intr method to make DOS interrupt calls */
/* #define FF_DOS_INTR */
#elif !defined(_WIN32) /* BC 4.5 Win32 compile, BC 5.5 defines it */
#define _WIN32
#endif

#define FF_MAXPATH MAXPATH      /* 260 under BCC55, 80 under TC10&TC30 */
#define FF_MAXDRIVE MAXDRIVE    /*   3 under BCC55,  3 under TC10&TC30 */
#define FF_MAXFILEDIR MAXDIR    /* 256 under BCC55, 66 under TC10&TC30 */
#define FF_MAXFILENAME MAXFILE  /* 256 under BCC55,  9 under TC10&TC30 */
#define FF_MAXFILEEXT MAXEXT    /* 256 under BCC55,  5 under TC10&TC30 */

#if (FF_MAXFILENAME + FF_MAXFILEEXT) > FF_MAXPATH
#define MAXFILENAME FF_MAXPATH
#else
#define MAXFILENAME (FF_MAXFILENAME + FF_MAXFILEEXT)
#endif

#define getCurrentDrive() getdisk()
#define setCurrentDrive(drive) setdisk(drive)

#define getCurrentDirectory() getcwd(NULL, FF_MAXPATH)
#define getCurrentDirectoryEx(buffer, size) getcwd(buffer, size)
#if (__TURBOC__ < 0x300)
#ifndef FF_INCLUDE_FUNCTION_DEFS
char * getCWD(int drive);
#else
char * getCWD(int drive)
{
  char *cwd;

  /* Allocate memory for current working directory of max dir length */
  if ((cwd = (char *)malloc(FF_MAXPATH * sizeof(char))) == NULL)
    return NULL;

  /* get the current drive and prepend it */
  cwd[0] = 'A' + getCurrentDrive();
  cwd[1] = ':';
  cwd[2] = '\\';
  cwd[3] = '\0';

  /* Get the directory, cleaning up on error */
  if (getcurdir(drive, cwd+3) != 0)
  {
    free(cwd);
    return NULL;
  }
  return cwd;
}
#endif
#else /* Turbo C/C++ 3.0 or above */
#include <direct.h>
#define getCWD(drive) _getdcwd((drive), NULL, FF_MAXPATH)
#endif

/* For BCC 4.5 when compiling for windows, these are in dir.h for BCC 5.5 */
#ifndef FA_LABEL
#include <dos.h>
#endif

#define FF_A_LABEL     FA_LABEL
#define FF_A_DIRECTORY FA_DIREC
#define FF_A_ARCHIVE   FA_ARCH
#define FF_A_READONLY  FA_RDONLY
#define FF_A_HIDDEN    FA_HIDDEN
#define FF_A_SYSTEM    FA_SYSTEM
#if (__TURBOC__ < 0x300) || (__BORLANDC__ < 0x300)
#define FF_A_NORMAL    0x00
#else
#define FF_A_NORMAL    FA_NORMAL
#endif

#define FF_FINDFILE_ATTR (FF_A_DIRECTORY | FF_A_ARCHIVE | FF_A_READONLY | FF_A_HIDDEN | FF_A_SYSTEM | FF_A_NORMAL)

/* Find File macros */
typedef struct ffblk FFDATA;

#define findFirst(filespec, ffdata) findFirstEx(filespec, ffdata, FF_FINDFILE_ATTR)
#define findFirstEx(filespec, ffdata, attr) findfirst(filespec, ffdata, attr)
#define findNext(ffdata) findnext(ffdata)
#define findClose(ffdata)

#define FF_GetFileName(ffdata)   (ffdata)->ff_name
#define FF_GetAttributes(ffdata) (ffdata)->ff_attrib

typedef long _fsize_t;
typedef unsigned long FSIZE_T;
#define FF_GetFileSize(ffdata) (FSIZE_T)((ffdata)->ff_fsize)

#ifdef _WIN32
#define FF_GETVOLLBL_WITH_W32GETVOLINFO
#define FF_GETSERIAL_WIN32
#else /* __MSDOS__ */
#define FF_GETVOLLBL_WITH_FINDFIRST
#define FF_GETSERIAL_DOS
#endif

#elif defined _MSC_VER || defined __MSC /* Only tested with MS Visual C/C++ 5 & MS C 6*/

#include <stdlib.h>
#include <direct.h>
#include <io.h>
#ifdef MSDOS
#include <dos.h>
#define FF_DOS_INT86X

/* Limits under DOS, so don't waste space with larger values */
#define FF_MAXPATH 80
#define FF_MAXDRIVE _MAX_DRIVE
#define FF_MAXFILEDIR 66
#define FF_MAXFILENAME 9
#define FF_MAXFILEEXT 5
#else
#define FF_MAXPATH _MAX_PATH      /* 260 under VC5, 256 under MSC6 */
#define FF_MAXDRIVE _MAX_DRIVE    /*   3 under VC5 and MSC6  */
#define FF_MAXFILEDIR _MAX_DIR    /* 256 under VC5 and MSC6 */
#define FF_MAXFILENAME _MAX_FNAME /* 256 under VC5 and MSC6  */
#define FF_MAXFILEEXT _MAX_EXT    /* 256 under VC5 and MSC6  */
#endif

#define MAXFILENAME FF_MAXPATH

#define getCurrentDrive() (_getdrive() - 1)
#define setCurrentDrive(drive) _chdisk(drive+1)

#define getCurrentDirectory() getcwd(NULL, FF_MAXPATH)
#define getCurrentDirectoryEx(buffer, size) getcwd(buffer, size)
#define getCWD(drive) _getdcwd(drive, NULL, FF_MAXPATH)

/* Find File macros */

#ifndef _A_VOLID
#define FF_A_LABEL     0x08
#else
#define FF_A_LABEL     _A_VOLID
#endif
#define FF_A_DIRECTORY _A_SUBDIR
#define FF_A_ARCHIVE   _A_ARCH
#define FF_A_READONLY  _A_RDONLY
#define FF_A_HIDDEN    _A_HIDDEN
#define FF_A_SYSTEM    _A_SYSTEM
#define FF_A_NORMAL    _A_NORMAL

#ifdef _WIN32

/* #define FF_FINDFILE_ATTR (FF_A_DIRECTORY | FF_A_ARCHIVE | FF_A_READONLY | FF_A_HIDDEN | FF_A_SYSTEM | FF_A_NORMAL) */

typedef struct FFDATA
{
  long hFile;
  struct _finddata_t ffd;
} FFDATA;

#define findFirst(filespec, ffdata) ((((ffdata)->hFile = _findfirst(filespec, &((ffdata)->ffd))) == -1L) ? -1 : 0)
/* #define findFirstEx(filespec, ffdata, attr) Unsupported */
#define findNext(ffdata)  ((_findnext((ffdata)->hFile, &((ffdata)->ffd)) != 0) ? -1 : 0)
#define findClose(ffdata) _findclose((ffdata)->hFile)

#define FF_GetFileName(ffdata) (ffdata)->ffd.name
#define FF_GetAttributes(ffdata) (ffdata)->ffd.attrib

/* typedef unsigned long _fsize_t; */
typedef unsigned long FSIZE_T;
#define FF_GetFileSize(ffdata) (FSIZE_T)((ffdata)->ffd.size)

#define FF_GETVOLLBL_WITH_W32GETVOLINFO
#define FF_GETSERIAL_WIN32

#else  /* MSDOS */

#define FF_FINDFILE_ATTR (FF_A_DIRECTORY | FF_A_ARCHIVE | FF_A_READONLY | FF_A_HIDDEN | FF_A_SYSTEM | FF_A_NORMAL)

typedef struct find_t FFDATA;

#define findFirst(filespec, ffdata) findFirstEx(filespec, ffdata, FF_FINDFILE_ATTR)
#define findFirstEx(filespec, ffdata, attr) ((_dos_findfirst(filespec, attr, ffdata) == 0) ? 0 : -1)
#define findNext(ffdata) ((_dos_findnext(ffdata) == 0) ? 0 : -1)
#define findClose(ffdata)

#define FF_GetFileName(ffdata) (ffdata)->name
#define FF_GetAttributes(ffdata) (ffdata)->attrib

typedef unsigned long FSIZE_T;
#define FF_GetFileSize(ffdata) (FSIZE_T)((ffdata)->size)

#define FF_GETVOLLBL_WITH_FINDFIRST
#define FF_GETSERIAL_DOS

#endif /* _WIN32 vs MSDOS */


#elif defined __SC__  || defined __RCC__ /* Digital Mars 8, Symantic C++, TODO also add check for Zortech C++ */

#ifdef __NT__
#ifndef _WIN32
#define _WIN32
#endif
#endif

#include <stdlib.h>
#include <direct.h>  /* #include <dir.h> */
#include <errno.h>
#include <dos.h>

#define FF_MAXPATH MAXPATH      /* 260 under Win32, 80 under DOS */
#define FF_MAXDRIVE MAXDRIVE    /*   3 under Win32,  3 under DOS */
#define FF_MAXFILEDIR MAXDIR    /* 256 under Win32, 66 under DOS */
#define FF_MAXFILENAME MAXFILE  /* 256 under Win32,  9 under DOS */
#define FF_MAXFILEEXT MAXEXT    /* 256 under Win32,  5 under DOS */

#define MAXFILENAME FF_MAXPATH

#define getCurrentDrive() getdisk()
#define setCurrentDrive(drive) setdisk(drive)

#define getCurrentDirectory() getcwd(NULL, FF_MAXPATH)
#define getCurrentDirectoryEx(buffer, size) getcwd(buffer, size)
#define getCWD(drive) _getdcwd((drive), NULL, FF_MAXPATH)

#define FF_A_LABEL     _A_VOLID
#define FF_A_DIRECTORY _A_SUBDIR
#define FF_A_ARCHIVE   _A_ARCH
#define FF_A_READONLY  _A_RDONLY
#define FF_A_HIDDEN    _A_HIDDEN
#define FF_A_SYSTEM    _A_SYSTEM
#define FF_A_NORMAL    0x00

#define FF_FINDFILE_ATTR (FF_A_DIRECTORY | FF_A_ARCHIVE | FF_A_READONLY | FF_A_HIDDEN | FF_A_SYSTEM | FF_A_NORMAL)

/* Find File */
/* Note: the DOS/Win32 find versions are used instead of findfirst/findnext variants because the
   library uses a static buffer for the find_t data instead of a user supplied one, so can not
   reliably have multiple finds going at once - This may actually be possible by using a
   user supplied buffer and copying the contents to/from the static buffer, but this approach
   is not very clean, and I do not have access to the runtime library source to see if the
   find functions actually set the DTA with each call using the static buffer (which would
   be needed for the copying to actually have any effect).
*/
#ifdef _WIN32

#define FF_WIN32_FINDFIRSTFILE
#define FF_GETVOLLBL_WITH_W32GETVOLINFO
#define FF_GETSERIAL_WIN32

#elif DOS386  /* DOSX */

#error DOSX currently not supported!

#else  /* DOS or Win16 */

typedef struct find_t FFDATA;

#define findFirst(filespec, ffdata) findFirstEx(filespec, ffdata, FF_FINDFILE_ATTR)
#define findFirstEx(filespec, ffdata, attr) ((_dos_findfirst(filespec, attr, ffdata) != 0) ? -1 : 0)
#define findNext(ffdata) ((_dos_findnext(ffdata) != 0) ? -1 : 0)
#define findClose(ffdata)

#define FF_GetFileName(ffdata)   (ffdata)->name
#define FF_GetAttributes(ffdata) (ffdata)->attrib

typedef unsigned long _fsize_t;
typedef _fsize_t FSIZE_T;
#define FF_GetFileSize(ffdata) (FSIZE_T)((ffdata)->size)

#define FF_GETVOLLBL_WITH_FINDFIRST
#define FF_GETSERIAL_DOS
#define FF_DOS_INT86X

#endif  /* Find File stuff, Win32 vs DOS */

#elif defined HI_TECH_C  /* Pacific-C - free compiler for DOS */

/* The #asm / #endasm statements in the Pacific-C code cause
   problems for Digital Mars, the best solution I could think
   of is to move that code (and for ease of maintainence
   all Pacific C chunk moved) to a separate compilation unit.
*/

#include "findfile.pac"

#else
#error Platform (OS) or compiler is not supported.
#endif
#endif /* All non Micro-C/PC compilers */


/**
 * This section contains macros/functions common to all/most compilers.
 * This includes Micro-C/PC, so limited preprocessor support.
 */


#ifndef MICROC
#define CONST const
#else
#if MICROC >= 321
#define CONST const
#else
#define CONST
#endif
#endif


#ifdef FF_LACKS_GETCWD
#ifndef FF_INCLUDE_FUNCTION_DEFS
char * getCWD(int drive);
#else
char * getCWD(int drive)
{
  char *cwd;
  int curdrive;

  curdrive = getCurrentDrive(); /* store current drive */

  if (drive == 0)               /* correct requested drive */
    drive = curdrive;           /* use current drive if 0 given */
  else
  {
    drive--;                    /* convert from 1=A to 0=A,... */
  } 

  setCurrentDrive(drive);       /* change to drive to get cwd from */

/* setting current drive does not return error status so we must        *
 * manually check if drive changed, but only if not using current drive */
  if (drive != curdrive)
    if (getCurrentDrive() != drive) return NULL; /* drive change failed */

  cwd = getCurrentDirectory();  /* Get the directory */
  setCurrentDrive(curdrive);    /* restore initial drive */

  return cwd;                   /* returns malloc'd path or NULL */
}
#endif
#endif

#ifdef FF_GETVOLLBL_WITH_FINDFIRST
/* Use FindFirst/FindNext to get file with volume label attribute set. */

#ifndef FF_INCLUDE_FUNCTION_DEFS
int getVolumeLabel(CONST char *drive, char *label, int bufferSize);
#else

#ifndef MICROC
#include <string.h>
#include <ctype.h>
#endif

int getVolumeLabel(CONST char *drive, char *label, int bufferSize)
{
  char volSearch[FF_MAXDRIVE+4]; /* hold drive (c:), root dir (\), and *.* eg c:\*.* */
  FFDATA ffdata;
  register char *tcharptr;

  if ((label == NULL) || (bufferSize < 1)) return -1;

  if (drive == NULL)
    strcpy(volSearch, "\\*.*");
  else
  {
    /* validate drive if given (not given means use current so is valid) */
    if (drive[1] == ':')
    {
      /* simple test, if we can get the cwd of drive, assume valid. */
      tcharptr = getCWD(toupper(drive[0]) - 'A' + 1);
      if (tcharptr == NULL)
        return -1;    /* invalid drive or at least unable to get cwd */
      free(tcharptr); /* we don't actually need this, so free it */
    }

    /* validated, so now perform search */
    strcpy(volSearch, drive);
    strcat(volSearch, "*.*");
  }

  /* Initially clear label */
  *label = '\0';  

  /* loop until no more files or other error */
  if (findFirstEx(volSearch, ADDRESSOF(ffdata), FF_A_LABEL) == 0)
  {
    do
    {
      /* if found entry, and entry has only label bit set then valid label (MS LFN docs), but also accept archive bit */
      if ((FF_GetAttributes(ADDRESSOF(ffdata)) & ~FF_A_ARCHIVE) == FF_A_LABEL)
      {
        /* Remove the . since volumes don't use it */
        if (strlen(FF_GetFileName(ADDRESSOF(ffdata))) > 8)
        {
          tcharptr = FF_GetFileName(ADDRESSOF(ffdata)) + 8;  /* point to . */
          if (*tcharptr == '.')
          {
            *tcharptr = *(tcharptr + 1);  tcharptr++;
            *tcharptr = *(tcharptr + 1);  tcharptr++;
            *tcharptr = *(tcharptr + 1);  tcharptr++;
            *tcharptr = '\0';
          }
        }
        
        strncpy(label, FF_GetFileName(ADDRESSOF(ffdata)), bufferSize);
        label[bufferSize-1] = '\0';  /* Ensure string terminated! */
        return 0; /* Found and set label :) */
      }
    } while (findNext(ADDRESSOF(ffdata)) == 0);
    findClose(ADDRESSOF(ffdata));    
  }

  /* 0x12 == No more files, may also return 0x02 == File not found */
  if ((errno == 0x12) || (errno == 0x02))  /* No label, but valid drive. */
    return 0;
  else
    return -1; /* Invalid drive */
}
#endif /* FF_INCLUDE_FUNCTION_DEFS    */ 
#endif /* FF_GETVOLLBL_WITH_FINDFIRST */


#ifdef FF_GETVOLLBL_WITH_W32GETVOLINFO
/* Use Win32 API's GetVolumeInformation to get volume label. */

#define WIN32_LEAN_AND_MEAN
#include <windows.h>  /* necessary since winbase.h needs, but may not directly include */

#define getVolumeLabel(drive, label, bufferSize) ((GetVolumeInformation(drive ,label, bufferSize, NULL, NULL, NULL, NULL, 0) == 0) ? -1 : 0)

#endif /* FF_GETVOLLBL_WITH_W32GETVOLINFO */


#ifndef MICROC  /* defined above in Micro-C section */
/* The serial number is a union to allow easy access
   as two words (16bit unsigned ints) or a single 
   double word (32bit unsigned long) on compilers that support it.
*/
/* #pragma pack(1) */ /* if needed, specify byte align */
typedef union media_serial
{
  struct {
    unsigned short lw; /* low word (on Little Endian machines) */
    unsigned short hw; /* high word                            */
  } wSerial;
  struct {
    unsigned short serial[2];
  } wa;  /* word array */
  unsigned long dwSerial; /* double word */
} media_serial;
#endif


#ifdef FF_GETSERIAL_DUMMY
/* This implements a getSerialNumber function that will always
   fail, it will also initialize serial to be 0 unless one or
   more of the arguments are NULL.  This can be used to allow
   programs to compile on platforms that do not support serial #s
*/
#ifndef FF_INCLUDE_FUNCTION_DEFS
int getSerialNumber(CONST char *drive, union media_serial *serial);
#else
int getSerialNumber(CONST char *drive, union media_serial *serial)
{
  if (drive != NULL && serial != NULL)
    serial->wSerial.lw = serial->wSerial.hw = 0;
  return -1;
}
#endif
#endif

#ifdef FF_GETSERIAL_DOS

#ifndef FF_INCLUDE_FUNCTION_DEFS
int getSerialNumber(CONST char *drive, union media_serial *serial);
#else

#ifndef FP_SEG 
#error FP_SEG must be defined
#define FP_OFF(ptr)	((unsigned short)(ptr))
#define FP_SEG(ptr)	((unsigned short)(((long)(far void *)(ptr)) >> 16))
#endif

/* Pacific-C can not handle structures defined within function blocks */
#pragma pack(1)
typedef struct media_info
{
  short dummy;
  unsigned long serial;
  char volume[11];
  short ftype[8];
} media_info;

#ifdef FF_DOS_INTR  /* use alternate intr(intno, REGPACK) instead of int86x */

#define FF_CARRYFLAG 1

#include <stdio.h>
int getSerialNumber(CONST char *drive, union media_serial *serial)
{
  /* Using DOS interrupt to get serial number */
  struct REGPACK regs;
  struct media_info media;
  int driveNum;

  if (serial == NULL) return -1;  /* error if no place to put value */

  if ((drive == NULL) || (*drive == '\0'))  /* not given use current drive */
    driveNum = 0;  /* getCurrentDrive(); */
  else
    if ((*(drive + 1) == ':') || (*(drive + 1) == '\0'))
    {
      driveNum = (*drive & '\xDF') - 'A' + 1; /* Clear BH, drive in BL */
    }
    else
    {
      /* possibly set errno here? */
      return -1; /* invalid drive specified */
    }

  /* Get Serial Number, only supports drives mapped to letters */
  regs.r_ax = 0x440D;           /* Generic IOCTL */
  regs.r_bx = driveNum;
  regs.r_cx = 0x0866;           /* CH=disk drive, CL=Get Serial # */
  regs.r_dx = FP_OFF(ADDRESSOF(media));
  regs.r_ds = FP_SEG(ADDRESSOF(media));
  intr(0x21, ADDRESSOF(regs));
  if (regs.r_flags & FF_CARRYFLAG)
  {
    serial->wSerial.lw = serial->wSerial.hw = 0;
    return -1;
  }
  else /* successfully got serial # */
  {
    serial->dwSerial = media.serial;
    return 0;
  }
}
#else /* All supported DOS compilers that have int86x, FF_DOS_INT86X */

int getSerialNumber(CONST char *drive, union media_serial *serial)
{
  /* Using DOS interrupt to get serial number */
  union REGS regs;
  struct SREGS sregs;
  struct media_info media;
  /* The next line is mostly for those compilers that require an lvalue for FP_SEG (eg MSC),
     but also allows makes sure &media to a far pointer (mostly for digital mars)
  */
  struct media_info far *media_ptr = &media;  
  int driveNum;

  if ((drive == NULL) || (*drive == '\0'))  /* not given use current drive */
    driveNum = 0;  /* getCurrentDrive(); */
  else
    if ((*(drive + 1) == ':') || (*(drive + 1) == '\0'))
    {
      driveNum = (*drive & '\xDF') - 'A' + 1;
    }
    else
    {
      /* possibly set errno here? */
      return -1; /* invalid drive specified */
    }

  /* Get Serial Number, only supports drives mapped to letters */
  regs.x.ax = 0x440D;           /* Generic IOCTL */
  regs.x.bx = driveNum;
  regs.x.cx = 0x0866;           /* CH=disk drive, CL=Get Serial # */
  regs.x.dx = FP_OFF(media_ptr);
  sregs.ds = FP_SEG(media_ptr);
  int86x(0x21, &regs, &regs, &sregs);
  if (regs.x.cflag != 0)
  {
    serial->dwSerial = 0;
    return -1;
  }
  else /* successfully got serial # */
  {
    serial->dwSerial = media.serial;
    return 0;
  }
}
#endif /* intr vs int86x, FF_DOS_INTR vs FF_DOS_INT86X */
#endif /* FF_INCLUDE_FUNCTION_DEFS */
#endif /* FF_GETSERIAL_DOS */

#ifdef FF_GETSERIAL_WIN32
#ifndef FF_GETVOLLBL_WITH_W32GETVOLINFO
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif
#define getSerialNumber(drive, serial) ((GetVolumeInformation(drive ,NULL, 0, (LPDWORD)serial, NULL, NULL, NULL, 0) == 0) ? -1 : 0)
#endif


#ifdef FF_WIN32_FINDFIRSTFILE
/* Use Win32 API's FindFirstFile/FindNextFile/FindClose functions. */

/* A common case is to use Win32 GetVolumeInfo & Win32 FindFile functions, 
   so don't include windows header files twice if we just included them.  */
#ifndef FF_GETVOLLBL_WITH_W32GETVOLINFO
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

typedef struct ff_data
{
  HANDLE handle;
  WIN32_FIND_DATA w32fdata;
} FFDATA;

#define findFirst(filespec, ffdata) ((((ffdata)->handle = FindFirstFile(filespec, &((ffdata)->w32fdata)))==INVALID_HANDLE_VALUE) ? -1 : 0)
/* Note findFirstEx is note available */
#define findNext(ffdata) ((FindNextFile((ffdata)->handle, &((ffdata)->w32fdata)) != TRUE) ? -1 : 0)
#define findClose(ffdata) FindClose((ffdata)->handle)

#define FF_GetFileName(ffdata)   (ffdata)->w32fdata.cFileName
#define FF_GetAttributes(ffdata) (unsigned int)((ffdata)->w32fdata.dwFileAttributes)

typedef unsigned long _fsize_t;
typedef _fsize_t FSIZE_T;
#define FF_GetFileSize(ffdata) (FSIZE_T)((ffdata)->w32fdata.nFileSizeLow)
#define FF_GetFileSizeHigh(ffdata) (FSIZE_T)((ffdata)->w32fdata.nFileSizeHigh)

#endif /* FF_WIN32_FINDFIRSTFILE */


#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* FINDFILE_H */


/**
 * The rest of this file is for demonstration and testing purposes.
 * To test/demonstrate, create a c source file and simply include
 * the three lines:
 *        #define FF_TEST
 *        #define FF_INCLUDE_FUNCTION_DEFS
 *        #include "findfile.h"
 * then compile and run.
 * For generic Win32 or POSIX support, define USE_WIN32 or USE_POSIX 
 * respectively before the #include "findfile.h" line.
 */
#ifdef FF_TEST

#ifndef MICROC
#include <stdio.h>  /* Do not include FindFile.h and stdio.h when using Micro-C/PC */
#endif 
/* 
#define FF_INCLUDE_FUNCTION_DEFS
#include "findfile.h" 
*/

int main(void)
{
  FFDATA ffdata;
  /* char *curDir; */
  char curDir[FF_MAXPATH];

  printf("Current drive is %c:\n", 'A' + getCurrentDrive());

  if (getCurrentDirectoryEx(curDir, FF_MAXPATH) == NULL)
  /* if ((curDir = getCurrentDirectory()) == NULL) */
  {
    printf("Error getting current directory!\n");
    return 1;
  }

  printf("Looking in %s for files matching *.c\n\n", curDir);

  if (findFirst("*.c", ADDRESSOF(ffdata)))
    printf("No files matching *.c found.\n");
  else
  {
    printf("DIR ARC RDO HID SYS  Name\n");
    do 
    {
      printf(" %c   %c   %c   %c   %c  %s\n",
        (FF_GetAttributes(ADDRESSOF(ffdata)) & FF_A_DIRECTORY) ? 'Y' : 'N',
        (FF_GetAttributes(ADDRESSOF(ffdata)) & FF_A_ARCHIVE) ? 'Y' : 'N',
        (FF_GetAttributes(ADDRESSOF(ffdata)) & FF_A_READONLY) ? 'Y' : 'N',
        (FF_GetAttributes(ADDRESSOF(ffdata)) & FF_A_HIDDEN) ? 'Y' : 'N',
        (FF_GetAttributes(ADDRESSOF(ffdata)) & FF_A_SYSTEM) ? 'Y' : 'N',
        FF_GetFileName(ADDRESSOF(ffdata))
      );
    } while (findNext(ADDRESSOF(ffdata)) == 0);
    findClose(ADDRESSOF(ffdata));
  }

  /* if (curDir != NULL) free(curDir); */

  return 0;
}

#endif /* FF_TEST */
