/***************************************************************************/
/* REPLACE                                                                 */
/*                                                                         */
/* A replacement for the MS DOS(tm) REPLACE command.                       */
/*-------------------------------------------------------------------------*/
/* compatible compilers:                                                   */
/* - Borland C++ (tm) v3.0 or higher                                       */
/* - DJGPP v2.02 or higher                                                 */
/*-------------------------------------------------------------------------*/
/* (C)opyright 2001 by Rene Ableidinger (rene.ableidinger@gmx.at)          */
/*                                                                         */
/* This program is free software; you can redistribute it and/or modify it */
/* under the terms of the GNU General Public License version 2 as          */
/* published by the Free Software Foundation.                              */
/*                                                                         */
/* This program is distributed in the hope that it will be useful, but     */
/* WITHOUT ANY WARRANTY; without even the implied warranty of              */
/* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU        */
/* General Public License for more details.                                */
/*                                                                         */
/* You should have received a copy of the GNU General Public License along */
/* with this program; if not, write to the Free Software Foundation, Inc., */
/* 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.                */
/***************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <io.h>
#include <dir.h>
#include <dos.h>
#include <sys/stat.h>

/*-------------------------------------------------------------------------*/
/* COMPILER SPECIFICA                                                      */
/*-------------------------------------------------------------------------*/
#if __DJGPP__

#include <unistd.h>

/* disable DJGPP filename globbing, so wildcards are handled the DOS-way */
char **__crt0_glob_function (char *arg) {
  return 0;
}

/* convert BC _fullpath function into DJGPP _fixpath */
#define _fullpath(buffer, path, buflen) _fixpath(path, buffer)

/* directory separator is the UNIX-slash */
#define DIR_SEPARATOR "/"

#else

/* directory separator is the DOS-backslash */
#define DIR_SEPARATOR "\\"

/* constants for "access" function (equal to DJGPP but with other values!) */
#define F_OK 0  /* exists */
#define R_OK 4  /* readable */
#define W_OK 2  /* writeable */
#define X_OK 1  /* executable */

#endif

/*-------------------------------------------------------------------------*/
/* GLOBAL VARIABLES                                                        */
/*-------------------------------------------------------------------------*/
char opt_add = 0,
     opt_hidden = 0,
     opt_preview = 0,
     opt_prompt = 0,
     opt_readonly = 0,
     opt_subdir = 0,
     opt_update = 0,
     opt_wait = 0,
     dest_drive;
int file_counter = 0;

/*-------------------------------------------------------------------------*/
/* PROTOTYPES                                                              */
/*-------------------------------------------------------------------------*/
void classify_args(char argc, char *argv[],
                   char *fileargc, char *fileargv[],
                   char *optargc, char *optargv[]);
void help(void);
void exit_fn(void);
void replace_files(char src_pathname[], char src_filemask[], char dest_pathname[]);
void replace_file(char src_filename[], char dest_filename[]);
void preview_file(char dest_filename[]);
void copy_file(char src_filename[], char dest_filename[]);

/*-------------------------------------------------------------------------*/
/* MAIN PROGRAM                                                            */
/*-------------------------------------------------------------------------*/
int main(int argc, char *argv[]) {
  char fileargc,
       *fileargv[255],
       optargc,
       *optargv[255],
       cur_pathname[MAXPATH] = "",
       src_pathname[MAXPATH] = "",
       src_filemask[MAXPATH] = "",
       dest_pathname[MAXPATH] = "",
       *ptr,
       option[255] = "",
       i;


  classify_args(argc, argv, &fileargc, fileargv, &optargc, optargv);

  if (fileargc < 1 || optargv[0] == "?") {
    help();
    exit(11);
  }

  /* activate termination function */
  /* (writes no. of replaced/added files at exit) */
  atexit(exit_fn);

  /* store current path */
  getcwd(cur_pathname, MAXPATH);

  /* get source pathname (with trailing backspace) and filename/-mask */
  _fullpath(src_pathname, fileargv[0], MAXPATH);
  ptr = strrchr(src_pathname, (char)* DIR_SEPARATOR);
  strcpy(src_filemask, ptr + 1);
  *ptr = '\0';
  if (strlen(src_pathname) <= 2) {
    /* source path is a root directory -> add previously removed */
    /* backspace again */
    strcat(src_pathname, DIR_SEPARATOR);
  }
  /* check source path */
  if (chdir(src_pathname) != 0) {
    printf("Source path not found - %s\n", src_pathname);
    exit(3);
  }
  /* restore path */
  chdir(cur_pathname);
  if (strlen(src_pathname) > 3) {
    /* source path is not a root directory -> add backspace */
    strcat(src_pathname, DIR_SEPARATOR);
  }

  /* get destination pathname (with trailing backspace) */
  if (fileargc < 2) {
    /* no destination path specified -> use current */
    strcpy(dest_pathname, cur_pathname);
  }
  else {
    /* destination path specified -> check it */
    _fullpath(dest_pathname, fileargv[1], MAXPATH);
    if (chdir(dest_pathname) != 0) {
      printf("Destination path not found - %s\n", dest_pathname);
      exit(3);
    }
    /* restore path */
    chdir(cur_pathname);
  }
  strcat(dest_pathname, DIR_SEPARATOR);

  /* check if source and destination path are equal */
  if (strcmp(src_pathname, dest_pathname) == 0) {
    printf("Source and destination path must not be the same\n");
    exit(11);
  }

  /* get destination drive (1 = A, 2 = B, 3 = C, ...) */
  dest_drive = toupper(dest_pathname[0]) - 64;

  /* get options */
  for (i = 0; i < optargc; i++) {
    strcpy(option, optargv[i]);
    strupr(option);
    if (strcmp(option, "A") == 0)
      opt_add = -1;
    else if (strcmp(option, "H") == 0)
      opt_hidden = -1;
    else if (strcmp(option, "N") == 0)
      opt_preview = -1;
    else if (strcmp(option, "P") == 0)
      opt_prompt = -1;
    else if (strcmp(option, "R") == 0)
      opt_readonly = -1;
    else if (strcmp(option, "S") == 0)
      opt_subdir = -1;
    else if (strcmp(option, "U") == 0)
      opt_update = -1;
    else if (strcmp(option, "W") == 0)
      opt_wait = -1;
    else {
      printf("Invalid parameter - %s\n", optargv[i]);
      exit(11);
    }
  }
  if ((opt_add && opt_subdir) ||
      (opt_add && opt_update)) {
    printf("Invalid combination of parameters\n");
    exit(11);
  }

  if (opt_wait) {
    printf("Press any key to continue...\n");
    getch();
    fflush(stdin);
  }

  replace_files(src_pathname, src_filemask, dest_pathname);

  return 0;
}

/*-------------------------------------------------------------------------*/
/* SUB PROGRAMS                                                            */
/*-------------------------------------------------------------------------*/
void classify_args(char argc, char *argv[],
                   char *fileargc, char *fileargv[],
                   char *optargc, char *optargv[]) {
/*-------------------------------------------------------------------------*/
/* Sorts the program-parameters into file- and option-parameters.          */
/*-------------------------------------------------------------------------*/
  char i,
       *argptr;


  *fileargc = 0;
  *optargc = 0;
  for (i = 1; i < argc; i++) {
    argptr = argv[i];
    if (argptr[0] == '/' || argptr[0] == '-') {
      /* first character of parameter is '/' or '-' -> option-parameter */
      optargv[*optargc] = argptr + 1;
      *optargc = *optargc + 1;
    }
    else {
      /* file-parameter */
      fileargv[*fileargc] = argptr;
      *fileargc = *fileargc + 1;
    }
  }
}

void help(void) {
  printf("Replaces files in the destination directory with files from the source\n"
         "directory that have the same name.\n"
         "\n"
         "REPLACE [drive1:][path1]filename [drive2:][path2] [/switches]\n"
         "\n"
         "  [drive1:][path1]filename    Specifies the source file or files.\n"
         "  [drive2:][path2]            Specifies the directory where files are to be\n"
	 "                              replaced.\n"
         "  /A    Adds new files to destination directory. Cannot use with /S or /U\n"
         "        switches.\n"
         "  /H    Adds or replaces hidden and system files as well as unprotected files.\n"
         "  /N    Preview mode - does not add or replace any file.\n"
         "  /P    Prompts for confirmation before replacing a file or adding a source\n"
         "        file.\n"
         "  /R    Replaces read-only files as well as unprotected files.\n"
         "  /S    Replaces files in all subdirectories of the destination directory.\n"
         "        Cannot use with the /A switch.\n"
         "  /W    Waits for you to insert a disk before beginning.\n"
         "  /U    Replaces (updates) only files that are older than source files. Cannot\n"
         "        use with the /A switch.\n");
}

void exit_fn(void) {
/*-------------------------------------------------------------------------*/
/* Gets called by the "exit" command and is used to write a                */
/* status-message.                                                         */
/*-------------------------------------------------------------------------*/
  char preview_txt[255] = "";


  if (opt_preview) {
    strcpy(preview_txt, "would be ");
  }

  if (opt_add) {
    printf("%d file(s) %sadded\n", file_counter, preview_txt);
  }
  else {
    printf("%d file(s) %sreplaced\n", file_counter, preview_txt);
  }
}

void replace_files(char src_pathname[], char src_filemask[], char dest_pathname[]) {
/*-------------------------------------------------------------------------*/
/* Searchs through the source directory (and destination subdirectories)   */
/* and calls function "replace_file" for every found file.                 */
/*-------------------------------------------------------------------------*/
  char filemask[MAXPATH],
       new_dest_pathname[MAXPATH],
       src_filename[MAXPATH],
       dest_filename[MAXPATH];
  struct ffblk fileblock;
  int fileattrib,
      done;


  fileattrib = FA_RDONLY + FA_ARCH;
  if (opt_hidden) {
    /* replace hidden and system files too */
    fileattrib = fileattrib + FA_HIDDEN + FA_SYSTEM;
  }

  strcpy(filemask, src_pathname);
  strcat(filemask, src_filemask);
  done = findfirst(filemask, &fileblock, fileattrib);
  while (! done) {
    /* rebuild source filename */
    strcpy(src_filename, src_pathname);
    strcat(src_filename, fileblock.ff_name);

    /* build destination filename */
    strcpy(dest_filename, dest_pathname);
    strcat(dest_filename, fileblock.ff_name);

    replace_file(src_filename, dest_filename);

    done = findnext(&fileblock);
  }

  if (opt_subdir) {
    /* replace files in sub directories too */
    strcpy(filemask, dest_pathname);
    strcat(filemask, "*.*");
    done = findfirst(filemask, &fileblock, FA_DIREC);
    while (! done) {
      if (fileblock.ff_attrib == FA_DIREC &&
          strcmp(fileblock.ff_name, ".") != 0 &&
          strcmp(fileblock.ff_name, "..") != 0) {
        /* build destination pathname */
        strcpy(new_dest_pathname, dest_pathname);
        strcat(new_dest_pathname, fileblock.ff_name);
        strcat(new_dest_pathname, DIR_SEPARATOR);

        replace_files(src_pathname, src_filemask, new_dest_pathname);
      }

      done = findnext(&fileblock);
    }
  }
}

void replace_file(char src_filename[], char dest_filename[]) {
/*-------------------------------------------------------------------------*/
/* Checks all dependencies of the source and destination file and calls    */
/* function "copy_file" or "preview_file".                                 */
/*-------------------------------------------------------------------------*/
  char ch,
       msg_prompt[255];
  int dest_file_exists,
      fileattrib;
  struct stat src_statbuf,
              dest_statbuf;
  struct dfree disktable;
  unsigned long free_diskspace;


  /* check source file for read permission */
  /* (only usefull under an OS with the ability to deny read access) */
  if (access(src_filename, R_OK) != 0) {
    printf("Read-access denied - %s\n", src_filename);
    exit(5);
  }

  /* get info of source and destination file */
  stat(src_filename, &src_statbuf);
  dest_file_exists = ! stat(dest_filename, &dest_statbuf);

  /* get amount of free disk space in destination drive */
  getdfree(dest_drive, &disktable);
  free_diskspace = (unsigned long) disktable.df_avail *
                   disktable.df_sclus * disktable.df_bsec;

  if (opt_add) {
    /* file can be added only if it doesn't exist */
    if (dest_file_exists) {
      return;
    }

    /* check free space on destination disk */
    if (src_statbuf.st_size > free_diskspace) {
      printf("Insufficient disk space in destination path - %s\n", dest_filename);
      exit(39);
    }
  }
  else {
    /* file can be replaced only if it already exists */
    if (! dest_file_exists) {
      return;
    }

    if (opt_update) {
      /* check if source file is older than destination file */
      if (src_statbuf.st_mtime <= dest_statbuf.st_mtime) {
        return;
      }
    }

    /* check free space on destination disk */
    if (src_statbuf.st_size > free_diskspace - dest_statbuf.st_size) {
      printf("Insufficient disk space in destination path - %s\n", dest_filename);
      exit(39);
    }

    /* get file attribute from destination file */
    fileattrib = _chmod(dest_filename, 0);

    /* check destination file for write permission */
    if (fileattrib & (64 ^ FA_RDONLY) != 0) {
      if (! opt_readonly) {
        printf("Write-access denied - %s\n", dest_filename);
        exit(5);
      }

      if (! opt_preview) {
        /* not in preview mode -> remove readonly attribute from */
        /* destination file */
        fileattrib = fileattrib ^ FA_RDONLY;
        _chmod(dest_filename, 1, fileattrib);
      }
    }

    /* check destination file for hidden or system attribute */
    if (((fileattrib & (64 ^ FA_HIDDEN)) != 0) ||
        ((fileattrib & (64 ^ FA_SYSTEM)) != 0)) {
      if (! opt_hidden)  {
        return;
      }

      if (! opt_preview) {
        /* not in preview mode -> remove hidden and system attribute from */
        /* destination file */
        fileattrib = fileattrib ^ FA_HIDDEN ^ FA_SYSTEM;
        _chmod(dest_filename, 1, fileattrib);
      }
    }
  }

  if (opt_prompt) {
    /* ask for confirmation */
    if (opt_add) {
      strcpy(msg_prompt, "Add %s (Y/N) ");
    }
    else {
      strcpy(msg_prompt, "Replace %s (Y/N) ");
    }

    do {
      printf(msg_prompt, dest_filename);
      scanf("%c", &ch);
      fflush(stdin);
      ch = toupper(ch);
    } while (ch != 'Y' && ch != 'N');
    if (ch != 'Y') {
      return;
    }
  }

  if (opt_preview) {
    /* preview mode -> don't copy file */
    preview_file(dest_filename);
  }
  else {
    copy_file(src_filename, dest_filename);
  }

  file_counter = file_counter + 1;
}

void preview_file(char dest_filename[]) {
  if (opt_add) {
    printf("Would add %s\n", dest_filename);
  }
  else {
    printf("Would replace %s\n", dest_filename);
  }
}

void copy_file(char src_filename[], char dest_filename[]) {
/*-------------------------------------------------------------------------*/
/* Copies the source into the destination file including file attributes   */
/* and timestamp.                                                          */
/*-------------------------------------------------------------------------*/
  FILE *src_file,
       *dest_file;
  char buffer[BUFSIZ];
  int buffersize,
      fileattrib;
  struct ftime filetime;


  /* open source file */
  src_file = fopen(src_filename, "rb");
  if (src_file == NULL) {
    printf("Cannot open source file - %s\n", src_filename);
    exit(30);
  }

  /* open destination file */
  dest_file = fopen(dest_filename, "wb");
  if (dest_file == NULL) {
    printf("Cannot create destination file - %s\n", dest_filename);
    fclose(src_file);
    exit(29);
  }

  if (opt_add) {
    printf("Adding %s\n", dest_filename);
  }
  else {
    printf("Replacing %s\n", dest_filename);
  }

  /* copy file data */
  buffersize = fread(buffer, sizeof(char), BUFSIZ, src_file);
  while (buffersize > 0) {
    if (fwrite(buffer, sizeof(char), buffersize, dest_file) != buffersize) {
      printf("Write error on destination file - %s\n", dest_filename);
      fclose(src_file);
      fclose(dest_file);
      exit(29);
    }
    buffersize = fread(buffer, sizeof(char), BUFSIZ, src_file);
  }

  /* copy file timestamp */
  getftime(fileno(src_file), &filetime);
  setftime(fileno(dest_file), &filetime);

  /* close files */
  fclose(src_file);
  fclose(dest_file);

  /* copy file attributes */
  fileattrib = _chmod(src_filename, 0);
  _chmod(dest_filename, 1, fileattrib);
}
