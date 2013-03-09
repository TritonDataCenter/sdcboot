/*
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifdef __WATCOMC__
#include <direct.h>
#include <stdlib.h>
#else
#include <dir.h>
#endif
#include <stdio.h>
#include <io.h>
#include <string.h>
#include <dos.h>
#include <sys/stat.h>

#ifdef USE_KITTEN
#include "kitten.h"   /* Translation functions */
#endif
#include "movedir.h"  /* MoveDirectory, MoveTree */
#include "misc.h"     /* addsep, dir_exist, SplitPath, strmcpy,
                         strmcat, error, copy_file, build_filename,
                         makedir */

/* P R O T O T Y P E S */
/*--------------------------------------------------*/

static int  xcopy_file(const char *src_filename, const char *dest_filename);

/*-------------------------------------------------------------------------*/
/* Strips the last file from the path.                       		   */
/*-------------------------------------------------------------------------*/

static void GetPreviousDir(char* path)
{
    char drive[MAXDRIVE], dir[MAXDIR], fname[MAXFILE], ext[MAXEXT];
    size_t len;

    SplitPath(path, drive, dir, fname, ext);
    strcpy(path, drive);

    /* DRR: Some Turbo C functions doesn't like trailing
     * '\' in directory names
     */
    len = strlen(dir);
    if (dir[len-1] == '\\')
       dir[len-1] = '\0';

    strcat(path, dir);
}

/*-------------------------------------------------------------------------*/
/* Gets the path name, returned by a search in the wildcard enabled path   */
/*-------------------------------------------------------------------------*/

static void GetIndividualFile(char* dest, char* path, char* file)
{
    strcpy(dest, path);
    GetPreviousDir(dest);
    addsep(dest);
    strcat(dest, file);
}

/*-------------------------------------------------------------------------*/
/* Adds a *.* wildcard to a path                           		   */
/*-------------------------------------------------------------------------*/

static void AddAllWildCard(char* path)
{
    addsep(path);
    strcat(path, "*.*");
}

/*-------------------------------------------------------------------------*/
/* Deletes a directory tree.                            		   */
/*-------------------------------------------------------------------------*/

static int DelTree(const char* path)
{
    int found, attrib;
    char temppath[MAXPATH];
    char temppath1[MAXPATH];
    char origpath[MAXPATH];
    struct ffblk info;

    strcpy(temppath, path);
    strcpy(origpath, path);
    GetPreviousDir(origpath);

    /* Two state machine: */

    /* As long as the complete tree is not deleted */
    AddAllWildCard(origpath);
    AddAllWildCard(temppath);
    while (strcmp(temppath, origpath) != 0)
    {
          /* 1) as long as there are still sub directories, enter the
                first one */
	  found = findfirst(temppath, &info, FA_DIREC) == 0;
	  while (found &&
		 (info.ff_name[0] == '.' ||
		  (info.ff_attrib & FA_DIREC) == 0))
	  {
	     found = findnext(&info) == 0;
	  }

	  if (found)
	  {
	     GetIndividualFile(temppath1, temppath, info.ff_name);
	     strcpy(temppath, temppath1);
             AddAllWildCard(temppath);
	  }

          /* 2) if there are no more sub directories in this directory, delete
                all the files, leave the directory, and delete it */
	  else
          {
	      found = findfirst(temppath, &info, FA_RDONLY|FA_HIDDEN|FA_SYSTEM|FA_ARCH) == 0;
	      while (found)
	      {
		  GetIndividualFile(temppath1, temppath, info.ff_name);
		  if (chmod(temppath1, S_IREAD|S_IWRITE) == -1)
		      return 0;

                  if (unlink(temppath1) == -1)
		      return 0;

		  found = findnext(&info) == 0;
	      }
              GetPreviousDir(temppath); /* Strip \*.* */

              /* DRR: FreeDOS cannot set attributes of directory without
               * clearing directory bit (but the directory will not be 
               * changed to a normal file)
               */
              if ((attrib = _chmod(temppath, 0)) == -1)
                 return 0;
              attrib &= ~FA_RDONLY;
              if (_chmod(temppath, 1, attrib & ~FA_DIREC) == -1)
                 return 0;

              if (rmdir(temppath) == -1)
	         return 0;

              GetPreviousDir(temppath);       /* Leave directory */
              AddAllWildCard(temppath);
	}
    }
    return 1;
}

/*-------------------------------------------------------------------------*/
/* Searchs through the source directory (and its subdirectories) and calls */
/* function "xcopy_file" for every found file.                             */
/*-------------------------------------------------------------------------*/
static int CopyTree(const char *src_pathname,
             const char *src_filename,
             const char *dest_pathname,
             const char *dest_filename) 
{
  char filepattern[MAXPATH],
       new_src_pathname[MAXPATH],
       new_dest_pathname[MAXPATH],
       src_path_filename[MAXPATH],
       dest_path_filename[MAXPATH],
       tmp_filename[MAXFILE + MAXEXT],
       tmp_pathname[MAXPATH];
  struct ffblk fileblock;
  int fileattrib,
      done;

  /* copy files in subdirectories  */
  strmcpy(filepattern, src_pathname, sizeof(filepattern));
  strmcat(filepattern, src_filename, sizeof(filepattern));
  done = findfirst(filepattern, &fileblock, FA_DIREC);
  while (!done)
  {
    if (fileblock.ff_attrib == FA_DIREC &&
        strcmp(fileblock.ff_name, ".") != 0 &&
        strcmp(fileblock.ff_name, "..") != 0)
    {
        /* build source pathname */
        strmcpy(new_src_pathname, src_pathname, sizeof(new_src_pathname));
        strmcat(new_src_pathname, fileblock.ff_name, sizeof(new_src_pathname));
        strmcat(new_src_pathname, DIR_SEPARATOR, sizeof(new_src_pathname));

        /* build destination pathname */
        strmcpy(new_dest_pathname, dest_pathname, sizeof(new_dest_pathname));
        strmcat(new_dest_pathname, fileblock.ff_name, sizeof(new_dest_pathname));
        strmcat(new_dest_pathname, DIR_SEPARATOR, sizeof(new_dest_pathname));

        if (!CopyTree(new_src_pathname, "*.*",
                 new_dest_pathname, "*.*")) return 0;
    }

    done = findnext(&fileblock);
  }

  fileattrib = FA_RDONLY+FA_ARCH+FA_HIDDEN+FA_SYSTEM;

  /* find first source file */
  strmcpy(filepattern, src_pathname, sizeof(filepattern));
  strmcat(filepattern, src_filename, sizeof(filepattern));
  done = findfirst(filepattern, &fileblock, fileattrib);

  /* check if destination directory must be created */
  if (!dir_exists(dest_pathname))
  {
    strmcpy(tmp_pathname, dest_pathname, sizeof(tmp_pathname));

    if (makedir(tmp_pathname) != 0)
    {
#ifdef USE_KITTEN
      error(1,28,"Unable to create directory");
#else
      error("Unable to create directory");
#endif
      return 0;
    }
  }

  /* copy files */
  while (!done) 
  {
      /* build source filename including path */
      strmcpy(src_path_filename, src_pathname, sizeof(src_path_filename));
      strmcat(src_path_filename, fileblock.ff_name, sizeof(src_path_filename));

      /* build destination filename including path */
      strmcpy(dest_path_filename, dest_pathname, sizeof(dest_path_filename));
      build_filename(tmp_filename, fileblock.ff_name, dest_filename);
      strmcat(dest_path_filename, tmp_filename, sizeof(dest_path_filename));

      if (!xcopy_file(src_path_filename, dest_path_filename))
	  return 0;

      done = findnext(&fileblock);
  }
  
  return 1;
}

/*-------------------------------------------------------------------------*/
/* Checks all dependencies of the source and destination file and calls    */
/* function "copy_file".                                                   */
/*-------------------------------------------------------------------------*/
static int xcopy_file(const char *src_filename,
                const char *dest_filename) 
{
  struct stat src_statbuf;
  struct dfree disktable;
  unsigned long free_diskspace;
  unsigned char dest_drive;

  /* Get the destination drive */
  if (dest_filename[1] == ':')
      dest_drive = dest_filename[0];
  else
      dest_drive = getdisk() + 'A' - 1;
  
  
  /* get info of source and destination file */
  stat((char *)src_filename, &src_statbuf);

  /* get amount of free disk space in destination drive */
  getdfree(dest_drive-'A'+1, &disktable);
  free_diskspace = (unsigned long) disktable.df_avail *
                   disktable.df_sclus * disktable.df_bsec;

  /* check free space on destination disk */
  if (src_statbuf.st_size > free_diskspace) 
  {
#ifdef USE_KITTEN
      error(1,29,"Insufficient disk space in destination path");
#else
      error("Insufficient disk space in destination path");
#endif
      return(0);
  }

  /* Copy file data */
  return copy_file(src_filename, dest_filename);
}

/*-------------------------------------------------------------------------*/
/* Moves a directory from one place to another                             */
/*-------------------------------------------------------------------------*/

int MoveDirectory(const char* src_filename, const char* dest_filename)
{
    char src_path[MAXPATH], src_file[MAXPATH];
    char dest_path[MAXPATH],dest_file[MAXPATH];

    char drive[MAXDRIVE], dir[MAXDIR], fname[MAXFILE], ext[MAXEXT];

    SplitPath(src_filename, drive, dir, fname, ext);
    
    strcpy(src_path, drive);
    strcat(src_path, dir);

    strcpy(src_file, fname);
    strcat(src_file, ext);

    SplitPath(dest_filename, drive, dir, fname, ext);

    strcpy(dest_path, drive);
    strcat(dest_path, dir);    

    strcpy(dest_file, fname);
    strcat(dest_file, ext);

    /* DRR: Ensure that I can move a dir to another drive
     * and with another name
     */
    if (stricmp(src_file, dest_file)) {
       strcat(dest_path, dest_file);
       strcat(dest_path, DIR_SEPARATOR);
       strcpy(dest_file, src_file);
    } 
    if (dir_exists(dest_filename))
        if (!DelTree(dest_filename))
	    return 0;

    if (!CopyTree(src_path, src_file, dest_path, dest_file))
    {
	DelTree(dest_filename);	
	return 0;
    }

    return DelTree(src_filename);
}

/*-------------------------------------------------------------------------*/
/* Moves a directory on the same volume.                                   */
/*-------------------------------------------------------------------------*/

int RenameTree(const char* src_filename, const char* dest_filename)
{    
    int found, attrib;
    char temppath[MAXPATH];
    char temppath1[MAXPATH];
    char origpath[MAXPATH];
    char tgtpath[MAXPATH];
    char tgtpath1[MAXPATH];
    
    struct ffblk info;

    char srcdrive[MAXDRIVE], srcdir[MAXDIR], srcfname[MAXFILE], srcext[MAXEXT];
    char dstdrive[MAXDRIVE], dstdir[MAXDIR], dstfname[MAXFILE], dstext[MAXEXT];
 
    /* If the target directory exists, delete it */
    if (dir_exists(dest_filename))
    {    
       if (!DelTree(dest_filename))
	  return 0;
    }
        
    /* First see wether we are not renaming a directory in the same path */
    
    SplitPath(src_filename, srcdrive, srcdir, srcfname, srcext);
    SplitPath(dest_filename, dstdrive, dstdir, dstfname, dstext);
    
    if ((strcmpi(srcdir, dstdir) == 0) &&
	(!dir_exists(dest_filename)))
    {
	return rename(src_filename, dest_filename) == 0;
    }
    
    /* Deep renaming a directory */
    strcpy(temppath, src_filename);
    strcpy(origpath, src_filename);
    GetPreviousDir(origpath);

    strcpy(tgtpath, dest_filename);
    if (mkdir(tgtpath) == -1)
	return 0;   

    /* Two state machine: */
    
    /* As long as the complete tree is not deleted */
    AddAllWildCard(origpath);
    AddAllWildCard(temppath);
    while (strcmp(temppath, origpath) != 0)
    {
          /* 1) as long as there are still sub directories, enter the
                first one */
	  found = findfirst(temppath, &info, FA_DIREC) == 0;
	  while (found &&
		 (info.ff_name[0] == '.' ||
		  (info.ff_attrib & FA_DIREC) == 0))
	  {
	     found = findnext(&info) == 0;
	  }

	  if (found)
	  {
	     GetIndividualFile(temppath1, temppath, info.ff_name);
	     strcpy(temppath, temppath1);
             AddAllWildCard(temppath);
	      
	     /* Make sure the target directory exists */	      
	     addsep(tgtpath); 
	     strcat(tgtpath, info.ff_name);
	      
	     if (mkdir(tgtpath) == -1)
		 return 0;
	  }

          /* 2) if there are no more sub directories in this directory, rename
                all the files and leave this directory, and delete this directory */
	  else
          {
	      found = findfirst(temppath, &info, FA_RDONLY|FA_HIDDEN|FA_SYSTEM|FA_ARCH) == 0;
	      while (found)
	      {
		  GetIndividualFile(temppath1, temppath, info.ff_name);
      
		  strcpy(tgtpath1, tgtpath);
		  addsep(tgtpath1);
		  strcat(tgtpath1, info.ff_name);

		  unlink(tgtpath1);

		  if (rename(temppath1, tgtpath1) == -1)
		      return 0;

		  found = findnext(&info) == 0;
	      }
	
	      GetPreviousDir(temppath);	/* Strip *.* */

              /* DRR: FreeDOS cannot set attributes of directory without
               * clearing directory bit (but the directory will not be 
               * changed to a normal file)
               */
              if ((attrib = _chmod(temppath, 0)) == -1)
                 return 0;
              attrib &= ~FA_RDONLY;
              if (_chmod(temppath, 1, attrib & ~FA_DIREC) == -1)
	         return 0;

	      if (rmdir(temppath) == -1)
	         return 0;
	    
	      
	      GetPreviousDir(temppath);       /* Leave directory */
              AddAllWildCard(temppath);
	      
	      GetPreviousDir(tgtpath);        /* Leave target directory */
	}
    }
    
    return 1;    
}
