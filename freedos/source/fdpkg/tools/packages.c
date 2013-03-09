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

 * The purpose of this tool is to create directories in %DOSIDIR%\PACKAGES
 * for packages that are installed but don't have directories (only for the
 * base diskset and some select others)
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

#ifdef FEATURE_INTERNAL_PKGDIRS
#define main(x,y) make_packages_dirs(x,y)
#define SUCCESS 20
#define FAILURE 21
#else
#define SUCCESS 0
#define FAILURE 1
#endif

char *dosdir = NULL;

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

/*
 * This is the help function - pretty self-explanatory; it prints the help message
 */
void help(void)
{
	kitten_printf(6,0,"PACKAGES v1.0 - GPL by Blair Campbell 2006\n");
	kitten_printf(6,1,"Makes a directory in %%DOSDIR%%\\PACKAGES for every package in\n");
	kitten_printf(6,2,"BASE if this package doesn't already have its own directory.\n");
	kitten_printf(6,3,"(so that if packages depend on them then they will be found)\n");
}

/*
 * Creates a PACKAGES directory for "package" if "file" is found in %DOSDIR%
 */
void find_create_dir(const char *file, const char *package) {
	char full[_MAX_PATH];
	sprintf(full, "%s\\%s", dosdir, file);
	if(access(full, 0) != 0) return;
	sprintf(full, "%s\\PACKAGES\\%s", dosdir, package);
	mkdir(full);
}

/*
 * Returns 1 for failure, 0 for success
 */
int main(int argc, char **argv) {
	dosdir = getenv("DOSDIR");
	kittenopen("FDPKG");
	if(argc&&(argv[1][0]=='/'||argv[1][0]=='-')&&(argv[1][1]=='?'||tolower(argv[1][1])=='h'))
	{
		help();
		kittenclose();
		return FAILURE;
	}
	find_create_dir("BIN\\APPEND.EXE",   "APPENDX" );
	find_create_dir("BIN\\ASSIGN.COM",   "ASSIGNX" );
	find_create_dir("BIN\\ATAPICDD.SYS", "ATAPICDX");
	find_create_dir("BIN\\ATTRIB.COM",   "ATTRIBX" );
	find_create_dir("BIN\\BOOTFIX.COM",  "BOOTFIXX");
	find_create_dir("BIN\\CDRCACHE.SYS", "CDRCACHX");
	find_create_dir("BIN\\CHANGE.BAT",   "CHANGEX" );
	find_create_dir("BIN\\CHKDSK.EXE",   "CHKDSKX" );
	find_create_dir("BIN\\CHOICE.EXE",   "CHOICEX" );
	find_create_dir("HELP\\FREECOM.EN",  "CMDCOREX");
	find_create_dir("BIN\\KSSF.COM",     "COMMANDX");
	find_create_dir("BIN\\COMP.COM",     "COMPX"   );
	find_create_dir("BIN\\CTMOUSE.EXE",  "CTMOUSEX");
	find_create_dir("BIN\\CWSDPMI.EXE",  "CWSDPMIX");
	find_create_dir("BIN\\DEFRAG.EXE",   "DEFRAGX" );
	find_create_dir("BIN\\DELTREE.COM",  "DELTREEX");
	find_create_dir("BIN\\DEVLOAD.COM",  "DEVLOADX");
	find_create_dir("BIN\\DISPLAY.EXE",  "DISPLAYX");
	find_create_dir("BIN\\DISPLAY.SYS",  "DISPLAYX");
	find_create_dir("BIN\\DOSFSCK.EXE",  "DOSFSCKX");
	find_create_dir("BIN\\DISKCOMP.COM", "DSKCOMPX");
	find_create_dir("BIN\\DISKCOPY.EXE", "DSKCOPYX");
	find_create_dir("BIN\\EDIT.EXE",     "EDITX"   );
	find_create_dir("BIN\\EDLIN.EXE",    "EDLINX"  );
	find_create_dir("BIN\\EMM386.EXE",   "EMM386X" );
	find_create_dir("BIN\\EXE2BIN.COM",  "EXE2BINX");
	find_create_dir("BIN\\FASTHELP.EXE", "FASTHLPX");
	find_create_dir("BIN\\FC.EXE",       "FCX"     );
	find_create_dir("BIN\\FDAPM.COM",    "FDAPMX"  );
	find_create_dir("BIN\\FDISK.EXE",    "FDISKX"  );
	find_create_dir("BIN\\FDPKG.BAT",    "FDPKGX"  );
	find_create_dir("BIN\\FDPKG.EXE",    "FDPKGX"  );
	find_create_dir("BIN\\FDXMS.SYS",    "FDXMSX"  );
	find_create_dir("BIN\\FIND.COM",     "FINDX"   );
	find_create_dir("BIN\\FORMAT.EXE",   "FORMATX" );
	find_create_dir("BIN\\FDSHIELD.COM", "FSHIELDX");
	find_create_dir("BIN\\FDXMS286.SYS", "FXMS286X");
	find_create_dir("COPYING",           "GPLV2X"  );
	find_create_dir("BIN\\HELP.EXE",     "HELPX"   );
	find_create_dir("BIN\\UNZIP.EXE",    "INFOZIPX");
	find_create_dir("BIN\\KEYB.EXE",     "KEYBX"   );
	find_create_dir("BIN\\LABEL.EXE",    "LABELX"  );
	find_create_dir("BIN\\LBACACHE.EXE", "LBACACHX");
	find_create_dir("BIN\\MEMA.COM",     "MEMAX"   );
	find_create_dir("BIN\\MEM.EXE",      "MEMX"    );
	find_create_dir("BIN\\MIRROR.EXE",   "MIRRORX" );
	find_create_dir("BIN\\MODE.COM",     "MODEX"   );
	find_create_dir("BIN\\MORE.EXE",     "MOREX"   );
	find_create_dir("BIN\\MOVE.EXE",     "MOVEX"   );
	find_create_dir("BIN\\NANSI.SYS",    "NANSIX"  );
	find_create_dir("BIN\\NLSFUNC.EXE",  "NLSFUNCX");
	find_create_dir("BIN\\PRINTQ.EXE",   "PRINTQX" );
	find_create_dir("BIN\\PRINT.COM",    "PRINTX"  );
	find_create_dir("BIN\\RECOVER.EXE",  "RECOVERX");
	find_create_dir("BIN\\REPLACE.EXE",  "REPLACEX");
	find_create_dir("POSTINST.BAT",      "SAMCFGX" );
	find_create_dir("BIN\\SHARE.COM",    "SHAREX"  );
	find_create_dir("BIN\\SHSUCDX.COM",  "SHSUCD3X");
	find_create_dir("BIN\\SORT.COM",     "SORTX"   );
	find_create_dir("BIN\\SUBST.EXE",    "SUBSTX"  );
	find_create_dir("BIN\\SYS.COM",      "SYSX"    );
	find_create_dir("BIN\\TDSK.EXE",     "TDSKX"   );
	find_create_dir("BIN\\TREE.COM",     "TREEX"   );
	find_create_dir("BIN\\UDMA2.SYS",    "UDMA2X"  );
	find_create_dir("BIN\\UNDELETE.EXE", "UNDELX"  );
	find_create_dir("BIN\\UNFORMAT.EXE", "UNFMTX"  );
	find_create_dir("BIN\\XCDROM.SYS",   "XCDROMX" );
	find_create_dir("BIN\\XCOPY.EXE",    "XCOPYX"  );
	find_create_dir("BIN\\DOS32A.EXE",   "DOS32AX" );
	find_create_dir("BIN\\DOS4GW.EXE",   "DOS4GWX" );
	find_create_dir("BIN\\GO32-V2.EXE",  "GO32V2X" );
	find_create_dir("BIN\\WGET.EXE",     "WGETX"   );
	find_create_dir("BIN\\CURL.EXE",     "CURLX"   );
	find_create_dir("BIN\\HTGET.EXE",    "HTGETX"  );
	find_create_dir("BIN\\DOSLFN.COM",   "DOSLFNX" );
	find_create_dir("BIN\\DOSLFNMS.COM", "DOSLFNX" );
	find_create_dir("BIN\\4DOS.EXE",     "4DOSX"   );
	find_create_dir("BIN\\PCISLEEP.COM", "PCISLEPX");
	return SUCCESS;
}

