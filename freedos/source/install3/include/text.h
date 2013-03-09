/* Copyright (C) 1998,1999,2000,2001 Jim Hall <jhall@freedos.org> */

/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

/* This header contains all hard coded strings and
   corresponding defines to use with catgets
*/

/* install.en (identifier :MSG_* must be removed) */
/*
0.0:FreeDOS Install - Copyright (C) 1998-2000 Jim Hall
0.1:This is free software, and you are welcome to redistribute it
0.2:under certain conditions; see the file COPYING for details.
0.3:Install comes with ABSOLUTELY NO WARRANTY
0:4:MSG_INSTALLOK:The Install program completed successfully.\n
0:5:MSG_INSTALLERRORS:There were %u errors and %u non-fatal warnings.\n
0:6:MSG_TITLE:FreeDOS Install
1:0:MSG_PRESSKEY:Press any key to continue
1:1:MSG_INSTALLFROM:Where are the install files? (where to install from?)
1:2:MSG_INSTALLTO:Where will files be installed? (where to install to?)
1:3:MSG_WILLINSTALLFROM:Installing from:
1:4:MSG_WILLINSTALLTO:Installing to:
1:5:MSG_INSTALLDIROK:Are above directories correct?
2:0:MSG_NO:No
2:1:MSG_YES:Yes
2:2:MSG_INSTALLSETYN:Do you want to install this disk set?
2:3:MSG_CONTINSTDISK:Continue installing this disk?"
2:4:MSG_INSTALLPKG:Install this package?
2.5:MSG_YESNOYyNn:YyNn
2.6:MSG_YESTOALL:Yes to All
3:0:MSG_DISKSET:Disk set:
3:1:missing - no message
3:2:MSG_INSTSERIES:Installing series: 
3:3:MSG_PACKAGE:Package: 
3:4:MSG_MISSINGDATAFILE:Can't find data file for this install disk!
3:5:MSG_INSTSERIESDONE:Done installing this disk series.
3:6:MSG_ERRREQPKG:ERROR!  Failed to install REQUIRED package.
3:7:MSG_WARNOPTPKG:WARNING!  Failed to install OPTIONAL package.
3:8:MSG_WRONGFLOPPY:You may not have the right install floppy in the drive.
3:9:MSG_STILLWRONGFLOPPY:Double check that you have the right disk and try again.
3:10:MSG_NEXTSERIESDISK:If you are installing other disk series, please insert
3:11:MSG_NEXTSERIESDISK2:disk #1 of the next series in the drive now.
3:12:MSG_INSERT1STDISK:Please insert disk #1 for disk set %s
4:0:MSG_OPTIONAL:OPTIONAL
4:1:MSG_REQUIRED:REQUIRED
4:2:MSG_SKIPPED:SKIPPED
5:0:MSG_USAGE:USAGE:  install [/mono] [/src <src path>] [/dst <dest path>] [/nopause] [/nolog]\n\n
5:1:MSG_UNKNOWNOPTION:%s: unknown option /%c\n
5:2:MSG_ARGMISSING:%s: /%c argument missing\n
6:0:MSG_ERRALLOCMEMDF:Unable to allocate memory for install data file!\n
6:1:MSG_ERREMPTYDATAFILE:The install data file is empty!\n
6:2:MSG_ERROR:Error!\n
6:3:MSG_ERRALLOCMEMFDF:Unable to allocate enough memory for install floppy data file!\n
6:4:MSG_ERREMPTYFLOPPYDATAFILE:The install floppy data file is empty!\n
6:5:MSG_ERRORALLOCMEM:Unable to allocate required memory!\n
6:6:MSG_ERRCREATELOG:Error %i, unable to create INSTALL log\n
7:0:MSG_SIGINTABORT:\nControl-C pressed, aborting install!\n\n
7:1:MSG_SIGINTVERIFYABORT_STR:Are you sure you wish to abort the install? [yn]
*/

#ifndef _TEXT_H
#define _TEXT_H

#define SET_GENERAL 0
#define SET_PROMPT_LOC 1
#define SET_PROMPT_YN 2
#define SET_PKG_GENERAL 3
#define SET_PKG_NEED 4
#define SET_USAGE 5
#define SET_ERRORS 6
#define SET_SIGINT 7

/* install.c */
/* SET_GENERAL */
#define MSG_INSTALLOK 4
#define MSG_INSTALLOK_STR "The Install program completed successfully.\n"
#define MSG_INSTALLERRORS 5
#define MSG_INSTALLERRORS_STR "There were %u errors and %u non-fatal warnings.\n"

/* SET_PROMPT_LOC */
#define MSG_INSTALLFROM 1
#define MSG_INSTALLFROM_STR "Where are the install files? (where to install from?)"
#define MSG_INSTALLTO 2
#define MSG_INSTALLTO_STR "Where will files be installed? (where to install to?)"
#define MSG_WILLINSTALLFROM 3
#define MSG_WILLINSTALLFROM_STR "Installing from:"
#define MSG_WILLINSTALLTO 4
#define MSG_WILLINSTALLTO_STR "Installing to:"
#define MSG_INSTALLDIROK 5
#define MSG_INSTALLDIROK_STR "Are above directories correct?"

/* SET_PROMPT_YN */
#define MSG_NO 0
#define MSG_NO_STR "No"
#define MSG_YES 1
#define MSG_YES_STR "Yes"
#define MSG_INSTALLSETYN 2
#define MSG_INSTALLSETYN_STR "Do you want to install this disk set?"
#define MSG_YESNOYyNn 5
#define MSG_YESNOYyNn_STR "YyNn"

/* SET_PKG_GENERAL */
#define MSG_DISKSET 0
#define MSG_DISKSET_STR "Disk set:"

/* SET_PKG_NEED */
#define MSG_OPTIONAL 0
#define MSG_OPTIONAL_STR "OPTIONAL"
#define MSG_REQUIRED 1
#define MSG_REQUIRED_STR "REQUIRED"
#define MSG_SKIPPED 2
#define MSG_SKIPPED_STR "SKIPPED"

/* SET_USAGE */
#define MSG_USAGE 0
#define MSG_USAGE_STR "USAGE:  install [/mono] [/src <src path>] [/dst <dest path>] [/nopause] [/nolog]\n\n"

/* SET_ERRORS */
#define MSG_ERRALLOCMEMDF 0
#define MSG_ERRALLOCMEMDF_STR "Unable to allocate memory for install data file!\n"
#define MSG_ERREMPTYDATAFILE 1
#define MSG_ERREMPTYDATAFILE_STR "The install data file is empty!\n"
#define MSG_ERRORALLOCMEM 5
#define MSG_ERRORALLOCMEM_STR "Unable to allocate required memory!\n"
#define MSG_ERRCREATELOG 6
#define MSG_ERRCREATELOG_STR "Error %i, unable to create INSTALL log\n"


/* inst.c */
/* SET_GENERAL */

/* SET_PROMPT_LOC */
/* 
#define MSG_PRESSKEY_STR "Press any key to continue" 
*/

/* SET_PROMPT_YN */
/*
#define MSG_NO 0
#define MSG_NO_STR "No"
#define MSG_YES 1
#define MSG_YES_STR "Yes"
#define MSG_YESNOYyNn 5
#define MSG_YESNOYyNn_STR "YyNn"
*/
#define MSG_CONTINSTDISK 3
#define MSG_CONTINSTDISK_STR "Continue installing this disk?"
#define MSG_INSTALLPKG 4
#define MSG_INSTALLPKG_STR "Install this package?"
#define MSG_YESTOALL 6
#define MSG_YESTOALL_STR "Yes to All"

/* SET_PKG_GENERAL */
#define MSG_INSTSERIES 2
#define MSG_INSTSERIES_STR "Installing series: "
#define MSG_PACKAGE 3
#define MSG_PACKAGE_STR "Package: "
#define MSG_MISSINGDATAFILE 4
#define MSG_MISSINGDATAFILE_STR "Can't find data file for this install disk!"
#define MSG_INSTSERIESDONE 5
#define MSG_INSTSERIESDONE_STR "Done installing this disk series."
#define MSG_ERRREQPKG 6
#define MSG_ERRREQPKG_STR "ERROR!  Failed to install REQUIRED package."
#define MSG_WARNOPTPKG 7
#define MSG_WARNOPTPKG_STR "WARNING!  Failed to install OPTIONAL package."
#define MSG_WRONGFLOPPY 8
#define MSG_WRONGFLOPPY_STR "You may not have the right install floppy in the drive."
#define MSG_STILLWRONGFLOPPY 9
#define MSG_STILLWRONGFLOPPY_STR "Double check that you have the right disk and try again."
#define MSG_NEXTSERIESDISK 10
#define MSG_NEXTSERIESDISK_STR "If you are installing other disk series, please insert"
#define MSG_NEXTSERIESDISK2 11
#define MSG_NEXTSERIESDISK2_STR "disk #1 of the next series in the drive now."
#define MSG_INSERT1STDISK 12
#define MSG_INSERT1STDISK_STR "Please insert disk #1 for disk set %s"
#define MSG_NODISKSPC 13
#define MSG_NODISKSPC_STR "ERROR! Not enough disk space for package."

/* SET_PKG_NEED */
/*
#define MSG_OPTIONAL 0
#define MSG_OPTIONAL_STR "OPTIONAL"
#define MSG_REQUIRED 1
#define MSG_REQUIRED_STR "REQUIRED"
#define MSG_SKIPPED 2
#define MSG_SKIPPED_STR "SKIPPED"
*/

/* SET_USAGE */

/* SET_ERRORS */
#define MSG_ERROR 2
#define MSG_ERROR_STR "Error!\n"
#define MSG_ERRALLOCMEMFDF 3
#define MSG_ERRALLOCMEMFDF_STR "Unable to allocate enough memory for install floppy data file!\n"
#define MSG_ERREMPTYFLOPPYDATAFILE 4
#define MSG_ERREMPTYFLOPPYDATAFILE_STR "The install floppy data file is empty!\n"


/* getopt.c - get option letter from argv */
/* SET_USAGE */
#define MSG_UNKNOWNOPTION 1
#define MSG_UNKNOWNOPTION_STR "%s: unknown option /%c\n"
#define MSG_ARGMISSING 2
#define MSG_ARGMISSING_STR "%s: /%c argument missing\n"


/* repaint.c */
/* SET_GENERAL */
#define MSG_TITLE 6
#define MSG_TITLE_STR "FreeDOS Install"


/* pause.c */
/* SET_PROMPT_LOC */
#define MSG_PRESSKEY 0
#define MSG_PRESSKEY_STR "Press any key to continue"


/* cchndlr.c */
/* SET_SIGINT */
#define MSG_SIGINTABORT 0
#define MSG_SIGINTABORT_STR "\nControl-C pressed, aborting install!\n\n"
#define MSG_SIGINTVERIFYABORT 1
#define MSG_SIGINTVERIFYABORT_STR "Are you sure you wish to abort the install? [yn]"


#endif /* _TEXT_H */
