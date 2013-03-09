#include <stdio.h>
#include <io.h>
#ifdef __WATCOMC__
#include <direct.h>
#include <stdlib.h>
#include <stdarg.h>
#else
#include <dir.h>
#endif /* __WATCOMC__ */
#include <dos.h>
#include <string.h>
#include <sys/stat.h>

#ifdef USE_KITTEN
#include "kitten.h"
#endif /* USE_KITTEN */
#include "misc.h"

#ifdef __WATCOMC__
#pragma pack(1)
#define DOS_GETFTIME _dos_getftime
#define DOS_SETFTIME _dos_setftime

/*-------------------------------------------------------------------------*/
/* _chmod - get/set file attributes                                        */
/*-------------------------------------------------------------------------*/

static int _chmod(const char *filename, int func, ...)
{
	va_list l;
	int attributes;
	unsigned attr;
	va_start(l, func);
	attributes = va_arg(l, int);
	va_end(l);
	if (func == 0)
	{
		if (_dos_getfileattr(filename, &attr) != 0) return -1;
		return attr;
	}
	if (_dos_setfileattr(filename, attributes) != 0) return -1;
	return 0;
}

/*-------------------------------------------------------------------------*/
/* _chmod - get file date                                                  */
/*-------------------------------------------------------------------------*/

int _cdecl getftime (int handle, struct ftime *ftimep)
{
      int retval = 0;
      union
      {
            struct
            {
                  unsigned time;
                  unsigned date;
            } msc_time;
            struct ftime bc_time;
      } FTIME;

      if (0 == (retval = DOS_GETFTIME(handle, (unsigned short *)&FTIME.msc_time.date,
            (unsigned short *)&FTIME.msc_time.time)))
      {
            *ftimep = FTIME.bc_time;
      }
      return retval;
}

/*-------------------------------------------------------------------------*/
/* _chmod - set file date                                                  */
/*-------------------------------------------------------------------------*/

int _cdecl setftime (int handle, struct ftime *ftimep)
{
      union
      {
            struct
            {
                  unsigned time;
                  unsigned date;
            } msc_time;
            struct ftime bc_time;
      } FTIME;

      FTIME.bc_time = *ftimep;

      return DOS_SETFTIME(handle, FTIME.msc_time.date, FTIME.msc_time.time);
}
#pragma pack()
#endif /* __WATCOMC__  */

#ifndef INLINE
/*-------------------------------------------------------------------------*/
/* Extracts the different parts of a path name            		   */
/*-------------------------------------------------------------------------*/

void SplitPath(const char* path, char* drive, char* dir, char* fname, char* ext)
{
    fnsplit(path, drive, dir, fname, ext);
}

/*-------------------------------------------------------------------------*/
/* Prints an error message			           		   */
/*-------------------------------------------------------------------------*/

#ifdef USE_KITTEN
void error(int a, int b, const char *error)
#else
void error(const char *error)
#endif
{
#ifdef USE_KITTEN
    fprintf(stderr, " [%s]\n", kittengets(a,b,error));
#else
    fprintf(stderr, " [%s]\n", error);
#endif

} /* end error. */
#endif /* INLINE */

/*-------------------------------------------------------------------------*/
/* Works like function strcpy() but stops copying characters into          */
/* destination when the specified maximum number of characters (including  */
/* the terminating null character) is reached to prevent bounds violation. */
/*-------------------------------------------------------------------------*/

char *strmcpy(char *dest, const char *src, const unsigned int maxlen)
{
    unsigned int i,tmp_maxlen;

    tmp_maxlen=maxlen-1;
    i=0;
    while (src[i] != '\0' && i<tmp_maxlen)
        {
        dest[i]=src[i];
        i=i+1;
        } /* end while. */

    dest[i]='\0';
    return dest;

} /* end strmcpy. */

/*-------------------------------------------------------------------------*/
/* Works like function strcat() but stops copying characters into          */
/* destination when the specified maximum number of characters (including  */
/* the terminating null character) is reached to prevent bounds violation. */
/*-------------------------------------------------------------------------*/

char *strmcat(char *dest, const char *src, const unsigned int maxlen)
{
    unsigned int i,tmp_maxlen;
    char *src_ptr;

    tmp_maxlen=maxlen-1;
    src_ptr=(char *)src;
    i=strlen(dest);
    while (*src_ptr != '\0' && i<tmp_maxlen)
        {
        dest[i]=*src_ptr;
        src_ptr=src_ptr+1;
        i=i+1;
        } /* end while. */

    dest[i]='\0';
    return dest;

} /* end strmcat. */

/*-------------------------------------------------------------------------*/
/* Appends a trailing directory separator to the path, but only if it is   */
/* missing.                                                                */
/*-------------------------------------------------------------------------*/

char *addsep(char *path)
{
    int path_length;

    path_length=strlen(path);
    if (path[path_length-1] != *DIR_SEPARATOR)
    {
	path[path_length]=*DIR_SEPARATOR;
        path[path_length+1]='\0';
    } /* end if. */

    return path;

} /* end addsep. */

/*-------------------------------------------------------------------------*/
/* Checks if the specified path is valid. The pathname may contain a       */
/* trailing directory separator.                                           */
/*-------------------------------------------------------------------------*/

int dir_exists(const char *path)
{
    char tmp_path[MAXPATH],i;
    int attrib;

    strmcpy(tmp_path, path, sizeof(tmp_path));
    i=strlen(tmp_path);
    if (i<3)
    {
	strmcat(tmp_path, DIR_SEPARATOR, sizeof(tmp_path));
    } /* end if. */
    else if (i>3)
    {
	i=i-1;
	if (tmp_path[i] == *DIR_SEPARATOR)
	{
	    tmp_path[i]='\0';
        } /* end if. */

    } /* end else. */

    attrib=_chmod(tmp_path, 0);
    if (attrib == -1 || (attrib & FA_DIREC) == 0)
    {
	return 0;
    } /* end if. */

    return -1;

} /* end dir_exists. */

/*--------------------------------------------------------------------------*/
/* Builds a complete path 	                           		    */
/*--------------------------------------------------------------------------*/

int makedir(char *path)
{
    char tmp_path1[MAXPATH],tmp_path2[MAXPATH],i,length,mkdir_error;

    if (path[0] == '\0')
    {
	return -1;
    } /* end if. */

    strmcpy(tmp_path1, path, sizeof(tmp_path1));
    addsep(tmp_path1);
    length=strlen(tmp_path1);
    strncpy(tmp_path2, tmp_path1, 3);
    i=3;
    while (i<length)
    {
	if (tmp_path1[i]=='\\')
	{
	    tmp_path2[i]='\0';
	    if (!dir_exists(tmp_path2))
	    {
		mkdir_error=mkdir(tmp_path2);
		if (mkdir_error)
		{
		    path[i]='\0';
                    return mkdir_error;
                } /* end if. */

            } /* end if. */

        } /* end if. */

        tmp_path2[i]=tmp_path1[i];
        i++;
    } /* end while. */

    return 0;

} /* end makedir. */

/*-----------------------------------------------------------------------*/
/* Auxiliar function for build_filename                                  */
/*-----------------------------------------------------------------------*/

static void build_name(char *dest, const char *src, const char *pattern)
{
    int i,pattern_i,src_length,pattern_length;

    src_length=strlen(src);
    pattern_length=strlen(pattern);
    i=0;
    pattern_i=0;
    while ((i<src_length || (pattern[pattern_i] != '\0' &&
	   pattern[pattern_i] != '?' && pattern[pattern_i] != '*')) &&
	   pattern_i<pattern_length)
    {
        switch (pattern[pattern_i])
	{
	    case '*':
                dest[i]=src[i];
                break;
            case '?':
                dest[i]=src[i];
                pattern_i++;
                break;
            default:
                dest[i]=pattern[pattern_i];
                pattern_i++;
                break;

        } /* end switch. */

        i++;
    } /* end while. */

    dest[i]='\0';

} /* end build_name. */

/*-------------------------------------------------------------------------*/
/* Uses the source filename and the filepattern (which may contain the     */
/* wildcards '?' and '*' in any possible combination) to create a new      */
/* filename. The source filename may contain a pathname.                   */
/*-------------------------------------------------------------------------*/

void build_filename(char *dest_filename,const char *src_filename,const char
*filepattern)
{
    char drive[MAXDRIVE], dir[MAXDIR], filename_file[MAXFILE],
         filename_ext[MAXEXT], filepattern_file[MAXFILE],
         filepattern_ext[MAXEXT], tmp_filename[MAXFILE], tmp_ext[MAXEXT];

    SplitPath(src_filename, drive, dir, filename_file, filename_ext);
    SplitPath(filepattern, drive, dir, filepattern_file, filepattern_ext);
    build_name(tmp_filename, filename_file, filepattern_file);
    build_name(tmp_ext, filename_ext, filepattern_ext);
    strmcpy(dest_filename, drive, MAXPATH);
    strmcat(dest_filename, dir, MAXPATH);
    strmcat(dest_filename, tmp_filename, MAXPATH);
    strmcat(dest_filename, tmp_ext, MAXPATH);

} /* end build_filename. */


/*-------------------------------------------------------------------------*/
/* Copies the source into the destination file including file attributes   */
/* and timestamp.                                                          */
/*-------------------------------------------------------------------------*/
int copy_file(const char *src_filename,
               const char *dest_filename)
{
  FILE *src_file,
       *dest_file;
  static char buffer[16384];
  unsigned int buffersize;
  int readsize, fileattrib;
  struct ftime filetime;


  /* open source file */
  src_file = fopen(src_filename, "rb");
  if (src_file == NULL)
  {
#ifdef USE_KITTEN
    error(1,25,"Cannot open source file");
#else
    error("Cannot open source file");
#endif
    return 0;
  }

  /* open destination file */
  dest_file = fopen(dest_filename, "wb");
  if (dest_file == NULL)
  {
#ifdef USE_KITTEN
    error(1,26,"Cannot create destination file");
#else
    error("Cannot create destination file");
#endif
    fclose(src_file);
    return 0;
  }

  /* copy file data */
  buffersize = sizeof(buffer);
  readsize = fread(buffer, sizeof(char), buffersize, src_file);
  while (readsize > 0)
  {
    if (fwrite(buffer, sizeof(char), readsize, dest_file) != readsize)
    {
#ifdef USE_KITTEN
      error(1,27,"Write error on destination file");
#else
      error("Write error on destination file");
#endif
      fclose(src_file);
      fclose(dest_file);
      return 0;
    }
    readsize = fread(buffer, sizeof(char), buffersize, src_file);
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

  return 1;
}
