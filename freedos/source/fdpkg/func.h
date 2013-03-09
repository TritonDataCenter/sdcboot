#ifndef __FUNC_H__
#define __FUNC_H__

#ifndef __INCLUDE_FILES_H
#include "misclib/misc.h"
#include "config.h"
#endif

#define REMOVE    1
#define CHECK     2
#define CONFIG    3
#define DISPLAYIT 4
#define INSTALL   5
#define LSMDESC   6
#define EXTHELP   7
#define FDPKGBAT  8

void  _setdisk(int d);
void  unix2dos(char *path);
void  dos2unix(char *path);
void  pause(void);
void  kitten_printf(short x, short y, char *fmt, ...);
void  writenewlines(void);
int   switchcharacter(void);
#ifndef __TURBOC__
int   getattr(const char *file);
void  setattr(const char *file, int attribs);
#endif
int   redirect(const char *dev, int handle);
void  unredirect(int oldhandle, int handle);
char *_searchpath(const char *name, const char *env, int handle);
char *readlsm(const char *lsmname, const char *field);
int   manippkg(const char *name, int type);
int   getthechar(int yes, int no, short x, short y, char *fmt, ...);
int   checkversions(const char *name, int mode);
void  truncinfo(const char *name);
int   callzip(const char *zipname, int mode, ...);
int   callinterp(const char *name, int rserr, int rsout);

#ifdef __TURBOC__
#define getattr(x)   _chmod(x, 0)
#define setattr(x,y) _chmod(x, 1, y)
#endif

#ifdef __WATCOMC__

void trueprag(const char *path, char *dest);
#pragma aux trueprag = \
"mov ah, 0x60" \
"int 0x21" \
parm [si] [di];

void _setdisk(int d);
#pragma aux _setdisk = \
"mov ah, 0x0E" \
"int 0x21" \
parm [dl];

void writenewlines(void);
#pragma aux writenewlines = \
"mov ah, 0x02" \
"mov dl, 0x0A" \
"int 0x21" \
"mov ah, 0x02" \
"mov dl, 0x0D" \
"int 0x21";

int switchcharacter(void);
#pragma aux switchcharacter = \
"mov ax, 0x3700" \
"mov dl, 0x2f" \
"int 0x21" \
"cmp al, 0" \
"jz finish" \
"mov dl, '/'" \
"finish:" \
value [dx];

int getattr(const char *file);
#pragma aux getattr = \
"mov ax, 0x4300" \
"int 0x21" \
"jnc noerror" \
"mov cx, 0xffff" \
"noerror:" \
parm [dx] value [cx];

void setattr(const char *file, int attribs);
#pragma aux setattr = \
"mov ax, 0x4301" \
"int 0x21" \
parm [dx] [cx];

#endif

#endif
