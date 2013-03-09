/***************************************************************************
 * This program is free software; you can redistribute it and/or modify it *
 * under the terms of the GNU General Public License version 2 as          *
 * published by the Free Software Foundation.                              *
 *                                                                         *
 * This program is distributed in the hope that it will be useful, but     *
 * WITHOUT ANY WARRANTY; without even the implied warranty of              *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU        *
 * General Public License for more details.                                *
 *                                                                         *
 * You should have received a copy of the GNU General Public License along *
 * with this program; if not, write to the Free Software Foundation, Inc., *
 * 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.                *
 ***************************************************************************

 * The purpose of this tool is to add lines to config.sys or autoexec.bat in
 * a configurable manner, an aid to batch files that can't directly do
 * complex editing of configuration files themselves

 */

#include <stdio.h>		/* printf(), etc... */
#include <stdlib.h>		/* _splitpath(), _makepath(), etc... */
#include <stdarg.h>             /* va_list, va_arg(), etc... */
#include <string.h>		/* strcpy(), strcmp(), etc... */
#include <conio.h>
#ifdef __WATCOMC__
#include <direct.h>		/* chdir() in Watcom C */
#include <process.h>		/* spawn...() */
#include <io.h>			/* unlink() */
#else/*__WATCOMC__*/
#ifndef __PACIFIC__
#include <dir.h>		/* chdir() in Turbo C */
#include <process.h>		/* spawn...() */
#else/* __PACIFIC__*/
#include <sys.h>		/* chdir() in Pacific C */
#include <unixio.h>		/* unlink() in Pacific C */
#endif/*__PACIFIC__*/
#endif/*__WATCOMC__*/
#include <ctype.h>		/* toupper() */
#include <dos.h>		/* getcwd(), etc... */
#include "../func.h"		/* manippkg(), pause(), etc... */
#include "../misclib/kitten.h"	/* kittenopen() , kittenclose() */
#include "../misclib/misc.h"    /* Compatibility macros, etc... */

#ifdef FEATURE_INTERNAL_AUTOADD
#define main(x,y) autoexec_add(x,y)
#define SUCCESS 30
#define FAILURE 31
#else
#define SUCCESS 0
#define FAILURE 1
#endif

#define AUTOEXEC 0
#define CONFIGSY 1
#define OTHERCFG 2

#define BEFORE 0
#define AFTER 1
#define AFTERPATH 2
#define BEFOREPATH 3
#define AFTERXMS 4
#define AFTEREMM 5

#define lastchar(string) string[strlen(string) - 1 ]

/*
 * The following is a macro-like function that uses kitten instead of directly doing printf
 */
void kitten_printf(short x, short y, char *fmt, ...) {
	va_list args;
	va_start(args, fmt);
#ifndef NO_KITTEN
	vprintf(kittengets(x,y,fmt), args);
#else
#ifdef __TURBOC__
	if(x||y);
#endif
	vprintf(fmt, args);
#endif
}

#ifndef __PACIFIC__
/*
 * String case-insensitive search within a string
 */
char *stristr(char *string, char *search) {
    return strstr(strlwr(string), strlwr(search));
}
#endif

unsigned short file = AUTOEXEC, loc = AFTER, bak = 1;
char filename[_MAX_PATH];
char *findtext = NULL;

#if !defined(__WATCOMC__) || !defined(__SMALL__)
/* Get the DOS switch character (undocumented) */
int switchcharacter(void) {
#if defined(__WATCOMC__)
	int ret, l;
	__asm {
		mov ax, 0x3700
		int 0x21
		mov (unsigned int)ret, dl
		mov (unsigned int)l, al
	}
	return l==0?ret:'/';
#else
	union REGS r;
	r.x.ax = 0x3700;
	intdos(&r, &r);
	return (r.h.al==0)?r.h.dl:'/';
#endif
}

#ifndef __TURBOC__
/* Get the attributes for a file */
int getattr(const char *file) {
	union REGS r;
	struct SREGS s;
	r.x.ax = 0x4300;
	s.ds = FP_SEG(file);
	r.x.dx = FP_OFF(file);
	intdosx(&r, &r, &s);
	return (r.x.cflag) ? -1 : (int)r.x.cx;
}
#endif
#endif

/*
 * This is the help function - pretty self-explanatory; it prints the help message
 */
void help(void)
{
    kitten_printf(7,0, "AUTOADD v1.0 - GPL by Blair Campbell 2006\n");
    kitten_printf(7,1, "Modifies autoexec.bat or config.sys to add, delete, or change lines.\n");
    kitten_printf(7,2, "Syntax: AUTOADD [/switches] addtext\n");
    kitten_printf(7,3, "Options:\n");
    kitten_printf(7,4, "   /C        Modify config.sys (default: autoexec.bat)\n");/**/
    kitten_printf(7,5, "   /F:File   Modify \"File\" instead of default file\n");/**/
    kitten_printf(7,6, "   /A:text   Add \"addtext\" after specified \"text\"\n");/**/
    kitten_printf(7,7, "   /B:text   Add \"addtext\" before specified \"text\"\n");/**/
    kitten_printf(7,8, "   /P:[B/E]  Modify the %%PATH%% variable by adding \"addtext\"\n");/**/
    kitten_printf(7,9, "             /P:B adds \"addtext\" at the beginning of %%PATH%%\n");
    kitten_printf(7,10,"             /P:E adds \"addtext\" at the end of %%PATH%%\n");
    kitten_printf(7,11,"   /X        Load after an XMS driver (config.sys)\n");/**/
    kitten_printf(7,12,"   /E        Load after EMM386 (config.sys)\n");/**/
    kitten_printf(7,13,"   /N        Don't backup autoexec.bat or config.sys\n");
    kitten_printf(7,14,"Note: /P modifies %%PATH%% in both autoexec and makes a batch file.\n");
    kitten_printf(7,15,"Default: \"addtext\" is added at the end of the file or %%PATH%%.\n");
}

/*
 * Argument parser
 */
static int checkarg(const char *opt)
{
	short i, len;

	len=strlen(opt);
	if(len==0)
	{
		kitten_printf(7, 16, "Invalid option in %s\n", opt);
		return FAILURE;
	}
	for(i=0;i<=len;i++) {
		switch(toupper(opt[i]))	{
		case 'H':
		case '?':
			help();
			return FAILURE;
		case 'C':
			file = CONFIGSY;
			break;
		case 'F':
			i++;
			if(opt[i] == ':' || opt[i] == '=') i++;
			strcpy(filename, &opt[i]);
			file = OTHERCFG;
			i += strlen(opt);
			break;
		case 'A':
			i++;
			if(opt[i] == ':' || opt[i] == '=') i++;
			findtext = (char *)malloc( strlen( &opt[i] ) + 1 );
			strcpy(findtext, &opt[i]);
			i += strlen(opt);
			break;
		case 'B':
			i++;
			if(opt[i] == ':' || opt[i] == '=') i++;
			findtext = (char *)malloc( strlen( &opt[i] ) + 1 );
			strcpy(findtext, &opt[i]);
			i += strlen(opt);
			loc = BEFORE;
			break;
		case 'P':
			i++;
			if(opt[i] == ':' || opt[i] == '=') i++;
			if(opt[i] == 'B') loc = BEFOREPATH;
			else loc = AFTERPATH;
			break;
		case 'X':
			loc = AFTERXMS;
			break;
		case 'E':
			loc = AFTEREMM;
			break;
		case 'N':
			bak = 0;
			break;
		}
	}
	return SUCCESS;
}

/* Copy one file to another file */
static void fcopy(const char *src, const char *dst)
{
	FILE *source, *dest;
	char buffer[270];
	buffer[0] = '\0';
	source = fopen(src, "r");
	dest = fopen(dst, "w");
	while(fgets(buffer, 270, source) != NULL) fputs(buffer, dest);
	fclose(source);
	fclose(dest);
}

/*
 * Returns 1 for failure, 0 for success
 */
int main(int argc, char **argv) {
#ifdef __TURBOC__
    char *dosdir;
#else
    char *dosdir = NULL;
#endif
    char bakfname[_MAX_PATH],
         drive[_MAX_DRIVE],
         dir[_MAX_DIR],
         fname[_MAX_FNAME],
         ext[_MAX_EXT],
         readbuf[256];
    int i = 1, retval = FAILURE;
    FILE *src, *dst;
    filename[0] = '\0';
    dosdir = getenv("DOSDIR");
    if(dosdir == NULL) return FAILURE;
    kittenopen("FDPKG");
    while( argc > i &&
           argv[i][0] == '/' ||
           argv[i][0] == '-' ||
           argv[i][0] == switchcharacter()
           ) {
        if(checkarg(&argv[i][1]) != SUCCESS) return FAILURE;
	i++;
    }
    if(!argv[i]) {
        kitten_printf(7, 17, "Insufficient arguments.\n");
        return FAILURE;
    }
    if( file == AUTOEXEC ) {
        if(getenv("AUTOFILE") != NULL) strcpy(filename, getenv("AUTOFILE"));
        else if(access("C:\\FDAUTO.BAT", 0) == 0) strcpy(filename, "C:\\FDAUTO.BAT");
        else if(access("C:\\AUTOEXEC.BAT", 0) == 0) strcpy(filename, "C:\\AUTOEXEC.BAT");
        else {
            kitten_printf(7, 18, "Could not find Autoexec.bat.\n");
            return FAILURE;
        }
    } else if (file == CONFIGSY) {
        if(getenv("CFGFILE") != NULL) strcpy(filename, getenv("CFGFILE"));
        else if(access("C:\\FDCONFIG.SYS", 0) == 0) strcpy(filename, "C:\\FDCONFIG.SYS");
        else if(access("C:\\CONFIG.SYS", 0) == 0) strcpy(filename, "C:\\CONFIG.SYS");
        else {
            kitten_printf(7, 19, "Could not find Config.sys.\n");
            return FAILURE;
        }
    }
    if( filename[0] == '\0' ) return FAILURE;
    _splitpath(filename, drive, dir, fname, ext);
    _makepath(bakfname, drive, dir, fname, ".BAK");
    while( access(bakfname, 0) == 0 ) {
        _makepath(bakfname, drive, dir, fname, _mktemp(".XXX"));
    }
    fcopy(filename, bakfname);
    src = fopen(bakfname, "rt");
    dst = fopen(filename, "wt");
    if(src == NULL || dst == NULL) {
        kitten_printf(7, 20, "Could not open all files.\n");
        return FAILURE;
    }
    while(fgets(readbuf, 256, src) != NULL) {
        if(findtext != NULL || loc != AFTER) {
            if(loc == AFTERPATH || loc == BEFOREPATH) {
                if(stristr(readbuf, "SET PATH=") != NULL) {
                    if(lastchar( readbuf ) == '\n')
                        lastchar(readbuf) = '\0';
                    if(loc == AFTERPATH) sprintf(readbuf, "%s;%s\n", readbuf, argv[i]);
                    else {
                        strcpy(readbuf, &readbuf[10]);
                        sprintf(readbuf, "SET PATH=%s;%s\n", argv[i], readbuf);
                    }
                    retval = SUCCESS;
                }
            } else if(loc == AFTER) {
                if(stristr(readbuf, findtext) != NULL) {
                    fputs(readbuf, dst);
                    sprintf(readbuf, "%s\n", argv[i]);
                    retval = SUCCESS;
                }
            } else if(loc == BEFORE) {
                if(stristr(readbuf, findtext) != NULL) {
                    fputs(argv[i], dst);
                    fputs("\n", dst);
                    retval = SUCCESS;
                }
            } else if(loc == AFTERXMS) {
                if(stristr(readbuf, "DEVICE") != NULL && (stristr(readbuf, "HIMEM.SYS") != NULL
                || stristr(readbuf, "HIMEM.EXE") != NULL)) {
                    fputs(readbuf, dst);
                    sprintf(readbuf, "%s\n", argv[i]);
                    retval = SUCCESS;
                }
            } else if(loc == AFTEREMM) {
                if(stristr(readbuf, "DEVICE") != NULL && (stristr(readbuf, "EMM386.SYS") != NULL
                || stristr(readbuf, "EMM386.EXE") != NULL)) {
                    fputs(readbuf, dst);
                    sprintf(readbuf, "%s\n", argv[i]);
                    retval = SUCCESS;
                }
            }
        }
        fputs(readbuf, dst);
    }
    if(findtext == NULL && loc == AFTER) {
        fputs(argv[i], dst);
        fputs("\n", dst);
        retval = SUCCESS;
    }
    fclose(src);
    fclose(dst);
    return retval;
}

