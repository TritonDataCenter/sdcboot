#ifndef MKHELP_H_
#define MKHELP_H_

#define VERSION "0.2"

#define TEMPFILE "MKHELP.TMP"

int CreateHelpFile(char* listfile, char* helpfile);

/* Compression not currently supported.
int CompressFile(char* infile, char* outfile);
*/

#endif
