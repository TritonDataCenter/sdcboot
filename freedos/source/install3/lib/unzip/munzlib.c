/* this is based on miniunz.c, Kenneth J. Davis <jeremyd@computer.org> */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <fcntl.h>

#ifdef unix
# include <unistd.h>
# include <utime.h>
#else
#if defined (__TURBOC__) || defined (_MSV_VER) || defined (__WATCOMC__)
#include <utime.h>
#include <time.h>
#endif
# include <direct.h>
# include <io.h>
#endif

#include "unzip.h"

#include "munzlib.h"

#define CASESENSITIVITY (0)
/* This effects speed & how often update function called, original 8192, must be <= MAX_SINT */
#define WRITEBUFFERSIZE (4096)   
#define MAXFILENAME (256)

#ifdef WIN32
#define USEWIN32IOAPI
#include "iowin32.h"
#endif


/* change_file_date : change the date/time of a file
    filename : the filename of the file where date/time must be modified
    dosdate : the new date at the MSDos format (4 bytes)
    tmu_date : the SAME new date at the tm_unz format */
void change_file_date(const char *filename,uLong dosdate,tm_unz tmu_date)
{
#ifdef WIN32
  HANDLE hFile;
  FILETIME ftm,ftLocal,ftCreate,ftLastAcc,ftLastWrite;

  hFile = CreateFile(filename,GENERIC_READ | GENERIC_WRITE,
                      0,NULL,OPEN_EXISTING,0,NULL);
  GetFileTime(hFile,&ftCreate,&ftLastAcc,&ftLastWrite);
  DosDateTimeToFileTime((WORD)(dosdate>>16),(WORD)dosdate,&ftLocal);
  LocalFileTimeToFileTime(&ftLocal,&ftm);
  SetFileTime(hFile,&ftm,&ftLastAcc,&ftm);
  CloseHandle(hFile);
#elif defined (unix) || defined (__TURBOC__) || defined (_MSV_VER) || defined (__WATCOMC__)
  struct utimbuf ut;
  struct tm newdate;
  newdate.tm_sec = tmu_date.tm_sec;
  newdate.tm_min=tmu_date.tm_min;
  newdate.tm_hour=tmu_date.tm_hour;
  newdate.tm_mday=tmu_date.tm_mday;
  newdate.tm_mon=tmu_date.tm_mon;
  if (tmu_date.tm_year > 1900)
      newdate.tm_year=tmu_date.tm_year - 1900;
  else
      newdate.tm_year=tmu_date.tm_year ;
  newdate.tm_isdst=-1;

  ut.actime=ut.modtime=mktime(&newdate);
  utime(filename,&ut);
#else
#error No implementation defined to set file date
#endif
}


/* mymkdir and change_file_date are not 100 % portable
   As I don't know well Unix, I wait feedback for the unix portion */

int mymkdir(const char *dirname)
{
    int ret=0;
#if defined WIN32 || defined __TURBOC__ || defined __WATCOMC__
	ret = mkdir(dirname);
#elif defined unix
	ret = mkdir (dirname, S_IWUSR /*0775*/);
#else
#error No implementation for mymkdir defined!
#endif
	return ret;
}

#if 1
int makedir (const char *path, int (* printErr)(const char *errmsg, ...) )
{
	char *complete;
	char *temp;
      int len;

	complete = malloc(strlen(path) + 2);
	strcpy(complete, path);

	/* normalize directory separators */
	temp = complete;
	while ((temp = strchr(temp, '\\')) != NULL)
		*temp = '/';

      /* assume any path given is a directory so add '/' to end if missing */
      len = strlen(complete);
	if (complete[len-1] != '/') 
	{
		complete[len] = '/';
		complete[len+1] = '\0';
	}

	/* don't try to create root dir */
	if (*complete == '/')
		temp = complete + 1;       /* point to 1st char after root    */
	else if (*(complete+1) == ':')
	{
		if (*(complete+2) == '/')
			temp = complete + 3; /* point to after drive & root     */
		else
			temp = complete + 2; /* point to after drive (relative) */
	}
	else
		temp = complete;           /* point to start of relative path */

	/* make each segment of path (slash '/' separated) */
	while ((temp = strchr(temp, '/')) != NULL)
	{
		*temp = '\0';
		if ((mymkdir(complete) == -1) && (errno == ENOENT))
		{
			printErr("couldn't create directory %s\n",complete);
			free(complete);
			return 0;
		}
		*temp = '/';
		temp++;
	}

	free(complete);
	return 1;
}
#else
int makedir (const char *newdir, int (* printErr)(const char *errmsg, ...) )
{
  char *buffer ;
  char *p;
  int  len = strlen(newdir);  

  if (len <= 0) 
    return 0;

  buffer = (char*)malloc(len+1);
  strcpy(buffer,newdir);
  
  if (buffer[len-1] == '/') {
    buffer[len-1] = '\0';
  }
  if (mymkdir(buffer) == 0)
    {
      free(buffer);
      return 1;
    }

  p = buffer+1;
  while (1)
    {
      char hold;

      while(*p && *p != '\\' && *p != '/')
        p++;
      hold = *p;
      *p = 0;
      if ((mymkdir(buffer) == -1) && (errno == ENOENT))
        {
          printErr("couldn't create directory %s\n",buffer);
          free(buffer);
          return 0;
        }
      if (hold == 0)
        break;
      *p++ = hold;
    }
  free(buffer);
  return 1;
}
#endif


#if 0
int do_list(uf)
	unzFile uf;
{
	uLong i;
	unz_global_info gi;
	int err;

	err = unzGetGlobalInfo (uf,&gi);
	if (err!=UNZ_OK)
		printf("error %d with zipfile in unzGetGlobalInfo \n",err);
    printf(" Length  Method   Size  Ratio   Date    Time   CRC-32     Name\n");
    printf(" ------  ------   ----  -----   ----    ----   ------     ----\n");
	for (i=0;i<gi.number_entry;i++)
	{
		char filename_inzip[256];
		unz_file_info file_info;
		uLong ratio=0;
		const char *string_method;
		err = unzGetCurrentFileInfo(uf,&file_info,filename_inzip,sizeof(filename_inzip),NULL,0,NULL,0);
		if (err!=UNZ_OK)
		{
			printf("error %d with zipfile in unzGetCurrentFileInfo\n",err);
			break;
		}
		if (file_info.uncompressed_size>0)
			ratio = (file_info.compressed_size*100)/file_info.uncompressed_size;

		if (file_info.compression_method==0)
			string_method="Stored";
		else
		if (file_info.compression_method==Z_DEFLATED)
		{
			uInt iLevel=(uInt)((file_info.flag & 0x6)/2);
			if (iLevel==0)
			  string_method="Defl:N";
			else if (iLevel==1)
			  string_method="Defl:X";
			else if ((iLevel==2) || (iLevel==3))
			  string_method="Defl:F"; /* 2:fast , 3 : extra fast*/
		}
		else
			string_method="Unkn. ";

		printf("%7lu  %6s %7lu %3lu%%  %2.2lu-%2.2lu-%2.2lu  %2.2lu:%2.2lu  %8.8lx   %s\n",
			    file_info.uncompressed_size,string_method,file_info.compressed_size,
				ratio,
				(uLong)file_info.tmu_date.tm_mon + 1,
                (uLong)file_info.tmu_date.tm_mday,
				(uLong)file_info.tmu_date.tm_year % 100,
				(uLong)file_info.tmu_date.tm_hour,(uLong)file_info.tmu_date.tm_min,
				(uLong)file_info.crc,filename_inzip);
		if ((i+1)<gi.number_entry)
		{
			err = unzGoToNextFile(uf);
			if (err!=UNZ_OK)
			{
				printf("error %d with zipfile in unzGoToNextFile\n",err);
				break;
			}
		}
	}

	return 0;
}
#endif


int do_extract_currentfile(
	unzFile uf,
	const int* popt_extract_without_path,
    int* popt_overwrite,
    int (* printErr)(const char *errmsg, ...), /* called if there is an error (eg printf) */
    int (* fileexistfunc)(void * extra, const char *filename), /* called to determine overwrite or not */
    /* if nonzero then clean up and return an error (user_interrupted) */
    int (* statusfunc)(void * extra, int bytesRead, char * msg, ...),
    void *extra  /* no used, simply passed to statusfunc */
)
{
	char filename_inzip[256];
	char* filename_withoutpath;
	char* p;
	int err=UNZ_OK;
	FILE *fout=NULL;
	void* buf;
	uInt size_buf;
	
	unz_file_info file_info;
	err = unzGetCurrentFileInfo(uf,&file_info,filename_inzip,sizeof(filename_inzip),NULL,0,NULL,0);

	if (err!=UNZ_OK)
	{
		printErr("error %d with zipfile in unzGetCurrentFileInfo\n",err);
		return err;
	}

	size_buf = WRITEBUFFERSIZE;
	buf = (void*)malloc(size_buf);
	if (buf==NULL)
	{
		printErr("Error allocating memory\n");
		return UNZ_INTERNALERROR;
	}

	p = filename_withoutpath = filename_inzip;
	while ((*p) != '\0')
	{
		if (((*p)=='/') || ((*p)=='\\'))
			filename_withoutpath = p+1;
		p++;
	}

	if ((*filename_withoutpath)=='\0')
	{
		if ((*popt_extract_without_path)==0)
		{
			statusfunc(extra, 0, "creating directory: %s\n",filename_inzip);
			makedir(filename_inzip, printErr);
		}
	}
	else
	{
		const char* write_filename;
		int skip=0;

		if ((*popt_extract_without_path)==0)
			write_filename = filename_inzip;
		else
			write_filename = filename_withoutpath;

		err = unzOpenCurrentFile(uf);
		if (err!=UNZ_OK)
		{
			printErr("error %d with zipfile in unzOpenCurrentFile\n",err);
		}

		if (((*popt_overwrite)==0) && (err==UNZ_OK))
		{
			int rep=-1;
			FILE* ftestexist;
			ftestexist = fopen(write_filename,"rb");
			if (ftestexist!=NULL)
			{
				fclose(ftestexist);
				/* called to determine overwrite or not */
                        rep = fileexistfunc(extra, write_filename);  
                        /* 0=[N]o, 1=[Y]es, 2=[A]ll */
			}

			if (rep == 0 /*'N'*/)
				skip = 1;

			if (rep == 2 /*'A'*/)
				*popt_overwrite=1;
		}

		if ((skip==0) && (err==UNZ_OK))
		{
			fout=fopen(write_filename,"wb");

			/* some zipfile don't contain directory alone before file */
			if ((fout==NULL) && ((*popt_extract_without_path)==0) && 
                                (filename_withoutpath!=(char*)filename_inzip))
			{
				char c=*(filename_withoutpath-1);
				*(filename_withoutpath-1)='\0';
  				statusfunc(extra, 0, "Creating directory: %s\n",write_filename);
				makedir(write_filename, printErr);
				*(filename_withoutpath-1)=c;
				fout=fopen(write_filename,"wb");
			}

			if (fout==NULL)
			{
				printErr("error opening [%s]\n",write_filename);
			}
		}

		if (fout!=NULL)
		{
			statusfunc(extra, 0, " extracting: %s\n", write_filename);

			do
			{
				err = unzReadCurrentFile(uf,buf,size_buf);
				if (err<0)	
				{
					printErr("error %d with zipfile in unzReadCurrentFile\n",err);
					break;
				}
				if (err>0)
				{
					if (fwrite(buf,err,1,fout)!=1)
					{
						printErr("error in writing extracted file\n");
						err=UNZ_ERRNO;
						break;
					}
					if (statusfunc(extra, err, NULL))
					{
						/* user has asked interrupt process */
						err=UNZ_USERINTERRUPT;
						break;
					}
				}
			}
			while (err>0);
			if (fout)
			        fclose(fout);

			if (err==0) 
				change_file_date(write_filename,file_info.dosDate,
					             file_info.tmu_date);
			else if (err < 0)
				remove(write_filename);  /* if there was an error, delete last [bad] file written */
		}
            else /* if (fout == NULL) */
			statusfunc(extra, 0, "Skipping file [%s]\n", write_filename);


        if (err==UNZ_OK)
        {
		    err = unzCloseCurrentFile (uf);
		    if (err!=UNZ_OK)
		    {
			    printErr("error %d with zipfile in unzCloseCurrentFile\n",err);
		    }
        }
        else
            unzCloseCurrentFile(uf); /* don't lose the error */       
	}

    free(buf);    
    return err;
}


int do_extract(
	unzFile uf,
	int opt_extract_without_path,
    int opt_overwrite,
    int (* printErr)(const char *errmsg, ...), /* called if there is an error (eg printf) */
    int (* fileexistfunc)(void * extra, const char *filename),  /* called to determine overwrite or not */
    /* if nonzero then clean up and return an error (user_interrupted) */
    int (* statusfunc)(void * extra, int bytesRead, char * msg, ...),
    void *extra  /* no used, simply passed to statusfunc */
)
{
	uLong i;
	unz_global_info gi;
	int err;

	err = unzGetGlobalInfo (uf,&gi);
	if (err!=UNZ_OK)
		printErr("error %d with zipfile in unzGetGlobalInfo \n",err);

	for (i=0;i<gi.number_entry;i++)
	{
        if ((err=do_extract_currentfile(uf,&opt_extract_without_path,
                                      &opt_overwrite, printErr, fileexistfunc, statusfunc, extra
						)) != UNZ_OK)
            break;

		if ((i+1)<gi.number_entry)
		{
			err = unzGoToNextFile(uf);
			if (err!=UNZ_OK)
			{
				if (err != UNZ_USERINTERRUPT)
					printErr("error %d with zipfile in unzGoToNextFile\n",err);
				return (err);
			}
		}
	}

	return err;
}

int do_extract_onefile(
	unzFile uf,
	const char* filename,
	int opt_extract_without_path,
    int opt_overwrite,
    int (* printErr)(const char *errmsg, ...), /* called if there is an error (eg printf) */
    int (* fileexistfunc)(void * extra, const char *filename),  /* called to determine overwrite or not */
    /* if nonzero then clean up and return an error (user_interrupted) */
    int (* statusfunc)(void * extra, int bytesRead, char * msg, ...),
    void *extra  /* no used, simply passed to statusfunc */
)
{
    int err;
    if ((err = unzLocateFile(uf,filename,CASESENSITIVITY))!=UNZ_OK)
    {
        printErr("file %s not found in the zipfile\n",filename);
        return err;
    }

    return (do_extract_currentfile(uf,&opt_extract_without_path, 
			&opt_overwrite, printErr, fileexistfunc, statusfunc, extra));
}


#if 0
int main(argc,argv)
	int argc;
	char *argv[];
{
	const char *zipfilename=NULL;
    const char *filename_to_extract=NULL;
    char filename_try[MAXFILENAME+16] = "";
	int i;
	int opt_do_list=0;
	int opt_do_extract=1;
	int opt_do_extract_withoutpath=0;
	int opt_overwrite=0;
	unzFile uf=NULL;

	do_banner();
	if (argc==1)
	{
		do_help();
		exit(0);
	}
	else
	{
		for (i=1;i<argc;i++)
		{
			if ((*argv[i])=='-')
			{
				const char *p=argv[i]+1;
				
				while ((*p)!='\0')
				{			
					char c=*(p++);;
					if ((c=='l') || (c=='L'))
						opt_do_list = 1;
					if ((c=='v') || (c=='V'))
						opt_do_list = 1;
					if ((c=='x') || (c=='X'))
						opt_do_extract = 1;
					if ((c=='e') || (c=='E'))
						opt_do_extract = opt_do_extract_withoutpath = 1;
					if ((c=='o') || (c=='O'))
						opt_overwrite=1;
				}
			}
			else
            {
				if (zipfilename == NULL)
					zipfilename = argv[i];
                else if (filename_to_extract==NULL)
                        filename_to_extract = argv[i] ;
            }
		}
	}

	if (zipfilename!=NULL)
	{

        #ifdef USEWIN32IOAPI
        zlib_filefunc_def ffunc;
        #endif

        strncpy(filename_try, zipfilename,MAXFILENAME-1);
        /* strncpy doesnt append the trailing NULL, of the string is too long. */
        filename_try[ MAXFILENAME ] = '\0';

        #ifdef USEWIN32IOAPI
        fill_win32_filefunc(&ffunc);
		uf = unzOpen2(zipfilename,&ffunc);
        #else
		uf = unzOpen(zipfilename);
        #endif
		if (uf==NULL)
		{
            strcat(filename_try,".zip");
            #ifdef USEWIN32IOAPI
            uf = unzOpen2(filename_try,&ffunc);
            #else
            uf = unzOpen(filename_try);
            #endif
		}
	}

	if (uf==NULL)
	{
		printf("Cannot open %s or %s.zip\n",zipfilename,zipfilename);
		exit (1);
	}
    printf("%s opened\n",filename_try);

	if (opt_do_list==1)
		return do_list(uf);
	else if (opt_do_extract==1)
    {
        if (filename_to_extract == NULL)
		    return do_extract(uf,opt_do_extract_withoutpath,opt_overwrite);
        else
            return do_extract_onefile(uf,filename_to_extract,
                                      opt_do_extract_withoutpath,opt_overwrite);
    }
	unzCloseCurrentFile(uf);

	return 0;  /* to avoid warning */
}
#endif
