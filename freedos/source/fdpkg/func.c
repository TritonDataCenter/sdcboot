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
 
 * FDPKG functions; remove, install, check dependencies, configure
 * Version 0.1: Correctly implemented Installation and Removal of packages, plus
 * dependency checking and configuring; no known bugs
 * Version 0.2: LSM parsing; checks if version to update is same as already
 * installed, LSM field displaying, ability to install a needed package in
 * depends.txt, " || " syntax for depends.txt, also "!package" syntax, ability
 * to display installed packages, code optimization, full support for 7zip
 * archives, error checking of unzip.exe and 7za.exe, .BAS, .PL, .PY, and .REX
 * install.xxx scripting support, some ASM replacements for simple functions
 * to save space, scan %FDPKG% for arguments, more or less fixed piping, dos
 * switch character support, kittenized, no known bugs
 * Version 1.0: Can detect URLs and attempt to download them with wget, /B
 * option to accept old fdpkg.bat syntax
  
 * Todo:
 * See wiki
 * Read a batch file at end of execution ( %DOSDIR%\PACKAGES\execute.bat )
 * Fix bugs in autoadd
 * Add an option to remove lines in autoadd
 * List installed files
 */

#include <stdio.h>	/* For lots of stuff, including stdout and stderr, fopen(), etc... */
#include <stdlib.h>	/* Hmm... something :-) */
#include <string.h>	/* strcmp(), strcmpi(), strnicmp(), etc... */
#include <stdarg.h>	/* va_arg(), va_list, va_start() */
#include <errno.h>	/* errno */
#include <ctype.h>	/* toupper() */
#include <conio.h>	/* getch(), kbhit() */
#include <dos.h>	/* MK_FP(), intdos(), etc... */
#ifndef __PACIFIC__
#include <process.h>/* spawnlp() */
#include <io.h>		/* For open(), dup(), etc... */
#include <fcntl.h>	/* For the O_RDONLY stuff */
#ifdef __WATCOMC__
#include <stddef.h>
#include <direct.h>	/* chdir(), mkdir(), rmdir() */
#else
#include <dir.h>	/* chdir(), mkdir(), rmdir() */
#include <sys/stat.h>/* S_IREAD, S_IWRITE */
#endif
#else
#include <unixio.h>	/* Pacific C's version of open(), dup(), etc... */
#include <sys.h>	/* Pacific C's version of chdir(), mkdir(), rmdir() */
#include <stat.h>	/* S_IREAD, S_IWRITE */
#endif
#include "func.h"	/* My own functions and asm #pragma aux's */
#include "kitten.h"	/* For kitten translations */

short extract = 0,	/* Used to know if we are extracting or not */
      force   = 0;	/* Used to know if the user wants to answer questions or not */
char *dosdir  = NULL;	/* Used to hold the %DOSDIR% environment variable */
char *comspec = NULL;	/* Used to hold the %COMSPEC% environment variable */
char  altdesc[15];	/* Used to hold a user-specified LSM field */

#ifdef __PACIFIC__

int standard_spawnlp( int mode, char *progname, ... )
{
    va_list list;

    va_start( list, progname );
    return( spawnvp( progname, ( char ** )list ) );
}

#define spawnlp standard_spawnlp

#endif

/*
 * The following function converts unix-style pathnames (with '/' in them) to DOS-style pathnames
 * (using '\' instead of '/')
 */
void unix2dos(char *path) {
      char *p = path;
      for (;*p; ++p) if ('/' == *p) *p = '\\';
}

/*
 * The following function does the opposite of unix2dos; it converts DOS-style pathnames
 * (with '\') to unix-style pathnames instead (with '/')
 */
void dos2unix(char *path) {
      char *p = path;
      for (;*p; ++p) if ('\\' == *p) *p = '/';
}

/* Pauses until a keypress */
void pause(void) {
	kitten_printf(2, 0, "Press any key...");
	while(kbhit()) getch();	/* Clear the keyboard buffer */
	fflush(stdout);		/* Flush stdout */
	getch();
	writenewlines();	/* Write newline characters */
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

/* Miscellaneous functions that use DOS interrupts */
#if !defined(__WATCOMC__) || !defined(__SMALL__)
/* Sets the current disk */
void _setdisk(int d) {
#if defined(__WATCOMC__)
	__asm {
		mov ah, 0x0E
		mov dl, d
		int 0x21
	}
#else
	union REGS r;
	r.h.ah = 0x0E;
	r.h.dl = d;
	intdos(&r, &r);
#endif
}

/* Write a character to stdout */
void writenewlines(void) {
#if defined(__WATCOMC__)
	__asm {
		mov ah, 0x02
		mov dl, '\n'
		int 0x21
                mov ah, 0x02
                mov dl, '\r'
                int 0x21
	}
#else
	union REGS r;
	r.h.ah = 0x02;
	r.h.dl = '\n';
	intdos(&r, &r);
        r.h.ah = 0x02;
        r.h.dl = '\r';
        intdos(&r, &r);
#endif
}

/* Get the DOS switch character (undocumented) */
int switchcharacter(void) {
#if defined(__WATCOMC__)
	unsigned char ret;
	__asm {
		mov ax, 0x3700
		int 0x21
        cmp al, 0
        jnz backup
		mov ret, dl
        jmp finish
        backup:
        mov ret, '/'
        finish:
	}
	return ret;
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

/* Set the attributes for a file */
void setattr(const char *file, int attribs) {
	union REGS r;
	struct SREGS s;
	r.x.ax = 0x4301;
	s.ds = FP_SEG(file);
	r.x.dx = FP_OFF(file);
	r.x.cx = attribs;
	intdosx(&r, &r, &s);
}
#endif
#endif
/* End of miscellaneous functions that use DOS interrupts */

/* 
 * Search the environment variable in "env" for the filename in "name"; the environment
 * variable may be seperated into several paths by ';'
 */
char *_searchpath(const char *name, const char *env, int handle) {
	/*
	 * Searchpath must take handle as an argument, because the output of the console is
	 * being redirected at the time of execution.  If searchpath needs to print a message
	 * to the console, it needs an argument to unredirect the console
	 */
	/* 
	 * This function derives most of its actual code from kitten, although it is modified to
	 * serve a different purpose
	 */
	char *envptr = getenv(env);
	static char file[_MAX_PATH];
	int ret = 0;
	while(envptr && envptr[0] && !ret) {	/* While there are paths left to parse */
		char *tok = strchr(envptr, ';');
		int toklen;
		if(tok == NULL) toklen = strlen(envptr);
		else toklen = tok - envptr;
	/* The next code copies the name to the path directory and checks for existance */
		memcpy(file, envptr, toklen);
		strcpy(file+toklen, "\\");
		sprintf(file, "%s%s", file, name);
		if(access(file,0)==0) return file;
		envptr = tok;
		if(envptr) envptr++;	/* Move on to the next one */
	}
	if( handle ) {
        unredirect(handle, 1);
        kitten_printf(2, 1, "Archiver not in path.\n");
    }
	/*
	 * Return file for success, and return null for failure
	 */
	return NULL;
}

/* Functions to redirect a file and then to restore once more */
int redirect(const char *dev, int handle) {
	int oldhandle = dup(handle);	/* Duplicate the file handle */
	close(handle);			/* Then close it */
	open(dev, O_CREAT|O_TRUNC|O_TEXT|O_WRONLY,S_IREAD|S_IWRITE);	/* Then open a new one */
	return oldhandle;		/* And return it */
}

void unredirect(int oldhandle, int handle) {
	close(handle);			/* Close the handle */
	dup2(oldhandle, handle);	/* And replace it with the old one */
    close(oldhandle);
}
/* End of redirection functions */

/* Read the specified field from the specified LSM file */
char *readlsm(const char *lsmname, const char *field) {
	FILE *fp;
	int where = 0, len = 0;
	char buffer[4096];
    static char buffer2[4096];
	buffer2[0] = '\0';
	if((fp = fopen(lsmname, "r")) == NULL) return NULL;
	while(fgets(buffer, 4096, fp) != NULL) {
		/*
		 * Where is an int that specifies whether the field has been found yet. If it has
		 * then it will be read into buffer2.
		 * If where is already equal to one, it will be set to zero if a new field is
		 * beginning to be read
		 */
		if(strchr(buffer, ':') != NULL) where = 0;
		if(strnicmp(field, buffer, strlen(field)) == 0) {
			where = 1;
			strcpy(buffer, &buffer[strlen(field)+1]);
		}
		/* Remove any whitespace, including tabs */
		while(buffer[0] == ' ' || buffer[0] == '\t') strcpy(buffer, &buffer[1]);
		if(where == 1) sprintf(buffer2, "%s%s",
				(buffer2==NULL)?"":buffer2, (buffer==NULL)?"":buffer);
	}
	fclose(fp);
	/* Remove any trailing newline characters */
	len = strlen(buffer2);
	if(buffer2[len-1] == '\n') buffer2[len-1] = '\0';
	return buffer2;
}

/* 
 * Asks a question specified in fmt... using vprintf, get a character, compare it to character,
 * if, the character is equal to yes, return the value of yes, otherwise return the value of no
 */
int getthechar(int yes, int no, short x, short y, char *fmt, ...) {
	va_list vars;
	int ret;
	va_start(vars, fmt);
#ifndef NO_KITTEN
	vprintf(kittengets(x,y,fmt), vars);
#else
#ifdef __TURBOC__
	if(x||y);
#endif
	vprintf(fmt, vars);
#endif
	fflush(stdout);
	while(kbhit())getch();
	/* 
	 * stdout needs to be flushed in OpenWatcom otherwise it will wait for output before
	 * the question is printed to the screen
	 */
	ret = (toupper(getche()) == kittengets(4,0,"Y")[0]) ? yes : no;
	/* Use writenewlines() because it is less size-consuming than write() */
        writenewlines();
return ret;
}

/* 
 * Function to check if the version in the LSM of the package to be installed is identical to the
 * version of the package that is already installed
 */
int checkversions(const char *name, int mode) {
	char buffer[_MAX_PATH], *buffer2, *buffer3;
	int ret = 1, c = 0;
	sprintf(buffer, "%s\\appinfo\\%s.lsm", dosdir, name);
	if(access(buffer,0)==0) buffer2 = readlsm(buffer, "Version");
	else {
		/* If the installed package has no LSM, print this message */
		sprintf(buffer, "%s\\appinfo\\%s%c.lsm", dosdir, name, mode?'x':'s');
		if(access(buffer,0)==0) buffer2 = readlsm(buffer, "Version");
		else {
			kitten_printf(2,2, "Couldn't get version of installed %s %s.\n",
					name,mode?kittengets(2,9,"binaries"):kittengets(2,10,
					"sources"));
			c++;
		}
	}
	if(!c) {
		sprintf(buffer, "%s%c.lsm", name, mode?'x':'s');
		if(access(buffer,0)==0) buffer3 = readlsm(buffer, "Version");
			/* If the package to be installed has no LSM, print this message */
		else {
			kitten_printf(2, 3, "Couldn't get version to be installed.\n");
			c++;
		}
		if (!c) {
			/* If the versions are the same, print this message */
			if(strcmp(buffer2, buffer3)==0) {
				kitten_printf(2, 4,
					"Version to be installed is the"
					" same as installed package.\n");
				ret = 0;
				/* If they are not, ask the user */
			} else ret = getthechar(1,5,2,5,
					"The version of the installed package is \"%s\"\n"
					"and the version to be installed is \"%s\".\n"
					"Proceed (Y/N)? ", buffer2, buffer3);
		}
	}
	return ret;
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
 * Truncate the output of "7za.exe l ..." so that it more or less matches the output of
 * "unzip.exe -Z -1 ..." just enough for removefiles() to recognize it
 */
#ifdef FEATURE_7ZIP
void truncinfo(const char *name) {
	int i = 0, j = 0, p7zip = 8;
	FILE *fp, *fp2;
	char temp[13],buffer[4096];
	if((fp = fopen(name, "r")) == NULL) return;
	temp[0] = '\0';
	strcpy(temp, _mktemp("XXXXXX"));
	while(access(temp,0)==0) strcpy(temp, _mktemp("XXXXXX"));
	/* 
	 * Use a temp file to print the first output to
	 */
	if((fp2 = fopen(temp, "w")) == NULL) {
		fclose(fp);
		return;
	}
	while(fgets(buffer,4096,fp) != NULL) {
	/*
	 * If seven lines have been read, and if 7zip extracted the package, there should be a
	 * line of -'s in the file.  So if there isn't, return.  Otherwise, remove all of the
	 * attribute stuff that isn't needed (&buffer[53]).  Also, at the end of all of the files
	 * there will be another line with -'s in it, so if that is the case, then close the file
	 * and copy the temp file to the real file.
     * Note that in p7zip (which will become the default), an extra line of output is created
	 */
		i++;
        j++;
		if(i!=0 && buffer[0] != '\0' && buffer[0] != '\n') {
			if(i==p7zip && buffer[0] != '-') {
				fclose(fp);
				fclose(fp2);
				remove(temp);
				return;
			}
			if(i>p7zip && buffer[0] != '-') {
				strcpy(buffer, &buffer[53]);
				fputs(buffer, fp2);
			}
			else if(i>p7zip) i = -1;
            if(i==3 && buffer[0] != 'p') p7zip--; 
		}
	}
	fclose(fp);
	fclose(fp2);
	/*
	 * Then copy the temp file into the actual file list; the temp file will contain
	 * the corrected list
	 */
	if(j) fcopy(temp,name);
    remove(temp);
}
#endif

/* Print an error if the unpackers return some sort of error or warning */
static int printerror(const char *zipname, int mode, int code) {
	/*
	 * Errorlevel >2 from unzip or 7zip + 1 means complete failure, and errorlevel 2 means a
	 * warning
	 */
	mode = (mode==1||mode>2)?1:0;
	if(mode&&code==2) kitten_printf(2, 6, "Warnings extracting %s.\n", zipname);
	else if(mode&&code>2) kitten_printf(2, 7, "Unpackaging failed for %s.\n", zipname);
	return (code>2&&mode)?9:0;
}

/* Call a certain unpacker depending on the file extension */
int callzip(const char *zipname, int mode, ...) {
    /* 
     * mode==0 is list, mode==1 is extract, mode==2 is extract packages\* only, mode>2 is
     * extract without packages\*.mode==0 takes an additional argument; the file to redirect
     * the output to
     */
    char    buffer2[_MAX_PATH];	/* Holds the extra argument for mode==0 */
    int	    ret,		/* Holds the return value */
    	    handle1 = -1,		/* Holds the original handle for stdout */
    	    handle2 = -1;		/* Holds the original handle for stderr */
    if(!mode) {
        va_list vargs;
        va_start(vargs, mode);	/* Lets get the extra argument */
        strcpy(buffer2, va_arg(vargs, char *));
        handle1 = redirect(buffer2, 1);	/* And redirect stdout */
    } else handle1 = redirect("NUL", 1);	/* Otherwise we'll redirect it to NUL */
    handle2 = redirect("NUL", 2);		/* And in any case lets redirect stderr to NUL */
    if(strcmpi(&strrchr(zipname, '.')[1], "ZIP") == 0) {
        if(_searchpath("UNZIP.EXE", "PATH", handle1) == NULL) return 6;
            /* All of the relevant UNZIP commands */
    /*
     * Notice that below I must use seperate function calls for Pacific C.  This is because
     * Pacific C's versions don't take a first argument to specify what type of spawn to do;
     * it only does the equivalent of P_WAIT
     */
        if(!mode) ret =      spawnlp(P_WAIT,"UNZIP","","-Z","-1",zipname,NULL);
        else if(mode==1)ret= spawnlp(P_WAIT,"UNZIP","","-qq","-o","-C","-d",dosdir,zipname,NULL);
        else if(mode==2)ret= spawnlp(P_WAIT,"UNZIP","","-qq","-o","-C","-d",dosdir,zipname,
                "packages/*",NULL);
        else            ret= spawnlp(P_WAIT,"UNZIP","","-qq","-o","-C","-d",dosdir,zipname,"-x",
                "packages/*",NULL);
#if FEATURE_7ZIP
    } else if(strcmpi(&strrchr(zipname, '.')[1], "7Z") == 0) {
        char    buffer[13],	/* Holds the inclusion or exclusion argument for 7za */
                buffer4[_MAX_PATH],
                buffer3[_MAX_PATH];	/* Holds the output directory for 7za */
        if(_searchpath("7ZA.EXE", "PATH", handle1) == NULL) return 6;
        strcpy( buffer4, zipname );
        strlwr( buffer4 );
    /*
     * For 7ZA it doesn't like spaces between arguments so we have to use buffers to hold
     * them.  buffer is used to include or exclude PACKAGES\*, and buffer3 is used to hold
     * the output directory (%DOSDIR%)
     */
        sprintf(buffer, "-%c!PACKAGES",(mode==2)?'i':'x');
        sprintf(buffer3,"-o%s",dosdir);
        /* All of the relevant 7ZA commands */
        if(!mode) ret =      spawnlp(P_WAIT,"7ZA","","l",zipname,NULL);
        else if(mode==1)ret= spawnlp(P_WAIT,"7ZA","","x","-y","-aoa",buffer3,buffer4,NULL);
        else            ret= spawnlp(P_WAIT,"7ZA","","x","-y","-aoa",buffer3,buffer4,buffer,
                                     NULL);
#endif
        ret++;
    } else ret = 3;
    unredirect(handle1, 1);		/* Lets unredirect stdout and stderr cuz we're done */
    unredirect(handle2, 2);
    ret = printerror(zipname, mode, ret);
#if FEATURE_7ZIP
    if(!mode) truncinfo(buffer2);
    /* The output of mode==0 might need to be truncated if 7zip did the unpacking */
#endif
    return ret;
}

/* 
 * Calls a scripting interpreter based on the script type pointed to by type
 * currently accepted scripters are REXX, PERL, PYTHON, BASIC, and of course BATCH
 */
int callinterp(const char *name, int rserr, int rsout) {
	char buffer[_MAX_PATH],	/* Stores the command interpretor name */
	     ext[_MAX_EXT];	/* Stores the extension of name */
	int  handle1,		/* Stores the original handle of stdout */
	     handle2,		/* Stores the original handle of stderr */
	     ret = 0;		/* Stores the return value */
	buffer[0] = ext[0] = '\0';
	/*
	 * The arguments rserr and rsout are used to say whether or not stderr and stdout
	 * respectively should be redirected to NUL.  handle1 and handle2 are used to backup the
	 * original handle associated with stderr and stdout
	 */
	if(rserr) handle2 = redirect("NUL", 2);
	if(rsout) handle1 = redirect("NUL", 1);
	strcpy(ext, strlwr(strrchr(name, '.')));
	if(strcmpi(ext, ".bas")==0) {
		char temp[11];
		sprintf(temp, "%s.EXE", _mktemp("XXXXXX"));
		spawnlp(P_WAIT,"FBC","",name,"-x",temp,NULL);
		if(errno==ENOENT) ret = spawnlp(P_WAIT,"BWBASIC","",name,NULL);
		else {
			ret = spawnlp(P_WAIT,temp,temp,NULL);
			remove(temp);
		}
	}
	else if(strcmpi(ext, ".bat")==0) ret = spawnlp(P_WAIT,comspec,"","/c",name,NULL);
	else {
		sprintf(buffer, "%s",(strcmpi(ext,".pl")==0)?"PERL":(strcmpi(ext, ".py")==0)?
				"PYTHON":"REXX");
		ret = spawnlp(P_WAIT,buffer,"",name,NULL);
	}
	/*
	 * Now lets restore stderr and stdout
	 */
	if(rserr) unredirect(handle2, 2);
	if(rsout) unredirect(handle1, 1);
	return ret;
}

/* Configure a package; the configuration script can be any type that callinterp() supports */
static int config(const char *name)
{
	struct find_t ffblk;
	char buffer[_MAX_PATH];
	int found;
	/*
	 * In this function we search for an install script and call the interpretor
	 */
	sprintf(buffer, "%s\\packages\\%sx\\install.*", dosdir, name);
	found = _dos_findfirst(buffer, _A_HIDDEN|_A_SYSTEM|_A_RDONLY|_A_ARCH, &ffblk);
	if (found != 0) return 0;
	/* Lets make sure we provide the whole path */
	sprintf(buffer, "%s\\packages\\%sx\\%s", dosdir, name, ffblk.name);
	kitten_printf(2, 8, "Configuring...\n");
	return (callinterp(buffer, 1, 0) == -1) ? 0 : 1;
}

/* 
 * Function that combines two values to delete a file, so it is possible to easily append a
 * filename to %DOSDIR%\PACKAGES
 */
static void delete_the_file(const char *one, const char *two)
{
	char full[_MAX_PATH];
	sprintf(full, "%s%s", one, *two ? two : "");
	remove(full);
}

/* Removes the files of a package; doesn't accept wildcards */
static int removefiles(const char *name, int mode)
{
	char buffer[282], buffer1[282], buffer2[4096];
	FILE *fp;
	int i = 0, len = 0;
	char *cwd = getcwd(NULL,0);

    /*
     * Call remove.bat first; it shouldn't remove any wanted files anymore
     */
    sprintf(buffer,"%s\\packages\\%s%c\\remove.bat",dosdir,name,mode?'x':'s');
    if(access(buffer,0)==0) callinterp(buffer, 1, 0);
    /*
	 * Since paths are relative in the zip lists, we must change to %DOSDIR% before we can
	 * read them
	 */
	if(dosdir[1] == ':') _setdisk(toupper(dosdir[0]) - 'A');
	chdir(dosdir);
	buffer2[0] = '\0';
	sprintf(buffer,"%s\\packages\\%s%c.lst",dosdir,name,mode?'x':'s');
	sprintf(buffer1, "%s\\packages\\%s%c",dosdir,name,mode?'x':'s');
	if((fp=fopen(buffer, "r")) != NULL) {
		while(fgets(buffer,282,fp) != NULL) {
			unix2dos(buffer);	/* Convert '/' to '\' */
			len = strlen(buffer);
			/* Remove trailing newlines and backslashes */
			if(buffer[len-1] == '\n') {
				buffer[len-1] = '\0';
				len--;
			}
			if(buffer[len-1] == '\\') buffer[len-1] = '\0';
			/* Don't remove any files starting with PACKAGES just yet */
			if(strnicmp(buffer, "PACKAGES", 8) != 0) {
				if(getattr(buffer) & _A_SUBDIR) {
					/* Print all of the directories into a buffer */
					sprintf(buffer2, "%s\n%s", buffer2, buffer);
					rmdir(buffer);
					i++;
				}
				else delete_the_file(buffer, NULL);
			}
		}
		fclose(fp);
		/* Delete the list file */
		delete_the_file(buffer1, ".lst");
	}
	/* Now lets delete the only files that should/could be in PACKAGES\pkgname */
	delete_the_file(buffer1, "\\REMOVE.BAT");
	delete_the_file(buffer1, "\\NEEDS.TXT");
	delete_the_file(buffer1, "\\INSTALL.BAT");
	delete_the_file(buffer1, "\\DEPENDS.BAT");
	delete_the_file(buffer1, "\\DEPENDS.TXT");
	delete_the_file(buffer1, "\\UPGRADE.BAT");
	if(buffer2[0] != '\0') for(;0<i;i--) {
		char *buff = NULL;
	/* 
	 * Now let's seperate all of those directories (from last to first) and delete them all
	 * one at a time
	 */
		buff = strrchr(buffer2, '\n');
		if(buff[0] != '\0') {
			rmdir(&buff[1]);
			buff[0] = '\0';
		}
	}
	/* And change back to the original dir */
	_setdisk(toupper(cwd[0]) - 'A');
	chdir(cwd);
	free(cwd);
	rmdir(buffer1);
	return (access(buffer1, 0) == 0) ? -1 : 0;
}

/* 
 * Checks the dependencies of the given package, also offers " || " syntax, !syntax, and the
 * ability to attempt to install required packages
 */
static int check_dependencies(const char *name)
{
    FILE *fp1;                  /* To open needs.txt and depends.txt */
    char dp[30],                /* For dependencies in depends.txt */
    	 dp2[13],               /* For secondary (" || ") dependencies in depends.txt */
	     buffer[_MAX_PATH],     /* For misc buffers */
         buffer2[_MAX_PATH],    /* A secondary misc buffer */
         depname[_MAX_PATH];    /* For sprintf() when you want to install or remove */
    char *buff;	                /* For strstr() */
    int  ret = 0;               /* The return value */

	/*
	 * This is the complicated part of the code.  Basically what it says is:
	 * if the !package syntax was used:
	 * 	if the package is installed, print a conflicting message and ask the user if
	 * 	they want to remove it; if so, do so, otherwise indicate failure in the return
	 * 	value
         * else if the &package syntax was used:
         *      if the package is not installed, simply print a message that it is recommended
	 * in all other situations:
	 * 	if you can't access the first dependency and if there is a second dependency
	 * 	and it can't be accessed, ask the user if he wants to install the first
	 * 	dependency; if so, do so, otherwise set the return value to one to indicate
	 * 	failure
	 * The return value is set if dp[0] != '\0'; the above two scenarios ensure that
	 * dp[0]=='\0' if they are successfully able to resolve the problem if there is a problem
	 * and the also set such if there wasn't a problem
	 */
    sprintf(buffer, "%s\\packages\\%sx\\depends.txt",dosdir,name);
    if((fp1 = fopen(buffer, "r")) != NULL) {
        while(fgets(dp, 30, fp1) != NULL) {
            if(dp[strlen(dp)-1] == '\n') dp[strlen(dp)-1] = '\0';
            if(dp[0] == '!') {	/* Check for the !package syntax */
                strcpy(dp, &dp[1]);
                sprintf(buffer, "%s\\packages\\%sx",dosdir,dp);
                if (getattr(buffer) != -1) {
                    if(getthechar(1,0,3,1,"This package conflicts with \"%s\"\nRemove "
                                "(Y/N)? ", dp)) {
                        sprintf(depname, "%sx",dp);
                        if(manippkg(depname,REMOVE) == 0) dp[0] = '\0';
                    }
                }
            } else if(dp[0] == '&') {   /* Check for the &package syntax (recommend) */
                strcpy(dp, &dp[1]);
                sprintf(buffer, "%s\\packages\\%sx",dosdir,dp);
                if(access(buffer,0) != 0) {
                    kitten_printf(3,17,"This package recommends \"%s\"\n", dp);
                }
                dp[0] = '\0';
            } else if(dp[0] != '\0') {
                buff = strstr(dp, " || ");	/* Check for the " || " syntax */
                if(buff != NULL) {
                    strcpy(dp2, &buff[4]);
                    buff[0] = '\0';
                    sprintf(buffer2,"%s\\packages\\%sx",dosdir,dp2);
                } else buffer2[0] = dp2[0] = '\0';
                sprintf(buffer, "%s\\packages\\%sx",dosdir,dp);
                if(access(buffer, 0) == 0) dp[0] = '\0';
                else {
                  depname[0] = '\0';
                  if(dp2[0] != '\0' && getattr(buffer2) != -1) dp[0] = '\0';
                  else if(getthechar(1,0,3,0,
                  "This package needs \"%s\" installed\nInstall (Y/N)? ", dp))
                    sprintf(depname, "%sx.zip", dp);
                  else if(dp2[0] != '\0' && getthechar(1,0,3,0,
                  "This package needs \"%s\" installed\nInstall (Y/N)? ", dp2))
                    sprintf(depname, "%sx.zip", dp2);
                  if(depname[0] != '\0') {
                    if((buff = _searchpath(depname, "PKGDIRS", 0)) != NULL)
                      strcpy(depname, buff);
                    if(manippkg(depname,INSTALL) == 0) dp[0] = '\0';
                  }
                }
            }
            if(dp[0] != '\0') ret = 1;
        }
    }
    fclose(fp1);
	/*
	 * The following exists only for historical purposes and so I don't have to modify tons
	 * of packages manually, notably, depends.bat support, where the batch file simply prints
	 * unmet dependencies into needs.txt.  So what we do is call it with command.com and then
	 * check needs.txt to find out if it has any text in it - if it does, then we return
	 * failure and remove needs.txt regardless of whether there is failure or not
	 * Note to self: Print dependencies in needs.txt
	 */
#ifdef FEATURE_BACKCOMPAT
    sprintf(buffer, "%s\\packages\\%sx\\depends.bat",dosdir,name);
    if(access(buffer,0) == 0) {
        kitten_printf(3, 2, "Running depends.bat...\n");
        callinterp(buffer, 1, 1);
        sprintf(buffer,"%s\\packages\\%sx\\needs.txt",dosdir,name);
        if((fp1 = fopen(buffer, "r")) != NULL) {
            if(fgets(dp,13,fp1)!=NULL) {
                fclose(fp1);
                if(dp[0]!='\n'&&dp[0]!='\0') ret = 1;
            }
            fclose(fp1);
            remove(buffer);
        }
    }
#endif
    return ret;	/* And return the chosen value */
}

/* 
 * Manipulate the package according to the action specified in type (look in func.h for
 * definitions for INSTALL, REMOVE, DISPLAYIT, etc...)
 */
int manippkg(const char *name, int type)
{
	struct find_t ffblk;
	short	found   = 0,		/* For _dos_findfirst() */
	    	ret     = 0,		/* Holds the return value */
	    	len     = 0,		/* Lengths of buffers */
		    mode    = 1,		/* Whether a packages is binary or source */
		    upgrade = 0;		/* To specify whether upgrading */
	char	tmpname[_MAX_PATH],	/* To modify const char *name */
		filen[_MAX_FNAME],	/* For _splitpath */
		ext[_MAX_EXT],		/* For _splitpath */
		buffer[_MAX_PATH],	/* For misc sprintf() (filenames and such) */
		buffer2[_MAX_PATH],	/* For misc sprintf() */
		*lsmbuf;		/* For printing LSM descriptions and such (big buffer) */
#ifndef NDEBUG
	kitten_printf(3, 3, "Checking existance...\n");	/* This message isn't needed */
#endif
	if(type==LSMDESC) {		/* When printing LSM descriptions, search for the LSM
					   first from the filename - ext + .LSM, then
					   %DOSDIR%\appinfo\filename - 'x' or 's' - ext + .LSM */
		strcpy(tmpname, name);
		/* Use a buffer here */
		if((lsmbuf = strrchr(tmpname, '.')) != NULL) lsmbuf[0] = '\0';
		len = strlen(tmpname);
		mode = (mode = toupper(tmpname[len-1]) == 'X' || mode != 'S') ? 1 : 0;
		if(toupper(tmpname[len-1])=='X'||toupper(tmpname[len-1])=='S')
			tmpname[len-1]='\0';
		sprintf(buffer2, "%s%c.lsm", tmpname, mode? 'x' : 's');
		found = _dos_findfirst(buffer2,
				_A_NORMAL|_A_RDONLY|_A_HIDDEN|_A_ARCH|_A_SYSTEM, &ffblk);
		if (found!=0) {
			len = -1;
			sprintf(buffer2,"%s\\appinfo\\%s%c.lsm",dosdir,tmpname,mode?'x':'s');
			found = _dos_findfirst(buffer2,
					_A_NORMAL|_A_RDONLY|_A_HIDDEN|_A_ARCH|_A_SYSTEM, &ffblk);
			if(found!=0) {
				sprintf(buffer2, "%s\\appinfo\\%s.lsm",dosdir,tmpname);
				found = _dos_findfirst(buffer2,
						_A_NORMAL|_A_RDONLY|_A_HIDDEN|_A_ARCH|_A_SYSTEM,
						&ffblk);
			}
		}
	} else {
		if(type!=INSTALL) sprintf(tmpname, "%s\\packages\\%s",dosdir,name);
		found = _dos_findfirst((type!=INSTALL)?tmpname:name,
				_A_HIDDEN|_A_NORMAL|_A_RDONLY|_A_ARCH|_A_SUBDIR, &ffblk);
	}
	if (found!=0) {
		kitten_printf(3, 4, "No package found: %s\n", name);
		ret = 2;
	}
	while (found == 0)
	{					/* Maybe I should replace _splitpath with
						   something more platform-independent for
						   Pacific C's sake */
		_splitpath(ffblk.name, NULL, NULL, filen, ext);
		if(type == INSTALL && strcmpi(ext, ".LSM") != 0) {
			len = strlen(filen);
			mode = (toupper(filen[len-1]) == 'X') ? 1 : 0;
			filen[len-1] = '\0';
			sprintf(buffer,"%s\\packages",dosdir);
			if(access(buffer,0) != 0) {
	/*
	 * %DOSDIR%\PACKAGES must be an existant directory or one that can be created
	 */
				if(mkdir(buffer) == -1) {
					kitten_printf(3, 5, "Invalid %%DOSDIR%% variable.\n");
					return 3;
				}
				setattr(buffer, _A_HIDDEN);
			}
            if( getattr( buffer ) & _A_HIDDEN ) setattr( buffer, _A_HIDDEN );
			sprintf(buffer,"%s\\packages\\%s%c",dosdir,filen,mode?'x':'s');
			sprintf(buffer2,"%s\\packages\\%s%c\\upgrade.bat",
					dosdir,filen,mode?'x':'s');
	/*
	 * If the packages\pkgname directory already exists, first try upgrading with upgrade.bat
	 * and then remove remove.bat if upgrade.bat exists, otherwise remove the way a package
	 * would normally be removed with /remove
	 */
			if(access(buffer, 0) == 0)
			{
				if(!force && (len = checkversions(filen, mode)) != 1)
					return len;
				sprintf(buffer,"%s\\packages\\%s%c\\remove.bat",
						dosdir,filen,mode?'x':'s');
				kitten_printf(3, 6, "Upgrading...\n");
				if(access(buffer2, 0) == 0) {
					callinterp(buffer2, 1, 0);
					remove(buffer);
				}
                removefiles(filen, mode);
				upgrade = 1;
			}
			sprintf(buffer,"%s\\packages\\%s%c.lst",dosdir,filen,mode?'x':'s');
			callzip(ffblk.name, 0, buffer);	/* Make the file list */
			if(extract || upgrade) {
	/* 
	 * Extract is used if you are extracting or upgrading, in the latter case, such will
	 * automatically be detected and /E will be unnecessary.  When extracting, all of the
	 * files are extracted at once and dependencies are not checked.  Which is why it would
	 * be a good idea to have another batch file to be executed after upgrade (which can
	 * then in turn call fdpkg /check blah just in case the dependencies have changed
	 */
				if(mode && check_dependencies(filen)) {
					removefiles(filen,1);
					ret = 1;
				} else {
					if(len = callzip(ffblk.name,1)!=0) ret = len;
					else {
						sprintf(buffer, "%s\\packages\\%s%c",dosdir,
								filen, mode?'x':'s');
						if(access(buffer,0)!=0) mkdir(buffer);
						kitten_printf(3, 7, "Package extracted\n");
	/*
	 * Here, if we are upgrading, we call a file called upgraded.bat in the package's
	 * directory which is a batch file to be executed after an upgrade, much like install.xxx
	 * is executed after an install
	 */
						if(upgrade) {
							sprintf(buffer,"%s\\packages\\%s%c\\"
									"upgraded.bat",dosdir,
									filen,mode?'x':'s');
							if(access(buffer,0)==0)
								callinterp(buffer,1,0);
						}
					}
				}
			} else {
	/*
	 * Now it is confirmed that we are not upgrading or extracting, so now we will extract
	 * only the packages\* directory from the zip to determine if there are unmet
	 * dependencies, in which case the packages\* directory will then be removed and
	 * installation will fail with exit code 1.  If all goes well and dependencies are met,
	 * the rest of the package (minus the packages\* directory) will be extracted
	 */
				callzip(ffblk.name,2);
				if(mode && check_dependencies(filen)) {
					removefiles(filen,1);
					ret = 1;
				}
				else {
					kitten_printf(3, 8, "Unpacking...\n");
					if(len = callzip(ffblk.name,3)!=0) ret = len;
					else {
	/*
	 * The next three lines create a %DOSDIR%\PACKAGES\pkgname[x|s] directory for the
	 * package if it doesn't already exist to allow removal of the package
	 */
						sprintf(buffer, "%s\\packages\\%s%c",dosdir,
								filen, mode?'x':'s');
						if(access(buffer,0)!=0) mkdir(buffer);
						if(mode) config(filen);
						kitten_printf(3, 9, "Package installed.\n");
					}
				}
			}
			/* End of installation mode stuff */
			/* Beginning of LSM description stuff */
		} else if(type==LSMDESC) {
			/* len==-1 here just means that the LSM was found in %DOSDIR%\appinfo */
			if(len==-1) sprintf(tmpname, "%s\\appinfo\\%s", dosdir, ffblk.name);
	/*
	 * If the LSM is found in %DOSDIR%\appinfo, use tmpname, otherwise the ffblk name.
	 * If the user wishes to view an alternate LSM field, main() will have assigned altdesc
	 * a value, and this will be used, otherwise "Description" will be used
	 */
			lsmbuf = readlsm((len==-1)?tmpname:ffblk.name,
						(altdesc[0]=='\0')?"Description":altdesc);
			if(lsmbuf[0]=='\0')
				kitten_printf(3, 10, "Couldn't get \"%s\" field for: %s\n",
					(altdesc[0]=='\0')?"description":altdesc,name);
			else printf("%s\n",lsmbuf);
			/* End of LSM description stuff */
			/* Beginning of stuff that works with installed packages */
		} else if(ffblk.attrib & _A_SUBDIR) {
			/* Remove the trailing 'x' or 's'; set mode */
			strcpy(tmpname, ffblk.name);
			len = strlen(tmpname);
			mode = (toupper(tmpname[len-1]) == 'X') ? 1 : 0;
			tmpname[len-1] = '\0';
			/* Beginning of remove mode */
			if(type == REMOVE) {
				/* 
				 * force == one if the user doesn't want to see any questions
				 */
				if(!force) {
					if(getthechar(1,5,3,11,
						"Are you sure? Remove the %s %s (Y/N)? ",
						tmpname,mode?
						kittengets(2,9,"binaries"):
						kittengets(2,10,"sources"))==5) {
						/* The user said no, return errorlevel 5 */
						printf("Aborting...\n");
						return 5;
					}
				}
	/* 
	 * removefiles() returns -1 if the %DOSDIR%\packages\pkgname directory still exists
	 */
				if(removefiles(tmpname, mode) == -1) {
					kitten_printf(3, 12, "Some files couldn't be removed\n");
					/* If it still exists, set errorlevel 8 */
					ret = 8;
				}
				/* End of remove mode */
				/* Beginning of check mode */
			} else if (type == CHECK) {
				if(mode && check_dependencies(tmpname)) {
					kitten_printf(3, 13, "Dependencies aren't met.\n");
					ret = 1;
				}
				else kitten_printf(3,14,"Dependencies met or there aren't "
						"dependencies.\n");
				/* End of check mode */
				/* Beginning of config mode */
			} else if (type == CONFIG) {	/* Not much to this one */
				if(mode && config(tmpname))
					kitten_printf(3, 15, "Configuration complete.\n");
				else kitten_printf(3, 16, "Configuration unnecessary.\n");
				/* End of config mode */
				/* Beginning of display mode (not much to this either) */
			} else if (type == DISPLAYIT) printf("%s\n", ffblk.name);
		}	/* End of stuff that works with installed packages */
		found = _dos_findnext(&ffblk);
		/*
		 * If in LSM description mode and there is another to be printed, it would
		 * be good for output's sake to have a pause and actually let the user read the
		 * output before the next LSM comes along
		 */
		if(type==LSMDESC && found==0) pause();
	}
	return ret;
}

