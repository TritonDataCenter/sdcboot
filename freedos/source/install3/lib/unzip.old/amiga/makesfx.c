/* MakeSFX: join UnZipSFX and a .zip archive into a single self-extracting   */
/* Amiga program.  On most systems simple concatenation does the job but for */
/* the Amiga a special tool is needed.  By Paul Kienitz, no rights reserved. */
/* This program is written portably, so if anyone really wants to they can   */
/* produce Amiga self-extracting programs on a non-Amiga.  We are careful    */
/* not to mix Motorola-format longwords read from files with native long     */
/* integers.  Not necessarily limited to use with only the Zip format --     */
/* just combine any archive with any self-extractor program that is capable  */
/* of reading a HUNK_DEBUG section at the end as an archive.                 */

#include <stat.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

typedef unsigned long ulong;
typedef unsigned short bool;
#define false 0
#define true  1

/* the following are extracted from Commodore include file dos/doshunks.h: */
#define HUNK_NAME       1000
#define HUNK_CODE       1001
#define HUNK_DATA       1002
#define HUNK_BSS        1003
#define HUNK_RELOC32    1004
#define HUNK_SYMBOL     1008
#define HUNK_DEBUG      1009
#define HUNK_END        1010
#define HUNK_HEADER     1011
#define HUNK_OVERLAY    1013
#define HUNK_BREAK      1014

/* Convert a big-endian (Motorola) sequence of four bytes to a longword: */
#define CHARS2LONG(b)   (((b)[0]<<24)|((b)[1]<<16)|((b)[2]<<8)|(b)[3])
/* b must be (unsigned char *) in the above.  Now the reverse: */
#define LONG2CHARS(b,l) ((b)[0]=(l)>>24,(b)[1]=(l)>>16,(b)[2]=(l)>>8,(b)[3]=l)

#define COPYBUFFER      16384


bool CopyData(FILE *out, FILE *inn, ulong archivesize,
              char *outname, char *inname)
{
    static unsigned char buf[COPYBUFFER];
    long bufend, written = 0;

    if (archivesize) {
        LONG2CHARS(buf, HUNK_DEBUG);
        bufend = (archivesize + 3) / 4;
        LONG2CHARS(buf + 4, (ulong) bufend);
        if (fwrite(buf, 1, 8, out) < 8) {
            printf("Error writing in-between data to %s\n", outname);
            return false;
        }
    }
    do {
        bufend = fread(buf, 1, COPYBUFFER, inn);
        if (ferror(inn)) {
            printf("Error reading data from %s\n", inname);
            return false;
        }
        if (!archivesize && !written) {   /* true only for first block read */
            if (CHARS2LONG(buf) != HUNK_HEADER) {
                printf("%s is not an Amiga executable.\n", inname);
                return false;
            }
        }
        if (fwrite(buf, 1, bufend, out) < bufend) {
            printf("Error writing %s to %s\n", archivesize ? "archive data" :
                                               "self-extractor code", outname);
            return false;
        }
        written += bufend;
    } while (!feof(inn));
    if (archivesize) {
        if (written != archivesize) {
            printf("Wrong number of bytes copied from archive %s\n", outname);
            return false;
        }
        LONG2CHARS(buf, 0);
        bufend = 3 - (written + 3) % 4;
        LONG2CHARS(buf + bufend, HUNK_END);
        bufend += 4;
        if (fwrite(buf, 1, bufend, out) < bufend) {
            printf("Error writing end-marker data to %s\n", outname);
            return false;
        }
    }
    return true;
}


void main(int argc, char **argv)
{
    FILE *out, *arch, *tool;
    char *toolname = argv[3];
    struct stat ss;
    int ret;
    ulong archivesize;

    if (argc < 3 || argc > 4) {
        printf("Usage: %s <result-file> <zip-archive> [<self-extractor-"
               "program>]\nThe third arg defaults to \"UnZipSFX\" in the"
               " current dir.\n", argv[0]);
        exit(20);
    }
    if (!(arch = fopen(argv[2], "rb"))) {
        printf("Could not find archive file %s\n", argv[2]);
        exit(10);
    }
    if (stat(argv[2], &ss) || !(archivesize = ss.st_size)) {
        fclose(arch);
        printf("Could not check size of archive %s, or file is empty.\n",
               argv[2]);
        exit(10);
    }
    if (argc < 4)
        toolname = "UnZipSFX";
    if (!(tool = fopen(toolname, "rb"))) {
        fclose(arch);
        printf("Could not find self-extractor program %s\n", toolname);
        exit(10);
    }
    if (!(out = fopen(argv[1], "wb"))) {
        fclose(arch);
        fclose(tool);
        printf("Could not create output file %s\n", argv[1]);
        exit(10);
    }
    ret = CopyData(out, tool, 0, argv[1], toolname)
          && CopyData(out, arch, archivesize, argv[1], argv[2]) ? 0 : 10;
    fclose(out);
    fclose(arch);
    fclose(tool);
    if (ret) {
        printf("Deleting %s\n", argv[1]);
        remove(argv[1]);
    } else
        printf("%s successfully written.\n", argv[1]);
    exit(ret);
}


#if (defined(AZTEC_C) && defined(MCH_AMIGA))
void _wb_parse(void) { }
#endif
