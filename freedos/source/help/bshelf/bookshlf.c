/*////////// BASE Defs AND INCLUDES ///////////////////////////////////*/

#include<stdlib.h>
#include<stdio.h>
#include<string.h>
#include<ctype.h>

#define  TRUE  (1==1)
#define  FALSE (1==0)

typedef unsigned char BYTE;
typedef unsigned short int WORD;
typedef unsigned long DWORD;
typedef BYTE BOOL;


/*////////// C O N S T A N T S  ///////////////////////////////////////*/

#define VerS    "1.00"
#define MaxLine 255

	  /* Critical errors */
#define  SC_WrongNParams         1
#define  SC_CantOpenSource       2
#define  SC_CantOpenTarget	   3
#define  SC_UnknownCommand       4
#define  SC_NoBookhelpFile       5
#define  SC_BooksCorrupted       6
#define  SC_CantOpenBI           7
#define  SC_CantRemoveFin	   8
#define  SC_BookNotFound	   9

/*////////// T Y P E S  ///////////////////////////////////////////////*/

enum commands { CM_Unknown=0, CM_List, CM_Add, CM_Del };

/*////////// V A R I A B L E S  ////////////////////////////////////*/

char   line[MaxLine]  = "";

char   finName[255]="";
char   fbackName[255]="";
char   foutName[255]="";	      /* file names */
char   BookName[255]="";
FILE   *fin, *fout, *fbook;		/* file variables */


enum commands curcommand = CM_Unknown;

/*////////// F U N C T I O N S  ////////////////////////////////////*/


void  CriticalError  ( BYTE errorType, char *ExtraLine)
{
	BYTE i;

	printf ("Critical error %u: ", errorType );
	switch (errorType)
	{
		case SC_WrongNParams:
			    puts ("Wrong number of parameters specified");
			    break;
		case SC_CantOpenSource:
			    printf ("Unable to open source file name:");
			    break;
		case SC_CantOpenTarget:
			    printf ("Unable to open target file name:");
			    break;
		case SC_UnknownCommand:
			    puts ("Unknown command");
			    break;
		case SC_NoBookhelpFile:
			    printf ("Help file does not contain anchor for books: ");
			    break;
		case SC_BooksCorrupted:
			    puts ("Booklist for index file is corrupted");
			    break;
		case SC_CantOpenBI:
			    printf ("Unable to open book index: ");
			    break;
		case SC_CantRemoveFin:
			    printf ("Unable to modify index file: ");
			    break;
		case SC_BookNotFound:
			    printf ("Book not found in index: ");
			    break;
		default:  puts ("Undefined error");
	}

	puts (ExtraLine);

	fcloseall();		/* close the files and */
	remove (foutName);	/* remove temp ones */

	exit (-1);
}


BOOL  IsWhite ( unsigned char c )   /* space, tab,...*/
{
	return ((c==32) || (c==9) || (c==10) || (c==13));
}


WORD  NoSpaces ( void )
{
	BYTE   lineptr    = 0;

	for (;IsWhite(line[lineptr]); lineptr++);
	strcpy (line, line+lineptr);
	for (;(strlen(line)>0) && IsWhite(line[strlen(line)-1]); line[strlen(line)-1]=0);
	strupr (line);
}

void FastHelp ( void )
{
	puts ("BOOKSHLF  LIST  [bookhelpfilename]");
	puts ("BOOKSHLF  ADD   bookname[.BI]  [bookhelpfilename]");
	puts ("BOOKSHLF  DEL   bookname       [bookhelpfilename]\n");
	puts ("Lists, adds and removes book entries from the help file containing");
	puts ("the books, respectively\n");
	exit (0);

}


BOOL GetBookName ( void )
{
	/* starts and ends with the appropriate strings */
	if ( strstr(line,"<!--BOOK=") != line ) return FALSE;
	if ( strcmp (line+strlen(line)-3,"-->") ) return FALSE;

	/* cuts begining and end */
	line [strlen(line)-3] = 0;
	strcpy (line, line+9);

	return TRUE;
}


void List ( void )
{
	int bookN = 0;

	puts ("List of available books:");
	while ( TRUE )
	{
		/* read line removing spaces */
		fgets (line, MaxLine, fin);
		NoSpaces ();

		/* if /BOOKSHELF or => break */
		if (!strcmp(line,"<!--/BOOKSHELF-->")) break;
		if (feof(fin)) CriticalError (SC_BooksCorrupted, "");

		bookN++;

		/* Line must start with <!--BOOK=bookname, and then print it */
		if ( !GetBookName() ) CriticalError (SC_BooksCorrupted, "");
		printf ("    %s\n",line);

		/* Strip until one gets to <!--/BOOK--> */
		do {
			fgets(line, MaxLine, fin);
			NoSpaces();
		} while ( strcmp(line,"<!--/BOOK-->") && !feof(fin) );

		if (feof(fin)) CriticalError (SC_BooksCorrupted, "");

	}
	printf ("Number of books found: %u\n", bookN);
}


void Add ( void )
{
	char tag[MaxLine]= "<!--BOOK=";
	char stout[255] = "";
	BOOL Updates = FALSE;

	/* Set the book header */
	for (; (strlen(BookName)>0) && (BookName[strlen(BookName)-1]!='.');
		 BookName[strlen(BookName)-1]=0);
	BookName[strlen(BookName)-1]=0;

	/* Get the tag name */
	strcat (tag,BookName);
	strcat (tag,"-->");
	strupr (tag);

	/* Pass all earlier */
	while (TRUE) {
		fgets  (line, MaxLine, fin);

		if (feof(fin)) CriticalError (SC_BooksCorrupted, "");
		strcpy (stout, line);
		NoSpaces();

		if (!strcmp(line,"<!--/BOOKSHELF-->")) break;
		if (strcmp(line,tag)>=0) break;

		strcat (line,"\n");
		fputs (line, fout);
		do {
			fgets (line, MaxLine,fin);
			fputs (line, fout);
			NoSpaces();
		} while (strcmp(line,"<!--/BOOK-->"));

	}
	   /* In all exits of this loop through here, stout is pending to be put */

	/* if present, update, that is, ignore previous book */
	if (!strcmp(line,tag))  {
		Updates = TRUE;		/* flag to say that we don't have to */
						/* append the saved line */
		do {
			fgets (line, MaxLine,fin);
			NoSpaces();
		} while (strcmp(line,"<!--/BOOK-->"));
	}

	/* Insert the book */
	fprintf (fout, "%s\n", tag);

	/* Transfer contents */
	while (TRUE) {
		fgets (line, MaxLine, fbook);
		if feof(fbook) break;
		fputs (line, fout);
	}

	/* Set the book end */
	fputs ("<!--/BOOK-->\n", fout);

	if (!Updates) fputs (stout, fout);
}


void Del ( void )
{
	char tag[MaxLine]= "<!--BOOK=";
	char stout[255] = "";

	strcat (tag,BookName);
	strcat (tag,"-->");
	strupr (tag);

	fgets  (line, MaxLine, fin);
	strcpy (stout, line);
	NoSpaces();

	while (strcmp(line,tag))
	{
		if (!strcmp(line,"<!--/BOOKSHELF-->"))
			CriticalError (SC_BookNotFound, BookName);
		if (feof(fin)) CriticalError (SC_BooksCorrupted, "");

		fputs (stout, fout);
		do {
			fgets (line, MaxLine, fin);
			if (feof(fin)) CriticalError (SC_BooksCorrupted, "");
			fputs (line, fout);
			NoSpaces();
		} while (strcmp(line,"<!--/BOOK-->"));

		fgets  (line, MaxLine, fin);
		strcpy (stout, line);
		NoSpaces();

	}

	do {
		fgets (line, MaxLine, fin);
		NoSpaces();
		if (feof(fin)) CriticalError (SC_BooksCorrupted, "");
	} while (strcmp(line,"<!--/BOOK-->"));

}


/*/////////// MAIN BEGINS HERE  ///////////////////////////////////////*/

void main ( BYTE argc, char *argv[] )
{
	int i=2 ;

  /*********** STARTUP AND PARAMETER PARSING **************/

	/* Opening strings */
	printf ("BOOKSHLF %s - FreeDOS Book Shelf manager\n", VerS);
	puts ("Copyright Aitor SANTAMARIA MERINO under the GNU GPL 2.0\n");

	/* Discard FastHelp */

	if ( (argc==1) ||
	     ( (argc==2) && (!strcmp(argv[1],"/?")) ) )
	     FastHelp ();

	/* Determine command (argv[1]) */

	if (!strcmp (strupr(argv[1]),"LIST") )
		curcommand = CM_List;
	else if (!strcmp (argv[1],"ADD"))
		curcommand = CM_Add;
	else if (!strcmp (argv[1],"DEL"))
		curcommand = CM_Del;
	else
		CriticalError (SC_UnknownCommand, "");

	/* Get the book name (ADD/DEL) */
	if ( (curcommand==CM_Add) || (curcommand==CM_Del) )
	{
		if (argc<3) CriticalError (SC_WrongNParams, "");
		strncpy (BookName, argv[i++], MaxLine);
	}

	/* More arguments? */
	if (argc > (i+1)) CriticalError (SC_WrongNParams, "");
	if (argc == (i+1))
		strncpy (finName, argv[i], 255);


  /*********** OPENING FILES **************/

	/*  Try to open the source HTML file */
	i=1;	/* Flag meaning: 2=no finName was specified, 1=was specified */
	if (!finName[0])
	{
		i++;
		strcpy (finName, "..\\HELP\\INDEX.HTM");
//		strcpy (finName, "INDEX.HTM");
// for easy testings
	}

	while (i--)
		if ((fin = fopen (finName,"rt"))!=NULL) i=0;
		else
		   if (i) {
			strcpy (foutName, getenv ("HELPPATH"));
			if (!foutName[0]) CriticalError (SC_CantOpenSource, finName);
			strcat (foutName, "\\INDEX.HTM");
			strcpy (finName, foutName);
		   }
		   else CriticalError (SC_CantOpenSource, finName);


	/* Add extension BI if not present */
	if ( curcommand==CM_Add )
	{
		for ( i = strlen(BookName)-1; (i>=0) && (BookName[i]!='\\') && (BookName[i]!='.'); i--);
		if (! ((i>=0) && (BookName[i]=='.')) )
			strcat (BookName, ".BI");

		/* Open book index */
		if ((fbook = fopen (BookName,"rt"))==NULL)
			CriticalError (SC_CantOpenBI, BookName);
	}

	/* Compose the target file name .$$$ and try to open it (ADD/DEL) */
	if ( (curcommand==CM_Add) || (curcommand==CM_Del) )
	{
		for ( i=strlen(finName)-1; (i>=0) && (finName[i]!='\\')
						   && (finName[i]!='.')         ; i--);
		if ( (i<0) || ( (i>=0) && (finName[i]=='\\') ) )
			i=strlen(finName);

		strncpy (foutName, finName, i);
		strcat  (foutName, ".$$$");

		if ((fout = fopen (foutName,"wt"))==NULL)
			CriticalError (SC_CantOpenTarget, foutName);
	}

  /*********** MAIN WORK **************/

	/* Find <!--BOOKSHELF-->*/
	do {
		fgets (line, MaxLine, fin);
		if ( (curcommand==CM_Add) || (curcommand==CM_Del) )
			fputs (line, fout);
		NoSpaces();
	} while ( strcmp(line,"<!--BOOKSHELF-->") &&
		    !feof (fin)	);
	if (feof(fin)) CriticalError (SC_NoBookhelpFile, finName);

	/* Main loop */
	switch ( curcommand )
	{
		case CM_List: List();
				  break;
		case CM_Add:  Add();
				  break;
		case CM_Del:  Del();
	}

  /*********** CLOSING TASKS **************/

	/* Close files */
	switch ( curcommand )
	{
	   case CM_Add:
		fclose (fbook);

	   case CM_Del:
		while (!feof(fin))
		{
			fgets (line, MaxLine, fin);
			if (feof(fin)) break;
			fputs (line, fout);
		}
		fclose (fout);

	   case CM_List:
		fclose (fin);
	}

	/* File renaming stuff */
	if ( (curcommand==CM_Add) || (curcommand==CM_Del) )
	{
		/* create the back name */
		strncpy (fbackName, finName, 255);
		if ( (strlen(fbackName)>4) &&
		     (fbackName[strlen(fbackName)-4]=='.') )
			fbackName[strlen(fbackName)-4]=0;
		strcat (fbackName,".BAK");

		/* File mangling */
		if (remove(fbackName)) CriticalError (SC_CantRemoveFin, finName);
		if (rename(finName,fbackName)) CriticalError (SC_CantRemoveFin, finName);
		if (rename(foutName,finName)) CriticalError (SC_CantRemoveFin, finName);

		if (curcommand==CM_Add) puts ("Book successfully added.");
		else		     	      puts ("Book successfully removed.");
	}

}
