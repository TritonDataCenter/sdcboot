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


#define Version 0x0101  /* version 1.1  of the produced files*/

#define MaxBuffer  16384
#define MaxPlanes  8
#define MaxBlocks  32

	   /*  Flags on the KEYTRANS section */
#define  LockKeyFlag  		 16
#define  ReplaceScanCodeFlag  128
#define  CapsLockFlag          64
#define  NumLockFlag           32

	   /*  Flags in the file */
#define  F_Diacritics   0x0001
#define  F_BEEP		0x0002
#define  F_BasicCom     0x0004
#define  F_ExtCom       0x0008
#define  F_DefShift     0x0010
#define  F_Japanese	0x0020
#define  F_Chained	0x0040
#define  F_APM          0x0080
#define  F_XString      0x0200

	     /* Flags not being modules */
#define  F_UseScroll    0x1000
#define  F_UseNum       0x2000
#define  F_UseCaps      0x4000

	   /* Types of object blocks */
#define  BT_KEYS        1
#define  BT_COMBI       2
#define  BT_STRING      3

	  /* Types of link sections */
#define  BT_GENERAL     1
#define  BT_SUBMAPPINGS 2
#define  BT_Planes      3

	  /* Shifting keys */
#define  SK_RShift      0x0001
#define  SK_LShift	0x0002
#define  SK_Control     0x0004
#define  SK_Alt         0x0008
#define  SK_Scroll	0x0010
#define  SK_Shift       0x0040
#define  SK_LControl    0x0100
#define  SK_LAlt        0x0200
#define  SK_RControl    0x0400
#define  SK_RAlt	      0x0800

	  /* Syntax errors */
#define  SE_UnknownFlag        1
#define  SE_SyntaxError	       2
#define  SE_TooManyPlanes      3
#define  SE_UnexpectedEOL      5
#define  SE_TooManyDiacritics  6
#define  SE_InvalidKeyName     7
#define  SE_InvalidFKeyName    8
#define  SE_TooManyVBars       9
#define  SE_InvalidSKeyName   10
#define  SE_IdentifierTooL    11
#define  SE_TooManyIdentifs   12
#define  SE_WrongNameSpec     13
#define  SE_TooManyDecChars   14
#define  SE_ErrInDecChar      15
#define  SE_UnorderedKeys 	16
#define  SE_WrongDataSection  17
#define  SE_SectionHeadExp    18
#define  SE_CNRequiresL1      19
#define  SE_SeparatorExpected 20


	  /* Critical errors */
#define  SC_WrongNParams         1
#define  SC_CantOpenSource       2
#define  SC_CantOpenTarget	   3
#define  SC_SyntaxError          6
#define  SC_UnknownSwitch        7
#define  SC_CantMAlloc           8
#define  SC_TooManyDBlocks       9
#define  SC_Only1Planes         12
#define  SC_Only1Submappings    13
#define  SC_Only1General        14
#define  SC_MissingLName        15
#define  SC_SubmappingsNeeded   16
#define  SC_GeneralNeeded       17
#define  SC_WrongSMChange       18
#define  SC_TooManyDiacritics   19
#define  SC_TooManyStrs	 	  20
#define  SC_TooManyPlanes       21
#define  SC_TooManySubmappings  22
#define  SC_2SubmRequired	  23
#define  SC_KEYSoverflowsplanes 24

/*////////// T Y P E S  ///////////////////////////////////////////////*/

typedef struct SubmChanges
{
	WORD   SourceLine;
	BYTE   NewSubmapping;
};

typedef struct ObjectBlock
{
	WORD   size;	/* Size of the block */
	WORD   offset;	/* absolute offset in the file */
	DWORD  bflags;	/* block computed flags */
	BYTE   type;	/* type of data block */
	BOOL   linked;	/* has the block been linked? */
	char   name[20];  /* name of the section */
	BYTE   *data;	/* pointer to a data buffer */

					   /* To issue a warning on submapping */
	WORD   WCS_ArraySize;      /* change if it changes codepage */
	struct SubmChanges WCS_SChanges[10];
};

typedef struct SubMapping
{
	WORD   codepage;  /* codepage of the submapping */
	BYTE   keysofs;   /* pointer to KEYS section (index in object list) */
	BYTE   combiofs;  /* pointer to COMBI section (index in object list) */
	BYTE   strofs;    /* pointer to String section (index in object list) */
};

typedef struct Plane
{
	WORD   wtStd;     /* Wanted standard flags */
	WORD   exStd;     /* Excluded standard flags */
	WORD   wtUsr;     /* Wanted user-defined flags */
	WORD   exUsr;     /* Excluded user-defined flags */
};


/*////////// V A R I A B L E S  ////////////////////////////////////*/

BYTE   buffer[MaxBuffer];		/* buffer and pointer */
WORD   bufferPtr  = 0;

char   line[255]  = "";
BYTE   lineptr    = 0;
WORD   lineNumber  = 0;

DWORD  flags       = 0;			/* flags */
BOOL   usestrings = FALSE;

char   finName[255]="";
char   foutName[255]="";	      /* file names */
FILE   *fin, *fout;			/* file variables */

WORD	 keycboffset = 0;			/* offset of file, for computations */

struct ObjectBlock blocks[MaxBlocks];	/* the array with the blocks */
BYTE   obnum = 0;				/* number of blocks */

struct Plane planes [8];		/* space for the planes  */
BYTE   planenumber = 0;

BYTE	 maxPlanes = 0;			/* used to track wether a KEYS line defines */
WORD   maxPlaneLine = 0;		/* more planes than allowed in the PLANES section */

struct SubMapping submappings[16];  /* space for the submappings */
BYTE   submnumber = 0;

char   names[256];			/* space for the names and IDs */
BYTE   namesptr=0;

char   DecChar = 0;			/* Decimal char */
BYTE	 prevkey = 0;			/* Previously parsed scancode: */
						/* to check that they are submitted in order*/

BOOL   bare    = FALSE;			/* Create bare KeybCB files? */

/*////////// F U N C T I O N S  ////////////////////////////////////*/

/* Displays an error on current line, pos lineptr */
void  SyntaxError  ( BYTE errorType )
{
	BYTE i;

	printf ("%s(%u): Syntax error %u: ", finName, lineNumber, errorType );
	switch (errorType)
	{
		case SE_UnknownFlag:
			    puts ("Unknown flag");
			    break;
		case SE_SyntaxError:
			    puts ("Syntax error");
			    break;
		case SE_TooManyPlanes :
			    puts ("Too many Planes ");
			    break;
		case SE_UnexpectedEOL:
			    puts ("Unexpected end of line");
			    break;
		case SE_TooManyDiacritics:
			    puts ("Too many diacritic pairs");
			    break;
		case SE_InvalidKeyName:
			    puts ("Invalid key name");
			    break;
		case SE_InvalidFKeyName:
			    puts ("Invalid function key number");
			    break;
		case SE_TooManyVBars:
			    puts ("Too many |'s");
			    break;
		case SE_InvalidSKeyName:
			    puts ("Invalid shift key name");
			    break;
		case SE_IdentifierTooL:
			    puts ("Identifier name too long");
			    break;
		case SE_TooManyIdentifs:
			    puts ("Too many identifiers");
			    break;
		case SE_WrongNameSpec:
			    puts ("Wrong name specification");
			    break;
		case SE_TooManyDecChars:
			    puts ("Too many decimal chars");
			    break;
		case SE_ErrInDecChar:
			    puts ("Error in the definition of the decimal char");
			    break;
		case SE_UnorderedKeys:
			    puts ("KEY section entries must be ordered");
			    break;
		case SE_WrongDataSection:
			    puts ("Wrong data section name (1-19 chars)");
			    break;
		case SE_SectionHeadExp:
			    puts ("Section header expected");
			    break;
		case SE_CNRequiresL1:
			    puts ("If flags C or N are used, then you must define at least 2 columns");
			    break;
		case SE_SeparatorExpected:
			    puts ("Separator character / expected");
			    break;
		default:  puts ("Undefined error");
	}

	  puts (line);
	  for (i=1; i<lineptr; i++)  putchar(' ');
	puts ("^");

	fcloseall();

	exit (errorType);
}


void  MainError  ( BYTE errorType, char *ExtraLine)
{
	BYTE i;

	printf ("Critical error %u: ", errorType );
	switch (errorType)
	{
		case SC_WrongNParams:
			    puts ("Wrong number of parameters specified");
			    break;
		case SC_CantOpenSource:
			    puts ("Unable to open source file name:");
			    break;
		case SC_CantOpenTarget:
			    puts ("Unable to open target file name:");
			    break;
		case SC_SyntaxError:
			    puts ("Syntax error");
			    break;
		case SC_UnknownSwitch:
			    puts ("Unknown switch");
			    break;
		case SC_CantMAlloc:
			    puts ("Couldn't assign memory for block");
			    break;
		case SC_TooManyDBlocks:
			    puts ("Too many data blocks");
			    break;
		case SC_Only1Planes :
			    puts ("Only ONE PLANES sections is allowed");
			    break;
		case SC_Only1Submappings:
			    puts ("Only ONE SUBMAPPINGS sections is allowed");
			    break;
		case SC_Only1General:
			    puts ("Only ONE GENERAL section is allowed");
			    break;
		case SC_MissingLName:
			    puts ("Missing layout name option");
			    break;
		case SC_SubmappingsNeeded:
			    puts ("Section [SUBMAPPINGS] is mandatory for linking");
			    break;
		case SC_GeneralNeeded:
			    puts ("Section [GENERAL] is mandatory for linking");
			    break;
		case SC_WrongSMChange:
			    printf ("Wrong submapping change number in line ");
			    break;
		case SC_TooManyDiacritics:
			    puts ("Too many diacritics");
			    break;
		case SC_TooManyStrs:
			    puts ("Too many Strings");
			    break;
		case SC_TooManyPlanes :
			    puts ("Too many planes ");
			    break;
		case SC_TooManySubmappings:
			    puts ("Too many submappings ");
			    break;
		case SC_2SubmRequired:
			    puts ("At least one particular submapping is required");
			    break;
		case SC_KEYSoverflowsplanes:
			    printf("Number of predefined planes is exceeded on line ");
			    break;
		default:  puts ("Undefined error");
	}

	puts (ExtraLine);

	fcloseall();

	exit (errorType+100);
}


BOOL  IsWhite ( unsigned char c )   /* space, tab,...*/
{
	return ((c==32) || (c==9) || (c==10) || (c==13));
}


WORD  NoSpaces ( void )
{
	for (;IsWhite(line[lineptr]); lineptr++);
	for (;IsWhite(line[strlen(line)-1]); line[strlen(line)-1]=0);
}


char *GetLine ( void )
{
	int i;
	do  {
		line[0] = 0;
		fgets (line, 255, fin);
		lineptr = 0;

		/* remove comments */
		for (i=0; line[i] && (line[i]!=';'); i++);
		line[i]=0;

		/* remove starting spaces*/
		NoSpaces ();

		lineNumber++;
		if (!line[lineptr])
			*line = 0;
	} while ( ((line[0]==';') || !line[0]) && !feof(fin) );
	lineptr = 0;
	return (line);
}

WORD WriteToBlock (BYTE bn, BYTE btype, BYTE *str, WORD len, char *bname)
{
	WORD i = 0;

	blocks[bn].size   = len;
	blocks[bn].offset = 0;
	blocks[bn].bflags = flags;
	blocks[bn].type   = btype;
	blocks[bn].linked = 0;
	strcpy (blocks[bn].name, bname);

	blocks[bn].data   = malloc (len);

	if (blocks[bn].data == NULL)
		MainError (SC_CantMAlloc, "");

	while  ( i<len )
		blocks[bn].data[i] = str[i++];

	flags = 0;

	return i;
}


WORD WriteToNextBlock (BYTE btype, WORD len, char *bname)
{
    if (obnum >= MaxBlocks)
		MainError (SC_TooManyDBlocks, "");

    return WriteToBlock (obnum++, btype, buffer, len, bname);
}



WORD  WriteToBuffer (BYTE *str, WORD len)
{
	WORD i = 0;

	while  ( (i<len) && (bufferPtr<=MaxBuffer))
		buffer [bufferPtr++] = str[i++];

	return i;
}


BYTE  ReadNumber ( void )
{
	char cvtstr[8];
	BYTE i=0;

	while ( (line[lineptr]>='0') && (line[lineptr]<='9'))
		cvtstr[i++] = line[lineptr++];
	cvtstr[i] = 0;

	return (  (BYTE) (atoi (cvtstr) & 0xFF) );
}

WORD  ReadWord ( void )
{
	char cvtstr[8];
	BYTE i=0;

	while ( (line[lineptr]>='0') && (line[lineptr]<='9'))
		cvtstr[i++] = line[lineptr++];
	cvtstr[i] = 0;

	return (  (WORD) (atoi (cvtstr) & 0xFFFF) );
}

BYTE ReadCharacter ( void )
{
	if (line[lineptr]=='#')
	{
		lineptr++;
		if ( (line[lineptr]=='#') || (line[lineptr]=='!'))
			return (line[lineptr++]);
		else
			return (ReadNumber());
	}
	else
		return (line[lineptr++]);
}


void UpdateFlags ( BYTE KEYcom )
{
	if ((200<=KEYcom) && (KEYcom<=234))   flags |= F_Diacritics;
	if (162==KEYcom)				  flags |= F_BEEP;
	if ((80<=KEYcom) && (KEYcom<=199))    flags |= F_BasicCom;

	if ((80<=KEYcom) && (KEYcom<=99))     flags |= F_ExtCom;
	if ((140<=KEYcom) && (KEYcom<=159))   flags |= F_ExtCom;
	if ((162<=KEYcom) && (KEYcom<=199))   flags |= F_ExtCom;

	if ((80<=KEYcom) && (KEYcom<=89))     flags |= F_DefShift;
	if (165==KEYcom)                      flags |= F_DefShift;

	if ((150<=KEYcom) && (KEYcom<=154))   flags |= F_APM;

	if ((120<=KEYcom) && (KEYcom<=139))
	{
		flags |= F_ExtCom;
		if ( blocks[obnum].WCS_ArraySize<10 )
		{
			blocks[obnum].WCS_SChanges [blocks[obnum].WCS_ArraySize ]. SourceLine = lineNumber;
			blocks[obnum].WCS_SChanges [blocks[obnum].WCS_ArraySize ]. NewSubmapping = KEYcom-119;
			blocks[obnum].WCS_ArraySize++;
		}
	}
}

void ParseTransTable ( void )
{
	BYTE tempbuffer [20] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
		columnptr,bufferptr,newscancode;

	NoSpaces ();

	/*** 1: preceeding R (release) and Scancode ***/
	if (toupper(line[lineptr])=='R')
	{
		tempbuffer[0] = 0x80;
		lineptr ++;
	}
	tempbuffer[0] |= ReadNumber();

	if (prevkey >= tempbuffer[0])
		SyntaxError ( SE_UnorderedKeys );

	prevkey = tempbuffer[0];

	/*** 2: Flags ***/
	while (line[lineptr] && (line[lineptr] != 32))
	{
		switch (toupper(line[lineptr]))
		{
			case 'C': tempbuffer[1] |= CapsLockFlag;
				    break;
			case 'N': tempbuffer[1] |= NumLockFlag;
				    break;
			case 'S': tempbuffer[1] |= ReplaceScanCodeFlag;
				    break;
			case 'X': tempbuffer[1] |= LockKeyFlag;
				    break;
			default:  SyntaxError  ( SE_UnknownFlag );
		}
		lineptr++;
	}

	NoSpaces ();

	/*** 3: Planes  ***/
	columnptr = (bufferptr= 3);

	while ( line[lineptr] )
	{
		if (tempbuffer[1] & ReplaceScanCodeFlag)
		{
			newscancode = ReadNumber();
			if (!line[lineptr] || (line[lineptr] != '/'))
				SyntaxError (SE_SeparatorExpected);
			lineptr++;
		}

		if (line[lineptr]=='!')	   /* XCommand */
		{
			lineptr++;
			tempbuffer[2] |= (1 << (columnptr-3));
			switch (toupper(line[lineptr++]))
			{
				case 'C':   tempbuffer[bufferptr] = 199;
						break;
				case 'L':   tempbuffer[bufferptr] = 99;
						break;
				case 'S':   tempbuffer[bufferptr] = 119;
						break;
				case 'A':   tempbuffer[bufferptr] = 169;
						break;
				case 'O':   tempbuffer[bufferptr] = 179;
						break;
				case 'H':   tempbuffer[bufferptr] =  89;
						break;
				default:    lineptr--;    /* there was no section specifier */
			}
			tempbuffer[bufferptr] += ReadNumber();
			UpdateFlags ( tempbuffer[bufferptr] );
		}
		else
			tempbuffer[bufferptr] = ReadCharacter ();
		if (tempbuffer[1] & ReplaceScanCodeFlag)

		if (line[lineptr] && (line[lineptr++] != 32))
				SyntaxError (SE_SyntaxError);

		if (((columnptr++) -2)>2+MaxPlanes )
				SyntaxError (SE_TooManyPlanes);

		if (tempbuffer[1] & ReplaceScanCodeFlag)
			tempbuffer[++bufferptr] = newscancode;
		NoSpaces ();
		bufferptr++;
	}

	/*** 4: update the length (with length-1)***/
	if (tempbuffer[1] & LockKeyFlag)
		tempbuffer[1] = LockKeyFlag;
	else
	{
		tempbuffer[1] |= ((columnptr-3) -1);
		if ((columnptr-3)>maxPlanes)
		{
			maxPlanes = columnptr-3;
			maxPlaneLine = lineNumber;
		}
	}

	/* Error if 'C' or 'N' and columns=0,1 */
	if ( (tempbuffer[1] & ( CapsLockFlag | NumLockFlag )) &&
	     !(tempbuffer[1] & 0x07))
		SyntaxError (SE_CNRequiresL1);

	/*** 5: Copy to buffer ***/
	columnptr = (tempbuffer[1] & 0x07) + 1;	/* columnptr here used for length */
	if (tempbuffer[1] & ReplaceScanCodeFlag )
		columnptr <<= 1;
	WriteToBuffer (tempbuffer, 3 + columnptr);

}

void ParseCombi ( void )
{
	BYTE tempbuffer[130], count=0;

	/*** 1: leading character ***/
	NoSpaces ();
	tempbuffer[0] = ReadCharacter();
	NoSpaces ();

	/*** 2: pairs ***/
	while (line[lineptr] /* && (count<63) */ )
	{
		tempbuffer[(count+1)*2] = ReadCharacter();
		NoSpaces();

		if (!line[lineptr])
				SyntaxError (SE_UnexpectedEOL);

		tempbuffer[(count+1)*2+1] = ReadCharacter();
		NoSpaces();

		if ((count++) >128)
				SyntaxError (SE_TooManyDiacritics);

	}

	tempbuffer[1] = count;

	/*** 3: copy to buffer ***/
	WriteToBuffer (tempbuffer, (count+1)<<1);
}


BYTE strpos (char *str1, char *str2)
{
	char *s = strstr (str1,str2);
	return (s==NULL) ? 0 : (s-str1+1);
}

BYTE ReadXKeyName ( void )
{
	BYTE b=0;

	switch (toupper(line[lineptr]))
	{
		case 'F': lineptr++;
			    switch (toupper(line[lineptr]))
			    {
				 case 'S': lineptr++;
					     b = ReadNumber ();
					     if (!b || (b>12))
								   SyntaxError (SE_InvalidFKeyName);
					     if ( (0<b) && (b<11) )
						     b += 83;
					     else
						     b += 124;
					     if (line[lineptr]!=']')
								   SyntaxError (SE_InvalidKeyName);
					     lineptr++;
					     return (b);
				 case 'C': lineptr++;
					     b = ReadNumber ();
					     if (!b || (b>12))
								   SyntaxError (SE_InvalidFKeyName);
					     if ( (0<b) && (b<11) )
						     b += 93;
					     else
						     b += 126;
					     if (line[lineptr]!=']')
								   SyntaxError (SE_InvalidKeyName);
					     lineptr++;
					     return (b);
				 case 'A': lineptr++;
					     b = ReadNumber ();
					     if (!b || (b>12))
								   SyntaxError (SE_InvalidFKeyName);
					     if ( (0<b) && (b<11) )
						     b += 103;
					     else
						     b += 128;
					     if (line[lineptr]!=']')
								   SyntaxError (SE_InvalidKeyName);
					     lineptr++;
					     return (b);
				 default:  b = ReadNumber ();
					     if (!b || (b>12))
								   SyntaxError (SE_InvalidFKeyName);
					     if ( (0<b) && (b<11) )
						     b += 58;
					     else
						     b += 122;
					     if (line[lineptr]!=']')
								   SyntaxError (SE_InvalidKeyName);
					     lineptr++;
					     return (b);
			    }
		default:  if (strpos( &line[lineptr], "HOME]") == 1 )
			    {
				  lineptr += 5;
				  return  ( 71 );
			    }
			    if (strpos( &line[lineptr], "END]") == 1 )
			    {
				  lineptr += 4;
				  return  ( 79 );
			    }
			    if (strpos( &line[lineptr], "PGUP]") == 1 )
			    {
				  lineptr += 5;
				  return  ( 73 );
			    }
			    if (strpos( &line[lineptr], "PGDN]") == 1 )
			    {
				  lineptr += 5;
				  return  ( 81 );
			    }
			    if (strpos( &line[lineptr], "UP]") == 1 )
			    {
				  lineptr += 3;
				  return  ( 72 );
			    }
			    if (strpos( &line[lineptr], "DOWN]") == 1 )
			    {
				  lineptr += 5;
				  return  ( 80 );
			    }
			    if (strpos( &line[lineptr], "LEFT]") == 1 )
			    {
				  lineptr += 5;
				  return  ( 75 );
			    }
			    if (strpos( &line[lineptr], "RIGHT]") == 1 )
			    {
				  lineptr += 6;
				  return  ( 77 );
			    }
			    if (strpos( &line[lineptr], "DEL]") == 1 )
			    {
				  lineptr += 4;
				  return  ( 83 );
			    }
			    if (strpos( &line[lineptr], "INS]") == 1 )
			    {
				  lineptr += 4;
				  return  ( 82 );
			    }
				    SyntaxError (SE_SyntaxError);
	}
}

/* reads a number of the form {xxx} */
BYTE ReadBracketNumber ( void )
{
	BYTE n;

	if ( line[lineptr]!='{' ) SyntaxError (SE_SyntaxError);
	lineptr++;
	n = ReadNumber();
	if ( line[lineptr]!='}' ) SyntaxError (SE_SyntaxError);
	lineptr++;
	return (n);
}


/* reads a pair of the form {hi,lo} */
WORD ReadBracketPair ( void )
{
	BYTE h,l;

	if ( line[lineptr]!='{' ) SyntaxError (SE_SyntaxError);
	lineptr++;
	h = ReadNumber();
	if ( line[lineptr]!=',' ) SyntaxError (SE_SyntaxError);
	lineptr++;
	l = ReadNumber();
	if ( line[lineptr]!='}' ) SyntaxError (SE_SyntaxError);
	lineptr++;
	return ( (h << 8 ) | l);
}


/* NOTE: hi=scancode, lo=character */
WORD ReadXChar ( void )
{
	if (line[lineptr]=='\\' )
	{
		lineptr++;
		switch (toupper(line[lineptr++]))
		{
			case 'A' : return (ReadBracketNumber());
			case 'S' : return (ReadBracketNumber() << 8);
			case 'K' : return (ReadBracketPair());
			case 'N' : return ( (28<<8) | 13 );
			case '\\': return ( '\\' );
			default  : return ( ReadXKeyName () << 8 );
		}
	}
	else
		return ( line[lineptr++] );
}


void ParseString ( void )
{
	WORD tempbuffer[256];
	BYTE count=0;

	usestrings = TRUE;

	while (line[lineptr])
		tempbuffer[count++] = ReadXChar();

	WriteToBuffer ( &count, 1);
	WriteToBuffer ( (BYTE *) &tempbuffer, count << 1);
}


void ShowHelp ( void )
{
	puts ("KC  filename [outputfilename] [/B] ");
	puts ("KC  /?\n");
	puts ("FreeDOS KEY compiler.");
	puts ("Compiles KEY format into the binary KL format.\n");
	puts ("filename       name of the source file to be opened. KEY extension");
	puts ("               is added if no extension is found");
	puts ("outputfilename name of the target compiled filename. If omitted,");
	puts ("               the same filename with KL extension is created");
	puts ("/B             Creates bare KeybCB files, with no KL header");

}


/* line must be [KEYS:xxx] or [DIACRITICS:xxx] or [STRINGS:xxx] */
/* xxx is extracted */
char *GetDataSectionName ( char *dsname )
{
	BYTE i=0, j=0;

	while ( line[i++] != ':' );

	for (j=0; ((line[i]>='0') && (line[i]<='9')) ||
		    ((line[i]>='A') && (line[i]<='Z')) ||
		     (line[i]=='_') && (j<20);
			dsname[j++] = line[i++] );
	if (j>20)
		    SyntaxError (SE_IdentifierTooL);

	if ((line[i]!=']') || (j<1) || (j>19))
		SyntaxError (SE_WrongDataSection);

	dsname[j] = 0;
	return dsname;
}


void ParseKeysSection ( void )
{
	char name[20];

	bufferPtr = 0;
	flags     = 0;
	GetDataSectionName (name);

	blocks[obnum].WCS_ArraySize = 0;

	prevkey = 0;

	while (!feof(fin) && strcmp (GetLine(), ""))
	{
		if (line[0]=='[') break;
		ParseTransTable ();
	}

	buffer[bufferPtr++]=0; 	/* this 0 to indicate no more scancodes */

	WriteToNextBlock (BT_KEYS, bufferPtr, name);

}


void ParseCombiSection ( void )
{
	char name[20];
	BYTE numcombi=0;

	bufferPtr = 0;
	flags     = 0;
	GetDataSectionName (name);

	while (!feof(fin) && strcmp (GetLine(), ""))
	{
		if (line[0]=='[') break;
		ParseCombi (); numcombi++;
		if (numcombi>35) MainError (SC_TooManyDiacritics, "");
	}

	buffer[bufferPtr++]=0; 	/* this 0 to indicate no more scancodes */

	WriteToNextBlock (BT_COMBI, bufferPtr, name);

}


void ParseStringSection ( void )
{
	char name[20];
	BYTE numstr    = 0;

	flags     = 0;
	bufferPtr = 0;
	GetDataSectionName (name);

	while (!feof(fin) && strcmp (GetLine(), ""))
	{
		if (line[0]=='[') break;
		ParseString (); numstr++;
		if (numstr>79) MainError (SC_TooManyStrs,"");
	}

	WriteToNextBlock (BT_STRING, bufferPtr, name);

}


/* Pos is set to the next character after the key name
   Returns:
	0: RShift     6: Shift
	1: LShift	  8: LControl
	2: Control    9: LAlt
	3: Alt       10: RControl
	4: Scroll    11: RAlt
	16: KanaLock / UserKey1
	17: UserKey2 ... (16..23)
	255: error(not found) */
BYTE GetShiftKeyName ( char *s, BYTE *pos )
{
    char *keynames[] = {"RSHIFT", "LSHIFT", "CTRL", "ALT",
				"SCROLLLOCK", "NUMLOCK", "CAPSLOCK", "",
				"LCTRL",	"LALT", "RCTRL", "ALTGR",
				"E0","", "SHIFT","",
				"KANALOCK"};
    int i;

    if ( strpos(s,"USERKEY") == 1)
    {
	i = atoi (&s[7]) + 15;
	*pos += 7;  					   /* 7==strlen("USERKEY") */
	while ( (s[*pos]>='0') && (s[*pos]<='9'))
		(*pos) ++;

	return (( (i<16) || (i>23) ) ? 0xFF : i);
    }
    for (i=16; i>=0; i--)
    {
	if ( (strpos (s, keynames[i]) == 1) &&
	     (keynames[i][0]) )
	{
		*pos += strlen (keynames[i])-1;
		if (i==4)  flags |= F_UseScroll;  /* use of standard lock keys */
		if (i==5)  flags |= F_UseNum;
		if (i==6)  flags |= F_UseCaps;

		if (i==16) i=23;		/* KanaLock == UserLock8 */

		return i;
	}
    }

    return 0xFF;
}


void ParsePlanesLine ( BYTE c )
{
	WORD *w  =&planes[c].wtStd;
	WORD *w2 =&planes[c].wtUsr;
	BYTE n;

	planes[c].wtStd = 0;
	planes[c].exStd = 0;
	planes[c].wtUsr = 0;
	planes[c].exUsr = 0;
	lineptr = 0;

	strupr (line);

	while ( line[lineptr] )
	{
		    switch ( line[lineptr] )
		{
			case '|': if (w==&planes[c].wtStd)
				    {
					 w  = &planes[c].exStd;
					 w2 = &planes[c].exUsr;
				    }
				    else SyntaxError (SE_TooManyVBars);
			case ' ': break;
			default : n = GetShiftKeyName (line+lineptr, &lineptr);
				    if (n==0xFF) SyntaxError (SE_InvalidSKeyName);
				    if (n<=15)
					 *w |= (1 << n );
				    else
					 *w2 |= ( 1 << (n-16));

		}
		    lineptr++;
	}
}


void ParsePlanesSection ( void )
{
	if (planenumber>0)
		MainError (SC_Only1Planes ,"");

	while (!feof(fin) && strcmp (GetLine(), "") && (planenumber<8))
	{
		if (line[0]=='[') break;
		ParsePlanesLine (planenumber++);
		if (planenumber>8) MainError (SC_TooManyPlanes ,"");
	}

}



BYTE FindBlockByName (char *oname)
{
	BYTE i;

	strupr (oname);

	for (i=0; i<obnum; i++)
	    if (!strcmp(oname, blocks[i].name))
	    {
		  blocks[i].linked = TRUE;
		  flags |= blocks[i].bflags;
		  return i;
	    }
	return 0xFF;
}



char *ReadIdentifier (char *id)
{
	BYTE i=0;

	while (line[lineptr] && (line[lineptr]!=' ') && (i<20))
		id[i++] = line[lineptr++];
	id[i] = 0;
	if (i>=20)
		    SyntaxError (SE_IdentifierTooL);

	return id;
}


void ParseSubMappingsLine ( BYTE subm )
{
	char id[20];

	NoSpaces ();
	submappings[subm].codepage = ReadWord ();
	submappings[subm].keysofs  = 0xFF;
	submappings[subm].combiofs = 0xFF;
	submappings[subm].strofs   = 0xFF;
	NoSpaces ();

	if (!line[lineptr])
		    SyntaxError (SE_UnexpectedEOL);

	if (line[lineptr]=='-')
		lineptr++;
	else
		submappings[subm].keysofs = FindBlockByName (ReadIdentifier (id));

	NoSpaces ();
	if (!line[lineptr]) return;

	if (line[lineptr]=='-')
		lineptr++;
	else
		submappings[subm].combiofs = FindBlockByName (ReadIdentifier (id));

	NoSpaces ();
	if (!line[lineptr]) return;

	if (line[lineptr]=='-')
		lineptr++;
	else
		submappings[subm].strofs = FindBlockByName (ReadIdentifier (id));

	NoSpaces ();
	if (line[lineptr])
		    SyntaxError (SE_TooManyIdentifs);


}


void ParseSubMappingsSection ( void )
{
	if (submnumber>0)
		MainError (SC_Only1Submappings,"");
	while (!feof(fin) && strcmp (GetLine(), "") && (submnumber<16))
	{
		if (line[0]=='[') break;
		ParseSubMappingsLine ( submnumber++ ) ;
	}
	if (submnumber>=16)
		MainError (SC_TooManySubmappings,"");
	if (submnumber<2)
		MainError (SC_2SubmRequired, "");
}


void InsertFlags ( char *modstr )
{
	BYTE i=0;

	for (; modstr[i]; i++)
	switch ( modstr[i] )
	{
		case 'C': flags |= F_Diacritics; break;
		case 'S': flags |= F_BEEP;  break;
		case 'B': flags |= F_BasicCom; break;
		case 'E': flags |= F_ExtCom;  break;
		case 'L': flags |= F_DefShift; break;
		case 'J': flags |= F_Japanese; break;
		case 'A': flags |= F_APM;  break;
		case 'X': flags |= F_XString;  break;
	}
}


void ParseGeneralLine ( void )
{
	WORD id = 0;

	strupr ( line );

	if ( strpos (line, "NAME=") == 1)
	{
		lineptr = 5;
		id = ReadWord();
		if (line[lineptr++] != ',')
				SyntaxError ( SE_WrongNameSpec );

		names [namesptr++] = id & 0xFF;
		names [namesptr++] = id >> 8;

		for (;line[lineptr]; lineptr++)
			names[namesptr++] = line[lineptr];

		names[namesptr++]=',';

		return;
	}

	if ( strpos (line, "DECIMALCHAR=") == 1)
	{
		if ( DecChar )
				SyntaxError ( SE_TooManyDecChars );

		lineptr = 12;
		DecChar = ReadCharacter();

		NoSpaces ();

		if ( line[lineptr] )
				SyntaxError ( SE_ErrInDecChar );

		return;
	}

	if (strpos (line, "FLAGS=") == 1)
	{
		lineptr = 6;
		InsertFlags (line+lineptr);
		return;
	}

	printf ("Warning: unknown option ignored: %s\n", line);
}


void ParseGeneralSection ( void )
{
	if (namesptr>0)
		MainError (SC_Only1General,"");

	while (!feof(fin) && strcmp (GetLine(), "") && (namesptr<255))
	{
		if (line[0]=='[') break;
		ParseGeneralLine ();
	}

        namesptr--;                     /* remove last comma */

	if (namesptr==0)
		MainError (SC_MissingLName,"");	/* at least one name */


}


/* Uses fout for write */
void fputbuffer (BYTE *buf, WORD bufsize)
{
      WORD j = 0;

	for (j=0; j<bufsize; j++)
		  fputc (buf[j], fout);
}

/* Puts a WORD to a file */
void fputw (WORD w, FILE *f)
{
      fputc ( w & 0xFF, f);     /* big endian */
	fputc ( w >> 8, f);
}


void fputsubmapping ( BYTE n )
{

	  fputw ( submappings[n].codepage, fout );

	  if ( submappings[n].keysofs != 0xFF )
		    fputw ( blocks[ submappings[n].keysofs ].offset, fout );
	  else
		    fputw ( 0, fout );

	  if ( submappings[n].combiofs != 0xFF )
		    fputw ( blocks[ submappings[n].combiofs ].offset, fout );
	  else
		    fputw ( 0, fout );

	  if ( submappings[n].strofs != 0xFF )
		    fputw ( blocks[ submappings[n].strofs ].offset, fout );
	  else
		    fputw ( 0, fout );
}

void fputcolumn ( BYTE n )
{
	  fputw ( planes[n].wtStd, fout );
	  fputw ( planes[n].exStd, fout );
	  fputw ( planes[n].wtUsr, fout );
	  fputw ( planes[n].exUsr, fout );
}


              
/*/////////// MAIN BEGINS HERE  ///////////////////////////////////////*/

void main ( BYTE argc, char *argv[] )
{
	int	   i=0, j=0, k=0;				/* other indices */
	WORD	   w=0;
	char	   s[10];

/*/////////// PART 1: check parameters ////////////////////////////////*/

	printf ("KEY file Compiler version 2.0 [2005/08/10] (creates KL ver %u.%u )\n", Version>>8, Version & 255);
	puts ("Copyright (c) 2003-2005 by Aitor Santamaria_Merino (under GPL)");

	if ( argc<2 )
		MainError (SC_WrongNParams, "");

	for (i=1; i<argc; i++)
	{
		if (argv[i][0]=='/')
		{
			if (strlen(argv[i])>2)
				MainError (SC_SyntaxError, argv[i]);
			switch ( toupper (argv[i][1]))
			{
				case '?':  ShowHelp ();
					     return;
				case 'B':  bare = TRUE;
					     break;
				default:   MainError (SC_UnknownSwitch, argv[i]);
			}
		}
		else
		{
			if (!finName[0])
				strcpy (finName, argv[i]);
			else if (!foutName[0])
				strcpy (foutName, argv[i]);
			else
				MainError (SC_WrongNParams, "");
		}
	}

	if (!finName[0])
		MainError (SC_WrongNParams, "");


/*/////////// PART 2: get filenames ///////////////////////////////////*/

	/* Check presence of extension */
	for ( i = strlen(finName)-1; (i>=0) && (finName[i]!='\\') && (finName[i]!='.'); i--);

	if (! ((i>=0) && (finName[i]=='.')) )
		strcat (finName, ".KEY");

	/* Get second argument */
	if (!foutName[0])
	{
		for ( i=strlen(finName)-1; finName[i]!='.'; i--);
		strncpy (foutName, finName,i+1);
		strcat (foutName, "KL");
	}
	if ( !strchr(foutName, '.') )
		strcat (foutName, ".KL");

	/* Try to open the files */
	if ((fin = fopen (finName,"rt"))==NULL)
		MainError (SC_CantOpenSource, finName);

/*/////////////////// PART 3: compile ///////////////////////////*/

	printf ("Compiling %s...\n",finName);

	/* 1: remove starting spaces/comments */
	while (!feof(fin) && !strcmp (GetLine(), ""));

	/* 2: parse the sections */
	while (!feof(fin))
	{
		if (line[0]!='[')
			SyntaxError ( SE_SectionHeadExp );

		strupr(line);

		if ( strpos(line,"[KEYS:") == 1 )
			ParseKeysSection ();
		else if ( strpos(line,"[DIACRITICS:") == 1 )
			ParseCombiSection ();
		else if ( strpos(line,"[STRINGS:") == 1 )
			ParseStringSection ();
		else if ( !strcmp(line,"[PRINT]") )
			while ( !feof(fin) && (GetLine()[0]!='[') )
				puts (line);
		else
			while (!feof(fin) && GetLine()[0]!='[');
	}

	fclose (fin);

/*/////////////////// PART 4: link 1: get link info /////////////*/

	printf ("Linking %s...\n",finName);

	flags = usestrings ?  F_XString : 0;  /* Initialise global flags */

	if ((fin = fopen (finName,"rt"))==NULL)
		MainError (SC_CantOpenSource, finName);

	/* 1: remove starting spaces/comments */
	while (!feof(fin) && !strcmp (GetLine(), ""));

	/* 2: parse the sections */
	while (!feof(fin))
	{
		if (line[0]!='[')
			SyntaxError (SE_SectionHeadExp);

		strupr(line);

		NoSpaces ();

		if ( !strcmp(line,"[PLANES]"))
			ParsePlanesSection ();
		else if ( !strcmp(line,"[SUBMAPPINGS]"))
			ParseSubMappingsSection ();
		else if ( !strcmp(line,"[GENERAL]"))
			ParseGeneralSection ();
		else
			while (!feof(fin) && GetLine()[0]!='[');
	}

	fclose (fin);		/* forever ;-) */

	/* 3: At least one submappings and one general sections */
	if (!submnumber)
		MainError (SC_SubmappingsNeeded, "");
	if (!namesptr)
		MainError (SC_GeneralNeeded, "");

	/* 3b: other possible problem: too many planes in a KEYS line */
	if ( (planenumber+2) < maxPlanes )
		MainError (SC_KEYSoverflowsplanes, itoa (maxPlaneLine,s,10));


	/* 4: Warning on change of codepage */
	for (i=0; i<submnumber; i++)
		if ((submappings[i].keysofs != 0xFF) &&
		    (blocks[submappings[i].keysofs].WCS_ArraySize>0) )
		for (j=0; j<blocks[submappings[i].keysofs].WCS_ArraySize; j++)
		{
			k = blocks[submappings[i].keysofs].WCS_SChanges[j].NewSubmapping;
			if (k>=submnumber)
				MainError (SC_WrongSMChange, itoa (blocks[submappings[i].keysofs].WCS_SChanges[j].SourceLine,s,10));
			w = submappings[k].codepage;
			if ( w && (submappings[i].codepage) && (submappings[i].codepage != w) )
				{
					printf ("Warning on line %u: change of submapping changes the codepage of KEYB, and\n", blocks[submappings[i].keysofs].WCS_SChanges[j].SourceLine);
					printf ("will leave KEYB in an inconsistent state (sm: %u -> %u).\n",i,k);
				}
		}


/*/////////////////// PART 5: link 2: fix-ups  //////////////////*/


	/* 1: Compute the size of the KeyCB header */

	keycboffset = 20 + 8*(submnumber + planenumber);

        /* 2: Compute the offsets of the blocks */

        for (i=0; i<obnum; i++)
        if (blocks[i].linked)
        {
		    blocks[i].offset = keycboffset;
		    keycboffset += blocks[i].size;
	  }


/*/////////////////// PART 6: Write to file  ////////////////////*/


        if ((fout = fopen (foutName,"wb"))==NULL)
		  MainError (SC_CantOpenTarget, foutName);

	  /* 1: KL file header */

	  if ( !bare )
	  {
		fputbuffer ("KLF",3);         /* signature */
		fputw (Version, fout);        /* version */
		fputc (namesptr, fout);       /* length of names string */
		fputbuffer (names, namesptr); /* now set the names */
	  }


	  /* 2: KeyCB header */

	  fputc ( submnumber, fout);      /* number of submappins */
	  fputc ( planenumber, fout);       /* number of Planes  */
	  fputc ( DecChar, fout );        /* decimal char */

	  fputc ( 0, fout );              /* EMPTY (current submapping) */

	  fputw ( 0, fout );		    /* EMPTY (my MCB) */

	  fputw ( flags, fout);           /* EMPTY: used for flags */

	  for (i=1; i<=6; i++)		    /* EMPTY: 12 bytes */
		fputw ( 0, fout );

	  /* 3: submappings */

        for (i=0; i<submnumber; i++)
		    fputsubmapping (i);

	  /* 4: Planes  */

	  for (i=0; i<planenumber; i++)
		    fputcolumn (i);

	  /* 5: data blocks */

	  for (i=0; i<obnum; i++)
	  if (blocks[i].linked)           /* SmartLinking ;-) */
		    fputbuffer ( blocks[i].data, blocks[i].size );

	  /* 6: File Alright */

	  fclose (fout);


/*/////////////////// PART 7: Goodbye!!  ////////////////////////*/

	printf ("File %s succesfully compiled (0 Errors)\n", foutName);

}
