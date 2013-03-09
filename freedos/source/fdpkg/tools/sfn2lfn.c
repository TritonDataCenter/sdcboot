#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define LASTCHAR( x ) x[ strlen( x ) - 1 ]

int main( int argc, char **argv )
{
    FILE *filename;
    char longfilename[ 500 ];
    char shortfilename[ _MAX_PATH ];
    char *buffer = NULL;
    
    if( argc < 1 ) return( 1 );
    if( ( filename = fopen( argv[ 1 ], "rt" ) ) == NULL ) return( 1 );

    while( fgets( longfilename, 500, filename ) != NULL ) {
        if( LASTCHAR( longfilename ) == '\n' ) LASTCHAR( longfilename ) = '\0';
        if( longfilename[ 0 ] != '\0' &&
            ( buffer = strstr( longfilename, " -> " ) ) != NULL ) {
            strcpy( shortfilename, &buffer[ 4 ] );
            buffer[ 0 ] = '\0';
            rename( shortfilename, longfilename );
        }
    }
}
