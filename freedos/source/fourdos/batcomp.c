

/*
  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  (1) The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.

  (2) The Software, or any portion of it, may not be compiled for use on any
  operating system OTHER than FreeDOS without written permission from Rex Conn
  <rconn@jpsoft.com>

  (3) The Software, or any portion of it, may not be used in any commercial
  product without written permission from Rex Conn <rconn@jpsoft.com>

  (4) THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.
*/


/**************************************************************************

   BATCOMP -- 4DOS etc. Batch File Compressor

   Copyright 1993-2004, JP Software Inc., Chestertown, MD, USA.
   All Rights Reserved.

   Author:  Tom Rawson, 8/26/93
   Revised: Rex Conn 11/2/93 (general tweeking & converting to bound app)
		5/31/94 - ignore comments (::)
		2/3/96 - ignore comments (REM) and fix for empty output
			file bug
		11/28/98 - Add encryption for runtime
		09/13/99 - Make kill comments optional
		06/21/00 - Add /Q, set up for 32-bit compile

   Usage:

      BATCOMP [/K /O /Q /Epass] [filename[.ext]]		(runtime version)

		pass		password, up to 4 characters
      filename.ext   name of file to compress

      If the filename is not given BATCOMP will prompt for it.  The output
      file will have the same name with an extension of .BTM.

      The output file format is as follows:

	    Bytes		Contents
	
	     0-1		Signature, 0xEBBE
	     2			Encryption flag, 0x48 if encrypted
		 3-6		Encryption password, if encrypted
		 7-36		Compression table
		 37+		Compressed text

	If encryption is enabled the compression table and compressed text
	are encrypted with the specified password.  The password itself
	is also encrypted, using the base key 'maru' (which is assembled
	here, not stored explicitly).

      The compressed text will contain the following nibble values:

         Nibble = 0:  a byte of text could not be compressed.  The
         next two nibbles of compressed text contain the low and high
         (respectively) nibbles of the uncompressed ASCII text.

         Nibble = 1:  a byte was partially compressed.  The value of the
         next nibble of compressed text, plus 14, is the location of
         the uncompressed ASCII value in the compression table.

         Nibble = 2 - 15:  a byte was fully compressed.  The value of
         this nibble of compressed text, minus 2, is the location of
         the uncompressed ASCII value in the compression table.

      If an error occurs the message "BATCOMP failed" is printed,
      followed by a specific error message.  See the array aszErrorMsgs below
      for the individual messages, which are largely self-explanatory.

***************************************************************************/

#include "product.h"

#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <io.h>
#include <conio.h>
#include <process.h>
#include <sys\types.h>
#include <sys\stat.h>
#include <share.h>
#include <ctype.h>

typedef unsigned char * LPTSTR;

#include "batcomp.h"
#include "strmenc.h"

#define BUFMAX 1024
#define DATAMAX 1024
#define OUTPUTMAX 65536L
#define INPUTMAX 65536L

#define ERR_SAMENAME 1
#define ERR_OVERWRITE 2
#define ERR_INPUT_OPEN 3
#define ERR_OUTPUT_OPEN 4
#define ERR_INPUT_SEEK 5
#define ERR_READ 6
#define ERR_WRITE 7
#define ERR_OUTPUT_SEEK 8
#define ERR_TOO_BIG 9
#define ERR_MAX 10
#define ERR_ALREADY_COMPRESSED 10
#define ERR_UNKNOWN 11

char PROGNAME[] = "BATCOMP";

char COPYRIGHT[] = "\n%s 7.5\nCopyright 1993-2004 JP Software Inc., All Rights Reserved\n";
const char *aszErrorMsgs[] = {
   "Input and output files must have different names",
   "Cannot overwrite existing output file",
   "Cannot open input file",
   "Cannot open output file",
   "Seek error on input file",		//  5
   "Error reading input file",
   "Error writing output file",
   "Seek error on output file",
   "File too large to compress",
   "Input file is already compressed",	// 10
   "Unknown error"
};
#define COMPMSG "\nCompressing %s to %s\n"
#define COMPLINE "Compressing line %u\r"
#define COMPDONE "Compression completed; file compressed to %d%% of original size\n"
#define FAILED "\n%s failed:  %s\n"
#define USAGEMSG "\nUsage:  %s [/Ekkkk /K /O /Q] input_file [output_file]\n    /E encryption key    /K remove comments\n    /O overwrite file\n    /Q quiet\n"
#define COMPWARN "Warning:  %s is not effective at compressing small files!\n"

int _fastcall getline(char *);
void _fastcall strip_trailing(char *);
void _fastcall nibble_out(char);
void data_out( char *, unsigned int, int, int);
void TripleKey(char *);
void usage(void);
void _fastcall error(unsigned int);

CompHead header;

char
	szInputName[260],			// file name to read
	szOutputName[260], 			// file name to write
	szInputBuffer[BUFMAX],		// line input buffer
	szOutputBuffer[DATAMAX]; 	// data output buffer

char
	cEncKey[BC_KEY_LEN],		// encryption key
	cKeyKey[BC_KK_LEN]; 		// key to encrypt user key

char cEncryptSig[4] = ENCRYPT_SIG;

char *dataptr;
char *dataend = szOutputBuffer + DATAMAX;
char *max_one_nib = header.cCommon + 14;
char *out_ext;

int
	in,						// input file handle
	out,					// output file handle
	i, 						// generic
	j, 						// generic
	nib_flag = 0,			// 0 for left half of nibble
	fQuiet = 0,				// quiet mode flag
	fOverwrite = 0,			// overwrite output flag
	fKillComments = 0,		// delete comments flag
	fEncrypt = 0,			// encryption flag
	pass = 1,				// pass 1 or pass 2
	high = 0,				// current high character count
	comp_pct;				// compression percentage

unsigned int uLines;			// line count
unsigned int nTotalBytes = 0; 		// total bytes written to file
unsigned int freqtab[256]; 	  	// character frequency table

// offsets to generate default key -- starting with ' ' (32) these
// offsets generate the key 'Enki' (a cat's name!).  In decimal
// these offsets are 37, 41, -3, -2.
signed char *szDefKeyOffsets = "\x025\x029\x0FD\x0FE";

// offsets to generate key to encrypt user key -- as above --
// these offsets generate the key 'maru' (a dog's name!)
signed char *szKeyKeyOffsets = KKOFFSETS;

long
	lCompressed,			// file already compressed flag
	lTotalInput = 0L, 		// input character count
	lTotalOutput = 0L;		// output character count


int main(int argc, register char **argv)
{
	register char *curchar;
	char cKeyChar;
	int nKeyLen;

	// initialize
//	memset(freqtab, '\0', 256 * sizeof(int));
//	memset(&header, '\0', sizeof(header));
	for (i = 0; (i < 256); i++)
		freqtab[i] = '\0';

	// set default encryption key
	for (i = 0, cKeyChar = ' '; i < BC_MAX_USER_KEY; i++) {
		cKeyChar += (int)szDefKeyOffsets[i];
		cEncKey[i] = cKeyChar;
	}

	header.usSignature = 0xEBBE;	// set compression signature
	szInputName[0] = szOutputName[0] = '\0';

	// check for too few args or "batcomp /?"
	if ((argc == 1) || (stricmp(argv[1],"/?") == 0))
		usage();

	// parse command line
	for (argv++ ; argc > 1; argv++, argc--) {

		if (**argv == '/') {

			switch (toupper((*argv)[1])) {
	
				case 'O':
					fOverwrite = 1;
					continue;

				case 'Q':
					fQuiet = 1;
					continue;

				case 'K':
					fKillComments = 1;
					continue;
	
				case 'E':
					// Set encryption flag, copy key (or as much of it as we need)
					fEncrypt = 1;
					if ((nKeyLen = (int)strlen(&((*argv)[2]))) > 0)
						memcpy(cEncKey, &((*argv)[2]), (nKeyLen > BC_MAX_USER_KEY) ? BC_MAX_USER_KEY : nKeyLen);
					continue;

				default:
					usage();
			}
		}

		if (szInputName[0] == '\0')
			strcpy(szInputName, *argv);
		else if (szOutputName[0] == '\0')
			strcpy(szOutputName, *argv);
		else
			usage();
	}

	if (szInputName[0] == 0)
		usage();

	if (fQuiet == 0)	
		printf(COPYRIGHT, PROGNAME);

	if (szOutputName[0] == 0) {
		strcpy(szOutputName, szInputName);

		// check if it gets too long?
		if ((out_ext = strrchr(szOutputName, '.')) == NULL)
			out_ext = szOutputName + strlen(szOutputName);
		strcpy(out_ext, ".btm");
	}

	if (fQuiet == 0)	
		printf(COMPMSG, szInputName, szOutputName);

	// need full filename expansion here??
	if (stricmp(szInputName, szOutputName) == 0)
		error(ERR_SAMENAME);

	// holler if no overwrite and output exists
	if ((fOverwrite == 0) && (sopen(szOutputName, (O_RDONLY | O_BINARY), SH_DENYNO) != -1))
		error(ERR_OVERWRITE);

	// open input, deny read-write mode in case we get fooled and this is output as well!
	if ((in = sopen(szInputName, (O_RDONLY | O_BINARY), SH_DENYRW)) == -1)
		error(ERR_INPUT_OPEN);

	// rewind & read the file header
	read(in,(void *)&lCompressed,4);

	// check for input file already compressed
	if ((lCompressed & 0xFFFF) == 0xBEEB)
		error(ERR_ALREADY_COMPRESSED);

	// rewind input file
	lseek(in,0L,0);

	// open output
	if ((out = sopen(szOutputName, (O_CREAT | O_TRUNC | O_RDWR | O_BINARY), SH_DENYRW, (S_IREAD | S_IWRITE))) == -1)
		error(ERR_OUTPUT_OPEN);

	// loop for two passes
	for (pass = 1; (pass <= 2); pass++) {

		uLines = 0;									// clear line count

		if (pass == 2) {

			// output (unfinished) header
			if (fEncrypt) {
				// set up encryption key, encrypt common character table
				TripleKey(cEncKey);
				EncryptDecrypt(NULL, (char _far *)cEncKey, (char _far *)header.cCommon, BC_COM_SIZE);
			}

			data_out((char *)(&header), sizeof(header), 0, 0);

			if (fEncrypt) {
				// put common character table back like it was, also
				// leaves encryption system in its post-table state
				// for use in encrypting text stream
				EncryptDecrypt(NULL, (char _far *)cEncKey, (char _far *)header.cCommon, BC_COM_SIZE);

				// output dummy (gibberish) data for encryption key
				data_out((char *)(&freqtab), BC_MAX_USER_KEY, 0, 0);
			}

			// set buffer address
			dataptr = szOutputBuffer;
		}

		// back up to start
		if (lseek(in, 0L, SEEK_SET) == -1L)
			error(ERR_INPUT_SEEK);

		while (getline(szInputBuffer) > 0) {

			// scan or translate line

			// strip trailing whitespace
			strip_trailing(szInputBuffer);

			// skip blank lines and comments
			if (fKillComments && ((szInputBuffer[0] == '\0') || (strnicmp(szInputBuffer,"::",2) == 0)))
				continue;

			// check for REM
			if (fKillComments && ((strnicmp(szInputBuffer, "rem ", 4) == 0) && (strnicmp(szInputBuffer, "rem >", 5) != 0)))
				continue;

			if (fQuiet == 0)	
				printf(COMPLINE, ++uLines);

			curchar = szInputBuffer;
			if (pass == 1) {

				// pass 1, scan the line

				// accum. char counts
				for ( ; (*curchar != '\0'); curchar++)
					freqtab[(int)*curchar]++;

			} else {

				// pass 2, translate the line

				// count line length including CR
				i = (int)strlen(szInputBuffer);
				nTotalBytes += i;
				if ((lTotalInput += (i + 1)) >= 0xFFE0L)
					error(ERR_TOO_BIG);

				// compress & output each character
				for ( ; *curchar; curchar++) {

					char *comptr;

					if ((comptr = memchr(header.cCommon, *curchar, BC_COM_SIZE)) == NULL) {
						// not in table, 3 nibbles
						nibble_out(0);
						nibble_out((char)(*curchar & 0xF));
						nibble_out((char)(*curchar >> 4));
					} else if (comptr >= max_one_nib) {
						// last part of table, 2 nibbles
						nibble_out(1);
						nibble_out((char)(comptr - max_one_nib));
					} else {
						// beginning of table, just one nibble
						nibble_out((char)(comptr - header.cCommon + 2));
					}
				}
			}
		}

		// figure out which are the most common characters
		if (pass == 1) {

			for (j = 0; (j < BC_COM_SIZE); j++) {

				high = 0;
				for (i = 0; (i < 256); i++) {
					if (freqtab[i] > freqtab[high])
						high = i;
				}

				header.cCommon[j] = (char)high;
				// remove this one from count
				freqtab[high] = 0;
			}
		}
	}

	if (nib_flag)									// clear out any extra nibble
		nibble_out(0);

	if (dataptr > szOutputBuffer)			// clean up buffer at end
		data_out(szOutputBuffer, (unsigned int)(dataptr - szOutputBuffer), 1, 1);

	// output encryption signature (password)
	if (fEncrypt)
		data_out(cEncryptSig, 4, 1, 0);

	// adjust header and rewrite
	header.usCount = nTotalBytes;

	// back up to start
	if (lseek(out, 0L, SEEK_SET) == -1L)
		error(ERR_OUTPUT_SEEK);

	// re-encrypt common character table, set encryption flag
	// flag value evaluates to 0 when the two nibbles are XORed
	if (fEncrypt) {
		EncryptDecrypt(NULL, (char _far *)cEncKey, (char _far *)header.cCommon, BC_COM_SIZE);
		header.fEncrypted |= 0xBB;
	} else {

		// Non-runtime version or no encryption ...
	
		// clear encryption flag -- flag value evaluates to
		// non-zero when the two nibbles are XORed
		header.fEncrypted |= 0x8C;
	
		// use quick XOR encryption on common character table
		for (i = (BC_COM_SIZE - 2); i >= 0; i--)
			header.cCommon[i] ^= header.cCommon[i + 1];
	}

	// output final header
	data_out((char *)(&header), sizeof(header), 0, 1);

	// output the encrypted user key
	if (fEncrypt) {

		// build the key to encrypt the user's key, do the
		// encryption, and zap the key (in an odd way)
		for (i = 0, cKeyChar = ' '; i < BC_KK_LEN; i++) {
			cKeyChar += szKeyKeyOffsets[i + 4];
			cKeyKey[i] = cKeyChar;
		}

		TripleKey(cKeyKey);
		EncryptDecrypt(NULL, (unsigned char _far *)cKeyKey, (unsigned char _far *)cEncKey, BC_MAX_USER_KEY);

		for (i = 1; i < BC_KEY_LEN; i++)
			cKeyKey[i] &= cKeyKey[i-1];

		data_out(cEncKey, BC_MAX_USER_KEY, 0, 0);
	}

	close(in);
	close(out);

// printf("\nCompression completed:\n	 %ld characters in\n   %ld characters out\n	 compressed to %d%%\n",
//  lTotalInput, lTotalOutput, (int)((100L * lTotalOutput) / lTotalInput));

	comp_pct = 0;
	if (lTotalInput > 0)
		comp_pct = (int)((100L * lTotalOutput) / lTotalInput);
	if (fQuiet == 0)	
		printf(COMPDONE, comp_pct);

	if (( comp_pct > 95 ) && ( fQuiet == 0 ) && ( fEncrypt == 0 ))
		printf(COMPWARN, PROGNAME);

	return 0;
}


int _fastcall getline(register char *line)
{
	register int maxsize;

	maxsize = read(in,line,BUFMAX);

	// get a line and set the file pointer to the next line
	for (i = 0; ; i++, line++) {

		if ((i >= maxsize) || (*line == 26))
			break;

		if (*line == '\n') {

			// skip a LF following a CR or LF
			if ((++i < maxsize) && (line[1] == '\n'))
				i++;

			break;
		}
	}

	// truncate the line
	*line = '\0';

	// reposition file pointer
	lseek( in, (long)(i - maxsize), SEEK_CUR );

	return i;
}


// remove trailing blanks from a string
void _fastcall strip_trailing(char *buffer)
{
	register char *bufptr = buffer + strlen(buffer);

	while ((--bufptr >= buffer) && ((*bufptr == ' ') || (*bufptr == '\t')))
		*bufptr = '\0';
}


// output one nibble to the data buffer
void _fastcall nibble_out(char nibdata)
{
	if (nib_flag) {
		*dataptr++ |= (nibdata & 0xF);
		if (dataptr >= dataend) {
			data_out(szOutputBuffer, DATAMAX, 1, 1);
			dataptr = szOutputBuffer;
		}
	} else
		*dataptr = (char)(nibdata << 4);

	nib_flag ^= 1; 								// toggle for next time
}


// output data to the output file
void data_out(char *buffer, register unsigned int length, int fEncryptOutput, int fUpdate)
{
	if (fEncryptOutput)
		EncryptDecrypt(NULL, (char _far *)NULL, buffer, length);

	if ((unsigned int)write(out, buffer, length) != length)
		error(ERR_WRITE);

	// count output bytes
	if (fUpdate)
		lTotalOutput += (long)length;
}


// triple up a 4-character key to make it 12 characters
// method is obscure to make hacking more confusing
void TripleKey(char *pKey)
{
	static int nIndexes[4] = {4, 2, 1, 3};
	register int i;

	for (i = 0; i < 4; i++)
		pKey[nIndexes[i] + 7] = pKey[nIndexes[i] + 3] = pKey[nIndexes[i] - 1];
}


// usage error
void usage( void )
{
	if (fQuiet == 0)	
		printf(USAGEMSG, PROGNAME);
	_exit(1);
}


void _fastcall error(unsigned int errnum)
{
	if (errnum > ERR_MAX)
		errnum = ERR_UNKNOWN;
	if (fQuiet == 0)	
		printf(FAILED, PROGNAME, aszErrorMsgs[errnum - 1]);
	_exit(2);
}


// disable MSC null pointer checking
int nullcheck(void)
{
	return 0;
}
