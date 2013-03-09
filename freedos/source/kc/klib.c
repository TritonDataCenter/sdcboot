/*////////// BASE Defs AND INCLUDES ///////////////////////////////////*/

#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<ctype.h>
#include<errno.h>
#include<conio.h>

#define  TRUE  (1==1)
#define  FALSE (1==0)

typedef unsigned char BYTE;
typedef unsigned short int WORD;
typedef unsigned long DWORD;
typedef BYTE BOOL;


/*////////// C O N S T A N T S  ///////////////////////////////////////*/


#define Version 0x0100  /* version 1.0  of the produced files*/
#define LibVerMax 1	/* Version of the generated and understood */
#define LibVerMin 0     /*    library files: 1.0 */
#define KLVerMax  1	/* Version of the admitted KL files */
#define KLVerMin	1

#define MaxBuffer  16384

	  /* Syntax errors */
#define  E_UnknownSwitch       1
#define  E_SyntaxError	       2
#define  E_WrongNParams        3
#define  E_TooManyKLfiles      4
#define  E_TooManyLibs         5
#define  E_MissingLib          6
#define  E_KLFileNotFound	 7
#define  E_LibFileDamaged      8
#define  E_BAKError		 9
#define  E_LibFileError		10
#define  E_InvalidChecksum	11
#define  E_ErrorReadLib		12
#define  E_RemoveNoLib        13
#define  E_KLinLibCorrupt	14
#define  E_Denied			15
#define  E_AlreadyAdded		16
#define  E_KLfileDamaged	17
#define  E_BakRemoveError     18

/*////////// T Y P E S  ///////////////////////////////////////////////*/

enum actions { listLib, addKL, removeKL };

/*////////// V A R I A B L E S  ////////////////////////////////////*/

BOOL	  closelib     = FALSE;
BOOL    libexists    = FALSE;
BOOL	  bakexists    = FALSE;
BOOL    removeBAK    = FALSE;
BOOL	  fixchecksum  = FALSE;
BOOL	  libwasclosed = FALSE;

enum    actions   action=listLib;

char    klfilename[255] = "";
char    libname[255] = "";
char	  bakfilename[255] = "";
FILE	  *klfile, *libfile, *bakfile;

BYTE    buffer[MaxBuffer];
char	  bufferName[13];
WORD	  buflen;
BYTE	  lchecksum = 0;

WORD	  total = 0;

/*////////// F U N C T I O N S  ////////////////////////////////////*/

/* Displays an error on current line, pos lineptr */
void  Error  ( BYTE errorType, char *ExtraLine )
{
	BYTE i;

	printf ("Error %u: ",errorType);
	switch (errorType)
	{
		case E_UnknownSwitch:
			    printf ("Unknown switch: ");
			    break;
		case E_SyntaxError:
			    printf ("Syntax error: ");
			    break;
		case E_WrongNParams:
			    printf ("Wrong number of parameters specified");
			    break;
		case E_TooManyKLfiles:
			    printf ("Too many actions (one action at a time)");
			    break;
		case E_TooManyLibs:
			    printf ("Too many libraries were specified");
			    break;
		case E_MissingLib:
			    printf ("Missing required library name");
			    break;
		case E_KLFileNotFound:
			    printf ("KL file to be added was not found");
			    break;
		case E_LibFileDamaged:
			    printf ("Incorrect version, or library file is damaged or has errors");
			    break;
		case E_BAKError:
			    printf ("Error creating temporary or BAK file");
			    break;
		case E_LibFileError:
			    printf ("Error when opening the library file");
			    break;
		case E_InvalidChecksum:
			    printf ("Invalid checksum for file:");
			    break;
		case E_ErrorReadLib:
			    printf ("Error when reading from library file");
			    break;
		case E_RemoveNoLib:
			    printf ("Library file not found. Can't remove layouts");
			    break;
		case E_KLinLibCorrupt:
			    printf ("Layout in library corrupt: ");
			    break;
		case E_Denied:
			    printf ("Permission denied: library is marked readonly");
			    break;
		case E_AlreadyAdded:
			    printf ("Such layout file has been already added to library");
			    break;
		case E_KLfileDamaged:
			    printf ("Incorrect version of KL file, or it is damaged or has errors");
			    break;
		case E_BakRemoveError:
			    printf ("All operations ok, but couldn't remove BAK file");
			    break;



		default:  puts ("Undefined error");
	}

	puts (ExtraLine);
	fcloseall();

	exit (errorType);
}



void ShowHelp ( void )
{
	puts ("KLIB  lib_filename [{+|-}KL_filename] [/B] [/Z] [/M=manufacturer] [/D=Desc]");
	puts ("KLIB  /?\n");
	puts ("FreeDOS KEY librarian.");
	puts ("Manipulates KL file libraries.\n");
	puts ("lib_filename    name of the library file to be manipulated");
	puts ("+KL_filename    adds the specified KL file to the library");
	puts ("-KL_filename    removes the specified KL file from the library");
	puts ("/B              (no backup) avoids creating backup files");
	puts ("/Z              closes the library, making it read-only");
	puts ("/M=Manufacturer Sets the manufacturer name (creation only)");
	puts ("/D=Desc         Sets the description (creation only)");


}


/* Puts a BUFFER into a file */
void fputbuffer (BYTE *buf, WORD bufsize, FILE *f)
{
	WORD j = 0;

	for (j=0; j<bufsize; j++)
		  fputc (buf[j], f);
}

/* Puts a WORD to a file */
void fputw (WORD w, FILE *f)
{
	fputc ( w & 0xFF, f);     /* big endian */
	fputc ( w >> 8, f);
}

/* Notes: if feof reached, 0 is returned (safe for our purposes) */
WORD  fgetw (FILE *f)
{
	WORD lo=fgetc(f), hi=0;

	if ((lo== (WORD) EOF) | feof(f)) return 0;
	hi =fgetc(f);
	return ((WORD) lo)  | (hi << 8);
}

/* buflen MUST BE already set!!! */
void ReadKLBuffer ( FILE *fromf, BOOL readonly )
{
	WORD	i=0;
	BYTE	checksum=0;

	memset (bufferName,0,13);
	if (!readonly)
	{
		if (fread ( bufferName,1,13, fromf) < 13)
			Error (E_ErrorReadLib, "");
		strupr (bufferName);
	}

	if ( fread(buffer, 1, buflen, fromf) < buflen )
		Error (E_ErrorReadLib, "");

	if feof (fromf)
		Error (E_ErrorReadLib, "");
	lchecksum = fgetc (fromf);

	for (i=0; i<buflen; i++)
		checksum += buffer[i];

	if (buflen < (1+buffer[0]+24) )		/* simple decent length test */
		Error (E_KLinLibCorrupt, bufferName);

	if (checksum != lchecksum)
	{
		if ( readonly || !fixchecksum )
			Error ( E_InvalidChecksum, bufferName );

		printf ("WARNING: Fixing checksum for file %s\n",bufferName);
		lchecksum = checksum;
	}
}

void ReadBufferFromKL ( void )
{
	int 	w=0;
	WORD 	act;

	/* BufferName */
	for (w=strlen(klfilename)-1; (w>=0) && (klfilename[w]!='\\'); w--);
	memset (bufferName,0,13);
	strncpy (bufferName,klfilename+w+1,13);

	/* buflen and buffer */
	buflen=0;
	do
	{
		act = fread (buffer+buflen,1,512,klfile);
		buflen += act;
	}
	while (act==512);

	lchecksum = 0;
	for (w=0; w<buflen; w++)
		lchecksum += buffer[w];
}


void WriteKLBufferToLib ( void )
{
	fputw (buflen,libfile);
	if (!closelib) fwrite (bufferName,1,13,libfile);
	fwrite (buffer,1,buflen,libfile);
	fputc (lchecksum,libfile);
}


BOOL KLFileNameFound ( void )
{
	return ( !strcmp(klfilename,bufferName));
}


void PrintBufferDescription ( void )
{
	char 		NameString[255],s[8];
	BYTE            l=0,n=0;
	WORD		i=0,w=0;

	/* Layout file name and size */
	printf ("%-12s%5u ",bufferName, buflen);

	/* Layout names */
	strncpy (NameString,buffer,buffer[0]);
	NameString [ buffer[0] ] = 0;

	n = buffer [i++];

	do
	{
		w = buffer[i] | ( ((WORD) buffer[i+1]) <<8 );
		i += 2;
		while ((buffer[i]!=',') && (i<=n))
		{
			putch (buffer[i++]);
			l++;
		}
		i++;
		if (l>39) break;
		if (w)
		{
			sprintf (s, "(%u)",w);
			printf (s);
			l += strlen (s);
		}
		if (l>39) break;
		if (i<=n)
		{
			putch (',');
			l++;
		}
	}
	while ((i<=n) && (l<=39));

	/* Pad with spaces */
	if (l<40)
	for (; l<40; l++) putch (32);

	/* Number of submappings: general+particulars */
	i = 29+buffer[0];			/* points to first particular submapping*/
	for ( n=buffer[1+buffer[0]]-1 ; n>0; n--)
	{
		printf ("%u",buffer[i] | (((WORD) buffer[i+1])<<8));
		if (n>1) putch(',');
		i += 8;
	}

	puts ("");

}


/* Simple Listing: read data from library file and exit */
void SimpleListing ( BOOL omitFilenames )
{
	WORD i,l;
	char s[255]="", *t;
	char c255[]="ÿ";				/* character 255 */

	/* Header */
	if feof (libfile)
		Error (E_ErrorReadLib, "");

	printf ("\nListing of %s:\n\n",libname);

	if feof (libfile)
		Error (E_ErrorReadLib, "");
	l = fgetc (libfile);

	if ( fgets (s,l+1,libfile) == NULL)
		Error (E_ErrorReadLib, "");

	t = strtok (s,c255);
	t = strtok (NULL,c255);

	printf ("Author: %s\n",s);
	printf ("Description: %s\n",t);

	if (omitFilenames) puts ("File is closed");

	puts ("\nFilename     Size Layout names                            Codepages");
	memset (s, '=', 78);
	s[79]=0;
	puts ( s );

	/* Items */
	if ( feof (libfile) )
		Error (E_ErrorReadLib, "");
	buflen = fgetw (libfile);
	while (buflen>0)
	{
		ReadKLBuffer ( libfile, omitFilenames );
		PrintBufferDescription ();
		total++;

		/* Read next length */
		if ( feof (libfile) )
			Error (E_ErrorReadLib, "");
		buflen = fgetw (libfile);

	}
	puts (s);
	printf ("     %u layout(s) found\n\n",total);

}



/*/////////// MAIN BEGINS HERE  ///////////////////////////////////////*/


void main ( BYTE argc, char *argv[] )
{

	int		i=0;
	char		buf[5],
			KCsignature[5] = {'K','C','F',LibVerMin,LibVerMax},
			KLsignature[5] = {'K','L','F',KLVerMin,KLVerMax};
	BYTE		b=0;
	char		s1[255]="", s2[255]="";


/*/////////// PART 1: check parameters ////////////////////////////////*/

	puts ("KEY file Librarian version 1.0 [2005/08/10]");
	puts ("Copyright (c) 2004-2005 by Aitor Santamaria_Merino (under GPL)");

	if ( argc<2 )
		Error (E_WrongNParams, "");

	for (i=1; i<argc; i++)
	{
		if (argv[i][0]=='/')
		{
			if (  (argv[i][1]=='D') || (argv[i][1]=='M') )
			{
				if (argv[i][2]!='=')
					Error (E_SyntaxError, argv[i]);
				switch ( toupper (argv[i][1]))
				{
					case 'D':  strcpy (s2, argv[i]+3);
						     break;
					case 'M':  strcpy (s1, argv[i]+3);
						     break;
				}
				continue;
			}
			if (strlen(argv[i])>2)
				Error (E_SyntaxError, argv[i]);
			switch ( toupper (argv[i][1]))
			{
				case '?':  ShowHelp ();
					     return;
				case 'B':  removeBAK = TRUE;
					     break;
				case 'Z':  closelib = TRUE;
					     break;
				case 'F':  fixchecksum = TRUE;
					     break;
				default:   Error (E_UnknownSwitch, argv[i]);
			}
		}
		else if ((argv[i][0]=='+') || (argv[i][0]=='-'))
		{
			if (action != listLib)  Error (E_TooManyKLfiles,"");

			if (argv[i][0]=='+') action = addKL;
				else		   action = removeKL;
			strncpy (klfilename,argv[i]+1,254);
			strupr (klfilename);
		}
		else
		{
			if (libname[0]) Error (E_TooManyLibs,"");
			strncpy (libname,argv[i],254);
		}
	}

	if (!libname[0])
		Error (E_MissingLib, "");


/*/////////// PART 2: get extensions and source KL (if add) /////////////*/

	/* Check presence of extension */
	for ( i = strlen(libname)-1; (i>=0) && (libname[i]!='\\') && (libname[i]!='.'); i--);

	if (! ((i>=0) && (libname[i]=='.')) )
		strcat (libname, ".KBD");

	for ( i = strlen(klfilename)-1; (i>=0) && (klfilename[i]!='\\') && (klfilename[i]!='.'); i--);

	if (! ((i>=0) && (klfilename[i]=='.')) )
		strcat (klfilename, ".KL");

	/* Try to open the source KL if we are to ADD */
	if (action==addKL)
	{
		if ( (klfile = fopen (klfilename, "rb")) == NULL )
			Error ( E_KLFileNotFound, "");
		if ( fread (buf, 1, 5, klfile) != 5 )
			Error ( E_KLfileDamaged, "");
		buf[5]=0;
		if ( memcmp (buf,KLsignature, 5) )
			Error ( E_KLfileDamaged, "");
	}


/*/////////////////// PART 3: check the library file //////////////////*/

	/* Check if library file exists: check simple listing and branch */
	/* validity: if it does not exist, and if removeKL, ERROR */

	libexists = (libfile = fopen (libname, "rb")) != NULL;

	if ( libexists )		   /* Test the first 5 bytes of the file */
	{
		if ( fread (buf, 1, 5, libfile) != 5 )
			Error ( E_LibFileDamaged, "");

		if ( memcmp (buf,KCsignature, 5) )
			Error ( E_LibFileDamaged, "");

		libwasclosed = fgetc (libfile) & 1;

		if (libwasclosed && ( (action==addKL) || (action==removeKL) ) )
			Error (E_Denied,"");

		if ( (action == listLib) && !closelib && !fixchecksum )
		{
			SimpleListing (libwasclosed);
			exit (0);
		}
	}
	else
		if (action == removeKL)
			Error ( E_RemoveNoLib, "");

	/* Check if backup file exist, and remove it */

	strncpy ( bakfilename, libname, 255 );
	for ( i = strlen(bakfilename)-1; (i>=0) && (bakfilename[i]!='.'); i--);
	bakfilename[i+1] = 0;
	strcat (bakfilename, "BAK");

	bakexists = (( bakfile = fopen (bakfilename, "rb")) != NULL) ;

	if ( !bakexists && (errno != ENOFILE) )
		Error ( E_BAKError, "1");

	if (bakexists)
	{
		fclose (bakfile);
		if ( remove (bakfilename) && (errno==EACCES) )
			Error ( E_BAKError, "2");
	}

	/* Prepare the library file */

	if ( !libexists  )
		puts ("Creating new library file . . . ");
	else
	{
		fclose (libfile);

		/* Library file becomes backup file */

		if ( rename ( libname, bakfilename ) )
			Error ( E_BAKError, "3");

		/* Open the BAK file for read, and skip the signature */

		libexists = ( ( bakfile = fopen (bakfilename, "rb") ) != NULL );

		if ( !libexists )
			Error ( E_BAKError, "4");

		if ( fread (buf, 1, 5, bakfile) != 5 )
		{
			rename (bakfilename, libname);
			Error ( E_BAKError, "5");
		}

		fgetc(bakfile);			/* skip the signature */
	}

/*/////////////////// PART 4: library file header /////////////////////*/

	/* Open */
	if ( (libfile = fopen (libname, "wb")) == NULL )
		Error ( E_LibFileError, "");

	/* Signature and version */
	fputbuffer (KCsignature ,3, libfile);         /* signature */
	fputw (Version, libfile);        		    /* version */

	/* Flags */
	closelib |= libwasclosed;
	fputc (closelib, libfile);

	/* Manufacturer and description */
	if ( libexists )
	{
		fputc ( b= fgetc (bakfile), libfile);
		for (i=0; i<b; i++) fputc ( fgetc(bakfile), libfile );
	}
	else
	{
		if (!s1[0])
		{
			printf ("Enter the manufacturer name: ");
			gets (s1);
		}
		else
			printf ("The manufacturer is %s\n",s1);
		if (!s2[0])
		{
			printf ("Enter a brief description: ");
			gets (s2);
		}
		else
			printf ("The description is %s\n",s1);
		s1 [strlen(s1)+1]=0;
		s1 [strlen(s1)]=255;
		strcat (s1,s2);

		fputc ( strlen (s1), libfile );
		fputs ( s1, libfile);
	}

/*/////////////////// PART 5: do the operation /////////////////////*/

	if ( bakexists && feof (bakfile) )
		Error (E_ErrorReadLib, "");

	buflen = bakexists ? fgetw (bakfile) : 0;

	while (buflen>0)
	{

		ReadKLBuffer ( bakfile, libwasclosed );
		total++;


		if (! ( KLFileNameFound() && (action==removeKL)) )
		{
			switch (action)
			{
				addKL:	if (KLFileNameFound()) Error (E_AlreadyAdded,"");
						break;
				listLib:	PrintBufferDescription ();
			}

			WriteKLBufferToLib ();
		}

		/* Read next length */
		if ( feof (bakfile) )
			Error (E_ErrorReadLib, "");
		buflen = fgetw (bakfile);
	}

	if (action==addKL)
	{
		ReadBufferFromKL ();
		WriteKLBufferToLib ();
	}

	fputw (0, libfile);

	fcloseall ();

/*/////////////////// PART 7: Goodbye!!  ////////////////////////*/

	if (removeBAK)
	if ( remove (bakfilename) && (errno==EACCES) )
		Error ( E_BAKError, "6");

	printf ("Library %s succesfully updated (0 Errors)\n", libname);

}

