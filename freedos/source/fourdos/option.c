

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


/*
 *************************************************************************
 ** OPTION.C
 *************************************************************************
 ** 4DOS OPTION command
 **
 ** Copyright 1997-2000 JP Software Inc., All rights reserved
 *************************************************************************
 ** Compiler #defines:
 **   __4DOSOPT   Required for correct compile
 **   DEBUG       Adds debugging printf's and control test routine
 **                _TestControls()
 **   STANDALONE  Produces code for test that can be executed without 4DOS
 *************************************************************************
 */

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <malloc.h>
#include <errno.h>
#include <process.h>
#include <i86.h>

#include "general.h"

// 4DOS
#include "product.h"
#include "build.h"
#include "resource.h"  // IDI values
#include "inistruc.h"  // INIFILE definition
INIFILE gaInifile;     // Must be defined before including inifile.h
#include "inifile.h"   // INIItemList structure

#include "general.h"
#include "iniistr.h"
#include "iniipar.h"
#include "dialog.h"
#include "iniio.h"
#include "iniutil.h"
#include "option.h"


/*
 *************************************************************************
 ** Structures
 *************************************************************************
 */
// INI file control data, used to find data in INIItemList
typedef struct {
	unsigned int uINIControlNum;	// index into INIItemList
	unsigned int fINIUpdate;		// type of update if data has been changed
} IDI_CONTROL;

// Data for each directive category
typedef struct {
	char *pszName;
	char *pszHelpContext;
	int	nFirstID;
	int	nLastID;
} CATEGORY;


/*
 *************************************************************************
 ** Internal function prototypes
 *************************************************************************
 */
static int  _ExecuteDialogs(int nCategory);
static int  _InitControl(unsigned int uIDIValue);
static int  _GetControlData(unsigned int uIDIValue);
static int  _WriteControlData(unsigned int uIDIValue, char *pszININame);
static void _MemoryError(char *pszMessage);
static int  _ParseCommandLine(void);

#ifdef DEBUG
static void _TestControls(void);
#endif


/*
 *************************************************************************
 ** Globals
 *************************************************************************
 */
char Copyright[] = " OPTION.EXE (c) Copyright 1997-2002, JP Software Inc., All Rights Reserved ";

char *WinColors[] = {
	"Black",
	"Blue",
	"Green",
	"Cyan",
	"Red",
	"Magenta",
	"Yellow",
	"White",
	"Bright Black",
	"Bright Blue",
	"Bright Green",
	"Bright Cyan",
	"Bright Red",
	"Bright Magenta",
	"Bright Yellow",
	"Bright White"
};

char WinColorDefault[] = "(Default)";

CATEGORY gaCatList[] = {
	{ "Startup", "Startup Options Dialog", IDI_STARTUP_START,
		IDI_STARTUP_END },
	{ "Display", "Display Options Dialog", IDI_DISPLAY_START,
		IDI_DISPLAY_END },
	{ "Command Line 1", "Command Line Options Dialog", IDI_CMDLINE_START,
		IDI_CMDLINE_END },
	{ "Windows", "Windows Dialog", IDI_WINDOWS_START,
		IDI_WINDOWS_END },
	{ "Options 1", "Options 1 Dialog", IDI_CONFIG1_START, IDI_CONFIG1_END },
	{ "Options 2", "Options 2 Dialog", IDI_CONFIG2_START, IDI_CONFIG2_END },
	{ "Commands", "Command Options Dialog", IDI_CMD_START, IDI_CMD_END }
};

#define CATEGORY_COUNT  sizeof(gaCatList) / sizeof(CATEGORY)
#define FIRST_CATEGORY  0  // Begin with the Startup dialog
#define INI_EMPTYSTR    ((unsigned int)-1)
#define INI_SECTION_NAME   "4DOS"
#define HELP_EXE           "4HELP.EXE"


enum {
	OP_NONE = RC_MAX + 1,
	OP_USE,
	OP_SAVE
};

IDI_CONTROL *gpINIControlData;
unsigned int guINIControlCount = (IDI_ID_MAX - IDI_BASE + 1);
INIFILE *gpINIPtr;


/*
 *************************************************************************
 ** MAIN
 *************************************************************************
 */
int main(int argc, char *argv[]) {
	register unsigned int i;
	register unsigned int uIDIValue;
	unsigned int uHoldLong;
	char *pszININame = NULL;
	int nIDIOffsetValue = 0;
	int nFinalOp = 0;
	int nBitByte = 0;
	int nMainRC = RC_OK;
	int fCmdLine = FALSE;
	int fUseINI = FALSE;
#ifndef STANDALONE
	INIFILE _far *fpINIData;
#endif
	unsigned int *puHoldKeys;
	char *psHoldStrData = NULL;
	union REGS reg;

	// Allocate a heap of 64K
	_heapgrow();

#ifdef DEBUG
	printf("DEBUG: Just started- ");
	printf("Mem avail: %u\n",_memavl());
#endif

	// -----------------------------------------------------------------------
	// - Handle address parameter
	// -----------------------------------------------------------------------
#ifndef STANDALONE
	if (argc < 2) {
		// Error
#ifdef DEBUG
		printf("DEBUG: Incorrect # of parameters- %d\n",argc);
#endif
		FatalError(RC_PARM_ERROR, "\nOPTION.EXE cannot be run manually -- you "
		          "must use the 4DOS OPTION command.  \nSee the OPTION "
			  "command in the online help for details.\r\n");
	}

	errno = 0;
	uHoldLong = strtoul(argv[1], NULL, 16);
	if ((errno) || (uHoldLong == 0)) {
		// Error
#ifdef DEBUG
		printf("DEBUG: Could not convert address parameter\n");
#endif
		FatalError(RC_PARM_ERROR, "\nOPTION.EXE cannot be run manually -- you "
		           "must use the 4DOS OPTION command.  \nSee the OPTION "
					  "command in the online help for details.\r\n");
	}

	// Address of 4DOS internal INIFILE structure
	fpINIData = (INIFILE _far *)MAKEP(uHoldLong, 0);
#endif  // STANDALONE


	gpINIPtr = &gaInifile;

	// -----------------------------------------------------------------------
	// - Copy INIFILE structure and verify that 4DOS and OPTION are using the
	// -  same structure
	// -----------------------------------------------------------------------
#ifndef STANDALONE
	// Copy INIFILE structure to near memory
	_fmemmove(gpINIPtr, fpINIData, sizeof(*gpINIPtr));

	// Verify that this is the correct structure and build
	if (gpINIPtr->INISig != 0x4dd4) {
		printf("\nThis version of OPTION.EXE is not compatible "
		       "with your current \nversion of 4DOS.");
		return(RC_SIG_ERROR);
	}
	if (gpINIPtr->INIBuild != VER_BUILD) {
		printf("\nThis version of OPTION.EXE (build #%d) is not compatible "
		       "with \nyour current version of 4DOS (build #%d).",
				 VER_BUILD, gpINIPtr->INIBuild);
		return(RC_BUILD_ERROR);
	}

	// Store pointers that may get changed
	psHoldStrData = gpINIPtr->StrData;
	puHoldKeys = gpINIPtr->Keys;
#else
	// Initialize structure instead
	IniClear(gpINIPtr);
	gpINIPtr->StrMax = 4096;
	gpINIPtr->KeyMax = 16;
	gpINIPtr->StrUsed = 0;
#endif  // STANDALONE

	
	// -----------------------------------------------------------------------
	// - Copy string and key data
	// -----------------------------------------------------------------------
	// Allocate space for string data area
	gpINIPtr->StrData = (char *)malloc(gpINIPtr->StrMax);
	if (gpINIPtr->StrData == NULL)
		_MemoryError("StrData");

#ifndef STANDALONE
	// Copy string data area to near memory
	_fmemmove(gpINIPtr->StrData, (char _far *)fpINIData + sizeof(*fpINIData),
	          gpINIPtr->StrMax);
#endif

	// Allocate space for key data area
	gpINIPtr->Keys = (unsigned int *)malloc(gpINIPtr->KeyMax);
	if (gpINIPtr->Keys == NULL)
		_MemoryError("Keys");

#ifndef STANDALONE
	// Copy key data area to near memory
	_fmemmove(gpINIPtr->Keys, (char _far *)fpINIData + sizeof(*fpINIData) +
	          gpINIPtr->StrMax, gpINIPtr->KeyMax);
#endif

#ifdef DEBUG
	printf("DEBUG: Finished prelim memory allocations- ");
	printf("Mem avail: %u\n",_memavl());
#endif

	// -----------------------------------------------------------------------
	// - Setup array to map dialog IDs to entries in the INI file item list
	// -----------------------------------------------------------------------
	gpINIControlData = (IDI_CONTROL *)malloc(guINIControlCount *
	                                         sizeof(IDI_CONTROL));
	if (gpINIControlData == NULL)
		_MemoryError("INIControlData");

	// Initialize map array
	memset(gpINIControlData, (char)0xFF,
	       guINIControlCount * sizeof(IDI_CONTROL));

	// Index each item in the item list
	for (i = 0; i < guINIItemCount; i++) {
	   if ((nIDIOffsetValue = (int)gaINIItemList[i].uControlID) > 0) {
			nIDIOffsetValue -= IDI_BASE;
		   gpINIControlData[nIDIOffsetValue].uINIControlNum = i;

			// Set the update flag if this value was previously modified
			// Otherwise clear update flag
			nBitByte = nIDIOffsetValue / 8;
			if ((nBitByte < gpINIPtr->OBCount) &&
			    ((gpINIPtr->OptBits[nBitByte] &
				   (1 << (nIDIOffsetValue % 8))) != 0))
			   gpINIControlData[nIDIOffsetValue].fINIUpdate = TRUE;
			else
		   	gpINIControlData[nIDIOffsetValue].fINIUpdate = FALSE;
		}
	}  // End for i

	// -----------------------------------------------------------------------
	// - Parse command line or start dialogs
	// -----------------------------------------------------------------------
	if (argc > 2) {
		// Parse command line
		fCmdLine = TRUE;
		nFinalOp = _ParseCommandLine();
	}
	else {
		// Use dialogs
		nFinalOp = _ExecuteDialogs(FIRST_CATEGORY);
	}

	// -----------------------------------------------------------------------
	// - Switch on final operation to perform: SAVE, USE or CANCEL
	// -----------------------------------------------------------------------
	switch (nFinalOp) {
	case OP_SAVE:
		fUseINI = TRUE;

#ifndef STANDALONE
		if (gpINIPtr->PrimaryININame != INI_EMPTYSTR) {
			// Get INI file name
			pszININame = (char *)malloc(strlen(gpINIPtr->StrData +
			                                   gpINIPtr->PrimaryININame) + 1);
			if (pszININame == NULL)
				_MemoryError("ININame");

			strcpy(pszININame, gpINIPtr->StrData + gpINIPtr->PrimaryININame);
		}
		else {
			printf("\nERROR: OPTION.EXE cannot determine INI file name.  No "
			       "values will be \nwritten to disk.\n");
		}
#else
		pszININame = (char *)malloc(13);
		if (pszININame == NULL)
			_MemoryError("ININame");

		strcpy(pszININame, "TEST.INI");
#endif  // STANDALONE

		if (pszININame) {
			// Save to file control data that has been modified
			for (uIDIValue = IDI_BASE; uIDIValue <= IDI_ID_MAX; uIDIValue++)
				_WriteControlData(uIDIValue, pszININame);

			// Clear the Option bit flags array
			memset(gpINIPtr->OptBits, '\0', gpINIPtr->OBCount);

			free(pszININame);
		}
		break;  // End OP_SAVE

	case OP_USE:
		fUseINI = TRUE;

		if (!fCmdLine) {
			// Save any item update flags in the option bit flags array
	   	for ( i = 0; ( i < guINIItemCount ); i++ ) {
		   	if ((nIDIOffsetValue = (int)gaINIItemList[i].uControlID) > 0) {
					nIDIOffsetValue -= IDI_BASE;
					nBitByte = nIDIOffsetValue / 8;
					if ((nBitByte < gpINIPtr->OBCount) &&
				    	gpINIControlData[nIDIOffsetValue].fINIUpdate)
						gpINIPtr->OptBits[nBitByte] |= (1 << (nIDIOffsetValue % 8));
				}
	   	}
		}  // If !fCmdLine
		break;

	case OP_NONE:
		break;

	default:
		// Error
		nMainRC = nFinalOp;

	}  // End switch

	if ( argc <= 2 ) {

		// kludge to home cursor
		memset((void *)&reg, 0, sizeof(reg));

		reg.h.ah = 2;
		reg.w.dx = 0;
		reg.w.bx = 0;
		int86(0x10, &reg, &reg);
	}

	// -----------------------------------------------------------------------
	// - Prepare to exit
	// -----------------------------------------------------------------------
#ifndef STANDALONE
	// Copy data back to 4DOS' far storage
	if (fUseINI) {
		_fmemmove(fpINIData, gpINIPtr, sizeof(*fpINIData));

		// Restore pointers that may have been changed
		fpINIData->StrData = psHoldStrData;
		fpINIData->Keys = puHoldKeys;

		// Copy string data
		_fmemmove((char _far *)fpINIData + sizeof(*fpINIData),
		          gpINIPtr->StrData, fpINIData->StrMax);

		// Copy key data
		_fmemmove((char _far *)fpINIData + sizeof(*fpINIData) +
		          gpINIPtr->StrMax, gpINIPtr->Keys, fpINIData->KeyMax);
	}
#endif  // STANDALONE

	free(gpINIPtr->StrData);
	free(gpINIPtr->Keys);
	free(gpINIControlData);

#ifdef DEBUG
	printf("DEBUG: End- ");
	printf("Mem avail: %u\n",_memavl());
#endif

	return(nMainRC);
}  // End main


/*
 *************************************************************************
 ** _ParseCommandLine
 *************************************************************************
 ** Parse the command line and use the new directive settings
 *************************************************************************
 */
static int _ParseCommandLine(void) {
	int nArgLen;
	int nLineRC;
	char *pArg;
	char *pNextArg;
	char *pErrMsg;
	char szIniArg[513];


	// Get command line parameters in a space-delimited string
	pArg = getcmd(szIniArg);

	// Jump over pointer info (1st arg)
	pArg += next_token(&pArg, 0, FALSE, "", " ") + 1;

	// Holler if first argument does not start with "//"
	if ((*pArg != '/') || (pArg[1] != '/')) {
		printf("Syntax error:\n  Argument does not begin with \"//\"\n");
		return (RC_ARG_SYNTAX_ERROR);
	}

	// Find each argument that starts with "//" and parse as an
	// INI file line
	while (*pArg != '\0') {
		pArg += 2;
		if ((pNextArg = strstr(pArg, "//")) == NULL)
			pNextArg = (char *)StrEnd(pArg);
		nArgLen = pNextArg - pArg;
		strncpy(szIniArg, pArg, nArgLen);
		szIniArg[nArgLen] = '\0';
#ifdef DEBUG
		printf("DEBUG: Argument - '%s'\n", szIniArg);
#endif  // DEBUG

		nLineRC = IniLine(szIniArg, &gaInifile, FALSE, FALSE, TRUE, &pErrMsg);
		if (nLineRC) {
			pArg[nArgLen] = '\0';

			// Special case for attempted include
			if (nLineRC == -1)
				printf("Error in command-line directive \"%s\":\n"
				       "  Include file not allowed when using OPTION\n",
				       szIniArg, pErrMsg);
			else
#ifdef DEBUG
				printf("Error in command-line directive \"%s\":\n  %s\n",
				       szIniArg, pErrMsg);
#else
				printf("Error in command-line directive \"%s\":\n", szIniArg);
#endif  // DEBUG
// FIXME when we have acceptable error messages for pErrMsg
//				printf("Error in command-line directive \"%s\":\n  %s\n",
//				       szIniArg, pErrMsg);

			return (RC_ARG_BAD_DIRECTIVE);
		}
		pArg = pNextArg;
	}  // End while

	return(OP_USE);
}  // End _ParseCommandLine


/*
 *************************************************************************
 ** _ExecuteDialogs
 *************************************************************************
 ** Build and execute the dialog screens
 *************************************************************************
 */
static int _ExecuteDialogs(int nCategory) {
	int nRC = 0;
	int nDlgRC = 0;
	int nWindowRC = 0;
	int nCurrentCategory = nCategory;
	int nHoldLen = 0;
	int fHelpSet = FALSE;
	register unsigned int uIDIValue;
	char szHelpName[MAXFILENAME];
	char *pszPath;
#ifdef DEBUG
	unsigned int uMemAvail;
	unsigned int uMemAvailStart;
#endif


#ifdef DEBUG
	uMemAvailStart = _memavl();
#endif  // DEBUG

#ifndef STANDALONE
	// Create fully qualified name to help exe
	pszPath = (char *)(gpINIPtr->StrData + gpINIPtr->InstallPath);
	nHoldLen = strlen(pszPath);

	if ((nHoldLen > 0) && ((nHoldLen + strlen(HELP_EXE)) < MAXFILENAME)) {
		strcpy(szHelpName, pszPath);
		if (ConcatDirFileNames(szHelpName, HELP_EXE)) {
#ifdef DEBUG
			printf("DEBUG: Qualified help exe name = '%s'\n", szHelpName);
			getchar();
#endif
			DlgSetHelpExe(szHelpName);
			fHelpSet = TRUE;
		}
	}
#endif  // Not STANDALONE

	// If we could not get the qualified name, send just the EXE name
	if (!fHelpSet)
		DlgSetHelpExe(HELP_EXE);

	// Initialize dialog
	if ((nDlgRC = DlgOpen(guINIControlCount)) != 0) {
		// Error
#ifdef DEBUG
		printf("\nDEBUG: Error #%d on DlgOpen\n", nDlgRC);
		getchar();
#endif
		return(-2);
	}

#ifdef DEBUG
	//printf("DEBUG: Finished DlgOpen- ");
	//printf("Mem avail: %u\n",_memavl());
	//getchar();

	_TestControls();
#endif  // DEBUG

	nWindowRC = nCurrentCategory;

	// -----------------------------------------------------------------------
	// - Main dialog loop
	// -----------------------------------------------------------------------
	while ((nWindowRC <= CATEGORY_MAX) && (nWindowRC >= 0)) {

		nCurrentCategory = nWindowRC;

#ifdef DEBUG
		// printf("DEBUG: About to build window- ");
		// uMemAvail = _memavl();
		// printf("Mem avail: %u\n",uMemAvail);
		// getchar();
#endif  // DEBUG

		// Create dialog screen
		if ((nDlgRC = DlgBuildWindow(nCurrentCategory)) != 0) {
			// Error
#ifdef DEBUG
			printf("\nDEBUG: Error #%d on DlgBuildWindow, window #%d\n",
			       nDlgRC, nCurrentCategory);
			uMemAvail = _memavl();
			printf("Mem avail: %u\n",uMemAvail);
			getchar();
#endif  // DEBUG
			return(-3);
		}

		// Initialize the data in each control
		for (uIDIValue = gaCatList[nCurrentCategory].nFirstID;
		     uIDIValue <= gaCatList[nCurrentCategory].nLastID; uIDIValue++)
			_InitControl(uIDIValue);

		// Display window
		nWindowRC = DlgShowWindow(nCurrentCategory);

		// Save temporary values
		if ((nWindowRC >= 0) && (nWindowRC != B_CANCEL)) {
			for (uIDIValue = gaCatList[nCurrentCategory].nFirstID;
			     uIDIValue <= gaCatList[nCurrentCategory].nLastID;
				  uIDIValue++) {
				_GetControlData(uIDIValue);
			}
		}

		// Close window
		DlgCloseWindow(nCurrentCategory);

#ifdef DEBUG
		//printf("DEBUG: Closed window %d- ", nCurrentCategory);
		//printf("DEBUG: Mem avail: Before build (%u), after close (%u)\n",
		       //uMemAvail, _memavl());
		//getchar();
#endif  // DEBUG
	}
	// -----------------------------------------------------------------------
	// - End main dialog loop
	// -----------------------------------------------------------------------


	// Clean up all remaining dialog stuff
	DlgClose();

#ifdef DEBUG
	if (nWindowRC < 0) {
		// Error
		printf("\nDEBUG: Error #%d on DlgClose, window %d",
		       nWindowRC, nCurrentCategory);
	}
	//printf("\nDEBUG: Mem avail: Before open (%u), after close (%u)\n",
	       //uMemAvailStart, _memavl());
	//getchar();
#endif  // DEBUG

	switch (nWindowRC) {
	case B_USE:
		nRC = OP_USE;
		break;
	case B_SAVE:
		nRC = OP_SAVE;
		break;
	case B_CANCEL:
		nRC = OP_NONE;
		break;
	default:
		nRC = nWindowRC;
	}  // End switch

	return(nRC);
}  // End _ExecuteDialogs


/*
 *************************************************************************
 ** _InitControl
 *************************************************************************
 ** Initialize individual control
 *************************************************************************
 */
static int _InitControl(unsigned int uIDIValue) {
	INI_ITEM *pListElement;
	void *pDataPtr;
	char szBuffer[2];
	char **pValid;
	int nControlIndex = 0;
	int nCtlType = 0;
	int nRadioCnt = 0;
	int nNewFG = 0;
	int nNewBG = 0;
	int nIntDataValue = 0;
	unsigned int uIntDataValue = 0;
	unsigned int uINIItemNum = 0;
	register unsigned int i;

	// Get the item number, initialize the item if it is in use
	nControlIndex = uIDIValue - IDI_BASE;
	uINIItemNum = gpINIControlData[nControlIndex].uINIControlNum;
	pListElement = &gaINIItemList[uINIItemNum];

	if (uINIItemNum != (unsigned int)-1) {

		nCtlType = (int)pListElement->cControlType;
		nRadioCnt = (int)pListElement->cRadioCnt;
		if ((pDataPtr = pListElement->pItemData) != NULL) {
			nIntDataValue = *((int *)pDataPtr);
			uIntDataValue = *((unsigned int *)pDataPtr);
		}

		// Send the ini directive
		DlgSetFieldDirective(nControlIndex, pListElement->pszItemName);

		// Initialize the control
		switch (nCtlType) {
		case INI_CTL_INT:
			DlgSetFieldVal(nControlIndex, nIntDataValue);

			// If there is a range, use it
			if (pListElement->pValidate != NULL)
				DlgSetFieldValRange(nControlIndex,
				                    ((int *)pListElement->pValidate)[0],
				                    ((int *)pListElement->pValidate)[1],
				                    ((int *)pListElement->pValidate)[2]);
			break;

		case INI_CTL_UINT:
			DlgSetFieldVal(nControlIndex, uIntDataValue);

			// If there is a range, use it
			if (pListElement->pValidate != NULL)
				DlgSetFieldValRange(nControlIndex,
				                    ((int *)pListElement->pValidate)[0],
				                    ((int *)pListElement->pValidate)[1],
				                    ((int *)pListElement->pValidate)[2]);
			break;

		case INI_CTL_TEXT:
			// if it's a character make it a string first
			if ((INI_PTMASK & pListElement->cParseType) == INI_CHAR) {
				szBuffer[0] = *((char *)pDataPtr);
				szBuffer[1] = '\0';
				DlgSetFieldString(nControlIndex, szBuffer);
			}
			else {
				if ( uIntDataValue != INI_EMPTYSTR ) {
					DlgSetFieldString(nControlIndex, (char *)
					        (gpINIPtr->StrData + uIntDataValue ));
				}
			}
			break;

		case INI_CTL_CHECK:
			DlgSetFieldVal(nControlIndex, (unsigned int)(*(char *)pDataPtr));
			break;

		case INI_CTL_RADIO:
			DlgSetFieldVal(nControlIndex, (unsigned int)
			               (*(unsigned char *)pDataPtr));
			break;

		case INI_CTL_COMBO:
			// Load list box with contents of V_ list
			pValid = ((TL_HEADER *)(pListElement->pValidate))->elist;
			for (i = 0; i < ((TL_HEADER *)(pListElement->pValidate))->num_entries;
			     i++) {
				DlgSetFieldString(nControlIndex, pValid[i]);
			}

			DlgSetComboDefault(nControlIndex, (unsigned int)(*(char *)pDataPtr));

			break;

		case INI_CTL_COLOR:
			// Load list boxes with color strings (FG and BG are the same)
			DlgSetFieldString(nControlIndex, WinColorDefault);

			for (i = 0; i < 16; i++) {
				DlgSetFieldString(nControlIndex, WinColors[i]);
			}

			// Set current foreground and background colors
			nNewFG = nNewBG = 0;
			if (nIntDataValue != 0) {
				nNewFG = (nIntDataValue & 0xF) + 1;
				nNewBG = ((nIntDataValue >> 4) & 0xF) + 1;
			}
			DlgSetColorDefault(nControlIndex, nNewFG, nNewBG);
			break;
		}  // End switch
	}  // End if (uINIItemNum ...

	return(0);
}  // End _InitControl


/*
 *************************************************************************
 ** _GetControlData
 *************************************************************************
 ** Get data from individual control
 *************************************************************************
 */
static int _GetControlData(unsigned int uIDIValue) {
	void *pDataPtr;
	char szBuffer[512];
	unsigned int *pfUpdate;
	int nNewFG = 0;
	int nNewBG = 0;
	int nCtlType = 0;
	int nRadioCnt = 0;
	int nIntNewValue = 0;
	int nIntDataValue = 0;
	unsigned int uIntDataValue = 0;
	unsigned int uIntNewValue = 0;
	unsigned int uINIItemNum = 0;
	unsigned int uIDIIndex = 0;
	unsigned int fStringChanged = 0;

	// Get the item number, initialize data for the item if it is
	// in use
	uIDIIndex = uIDIValue - IDI_BASE;
	uINIItemNum = gpINIControlData[uIDIIndex].uINIControlNum;
	pfUpdate = &(gpINIControlData[uIDIIndex].fINIUpdate);

	if (uINIItemNum != (unsigned int)-1) {

		nCtlType = (int)gaINIItemList[uINIItemNum].cControlType;
		nRadioCnt = (int)gaINIItemList[uINIItemNum].cRadioCnt;
		if ((pDataPtr = gaINIItemList[uINIItemNum].pItemData) != NULL) {
			nIntDataValue = *((int *)pDataPtr);
			uIntDataValue = *((unsigned int *)pDataPtr);
		}

#ifdef STANDALONE
#ifdef DEBUG
		// When running in debug and standalone mode, we want to make sure
		//  that we see all the changes
		*pfUpdate = TRUE;
#endif
#endif
		
		// Get the data for the control, see if it has changed, and
		// if so store it back in the INI file data structure
		switch (nCtlType) {
		case INI_CTL_INT:
			if (DlgGetFieldVal(uIDIIndex, (unsigned int *)&nIntNewValue) != 0) {
				// Error
#ifdef DEBUG
				printf("Could not get integer value for %u\n", uIDIIndex);
				getchar();
#endif  // DEBUG
			}
			else {
				if (nIntNewValue != nIntDataValue) {
					*((int *)pDataPtr) = nIntNewValue;
					*pfUpdate = TRUE;
				}
			}
			break;

		case INI_CTL_UINT:
			if (DlgGetFieldVal(uIDIIndex, &uIntNewValue) != 0) {
				// Error
#ifdef DEBUG
				printf("Could not get unsigned integer value for %u\n", uIDIIndex);
				getchar();
#endif  // DEBUG
			}
			else {
				if (uIntNewValue != uIntDataValue) {
					*((unsigned int *)pDataPtr) = uIntNewValue;
					*pfUpdate = TRUE;
				}
			}
			break;

		case INI_CTL_TEXT:
			// Get the text from the control
			if (DlgGetFieldString(uIDIIndex, szBuffer, 511) != 0) {
				// Error
#ifdef DEBUG
				printf("Could not get text value for %u\n", uIDIIndex);
				getchar();
#endif  // DEBUG
			}
			else {
				// Handle characters and strings separately
				if ((INI_PTMASK & gaINIItemList[uINIItemNum].cParseType) ==
				    INI_CHAR) {

					if (szBuffer[0] != *((char *)pDataPtr)) {
						*((char *)pDataPtr) = szBuffer[0];
						*pfUpdate = TRUE;
					}

				}
				else {
					// Check if the string changed before we write it out
					fStringChanged = FALSE;

					// If old string is empty then set flag based on whether
					// new string is also empty
					if ((*(unsigned int *)pDataPtr) == INI_EMPTYSTR)
						fStringChanged = (strlen(szBuffer) != 0);

					// if old string is not empty and new string is empty
					// then set flag TRUE
					else if (szBuffer[0] == '\0')
						fStringChanged = TRUE;

					// both strings are not empty, compare them
					else
						fStringChanged =
							(stricmp((gpINIPtr->StrData + *(unsigned int *)pDataPtr),
							         szBuffer) != 0);

					// save the string in the INI file string area if it changed
					if (fStringChanged) {
						ini_string(gpINIPtr, (int *)pDataPtr, szBuffer,
						           strlen(szBuffer));
						*pfUpdate = TRUE;
					}
				}  // End else not character
			}  // End else not error
			break;

		case INI_CTL_CHECK:
			if (DlgGetFieldVal(uIDIIndex, &uIntNewValue) != 0) {
				// Error
#ifdef DEBUG
				printf("Could not get checkbox value for %u\n", uIDIIndex);
				getchar();
#endif  // DEBUG
			}
			else {
				if (uIntNewValue != 0)
					uIntNewValue = 1;
				if ((unsigned char)uIntNewValue != (*(char *)pDataPtr)) {
					(*(unsigned char *)pDataPtr) = (unsigned char)uIntNewValue;
					*pfUpdate = TRUE;
				}
			}
			break;

		case INI_CTL_RADIO:
			if (DlgGetRadioVal(uIDIIndex, &uIntNewValue) != 0) {
				// Error
#ifdef DEBUG
				printf("Could not get radio button value for %u\n", uIDIIndex);
				getchar();
#endif  // DEBUG
			}
			else {
				if ((unsigned char)uIntNewValue != (*(unsigned char *)pDataPtr)) {
					*(unsigned char *)pDataPtr = (unsigned char)uIntNewValue;
					*pfUpdate = TRUE;
				}
			}
			break;

		case INI_CTL_COLOR:
			// Get foreground and background colors
			if (DlgGetColorVal(uIDIIndex, (unsigned int *)&nNewFG,
			                   (unsigned int *)&nNewBG) != 0) {
				// Error
#ifdef DEBUG
				printf("Could not get color value for %u\n", uIDIIndex);
				getchar();
#endif  // DEBUG
			}
			else {
				if ((nNewFG == 0) || (nNewBG == 0))
					nIntNewValue = nNewFG = nNewBG = 0;
				else
					nIntNewValue = (((nNewBG - 1) << 4) | (nNewFG - 1));
				if (nIntNewValue != nIntDataValue) {
					*((int *)pDataPtr) = nIntNewValue;
					*pfUpdate = TRUE;
				}
			}
			break;

		case INI_CTL_COMBO:
			if (DlgGetComboVal(uIDIIndex, &uIntNewValue) != 0) {
				// Error
#ifdef DEBUG
				printf("Could not get combo value for %u\n", uIDIIndex);
				getchar();
#endif  // DEBUG
			}
			else {
				if ((unsigned char)uIntNewValue != (*(unsigned char *)pDataPtr)) {
					*(unsigned char *)pDataPtr = (unsigned char)uIntNewValue;
					*pfUpdate = TRUE;
				}
			}
			break;
		}  // End switch on type
	}  // End if uINIItemNum != 0xFF

	return(0);
}  // End _GetControlData


/*
 *************************************************************************
 ** _WriteControlData
 *************************************************************************
 ** Write data to .ini file from individual control
 *************************************************************************
 */
static int _WriteControlData(unsigned int uIDIValue, char *pszININame) {
	void *pDataPtr;
	char szBuffer[512];
	int nIntDataValue;
	unsigned int uIntDataValue;
	unsigned int uINIItemNum;
	TL_HEADER *pValid;


	uINIItemNum = gpINIControlData[uIDIValue - IDI_BASE].uINIControlNum;

	// If the item is in the INI file and was modified, write it back
	if ((uINIItemNum != (unsigned int)-1) &&
		 gpINIControlData[uIDIValue - IDI_BASE].fINIUpdate) {

		if ((pDataPtr = gaINIItemList[uINIItemNum].pItemData) != NULL) {
			nIntDataValue = *((int *)pDataPtr);
			uIntDataValue = *((int *)pDataPtr);
		}
		pValid = gaINIItemList[uINIItemNum].pValidate;

		// Assume we are writing an empty string
		szBuffer[0] = '\0';

		// Select appropriate writeback string based on INI data type
     	switch (INI_PTMASK & gaINIItemList[uINIItemNum].cParseType) {
		case INI_INT:
			sprintf(szBuffer, "%d", nIntDataValue);
			break;

		case INI_UINT:
			sprintf(szBuffer, "%u", uIntDataValue);
			break;

		case INI_CHAR:
			szBuffer[0] = *((char *)pDataPtr);
			szBuffer[1] = '\0';
			break;

		case INI_STR:
		case INI_PATH:
			if (nIntDataValue != INI_EMPTYSTR)
				strcpy(szBuffer,(char *)(gpINIPtr->StrData + nIntDataValue));
			break;

		case INI_CHOICE:
			strcpy(szBuffer,((pValid->elist)[*((unsigned char *)pDataPtr)]));
			break;

		case INI_COLOR:
			if (nIntDataValue != 0) {
				sprintf(szBuffer,"%s on %s", WinColors[nIntDataValue & 0xF],
				        WinColors[(nIntDataValue >> 4) & 0xF]);
			}
			break;
		}  // End switch

		// Delete the item if it is empty, otherwise write the new value
		if (szBuffer[0] == '\0')
			WriteINIFileStr(INI_SECTION_NAME,
			                gaINIItemList[uINIItemNum].pszItemName,
								 NULL, pszININame);
		else
			WriteINIFileStr(INI_SECTION_NAME,
			                gaINIItemList[uINIItemNum].pszItemName,
								 szBuffer, pszININame);

		// Clear the item's update flag
		gpINIControlData[uIDIValue - IDI_BASE].fINIUpdate = FALSE;

	}  // End if the item is in the INI file and was modified

	return(0);
} // End _WriteDialogData


/*
 *************************************************************************
 ** MemoryError
 *************************************************************************
 ** Print memory error message and exit
 *************************************************************************
 */
static void _MemoryError(char *pszMessage) {

#ifdef DEBUG
	printf("\n\n ERROR: Could not allocate memory for %s.\n", pszMessage);
#endif  // DEBUG

	FatalError(RC_MEM_ERROR, "Out of memory");
}  // End _MemoryError


#ifdef DEBUG
/*
 *************************************************************************
 ** _TestControls
 *************************************************************************
 ** Test each item to see if it has a matching control
 *************************************************************************
 */
static void _TestControls(void) {
	int nTempRC = 0;
	int nIDIOffsetValue = 0;
	int fError = FALSE;
	unsigned int i = 0;


	printf("DEBUG: Testing control #    ");
	for (i = 0; i < guINIItemCount; i++) {
		printf("\b\b\b\b%4.4d", i);
		fflush(stdout);
   	if ((nIDIOffsetValue = (int)gaINIItemList[i].uControlID) > 0) {
			nIDIOffsetValue -= IDI_BASE;
			nTempRC = DlgIndexTest(nIDIOffsetValue,
			                       (unsigned int)gaINIItemList[i].cControlType);

			if (nTempRC == -1) {
				printf("\nDEBUG: A TUI control does not exist for '%s' "
				       "- (IDI = %d)\n",
				       gaINIItemList[i].pszItemName, nIDIOffsetValue);
				getchar();
				fError = TRUE;
				printf("DEBUG: Testing control #    ");
			}

			if (nTempRC == -2) {
				printf("\nDEBUG: The TUI control for '%s' - (IDI = %d) has a "
				       "different type\n",
				       gaINIItemList[i].pszItemName, nIDIOffsetValue);
				getchar();
				fError = TRUE;
				printf("DEBUG: Testing control #    ");
			}

		}
	}  // End for

	if (fError)
		exit(-1);
	else
	{
		printf("\nDEBUG: Each member of the item list has a matching TUI "
		       "control\n");
		getchar();
	}
}  // End _TestControls
#endif  //DEBUG

