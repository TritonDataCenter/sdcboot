/* 
 * MSET - Public Domain.  Portions written by the snippets.org folks and by
 * Blair Campbell.  This works with all versions of MS-DOS before 7.0 (and
 * possibly 7.0 too), DR-DOS, probably PC-DOS, and FreeCOM versions
 * 0.84pre2 CVS and higher.  Supports the same (except for command-line parsing)
 * syntax as SET internal to FreeCOM 0.84pre2 CVS.
 */

#include <stdio.h>
#include <dos.h>
#include <string.h>
#include <ctype.h>
#ifdef __WATCOMC__
#include <malloc.h>
#include <process.h>
#else
#include <alloc.h>
#endif
#include <io.h>
#include <conio.h>
#include <fcntl.h>

#define MAXPATH 128

struct __environrec {
    unsigned environseg;  /*Segment of the environment*/
    unsigned environlen;  /*Usable length of the environment*/
};

struct __environrec *_getmastenv(void)
{
    unsigned owner;
    unsigned mcb;
    unsigned eseg;
    static struct __environrec env;

    env.environseg = env.environlen = 0;
    owner = * ((unsigned far *) MK_FP(0, (2+4*0x2e)));

    /* int 0x2e points to command.com */

    mcb = owner -1;

    /* Mcb points to memory control block for COMMAND */

      if ( (*((char far *) MK_FP(mcb, 0)) != 'M') &&
           (*((unsigned far *) MK_FP(mcb, 1)) != owner) )
            return (struct __environrec *) 0;

      eseg = *((unsigned far *) MK_FP(owner, 0x2c));

    /* Read segment of environment from PSP of COMMAND} */
    /* Earlier versions of DOS don't store environment segment there */

    if( !eseg )
    {

        /* Master environment is next block past COMMAND */

        mcb = owner + *((unsigned far *) MK_FP(mcb, 3));
        if ( (*((char far *) MK_FP(mcb, 0)) != 'M') &&
             (*((unsigned far *) MK_FP(mcb, 1)) != owner) )
            return (struct __environrec *) 0;
        eseg = mcb + 1;
    }
    else mcb = eseg-1;

    /* Return segment and length of environment */

    env.environseg = eseg;
    env.environlen = *((unsigned far *) MK_FP(eseg - 1, 3)) << 4 ;
    return &env;
}

/*
** Then a function to find the string to be replaced.   This one'll
** return a pointer to the string, or a pointer to the first (of 2)
** NUL byte at the end of the environment.
*/

char far *_mastersearchenv( char far *eptr, char *search )
{
    char far *e;
    char *s;

    while( *eptr )
    {
        for( s = search, e = eptr; *e && *s && (*e == *s); e++, s++ )
            ;  /* NULL STATEMENT */
        if( !*s )
            break;
        while( *eptr )
            eptr++;           /* position to the NUL byte */
        eptr++;                          /* next string */
    }
    return( eptr );
}

/*
** Now, the function to replace, add or delete.  If a value is not
** given, the string is deleted.
*/

int _masterputenv( struct __environrec *env, char *search, char *value )
{
      /* -Set environment string, returning true if successful */

    char far *envptr;
    register char far *p;
    char *s;
    int newlen;
    int oldlen;
    int i;

    if( !env->environseg || !search )
        return 0;

    /* get some memory for complete environment string */

    newlen = strlen( search ) + strlen( value ) + 3;
    if( ( s = ( char * )malloc( newlen ) ) == NULL )
        return 0;
    for( i = 0; *search; search++, i++ )
        s[i] = *search;
    s[i++] = '=';
    s[i] = '\0';
      envptr = _mastersearchenv((char far *) MK_FP(env->environseg, 0), s );
    if( *envptr )
    {
        for( p = envptr, oldlen = 0; *p; oldlen++, p++ )
            ;     /* can't use strlen() because of far pointer */
    }             /* will set p to point to terminating NUL */

    if( *value && (newlen > (int)env->environlen) ) /* not a deletion */
    {
        free( s );
        return 0;                           /* won't fit */
    }

    if( *envptr )                            /* shift it down */
    {
        for( ++p; (*p || *(p+1)); envptr++, p++ )
            *envptr = *p;
        *envptr++ = '\0';
        *envptr = '\0';
    }
    if( *value )                             /* append it */
    {
        /* Could use strcat here, but why link it in? */
        strcpy( &s[ strlen( s ) ], value );
        while( *s )
            *(envptr++) = *s++;
        *envptr++ = '\0';
        *envptr = '\0';
    }
    free( s );
    return 1;
}

char * _getmenv( char *varname, struct __environrec *env )
{
    char far *envval = _mastersearchenv( ( char far * )MK_FP( env->environseg,
                                                              0 ),
                                         varname );
    static char retval[ 80 ];

    while( *envval != '=' && *envval ) *envval++;

    fartonearcpy( retval, &envval[ 1 ] );

    return( retval );
}

char getstdin( void );

char ** _getmenviron( struct __environrec *env )
{
    char far *eptr = MK_FP( env->environseg, 0 ), *e;
    char **retary;
    int len = 0;

    retary = ( char ** )malloc( env->environlen );

    while( *eptr )
    {
        fartonearcpy( retary[ len ], &*eptr );
        while( *eptr ) {
            eptr++;           /* position to the NUL byte */
        }
        eptr++;                          /* next string */
        len++;
    }
    retary[ len ] = NULL;

    return( retary );
}

/*
** Main executable routines
*/

void writeout( char *string );

#ifdef __WATCOMC__

void getcharacter( int handle, char * data );
#pragma aux getcharacter =  \
    "mov ah, 0x3F"          \
    "mov cx, 1"             \
    "int 0x21"              \
modify [ax cx bx dx] parm [bx] [dx];

char getstdin( void );
#pragma aux getstdin =      \
    "mov ah, 0x01"          \
    "int 0x21"              \
modify [ax] value [al];

void writestring( char *string, int len );
#pragma aux writestring =   \
    "mov ah, 0x40"          \
    "mov bx, 1"             \
    "int 0x21"              \
modify [ax dx cx] parm [dx] [cx];

int dupout( void );
#pragma aux dupout =        \
    "mov ah, 0x45"          \
    "mov bx, 1"             \
    "int 0x21"              \
modify [ax bx] value [ax];

void dup2out( int handle );
#pragma aux dup2out =       \
    "mov ah, 0x46"          \
    "mov cx, 1"             \
    "int 0x21"              \
modify [ax bx cx] parm [bx];

void docreattrunc( const char *file );
#pragma aux docreattrunc =  \
    "mov ah, 0x3C"          \
    "mov cx, 0"             \
    "int 0x21"              \
modify [ax cx dx] parm [dx];

int doopen( const char *file, unsigned char attribs );
#pragma aux doopen =        \
    "mov ah, 0x3D"          \
    "int 0x21"              \
modify [ax dx] parm [dx] [al] value [ax];

void doclose( int handle );
#pragma aux doclose =       \
    "mov ah, 0x3E"          \
    "int 0x21"              \
modify [ax bx] parm [bx];

void deletef( char *file );
#pragma aux deletef =       \
    "mov ah, 0x41h"         \
    "int 0x21"              \
modify [ax dx] parm [dx];

int exists( char *file );
#pragma aux exists =        \
    "mov ax, 0x4300"        \
    "int 0x21"              \
    "mov ax, 0"             \
    "jc end"                \
    "mov ax, 1"             \
    "end:"                  \
modify [ax dx cx] parm [dx] value [ax];

#else

#if defined( FEATURE_PROMPT ) || defined( FEATURE_EXECUTE )
void getcharacter( int handle, char *data )
{
    _DX = FP_OFF( data );
    _AH = 0x3F;
    _BX = handle;
    _CX = 1;

    geninterrupt( 0x21 );

    return( _AL );
}

char getstdin( void )
{
    _AH = 0x01;

    geninterrupt( 0x21 );

    return( _AL );
}
#endif

void writestring( char *string, int len )
{
    _DX = FP_OFF( string );
    _AH = 0x40;
    _BX = 1;
    _CX = len;

    geninterrupt( 0x21 );
}

#ifdef FEATURE_EXECUTE
int dupout( void )
{
    _AH = 0x45;
    _BX = 1;

    geninterrupt( 0x21 );

    return( _AX );
}

void dup2out( int handle )
{
    _BX = handle;
    _CX = 1;
    _AH = 0x46;

    geninterrupt( 0x21 );
}

void docreattrunc( const char *file )
{
    _DX = FP_OFF( file );
    _CX = 0;
    _AH = 0x3C;

    geninterrupt( 0x21 );
}

int doopen( const char *file, unsigned char attribs )
{
    _DX = FP_OFF( file );
    _AH = 0x3D;
    _AL = attribs;

    geninterrupt( 0x21 );
    
    return( _AX );
}

void doclose( int handle )
{
    _BX = handle;
    _AH = 0x3E;

    geninterrupt( 0x21 );
}

void deltef( char *file )
{
    _DX = FP_OFF( file );
    _AH = 0x41;

    geninterrupt( 0x21 );
}

int exists( char *file )
{
    _DX = FP_OFF( file );
    _AX = 0x4300;

    geninterrupt( 0x21 );

    return( _CFLAG ? 0 : 1 );
}
#endif

#endif

#ifdef FEATURE_EXECUTE
/* Functions to redirect a file and then to restore once more */
int redirectout( const char *dev )
{
    int oldhandle = dupout();    /* Duplicate the file handle */
    doclose( 1 );          /* Then close it */
    docreattrunc( dev );    /* Then open a new one */
    return( oldhandle );       /* And return it */
}

void unredirectout( int oldhandle )
{
    doclose( 1 );          /* Close the handle */
    dup2out( oldhandle );    /* And replace it with the old one */
    doclose( oldhandle );
}

/* Create a unique temp file name */
char * tempname( struct __environrec *p )
{
    static char tmp[MAXPATH];

    strcpy( tmp, _getmenv( "TEMP", p ) );
    if( tmp[ strlen( tmp ) - 1 ] != '\\' ) tmp[ strlen( tmp ) - 1 ] = '\\';
    strcpy( &tmp[ strlen( tmp ) ], _mktemp( "XXXXXX" ) );

    return( tmp );
}
#endif

#if defined( FEATURE_PROMPT ) || defined( FEATURE_EXECUTE )
/* Avoid linking in stream io stuff */
void hgets( int handle, char *dest )
{
    char readbyte;
    int len = 0;

    do {
        if( handle == 0 ) readbyte = getstdin();
        else getcharacter( handle, &readbyte );

        if( readbyte != '\r' &&
            readbyte != '\b' &&
            readbyte != '\n' && ) 
            *dest++ = readbyte;
        else if( readbyte == '\b' ) {
            writeout( " \b" );
            *dest--;
            len -= 2;
        } else if ( readbyte == '\r' ) writeout( "\n" );
        len++;
    } while( readbyte != '\r' &&
             readbyte != '\n' &&
             len < 80 );
    *dest = '\0';
}
#endif

void writeout( char *string )
{
    writestring( string, strlen( string ) );
}

void fartonearcpy( char *dest, char far *source )
{
    do {
        *dest = *source;
        *dest++;
    } while( *source++ );
}

void help( void )
{
    writeout( "MSET 1.0 - Sets the Master COMMAND.COM environment\r\n" );
    writeout( "Syntax: MSET [/C] [/U] [/P] [/E] <VARNAME>=[<VALUE>]\r\n" );
    writeout( "Options:\r\n" );
    writeout( "\t/C\tKeep case of <VARNAME> (default is uppercase)\r\n" );
#ifdef FEATURE_CASE
    writeout( "\t/U\tUppercase <VALUE>\r\n" );
#endif
#ifdef FEATURE_PROMPT
    writeout( "\t/P\tPrompt the user for the value, print <VALUE>\r\n" );
#endif
#ifdef FEATURE_EXECUTE
    writeout( "\t/E\tSet the variable to the first line of output of the\r\n" );
    writeout( "\t\tcommand pointed to by <VALUE>\r\n" );
#endif
}

#define KEEPCA 0x01
#define PROMPT 0x02
#define EXECUT 0x04
#define UPCASE 0x08

int main( int argc, char **argv )
{
    char varname[80], varval[80], *ptr;
    char optflags = 0;
    struct __environrec *p = _getmastenv();
    int i = 1;

    if( argc < 1 ) {
        help();
        return( 1 );
    }

    for( ; i < argc && argv[i][0] == '/'; i++ ) {
        switch( toupper( argv[i][1] ) ) {
            case '?':
            case 'H':
                help();
                return( 1 );
            case 'C':
                optflags |= KEEPCA;
                break;
#ifdef FEATURE_CASE
            case 'U':
                optflags |= UPCASE;
                break;
#endif
#ifdef FEATURE_PROMPT
            case 'P':
                optflags |= PROMPT;
                break;
#endif
#ifdef FEATURE_EXECUTE
            case 'E':
                optflags |= EXECUT;
                break;
#endif
            default:
                writeout( "Unknown argument: " );
                writeout( argv[i] );
                writeout( "\r\n" );
                help();
                return( 1 );
        }
    }

    if( ( argc - 1 ) < i ) {
        char **master = _getmenviron( p );
        int j;
        
        for( j = 0; master[ j ] != NULL; j++ ) {
            writeout( master[ j ] );
            writeout( "\r\n");
        }

        free( master );
    }

    ptr = strchr( argv[ i ], '=' );
    if( ptr == NULL
#ifdef FEATURE_PROMPT
        && !( optflags & PROMPT )
#endif
        ) {
        if( !( optflags & KEEPCA ) ) strupr( argv[ i ] );
        writeout( _getmenv( argv[ i ], p ) );
        return( 0 );
    } else if( ptr == NULL ) {
        *varval = '\0';
    } else {
        *ptr++;
        strcpy( varval, &*ptr );
    }

    *ptr--;
    *ptr-- =  NULL;
    strcpy( varname, argv[i] );

    if( !( optflags & KEEPCA ) ) strupr( varname );
#ifdef FEATURE_CASE
    if( optflags & UPCASE ) strupr( varval );
#endif

#ifdef FEATURE_PROMPT
    if( optflags & PROMPT ) {
        if( *varval != '\0' ) writeout( varval );
        hgets( 0, varval );
    }
#endif

#ifdef FEATURE_EXECUTE
    if( optflags & EXECUT ) {
        char *name = tempname( p );
        int oldhandle = redirectout( name );
        int tmphandle;

        spawnlp( P_WAIT, "COMMAND", "COMMAND", "/C", varval );

        unredirectout( oldhandle );
        tmphandle = doopen( name, O_WRONLY );
        doclose( tmphandle );
        tmphandle = doopen( name, O_RDONLY );
        hgets( tmphandle, varval );
        doclose( tmphandle );
        getstdin();
        deletef( name );
    }
#endif

    /* 
     * Environment replacing doesn't work, so to make it work, delete the old
     * variable first
     */
    _masterputenv( p, varname, "" );
    i = _masterputenv( p, varname, varval );
    return( ( i == 1 ) ? 0 : 1 );
}

