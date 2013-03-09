

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


// STRENCC.C - Stream encryption / decryption filter
// (outputs C code)
// Copyright 1998-2000, JP Software Inc., All Rights Reserved

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <io.h>
#include <fcntl.h>
#include <sys\stat.h>
#include "strmenc.h"
#include "brand2.h"

#define KEY_LEN 12
#define LINE_MAX 255

#define NUM_PRODUCTS 7

char *pszProdList[NUM_PRODUCTS] = {
	"BETA",
	"4DOS",
	"4NT",
	"TC32",
	"4DOSRT",
	"4NTRT",
	"TC32RT"
};

unsigned char acProdKeys[NUM_PRODUCTS][KEY_LEN] = {
	{STRKEY_BETA},
	{STRKEY_4DOS},
	{STRKEY_4NT},
	{STRKEY_TC32},
	{STRKEY_4DOSRT},
	{STRKEY_4NTRT},
	{STRKEY_TC32RT}
};

int ReadLine(int, char *);
void WriteText(int, char *);
void error(char *);

// insert a string inside another one
char * strins( char * pszString, char * pszInsert )
{
	register unsigned int uInsertLength;

	// length of insert string
	if (( uInsertLength = strlen( pszInsert )) > 0 ) {

		// move original
		memmove( pszString + uInsertLength, pszString, ( strlen( pszString ) + 1 ));

		// insert the new string into the hole
		memmove( pszString, pszInsert, uInsertLength );
	}

	return pszString;
}


int main(int argc, char **argv) {
	unsigned char szLine[LINE_MAX + 1], szKey[KEY_LEN + 1];
	char szVarName[32], szVarVal[LINE_MAX + 1];
	char szInName[256], szOutName[256], szOutBuf[LINE_MAX + 1];
	char *pExt, *pNewLine;
	int fhIn, fhOut, i, nLen;

	if (argc < 3)
		error("Too few parameters");

	// Copy the key
	if (strnicmp(argv[1], "/p", 2) == 0) {
		// /P means we have a product name
		for (i = 0; i < NUM_PRODUCTS; i++) {
			if (stricmp(argv[1] + 2, pszProdList[i]) == 0) {
				memcpy(szKey, acProdKeys[i], KEY_LEN);
				szKey[KEY_LEN] = '\0';
				break;
			}
			if (i == (NUM_PRODUCTS - 1))
				error("Invalid product name");
		}
	} else {
		// Not a product name, must be the real key
		strcpy(szKey, argv[1]);
		if (strlen(szKey) != KEY_LEN)
			error("Key length error");
	}

	// Get input name
	strcpy(szInName, argv[2]);

	// Get or set output name
	if (argc > 3)
		strcpy(szOutName, argv[3]);
	else {
		strcpy(szOutName, szInName);
		if ((pExt = strrchr(szOutName, '.')) != NULL)
			*pExt = '\0';
		strcat(szOutName, ".h");
	}

	// Open files
	if ((fhIn = _open(szInName, _O_RDONLY | _O_BINARY)) == -1)
		error("Cannot open input file");
	if ((fhOut = _open(szOutName, (_O_CREAT | _O_TRUNC | _O_RDWR | _O_TEXT),
			(_S_IREAD | _S_IWRITE))) == -1)
		error("Cannot open output file");

	// Process each line
	while (ReadLine(fhIn, szLine)) {

		// If it starts with #enc, encrypt it and output original text
		// as a comment and encrypted text as an array
		if (strnicmp(szLine, "#enc ", 5) == 0) {

			// Parse name and value
			if (sscanf(szLine, "%*s %32s \"%255[^\"]\"", szVarName, szVarVal) != 2)
				error("Invalid input data");
	  		nLen = strlen(szVarVal);

			// Output original name and text as comment
			sprintf(szOutBuf, "//    %s = \"%s\"\n", szVarName, szVarVal);

			// Replace ASCII 04s with CR/LF
			for (pNewLine = szVarVal; ((pNewLine = strchr(pNewLine, '\x04')) != NULL); pNewLine += 2 ) {
*pNewLine = '\n';
//				strcpy( pNewLine, pNewLine + 1 );
//				strins( pNewLine, "\r\n" );
			}

			// Encrypt it
	  		EncryptDecrypt(NULL, szKey, szVarVal, nLen);

			// Dump to output, max 64 charactes per line
			WriteText(fhOut, szOutBuf);
			sprintf(szOutBuf, "unsigned char %s[] = {%d, ", szVarName, nLen);
			WriteText(fhOut, szOutBuf);
	  		for (i = 0; i < nLen; i++) {
	  			if ((i > 0) && ((i % 64) == 0))
	  				WriteText(fhOut, "\\\n   ");
	  			sprintf(szOutBuf, "0x%.02X", (unsigned int)szVarVal[i]);
				WriteText(fhOut, szOutBuf);
				if (i != (nLen - 1))
					WriteText(fhOut, ", ");
	  		}
	  		WriteText(fhOut, "};\n");

		} else {
			// Not an encryption line, just copy it to output
			strcat(szLine, "\n");
			WriteText(fhOut, szLine);
		}
	}
	return 0;
}


// Read a line from the input file and remove trailing blanks
int ReadLine(int fhIn, char *szLineBuf) {

	static char acInBuf[LINE_MAX];
	static int nCharsLeft = 0, fCR = 0;
	static char *pInBuf = acInBuf;
	static char szDelims[] = "\r\n\x1A";

	int fBol = 1, nLineLen = 0, nCopy;
	char *pLineBuf = szLineBuf;

	while (1) {
		if (nCharsLeft <= 0) {
			if (eof(fhIn))						// quit if end of file
				if (fBol)
					return 0;					// done if EOF at start of line
				else
					break;						// otherwise return partial line
			if ((nCharsLeft = _read(fhIn, acInBuf, LINE_MAX)) <= 0)
				error("Input read error");
			pInBuf = acInBuf;
		}

		// Skip LF after a previous CR, loop if it's last char
		// in buffer
		if (fCR && (*pInBuf == '\n')) {
			pInBuf++;
			if (--nCharsLeft <= 0)
				continue;
		}

		// Find end of line if any, and copy to end
		for (nCopy = 0; (nCopy < nCharsLeft) && ((nCopy + nLineLen) < LINE_MAX) && (strchr(szDelims, pInBuf[nCopy]) == NULL); nCopy++)
			;
//printf("Copy %d ... ", nCopy);
		memcpy(pLineBuf, pInBuf, nCopy);

		// Update pointers and counts
		pInBuf += nCopy;
		nCharsLeft -= nCopy;
		pLineBuf += nCopy;
		nLineLen += nCopy;

		// If at end of line remember terminating CR, skip
		// terminator, and return line
		if (nCharsLeft > 0) {
			fCR = (*pInBuf == '\r');
			pInBuf++;
			nCharsLeft--;
			break;
		}
	}

	// Remove trailing blanks
	for (pLineBuf--; ((pLineBuf >= szLineBuf) && (*pLineBuf == ' ')); pLineBuf--)
		;
	*(++pLineBuf) = '\0';
//printf("... terminate\n");

	return 1;
}


void WriteText(int fhOut, char *szOutBuf) {
	int nLen = strlen(szOutBuf);

	if (_write(fhOut, szOutBuf, nLen) != nLen)
		error("Write failure");
}

void error(char *szErrText) {
	printf("\nSTRENCC failed -- %s\n", szErrText);
	exit(1);
}

