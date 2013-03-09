#include "FTE.h"

#include <string.h>
#include <stdio.h>
#include <time.h>
#include <stdlib.h>

#ifdef _WIN32
#undef BOOL
#include <windows.h>
#endif

static char buffer[BYTESPERSECTOR];

int main(int argc, char** argv)
{
    RDWRHandle handle;
    unsigned long i, numsectors;

    FILE* fptr;

    printf("creatimg v0.1 <source> <target>\n");

    if (argc != 3)
        return 1;

    if (strcmp(argv[1], "/?") == 0)
        return 1;

    if (!InitReadWriteSectors(argv[1], &handle))
    {
        printf("Cannot initialise drive %s\n", argv[1]);
        return 1;
    }
    
    fptr = fopen(argv[2], "wb");
    if (!fptr) 
    {
        printf("cannot open destination\n");
        return 1;
    }

    numsectors = GetNumberOfSectors(handle);    

    for (i=0; i < numsectors; i++)
    {
        if (ReadSectors(handle, 1, i, buffer) == -1)
        {
            printf("could not read sector");
            return -1;
        }
    
        if (!fwrite(buffer, BYTESPERSECTOR, 1, fptr) != 1)
        {
            printf("could not write sector");
            return -1;
        }
    }

    fclose(fptr);

    CloseReadWriteSectors(&handle);

    return 0;
}
