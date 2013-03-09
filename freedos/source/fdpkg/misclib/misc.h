/*
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free
 * Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifndef __MISC_H
#define __MISC_H 1

#ifndef _MICROC_
#if (!defined(__SMALL__) && !defined(__TINY__)) || !defined(__WATCOMC__)
#define USE_C 1
#endif
#endif

#ifdef _MICROC_
#define DOSREGPACK struct REGPACK
#else
#if defined(__WATCOMC__)
#define DOSREGPACK union REGPACK
#define r_ds w.ds
#define r_dx w.dx
#define r_ax w.ax
#define r_es w.es
#define r_bx w.bx
#define r_cx w.cx
#define r_cl h.cl
#define r_bl h.bl
#define r_al h.al
#define r_dl h.dl
#define r_bl h.bl
#define r_si w.si
#define r_di w.di
#define r_flags w.flags
#elif defined(__PACIFIC__)
#define DOSREGPACK union REGPACK
#define r_ds x.ds
#define r_dx x.dx
#define r_ax x.ax
#define r_es x.es
#define r_bx x.bx
#define r_cx x.cx
#define r_cl h.cl
#define r_bl h.bl
#define r_al h.al
#define r_dl h.dl
#define r_bl h.bl
#define r_si x.si
#define r_di x.di
#define r_flags x.flags
#else
#define DOSREGPACK struct REGPACK
#endif
#endif

#ifdef __WATCOMC__

#include <graph.h>
#include <time.h>
#include <malloc.h>
#include <dos.h>

#ifdef __INCLUDE_FILES_H
#include <conio.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <fcntl.h>
#include <string.h>
#include <math.h>
#include <bios.h>
#include <io.h>
#include <errno.h>
#include <direct.h>
#include <assert.h>
#include "../func.h"
#endif

struct ftime /* As defined by Borland C */
{
	unsigned		ft_tsec  : 5;	/* Two second interval */
	unsigned		ft_min   : 6;	/* Minutes */
	unsigned		ft_hour  : 5;	/* Hours */
	unsigned		ft_day   : 5;	/* Days */
	unsigned		ft_month : 4;	/* Months */
	unsigned		ft_year  : 7;	/* Year */
};

struct fatinfo
{
	char			fi_sclus;	/* Sectors per cluster */
	char			fi_fatid;	/* The FAT id byte */
	int			fi_nclus;	/* Number of clusters */
	int			fi_bysec;	/* Bytes per sector */
};

struct fcb
{
	char			fcb_drive;	/* 0 = default, 1 = A, 2 = B */
	char			fcb_name[8];	/* File name */
	char			fcb_ext[3];	/* File extension */
	short			fcb_curblk;	/* Current block number */
	short			fcb_recsize;	/* Logical record size in bytes */
	long			fcb_filsize;	/* File size in bytes */
	short			fcb_date;	/* Date file was last written */
	char			fcb_resv[10];	/* Reserved for DOS */
	char			fcb_currec;	/* Current record in block */
	long			fcb_random;	/* Random record number */
};

struct text_info
{
	unsigned char		winleft;	/* left window coordinate */
	unsigned char		wintop;		/* top window coordinate */
	unsigned char		winright;	/* right window coordinate */
	unsigned char		winbottom;	/* bottom window coordinate */
	unsigned char		attribute;	/* text attribute */
	unsigned char		normattr;	/* normal attribute */
	unsigned char		currmode;	/* video mode */
	unsigned char		screenheight;	/* bottom - top */
	unsigned char		screenwidth;	/* right - left */
	unsigned char		curx;		/* x coordinate in window */
	unsigned char		cury;		/* y coordinate in window */
};

struct country
{
	int			co_date;	/* date format */
	char			co_curr[5];	/* currency symbol */
	char			co_thsep[2];	/* thousands separator */
	char			co_desep[2];	/* decimal separator */
	char			co_dtsep[2];	/* date separator */
	char			co_tmsep[2];	/* time separator */
	char			co_currstyle;	/* currency style */
	char			co_digits;	/* significant digits in currency */
	char			co_time;	/* time format */
	long			co_case;	/* case map */
	char			co_dasep[2];	/* data separator */
	char			co_fill[10];	/* filler */
};

extern union  REGS  BIP_regs_;
extern struct SREGS BIP_sregs_;

#define _AX			BIP_regs_.x.ax
#define _BX			BIP_regs_.x.bx
#define _CX			BIP_regs_.x.cx
#define _DX			BIP_regs_.x.dx
#define _AH			BIP_regs_.h.ah
#define _AL			BIP_regs_.h.al
#define _BH			BIP_regs_.h.bh
#define _BL			BIP_regs_.h.al
#define _CH			BIP_regs_.h.ch
#define _CL			BIP_regs_.h.al
#define _DH			BIP_regs_.h.dh
#define _DL			BIP_regs_.h.al
#define _SI			BIP_regs_.x.si
#define _DI			BIP_regs_.x.di
#define _CF			BIP_regs_.x.cflag
#define _FF			BIP_regs_.x.cflag
#define _ES			BIP_sregs_.es
#define _CS			BIP_sregs_.cs
#define _SS			BIP_sregs_.ss
#define _DS			BIP_sregs_.ds
#define regload_()		segread(&BIP_sregs_)

#define _argv			__argv
#define _argc			__argc

#define farmalloc		_fmalloc
#define farfree			_ffree
#define allocmem(x,y)		_dos_allocmem(x,(unsigned short *)y)
#define freemem			_dos_freemem
#define harderr			_harderr
#define hardresume		_hardresume
#define hardretn		_hardretn

#define asm			_asm
#define far			_far

#define MAXPATH			_MAX_PATH
#define MAXFILE			_MAX_FNAME
#define MAXEXT			_MAX_EXT
#define MAXDRIVE		_MAX_DRIVE
#define MAXDIR			_MAX_DIR

#define FA_RDONLY		_A_RDONLY
#define FA_ARCH			_A_ARCH
#define FA_SYSTEM		_A_SYSTEM
#define FA_HIDDEN		_A_HIDDEN
#define FA_DIREC		_A_SUBDIR
#define FA_LABEL		_A_VOLID
#define FA_NORMAL		_A_NORMAL
#define mktemp			_mktemp

#define findfirst(x,y,z)	_dos_findfirst(x,z,y)
#define findnext		_dos_findnext
#define ffblk			find_t
#define ff_name			name
#define ff_attrib		attrib
#define ff_reserved		reserved
#define ff_ftime		wr_time
#define ff_date			wr_date
#define ff_fsize		size

#define getdfree		_dos_getdiskfree
#ifdef USE_C
#define getdisk()		_getdrive()-1
#endif
#define dfree			_diskfree_t
#define df_total		total_clusters
#define df_avail		avail_clusters
#define df_sclus		sectors_per_cluster
#define df_bsec			bytes_per_sector

#define fnmerge			_makepath
#define SKIPSEP			0x1
#define NODRIVE			0x2
#define NOFNAME			0x4
#define NOEXT			0x8
#define WILDCARDS		0x01
#define EXTENSION		0x02
#define FILENAME		0x04
#define DIRECTORY		0x08
#define DRIVE			0x10

#define date			dosdate_t
#define da_year			year
#define da_mon			month
#define da_day			day
#define ti_hour			hour
#define ti_min			minute
#define ti_sec			second
#define ti_hund			hsecond
#define time			dostime_t
#define getdate			_dos_getdate
#define gettime			_dos_gettime
#define setdate			_dos_setdate
#define settime			_dos_settime

#define bioscom(x,y,z)		_bios_serialcom(x,z,y)
#define biosequip		_bios_equiplist
#define bioskey			_bios_keybrd
#define biosprint(x,y,z)	_bios_printer(x,z,y)

#define inport			inpw
#define inportb			inp
#define outport			outpw
#define outportb		outp
#define enable			_enable
#define disable			_disable
#define poke(a,b,c)		(*((int far*)MK_FP((a),(b))) = (int)(c))
#define pokeb(a,b,c)		(*((char far*)MK_FP((a),(b))) = (char)(c))
#define peek(a,b)		(*((int far*)MK_FP((a),(b))))
#define peekb(a,b)		(*((char far*)MK_FP((a),(b))))

#define r_ax			w.ax
#define r_bx			w.bx
#define r_cx			w.cx
#define r_dx			w.dx
#define r_bp			w.bp
#define r_si			w.si
#define r_di			w.di
#define r_ds			w.ds
#define r_es			w.es
#define r_flags			w.flags
#define flags			cflag

#ifdef USE_C
#define setverify(value)	bdos(0x2e,0,(value))
#define getverify()		bdos(0x54,0,0)&0xff
#endif
#define getvect			_dos_getvect
#define setvect			_dos_setvect
#define keep			_dos_keep

typedef int			_mexcep;
#define pow10(x)		pow(10,(double)x)
#define _matherr		__matherr
typedef struct _complex		complex;
#define complex(x,y)		__complex(x,y)
#define imag(a)			(a.y)
#define real(a)			(a.x)
#define arg(a)			(hypot(a.x, a.y))
#define randomize()		srand( ((unsigned int)time(NULL)) | 1)
#define random(num)		(rand() % num)

#define strncmpi		strnicmp

#define BLACK			0
#define BLUE			1
#define GREEN			2
#define CYAN			3
#define RED			4
#define MAGENTA			5
#define BROWN			6
#define LIGHTGRAY		7
#define DARKGRAY		8
#define LIGHTBLUE		9
#define LIGHTGREEN		10
#define LIGHTCYAN		11
#define LIGHTRED		12
#define LIGHTMAGENTA		13
#define YELLOW			14
#define WHITE			15
#define BLINK			16

#define COLORMODE		((*(char far *)0x0449) != 7)
#define EXT_KBD			(*(char far *)0x0496 & 16)
#define VIDPAGE			(*((unsigned char far *)0x0462))
#define ROWSIZE			(*(int far *)0x044A)
#define SCANLINES		((int)*(char far*)0x0461)
#define SCRBUFF			((unsigned far *)((COLORMODE)?0xB8000000:0xB0000000))
#define SCREENSEG		((unsigned)((COLORMODE)?0xB800:0xB000))
#define SCREENSIZE		((*(int far *)0x044C) >> 1)
#define SCREENCOLS		(*(int far *)0x044A)
#define SCREENROWS		((*(char far *)0x0484)?1+(*(char far *)0x0484):25)
#define SCROLL_UP		0x06
#define SCROLL_DN		0x07

#define SCRNSG			SCREENSEG
#define MAX_X			(*(unsigned int  far*)MK_FP(0x40, 0x4a))
#define MAX_Y			(*(unsigned char far*)MK_FP(0x40, 0x84))
#define CUR_ATR			(*((unsigned far*)MK_FP(SCRNSG,(wherey()*MAX_X+wherex())<<1))>>8)
#define SCREEN_COLS		MAX_X
#define SCREEN_ROWS		(MAX_Y + 1)
									       
#define LASTMODE		_DEFAULTMODE
#define BW40			_TEXTBW40
#define C40			_TEXTC40
#define BW80			_TEXTBW80
#define C80			_TEXTC80
#define MONO			_TEXTMONO
#define textmode		_setvideomode
#define window(a,b,c,d)		_settextwindow(b,a,d,c)
#define cleardevice()		_clearscreen(_GCLEARSCREEN)
#define clearviewport()		_clearscreen(_GVIEWPORT)
#define closegraph()		_setvideomode(_DEFAULTMODE)
#define rectangle(a,b,c,d)	_rectangle(_GFILLINTERIOR,(short)a,(short)b,(short)c,(short)d)

#define _NOCURSOR		0x2000
#define _SOLIDCURSOR		0x0007
#define _NORMALCURSOR		0x0607
#define _setcursortype		_settextcursor

#define toascii(c)		((c)&0177)

#ifdef USE_C
#define absread(a,b,c,d)	absrdwr(a,d,0x25)
#define abswrite(a,b,c,d) 	absrdwr(a,d,0x26)
#endif

int bdosptr(int dosfun, void *argument, unsigned dosal);
#ifndef USE_C
#pragma aux bdosptr =  \
"int 0x21"          \
"jnc noerror" \
"mov ax, 0xffff" \
"noerror:" \
parm [ah] [dx] [al] value [ax];
#endif

void setdta(char far *dta);
#ifndef USE_C
#pragma aux setdta = \
"mov ah, 0x1A" \
"int 0x21" \
parm [dx];
#endif

int randbrd(struct fcb *fcbptr, int reccnt);
#ifndef USE_C
#pragma aux randbrd = \
"mov ah, 0x27" \
"int 0x21" \
parm [dx] [cx] value [al];
#endif

int randbwr(struct fcb *fcbptr, int reccnt);
#ifndef USE_C
#pragma aux randbwr = \
"mov ah, 0x28" \
"int 0x21" \
parm [dx] [cx] value [al];
#endif

#ifndef USE_C
int		getdisk(void);
void		setverify(unsigned char value);
int		getverify(void);
#endif
struct _complex complex(double real, double imag);
int		stime(time_t *tp);
int		_chmod(const char *filename, int func, ...);
int		getftime (int handle, struct ftime *ftimep);
int		setftime (int handle, struct ftime *ftimep);
char*		searchpath(const char *filename);
int		biosdisk(int cmd, int drv, int head, int track, int sect, int sects, void *buf);
long*		biostime(int cmd, long *newtime);
void		ctrlbrk(int (*fptr)(void));
int		getcurdir(int drive, char *direc);
int		fnsplit(const char *path, char *drive, char *dir, char *name, char *ext);
char far*	getdta();
void		getfat(unsigned char drive, struct fatinfo *fatblkp);
void		getfatd(struct fatinfo *fatblkp);
unsigned	getpsp(void);
int		ioctl(int handle, int cmd, ...);
char*		getpass(const char *prompt);
double		_matherr(_mexcep why, char *fun, double arg1p, double arg2p, double retval);
int		_read(int handle, void *buf, unsigned nbyte);
char*		parsfnm(const char *cmdline, struct fcb *fcbptr, unsigned char option);
int		setcbrk(unsigned char value);
int		getcbrk(void);
int		setdisk(unsigned char drive);
unsigned	geninterrupt(int);
char*		stpcpy (char *dest, const char *src);
void		textbackground(int color);
void		textcolor(int color);
void		textattr(int attr);
void		highvideo (void);
void		lowvideo (void);
void		normvideo (void);
int		_write(int handle, void *buf, int nbyte);
int		_creat(const char *filename, int attrib);
int		creatnew(const char *filename, int attrib);
int		creattemp(char *filename, int attrib);
void		gettextinfo(struct text_info *inforec);
int		wherex(void);
int		wherey(void);
void		clrscr(void);
void		gotoxy(const unsigned char x, const unsigned char y);
int		gettext(char x1, char y1, char x2, char y2, char *dest);
int		puttext(char x1, char y1, char x2, char y2, char *srce);
void		clreol(void);
struct country* country(unsigned char xcode, struct country *cp);
unsigned long	dostounix (struct date *d, struct time *t);
void		unixtodos (unsigned long time, struct date *d, struct time *t);

/*
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef USE_C
int absread(int DosDrive, int nsects, int foo, void *diskReadPacket);
#pragma aux absread = \
"push bp" \
"int 0x25" \
"sbb ax, ax" \
"popf" \
"pop bp" \
parm [ax] [cx] [dx] [bx] \
modify [si di] \
value [ax];

int abswrite(int DosDrive, int nsects, int foo, void *diskReadPacket);
#pragma aux abswrite = \
"push bp" \
"int 0x26" \
"sbb ax, ax" \
"popf" \
"pop bp" \
parm [ax] [cx] [dx] [bx] \
modify [si di] \
value [ax];
#endif

#endif /*__WATCOMC__*/

#ifdef __TURBOC__

#define __argv _argv
#define __argc _argc

#define _fmalloc farmalloc
#define _ffree farfree
/*#define _dos_allocmem(x,(unsigned short *)y) allocmem(x,y)*/
#define _dos_freemem freemem
#define _harderr harderr
#define _hardresume hardresume
#define _hardretn hardretn

#define __far far
#define _far far

#define find_t ffblk
#define _find_t ffblk
#define name ff_name
#define attrib ff_attrib
#define reserved ff_reserved
#define wr_time ff_ftime
#define wr_date ff_date
#define size ff_fsize
#define _dos_findfirst(x,y,z) findfirst(x,z,y)
#define _dos_findnext findnext
#define _dos_findclose

#define _A_NORMAL FA_RDONLY
#define _A_RDONLY FA_RDONLY
#define _A_SYSTEM FA_SYSTEM
#define _A_SUBDIR FA_DIREC
#define _A_HIDDEN FA_HIDDEN
#define _A_VOLID FA_LABEL
#define _A_ARCH FA_ARCH

#define _mktemp mktemp

#define _dos_getdiskfree getdfree
#define _getdrive() getdisk()+1
#define _dos_setdrive(x,y) setdisk(x-1)
#define _diskfree_t dfree
#define total_clusters df_total
#define avail_clusters df_avail
#define sectors_per_cluster df_sclus
#define bytes_per_sector df_bsec

#define _splitpath fnsplit
#define _makepath fnmerge

#define dosdate_t date
#define year da_year
#define month da_mon
#define day da_day
#define hour ti_hour
#define minute ti_min
#define second ti_sec
#define hsecond ti_hund
#define dostime_t time
#define _dos_getdate getdate
#define _dos_gettime gettime
#define _dos_setdate setdate
#define _dos_settime settime

#define inpw inport
#define outpw outport
#define _enable enable
#define _disable disable

#define _bios_serialcom(x,z,y) bioscom(x,y,z)
#define _bios_equiplist biosequip
#define _bios_keybrd bioskey
#define _bios_printer(x,z,y) biosprint(x,y,z)

#define cflag flags

#define _dos_getvect getvect
#define _dos_setvect setvect
#define _dos_keep keep

#define _DEFAULTMODE LASTMODE
#define _TEXTBW40 BW40
#define _TEXTC40 C40
#define _TEXTBW80 BW80
#define _TEXTC80 C80
#define _TEXTMONO MONO
#define _setvideomode textmode
#define _settextwindow(b,a,d,c) window(a,b,c,d)

#define _settextcursor _setcursortype

#define S_ISDIR( m )    (((m) & S_IFMT) == S_IFDIR)

#define F_OK 00

#define PATH_MAX MAXPATH
#define _MAX_PATH MAXPATH
#define _MAX_FNAME MAXFILE
#define _MAX_NAME MAXFILE
#define _MAX_DIR MAXDIR
#define _MAX_EXT MAXEXT
#define _MAX_DRIVE MAXDRIVE

void _fmemcpy(void far *d1, void far *s1, unsigned len);
int snprintf(char * dest_string, int maxlen, const char *format_string, ...);

#endif /* __TURBOC__ */

#ifdef __PACIFIC__

#ifdef __INCLUDE_FILES_H
#include <stdio.h>
#include <assert.h>
#include <sys.h>
#include <string.h>
#include <stdlib.h>
#include <unixio.h>
#include <ctype.h>
#include <dos.h>
#include <stat.h>
#endif

struct find_t {
   char   reserved[21];
   char   attrib;
   unsigned short  wr_time;
   unsigned short  wr_date;
   unsigned long   size;
   char   name[13];
};

#define _far far
#define __far far
#define cdecl
#define _Cdecl
#define _cdecl
#define __cdecl

#define strcmpi stricmp
#define _fmemcpy farmemcpy

#define _mktemp mktemp

#define fnmerge _makepath

#define _MAX_PATH 144
#define PATH_MAX 144
#define _MAX_DRIVE 3
#define _MAX_DIR 130
#define _MAX_FNAME 9
#define _MAX_EXT 5
#define _MAX_NAME 13

#define _find_t find_t

#define _A_NORMAL 0x00
#define _A_RDONLY 0x01
#define _A_HIDDEN 0x02
#define _A_SYSTEM 0x04
#define _A_VOLID  0x08
#define _A_SUBDIR 0x10
#define _A_ARCH   0x20

#define WILDCARDS 0x01
#define EXTENSION 0x02
#define FILENAME 0x04
#define DIRECTORY 0x08
#define DRIVE 0x10

#define R_OK 4
#define W_OK 2
#define X_OK 1
#define F_OK 0

#define P_WAIT      0
#define P_NOWAIT    1
#define P_OVERLAY   2
#define P_NOWAITO   3

#define S_IFCHR 0020000
#define S_IFIFO 0010000
#define S_IFBLK 0060000
#define S_IFLNK 0120000
#define S_IFSOCK 0140000
#define S_IRUSR S_IREAD
#define S_IWUSR S_IWRITE
#define S_ISBLK( m )  0
#define S_ISFIFO( m ) (((m) & S_IFMT) == S_IFFIFO)
#define S_ISCHR( m )  (((m) & S_IFMT) == S_IFCHR)
#define S_ISDIR( m )  (((m) & S_IFMT) == S_IFDIR)
#define S_ISREG( m )  (((m) & S_IFMT) == S_IFREG)

#define O_RDONLY 0x0000
#define O_WRONLY 0x0001
#define O_RDWR 0x0002
#define O_APPEND 0x0010
#define O_CREAT 0x0020
#define O_TRUNC 0x0040
#define O_NOINHERIT 0x0080
#define O_TEXT 0x0100
#define O_BINARY 0x0200
#define O_EXCL 0x0400

#define _dos_findclose(x)

#define getcwd(x,y) getcurrentworkingdirectory(x,y)

char *strrev(char *s);
int _dos_findfirst(const char *name, int attr, struct find_t *ff);
int _dos_findnext(struct find_t *ff);
void setdta(char far *dta);
char far *getdta(void);
char *strupr(char *string);
char *strlwr(char *string);
int fnsplit(const char *path, char *drive, char *dir, char *name, char *ext);
void _splitpath(const char *path,char *drive,char *dir,char *fname,char *ext);
void _makepath(char *path,const char *drive,const char *dir,const char *fname, const char *ext);
char *_getdcwd(int drive, char *dest, int len);
char *getcurrentworkingdirectory(char *dest, int len);
int _getdrive(void);
void _dos_getdrive(unsigned *drive);
void _dos_setdrive(unsigned a, unsigned *total);
int snprintf(char *dest_string, int maxlen, const char *format_string, ...);
int memicmp(const void *s1, const void *s2, size_t n);

#endif /* __PACIFIC__ */

#endif
