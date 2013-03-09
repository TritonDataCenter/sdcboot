/****************************************************************************

  findfile: Generic FindFirst and FindNext routines for various
            compilers.

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

/* The #asm / #endasm statements below cause problems as Digital Mars
   will preprocess all statements that start with #, so the Pacific-C code 
  (#asm/#endasm) will cause an error when compiling with Digital Mars.
   Personally I think thats messed up, but hey, the best work around I have
   [ please let me know if you know another ] is to have findfile.h 
   include a second file that actually contains the #asm / #endasm 
   statements, so Digital Mars will never see them.
*/

/* Possible bug - may fail when shouldn't, currently being looked at */

#if defined HI_TECH_C  /* Pacific-C - free compiler for DOS */

#include <stdlib.h>
#include <string.h>
#include <sys.h>
#include <dos.h>
extern int	errno;

#define FF_MAXPATH 80
#define FF_MAXDRIVE 3
#define FF_MAXFILEDIR 66
#define FF_MAXFILENAME 9
#define FF_MAXFILEEXT 5

#if (FF_MAXFILENAME + FF_MAXFILEEXT) > FF_MAXPATH
#define MAXFILENAME FF_MAXPATH
#else
#define MAXFILENAME (FF_MAXFILENAME + FF_MAXFILEEXT)
#endif


#ifndef FF_INCLUDE_FUNCTION_DEFS

int getCurrentDrive(void);
void setCurrentDrive(int drive);
char *getCurrentDirectory(void);
char *getCurrentDirectoryEx(char *buffer, int size);
char * getCWD(int drive);

#else

int getCurrentDrive(void)
{
  char *driveString = getdrv();
  return (int)(*driveString - 'A');
}

void setCurrentDrive(int drive)
{
  char *driveString = "A:";
  *driveString += drive;
  chdrv(driveString);
}

char *getCurrentDirectory(void)
{
  char *buf1, *buf2;
  buf1 = getcwd("");
  if (buf1 == NULL) return NULL;
  buf2 = (char *)malloc((strlen(buf1)+1)*sizeof(char));
  if (buf2 == NULL) return NULL;
  strcpy(buf2, buf1);
  return buf2;
}

char *getCurrentDirectoryEx(char *buffer, int size)
{
  char *buf1;
  buf1 = getcwd("");
  if (buf1 == NULL) return NULL;
  if ( (buffer == NULL) || ((strlen(buf1)+1) > size) ) return NULL;
  strcpy(buffer, buf1);
  return buffer;
}

char * getCWD(int drive)
{
  char *buf1, *buf2;
  char *driveString = "A:";
  *driveString += drive;
  buf1 = getcwd(driveString);
  if (buf1 == NULL) return NULL;
  buf2 = (char *)malloc((strlen(buf1)+1)*sizeof(char));
  if (buf2 == NULL) return NULL;
  strcpy(buf2, buf1);
  return buf2;
}

#endif  /* FF_INCLUDE_FUNCTION_DEFS */


#define FF_A_LABEL     0x08
#define FF_A_DIRECTORY 0x10
#define FF_A_ARCHIVE   0x20
#define FF_A_READONLY  0x01
#define FF_A_HIDDEN    0x02
#define FF_A_SYSTEM    0x04
#define FF_A_NORMAL    0x00

#define FF_FINDFILE_ATTR (FF_A_DIRECTORY | FF_A_ARCHIVE | FF_A_READONLY | FF_A_HIDDEN | FF_A_SYSTEM | FF_A_NORMAL)

/* Find File macros */
typedef struct ffdata
{
  char dontuse[21];
  char attrib;
  unsigned int ftime;
  unsigned int fdate;
  unsigned long fsize;
  char name[13];
} FFDATA;

#define findFirst(filespec, ffdata) findFirstEx(filespec, ffdata, FF_FINDFILE_ATTR)
#define findClose(ffdata)

#ifndef FF_INCLUDE_FUNCTION_DEFS
int findFirstEx(far char *filespec, far FFDATA *ffdata, int attr);
int findNext(far FFDATA *ffdata);
#else
int findFirstEx(far char *filespec, far FFDATA *ffdata, int attr)
{
  int retcode = 0;

#asm
  push DS           /*  ; Save registers compiler may care about */
  push ES

  mov AH, #2Fh       /*  ; Get the DTA to save it, returned in ES:BX */
  int #21h
  push BX           /*  ; Actually save it */
  push ES

  mov AH, #1Ah       /*  ; Point to DTA to our FindFile Structure (ffdata), DS:DX */
  LDS DX, 0+8[bp]
  int #21h


  mov AX, #4E00h        ; Actually perform DOS findfirst, filespec in DS:DX
  LDS DX, 0+4[bp]
  mov CX, 0+12[bp],word /* mov CX, [attr] */
  int #21h
  jnc 1f                ; If carry is set then error occured
  mov 0+-2[bp],AX,word  /* mov [retcode], AX */
1:
  mov AH, #1Ah           ; Restore DTA
  pop DS                ; from saved ES:BX into DS:DX
  pop DX
  int #21h

  pop ES                ; Restore Segments we saved earlier
  pop DS
#endasm
  
  errno = retcode;
  if (retcode) return -1;
  else return 0;
}

int findNext(far FFDATA *ffdata)
{
  int retcode = 0;

#asm
  push DS             ; Save registers compiler may care about
  push ES

  mov AH, #2Fh         ; Get the DTA to save it, returned in ES:BX
  int #21h
  push BX             ; Actually save it
  push ES

  mov AH, #1Ah         ; Point to DTA to our FindFile Structure (ffdata), DS:DX
  LDS DX, 0+4[bp]
  int #21h


  mov AX, #4F00h        ; Actually perform DOS findfirst, filespec in DS:DX
  int #21h
  jnc 1f                ; If carry is set then error occured
  mov 0+-2[bp],AX,word  /* mov [retcode], AX */
1:
  mov AH, #1Ah           ; Restore DTA
  pop DS                ; from saved ES:BX into DS:DX
  pop DX
  int #21h

  pop ES                ; Restore Segments we saved earlier
  pop DS
#endasm
  
  errno = retcode;
  if (retcode) return -1;
  else return 0;
}
#endif

#define FF_GetFileName(ffdata)   (ffdata)->name
#define FF_GetAttributes(ffdata) (ffdata)->attrib

typedef long _fsize_t;
typedef unsigned long FSIZE_T;
#define FF_GetFileSize(ffdata) (FSIZE_T)((ffdata)->fsize)

#define FF_GETVOLLBL_WITH_FINDFIRST
#define FF_GETSERIAL_DOS
#define FF_DOS_INT86X


#endif
