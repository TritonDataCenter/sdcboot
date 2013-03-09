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

 * The purpose of this tool is to set any environment variables that are
 * missing on the system
 */

#include <stdio.h>		/* printf(), etc... */
#include <stdlib.h>		/* _splitpath(), _makepath(), etc... */
#include <string.h>		/* strcpy(), strcmp(), etc... */
#include <stdarg.h>
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

#ifdef FEATURE_INTERNAL_SETVARS
#define main(x,y) setvars(x,y)
#endif

#ifdef __WATCOMC__
#define enable _enable
#define disable _disable
#endif

#ifdef __PACIFIC__
#define enable()
#define disable()
#endif

static unsigned head, tail, start, end;
static int idx = 0;
static unsigned short keystack[16][2];

#if !defined(__WATCOMC__) || !defined(__SMALL__)
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

unsigned short Peekw(unsigned seg, unsigned ofs) {
	unsigned far *ptr;
	ptr = MK_FP(seg, ofs);
	return *ptr;
}

void Pokew(unsigned seg, unsigned ofs, unsigned short num) {
	unsigned far *ptr;
	ptr = MK_FP(seg, ofs);
	*ptr = num;
}

/*
 * Stuff a key into the keyboard buffer
 */
int ungetkey(unsigned short key) {
	int count;

	head  = Peekw(0x40, 0x1a);
	tail  = Peekw(0x40, 0x1c);
	start = Peekw(0x40, 0x80);
	end   = Peekw(0x40, 0x82);

	count = tail - head;
	if (0 > count) count += (16 * sizeof(unsigned));
	count >>= 1;

	if (15 > count) {
		disable();
		keystack[idx][0] = Peekw(0x40, tail);
		keystack[idx][1] = tail;
		Pokew(0x40, tail, key);
		tail += sizeof(unsigned);
		if (end <= tail)
		tail = start;
		Pokew(0x40, 0x1c, tail);
		enable();
		return key;
	}
	return EOF;
}

/*
 * Stuff a string into the keyboard buffer
 */
int kb_stuff(char *str) {
	int ercode = 0;

	idx = 0;
	while (*str) {
		if (EOF == ungetkey((unsigned)(*str++))) {	/* Check for EOF */
			disable();
			while (0 <= --idx) {
				tail = keystack[idx][1];
				Pokew(0x40, tail, keystack[idx][0]);
			}
			Pokew(0x40, 0x1c, tail);
			enable();
			ercode = -1;
			break;
		}
		else  ++idx;
	}
	idx = 0;
	return ercode;
}

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

/*
 * This is the help function - pretty self-explanatory; it prints the help message
 */
void help(void)
{
	kitten_printf(5,0,"FDPKGVARS v1.0 - GPL by Blair Campbell 2006\n");
	kitten_printf(5,1,"Sets up environment variables on systems that don't have them set\n");
	kitten_printf(5,2,"This tool takes no arguments - It should set the following:\n");
	kitten_printf(5,3,"%%DOSDIR%%, %%AUTOFILE%%, and %%CFGFILE%%\n");
}

/*
 * Search for appropriate directories in a would-be %DOSDIR%
 */
int find_dir(const char *base) {
	char full[_MAX_PATH];
	sprintf(full, "%s\\BIN", base);
	if(access(full, 0) != 0) return -1;
	sprintf(full, "%s\\DOC", base);
	if(access(full, 0) != 0) return -1;
	sprintf(full, "%s\\NLS", base);
	if(access(full, 0) != 0) return -1;
	sprintf(full, "%s\\HELP", base);
	if(access(full, 0) != 0) return -1;
	return 0;
}

/*
 * Returns 1 for failure, 0 for success
 */
int main(int argc, char **argv) {
	char *dosdir,
	     *fdauto,
	     *fdconfig,
	     dd[_MAX_PATH],
	     fa[_MAX_PATH],
	     fc[_MAX_PATH],
	     temp[12],
	     executestring[_MAX_PATH];
	int ret = -1;
	FILE *fp;
	kittenopen("FDPKG");
	if((argc&&argv[1][0]=='/'||argv[1][0]=='-')&&(argv[1][1]=='?'||tolower(argv[1][1])=='h'))
	{
		help();
		kittenclose();
		return 1;
	}
        sprintf(temp, "%s.BAT", _mktemp("XXXXXX"));
	while(access(temp, 0) == 0) sprintf(temp, "%s.BAT", _mktemp("XXXXXX"));
	if((fp = fopen(temp, "w")) == NULL) {
		kitten_printf(5,4,"Could not open temp files\n");
		return -1;
	}
	while(kbhit()) getch();
	sprintf(executestring, "%s\r", temp);
	kb_stuff(executestring);
	fprintf(fp, "@echo off\n");
	if((dosdir = getenv("DOSDIR")) == NULL) {
		if		(access("C:\\FDOS"   , 0) == 0) {
			ret = find_dir("C:\\FDOS");
			if(ret == 0) strcpy(dd, "C:\\FDOS");
		} if(ret != 0 && access("C:\\FREEDOS", 0) == 0) {
			ret = find_dir("C:\\FREEDOS");
			if(ret == 0) strcpy(dd, "C:\\FREEDOS");
		} if(ret != 0 && access("C:\\DOS"    , 0) == 0) {
			ret = find_dir("C:\\DOS");
			if(ret == 0) strcpy(dd, "C:\\DOS");
		} if(ret != 0 && access("D\\FDOS"    , 0) == 0) {
			ret = find_dir("D:\\FDOS");
			if(ret == 0) strcpy(dd, "D:\\FDOS");
		} if(ret != 0 && access("D\\FREEDOS" , 0) == 0) {
			ret = find_dir("D:\\FREEDOS");
			if(ret == 0) strcpy(dd, "D:\\FREEDOS");
		} if(ret != 0 && access("D\\DOS"     , 0) == 0) {
			ret = find_dir("D:\\DOS");
			if(ret == 0) strcpy(dd, "D:\\DOS");
		} if(ret != 0) {
			kitten_printf(5,5,"Could not find suitable directory for %%DOSDIR%%\n");
			fclose(fp);
			return -1;
		} else fprintf(fp, "SET DOSDIR=%s\n", dd);
	}
	if((fdauto = getenv("AUTOFILE")) == NULL) {
		if(	access("C:\\FDAUTO.BAT",   0) == 0) strcpy(fa, "C:\\FDAUTO.BAT");
		else if(access("C:\\AUTOEXEC.BAT", 0) == 0) strcpy(fa, "C:\\AUTOEXEC.BAT");
		else {
			kitten_printf(5,6,"Could not find suitable autoexec.bat\n");
			fprintf(fp, "DEL %s\x1a", temp);
			fclose(fp);
			return -1;
		}
		fprintf(fp, "SET AUTOFILE=%s\necho SET AUTOFILE=%s >> %s\n", fa, fa, fa);
		if(dosdir == NULL) fprintf(fp, "echo SET DOSDIR=%s >> %s\n", dd, fa);
	} else if(dosdir == NULL) fprintf(fp, "echo SET DOSDIR=%s >> %s\n", dd, fdauto);
	if((fdconfig = getenv("CFGFILE")) == NULL) {
		if(	access("C:\\FDCONFIG.SYS", 0) == 0) strcpy(fc, "C:\\FDCONFIG.SYS");
		else if(access("C:\\CONFIG.SYS"  , 0) == 0) strcpy(fc, "C:\\CONFIG.SYS");
		else {
			kitten_printf(5,7,"Could not find suitable config.sys\n");
			fprintf(fp, "DEL %s\x1a", temp);
			fclose(fp);
			return -1;
		}
		fprintf(fp, "SET CFGFILE=%s\necho SET CFGFILE=%s >> %s\n",
				fc, fc, (fdauto == NULL) ? fa : fdauto);
	}
	fprintf(fp, "DEL %s\x1a", temp);
	fclose(fp);
	return 0;
}

