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

 * Exit codes:
 *  0:   No problem
 *  1:   Unresolved dependencies
 *  2:   No matching package
 *  3:   Invalid or inexistant environment variables
 *  4:   Invalid options or insufficient arguments
 *  5:   User aborted
 *  6:   Unzip not found
 *  7:   Insufficient memory
 *  8:   All files could not be removed
 *  9:   Failure extracting package
 *  10:  Slash-converting mode completed successfully
 *  11:  Slash-converting mode failed
 *  255: Invalid path or package name
 */

#include <stdio.h>	/* printf(), etc... */
#include <stdlib.h>	/* _splitpath(), _makepath(), etc... */
#include <string.h>	/* strcpy(), strcmp(), etc... */
#ifdef __WATCOMC__
#include <direct.h>	/* chdir() in Watcom C */
#include <process.h>	/* spawn...() */
#include <io.h>		/* remove() */
#else/*__WATCOMC__*/
#ifndef __PACIFIC__
#include <dir.h>	/* chdir() in Turbo C */
#include <process.h>	/* spawn...() */
#else/* __PACIFIC__*/
#include <sys.h>	/* chdir() in Pacific C */
#include <unixio.h>	/* remove() in Pacific C */
#endif/*__PACIFIC__*/
#endif/*__WATCOMC__*/
#include <ctype.h>	/* toupper() */
#include <dos.h>	/* getcwd(), etc... */
#include "func.h"	/* manippkg(), pause(), etc... */
#include "kitten.h"	/* kittenopen() */

extern short	extract,	/* To specify extraction-only of a package, no configuration */
		force;		/* Force yes to all questions */
extern char	*dosdir,	/* Buffer to hold %DOSDIR% */
		*comspec;	/* Buffer to hold %COMSPEC% */
short		manip = 5;	/* What to do with the package */
extern char	altdesc[];	/* Buffer to hold an alternative LSM field */

/*
 * This is the help function - pretty self-explanatory; it prints the help message
 */
void help(void)
{
    kitten_printf(0, 0,
        "FDPKG v0.2 - GPL by Blair Campbell 2005\n");
    kitten_printf(0, 1,
        "Installs FreeDOS packages both in specialized format and regular format.\n");
    kitten_printf(0, 2,
    	"Syntax: FDPKG [/INSTALL] [/REMOVE] [/CHECK] [/CONFIGURE] [/E] PKGX.ZIP ...\n");
    kitten_printf(0, 3,
    	"   /INSTALL       Installation mode; install package (default)\n");
    kitten_printf(0, 4,
    	"   /REMOVE        Removal mode; remove package\n");
    kitten_printf(0, 5,
    	"   /CHECK         Checking mode; check dependencies of package\n");
    kitten_printf(0, 6,
    	"   /CONFIGURE     Configuration mode; configure package\n");
    kitten_printf(0, 7,
    	"   /DESC[:FIELD]  View the description from an LSM.  Field (optional) would\n");
    kitten_printf(0, 8,
    	"                  be field of the LSM to print.  LSMs are searched for in\n");
    kitten_printf(0, 9,
    	"                  the current directory first, otherwise in %%DOSDIR%%\\APPINFO\n");
    kitten_printf(0, 10,
    	"   /DISPLAY       List all installed packages matching argument\n");
    kitten_printf(0, 11,
    	"   /E             Extract only, do not check dependencies (unless already\n");
    kitten_printf(0, 12,
    	"                  installed), do not configure\n");
#ifdef FEATURE_BACKCOMPAT
    kitten_printf(0, 13,
        "   /B             Be backwards compatible with fdpkg.bat\n");
#endif
#ifdef FEATURE_ENV_SWITCHING
    kitten_printf(0, 14,
        "   /S[:|=]ARG     Switch ARG's backslashes to forward slashes\n");
#endif
    kitten_printf(0, 15,
    	"   /[-]F|/[-]Y    Force; Don't ask questions (POSSIBLY DANGEROUS)\n");
    kitten_printf(0, 16,
    	"   /?[E]|/H[E]    This help; /?E or /HE prints extended help\n");
    kitten_printf(0, 17,
    	"   PKGX.ZIP       The package to install; multiple names / wildcards accepted\n");
    if(manip != EXTHELP) return; /* If the user doesn't want extended help */
    pause(); /* Let's give the user a chance to read (unrelated to above comment) */
    kitten_printf(0, 18,"Exit codes:\n");
    kitten_printf(0, 19,"   255            Invalid path\n");
    kitten_printf(0, 20,"   0              No problems\n");
    kitten_printf(0, 21,"   1              Unresolved dependencies\n");
    kitten_printf(0, 22,"   2              Invalid name\n");
    kitten_printf(0, 23,"   3              Invalid environment variables\n");
    kitten_printf(0, 24,"   4              Invalid options or arguments\n");
    kitten_printf(0, 25,"   5              Aborted by user\n");
    kitten_printf(0, 26,"   6              Unpacker not in path\n");
    kitten_printf(0, 27,"   7              Insufficient memory\n");
    kitten_printf(0, 28,"   8              Not all files could be removed\n");
    kitten_printf(0, 29,"   9              Failure extracting\n");
#ifdef FEATURE_ENV_SWITCHING
    kitten_printf(0, 30,"   10             Switch successful\n");
    kitten_printf(0, 31,"   11             Switch failed\n");
#endif
}

#ifdef FEATURE_ENV_SWITCHING
/*
 * This is a very simple yet potentially very useful function that converts backslashes to
 * forward slashes in an argument and then prints it in a form that could be echoed into a batch
 * file and then executed.  That way programs like DJGPP or PYTHOND which need environment
 * variables with forward slashes can be set up properly without unnecessary user interaction.
 */
static int slash_convert(const char *variable) {
	char *p = (char *)variable;
	dos2unix(p);	/* See func.c */
	if(p==NULL) return 11;
	printf("SET %s\n", p);	/* A batch file can redirect this useful output */
	return 10;
}
#endif

/*
 * This function is essential to parse command-line options; it accepts a wide variety of
 * option styles including concatenated options like /eyinstall or even /installye, and calls
 * the help function if a suitable help argument is used.  For options like /desc which accept
 * an argument in themselves, it will accept three ways of parsing the argument:
 * /descDescription, /desc:Description, or /desc=Description (or simply /desc if an alternate
 * field is not intended to be specified).  This code was largely derived from the Free-XDEL
 * sources by Alain (although expanded upon by myself)
 */
static int checkarg(const char *opt)
{
	short i, len;						/* Define variables */
	
	len=strlen(opt);					/* Find out the length of opt */
	if(len==0)						/* opt is not an arg, exit*/
	{
		kitten_printf(1, 3, "Invalid option in %s\n", opt);
		return 4;					/* Return 4 means invalid args */
	}
	for(i=0;i<=len;i++)					/* Parse options */
	{
		switch(toupper(opt[i]))	{			/* Make opt upper case */
			case 'H':				/* The recognized help opts */
			case '?':
				i++;
				if(toupper(opt[i]) == 'E') manip = EXTHELP; /* Extended help */
				help();				/* Call the help function */
				return 255;
			case '-':				/* Indication of disabling */
				i++;
				switch(toupper(opt[i])) {
					case 'Y':		/* Disable the Force option */
					case 'F':
						force = 0;
						break;
					case 'E':		/* Disable the extract option */
						extract = 0;
						break;
				}
#ifdef FEATURE_BACKCOMPAT
			case 'B':
				manip = FDPKGBAT;
				break;
#endif
#ifdef FEATURE_ENV_SWITCHING
			case 'S':
				i++;
				if(opt[i] == ':' || opt[i] == '=') i++;
				return slash_convert(&opt[i]);
#endif
			case 'E':
				extract = 1;
				break;
			case 'Y':
			case 'F':
				force = 1;
				break;
			case 'I':				/* The install option */
				if(strcmpi(&opt[i], "install") == 0) {
					manip = INSTALL;
					i+=7;
				}
				break;
			case 'R':				/* The remove option */
				if(strcmpi(&opt[i], "remove") == 0) {
					manip = REMOVE;
					i+=6;
				}
			case 'C':				/* The checking option */
				if(strcmpi(&opt[i], "check") == 0) {
					manip = CHECK;
					i+=5;
								/* The configuration option */
				} else if(strcmpi(&opt[i], "configure") == 0) {
					manip = CONFIG;
					i+=9;
				}
				break;
			case 'D':				/* The LSM description option */
				if(strnicmp(&opt[i], "desc", 4) == 0) {
					manip = LSMDESC;
					i+=4;
					if(opt[i]==':') {
						i++;
						strcpy(altdesc, &opt[i]);
						if(strlen(&opt[i])>14) {
							kitten_printf(1, 4,
								"Invalid LSM field : %s\n",
								&opt[i]);
							return 4;
						}
						i+=strlen(altdesc);
					} else altdesc[0] = '\0';
								/* Display installed option */
				} else if(strcmpi(&opt[i], "display") == 0) {
					manip = DISPLAYIT;
					i+=7;
				}
				break;
		}
	}
	return 0;
}

#ifdef FEATURE_WGET
/*
 * The following function is used to download a package from http or ftp using wget or htget
 * to a temp file in %TEMP%, which can then be used by manippkg.
 */
static char *downtemp(const char *name) {
	int handle, ret = 0;
	static char temp[_MAX_PATH];
    char *buff = NULL;
	strcpy(temp, getenv("TEMP"));	/* We'll save it into the temp directory */
	if(temp[0] == '\0') return NULL;
	/* Remove trailing \ */
	if(temp[(handle = strlen(temp))-1] == '\\') temp[handle-1] = '\0';
	/* Get the filename in the URL */
	if((buff = strrchr(name, '/')) == NULL) return NULL;
	/* And make a filename out of that that contains %TEMP%\FILETODOWNLOAD.ZIP */
	sprintf(temp, "%s\\%s", temp, buff);
	handle = redirect("NUL", 1);
	/* 
	 * Here is code to support both WGET and HTGET; WGET uses --output-document=filename to
	 * specify an output filename, whereas HTGET uses -o filename, so we must support both.
	 * If WGET returns -1 (spawnlp itself failed, indicating wget is non-existant or invalid)
	 * try HTGET
	 */
#ifndef __PACIFIC__
	if((ret = spawnlp(P_WAIT,"WGET","","-O",temp,name)) == -1)
		ret=spawnlp(P_WAIT,"HTGET","",name,"-o",temp);
#else
	if((ret = spawnlp("WGET","","-O",temp,name)) == -1)
		ret=spawnlp("HTGET","",name,"-o",temp);
#endif
	unredirect(handle, 1);
	if(ret==-1) return NULL;
	/*
	 * WGET returns 0 if it is successful.  I have to check out HTGET's functionality, but
	 * usually 0 means success
	 */
	if(ret==0) {
		return( temp );
	}
	else return NULL;
}
#endif

#ifdef FEATURE_BACKCOMPAT
/* 
 * This function is simply to be used to be backwards compatible with FDPKG.BAT, so that old
 * packages are completely compatible with this new version of FDPKG
 */
int fdpkg_bat(int argc, char **argv) {
	short binary, source, inst, rem, chk, conf;
	char buffer[_MAX_PATH];
	buffer[0] = binary = source = inst = rem = chk = conf = 0;
	if(argc < 2) return 4;
	/*
	 * Of the four arguments listed below, at least one is REQUIRED
	 */
	if     (strcmpi(argv[1], "configure") == 0) conf   = 1;
	else if(strcmpi(argv[1], "check"    ) == 0) chk    = 1;
	else if(strcmpi(argv[1], "binary"   ) == 0) binary = 1;
	else if(strcmpi(argv[1], "source"   ) == 0) source = 1;
	else return 4;
	if(binary || source) {
		/*
		 * If the arguments binary or source were used, one of the following four are
		 * required
		 */
		if      (strcmpi(argv[2], "install") == 0) inst = 1;
		else if (strcmpi(argv[2], "remove" ) == 0) rem  = 1;
		else if (strcmpi(argv[2], "extract") == 0) inst = extract = 1;
		else if (strcmpi(argv[2], "upgrade") == 0) inst = 1;
		else return 4;
		if(argc < 3) return 4;
	}
	/* Now lets simply convert the filename into something manippkg can read */
	if       (conf) {
		sprintf(buffer, "%sx", argv[2]);
		manippkg(buffer, CONFIG);
	} else if(chk)  {
		sprintf(buffer, "%sx", argv[2]);
		manippkg(buffer, CHECK);
	} else if(inst) {
		if(argc < 4) return 4;
		/* This might be better if the directory were actually changed to argv[4] */
		sprintf(buffer, "%s\\%s%c", argv[4], argv[3], (binary) ? 'x' : 's');
		manippkg(buffer, INSTALL);
	} else if(rem)  {
		sprintf(buffer, "%s%c", argv[3], (binary) ? 'x' : 's');
		manippkg(buffer, REMOVE);
	} else return 4;
#ifndef __TURBOC__
	return 0;
#endif
}
#endif

int main(int argc, char **argv)
{
	int	i,			/* To increment args */
		ret;			/* To hold the return value */
	short	c = 0;			/* To indicate not to enter into action with manippkg */
#ifdef FEATURE_WGET
	short	w = 0;			/* To indicate usage of wget */
#endif
	char	drive[_MAX_DRIVE],	/* For _splitpath */
		dir[_MAX_DIR],		/* For _splitpath */
		file[_MAX_FNAME],	/* For _splitpath */
		ext[_MAX_EXT];		/* For _splitpath */
	char	*cwd = NULL,		/* To hold the current directory */
		*fdpkg = NULL;		/* To hold the %FDPKG% environment variable */
#ifdef FEATURE_WGET
	char	*wget = NULL;		/* To hold the temp file outputted by wget */
#endif

	kittenopen("FDPKG");
	dosdir = getenv("DOSDIR");
	comspec = getenv("COMSPEC");
	if(dosdir == NULL || comspec == NULL) {
	/* 
	 * %DOSDIR% must point to the FreeDOS directory (often C:\FDOS), and %COMSPEC% must point
	 * to FreeCOM or another command interpretor
	 */		
		kitten_printf(1, 0, "Invalid environment variables.\n");
		ret = c = 3;
	}
	cwd = getcwd(NULL,0);	/* Get the current directory, allocate what is necessary */
#ifdef FEATURE_BATCH_WRAPPERS /* Not functional */
	if     (strstr(argv[0], "FDCDROM" ) != NULL) return fdcdrom (argc, argv);
	else if(strstr(argv[0], "WFDGET"  ) != NULL) return wfdget  (argc, argv);
	else if(strstr(argv[0], "FDGET"   ) != NULL) return fdget   (argc, argv);
	else if(strstr(argv[0], "FDSOURCE") != NULL) return fdsource(argc, argv);
#endif
	/* Parse %FDPKG% for options */
	if((fdpkg = getenv("FDPKG")) != NULL) checkarg(fdpkg);
	/*
	 * The following checks arguments with the relative argv if the argument either starts
	 * with /, -, or the DOS switch character (INT 21h/AX=3700h)
	 */
	for(i=1;(i<argc)&&(argv[i][0]=='/'||argv[i][0]=='-'||argv[i][0]==switchcharacter())&&!c;i++)
		if((c = checkarg(&argv[i][1]))!=0) ret = c;
#ifdef FEATURE_BACKCOMPAT
	if(manip == FDPKGBAT) return fdpkg_bat(argc - i, &argv[i]);
#endif
	if(i>=argc&&ret<10) {	/* We must have more than just the options */
		kitten_printf(1, 1, "Insufficient arguments.\n");
		ret = c = 4;
	}
	while((i<argc) && !c) {
	/*
	 * Here, the path gets split up, the drive gets changed (if necessary), the dir gets
	 * changed (if necessary), and the remainder (file and extention) gets passed to
	 * manippkg where the real fun begins.  And of course we change back to the drive and
	 * directory where we started :-).  Possibly some non-splitpath code could be used if
	 * Pacific C is compiling since Pacific C seems to have such a hard time with a
	 * _splitpath/fnsplit implementation
	 */
#ifdef FEATURE_WGET
		/* Check for URLs to download */
		if(strnicmp(argv[i], "http://", 7) == 0 || strnicmp(argv[i], "ftp://", 6) == 0) {
			wget = downtemp(argv[i]);
			w = 1;
			if(wget == NULL) { 
				printf("Could not download package from internet.\n");
				ret = w = 9;
			} else _splitpath(wget, drive, dir, file, ext);
		} else {
			w = 0;
#endif
			/* Convert '/' to '\' */
			unix2dos(argv[i]);
			/* Split the path to make it easier to change drive and directory */
            /*
            * Check %PKGDIRS% for the package too
            * Note: %PKGDIRS% does NOT accept wildcards
            */
            if( access( argv[i], 0 ) != 0 &&
                ( fdpkg = _searchpath( argv[i], "PKGDIRS", 0 ) ) != NULL )
                _splitpath( fdpkg, drive, dir, file, ext );
            else _splitpath(argv[i], drive, dir, file, ext);
#ifdef FEATURE_WGET
		}
		if((w && w != 9) || !w) {
#endif
			if(drive[0] != '\0') _setdisk(toupper(drive[0]) - 'A');
			if(dir[strlen(dir)-1]=='\\' && strlen(dir) != 1) dir[strlen(dir)-1] = '\0';
			if(dir[0]!='\0'&&chdir(dir)==-1) {
				/* chdir failed */
				kitten_printf(1, 2, "Drive or directory nonexistant.\n");
				ret = 255;
			} else {
				/* Concatenate file and ext and pass to manippkg */
				_makepath(dir, NULL, NULL, file, ext);
                ret = manippkg( dir, manip );
			}
			/* Change back to original drive and directory */
			_setdisk(toupper(cwd[0]) - 'A');
			chdir(cwd);
#ifdef FEATURE_WGET
			/* Delete the temp package downloaded by wget or htget */
			if(w && w != 9) remove(wget);
		}
#endif
		i++;
	}
	free(cwd);	/* Free the current working directory buffer */
	kittenclose();
	return ret;
}

