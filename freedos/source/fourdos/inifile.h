

//  Permission is hereby granted, free of charge, to any person obtaining a copy
//  of this software and associated documentation files (the "Software"), to deal
//  in the Software without restriction, including without limitation the rights
//  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
//  copies of the Software, and to permit persons to whom the Software is
//  furnished to do so, subject to the following conditions:
//
//  (1) The above copyright notice and this permission notice shall be included in all
//  copies or substantial portions of the Software.
//
//  (2) The Software, or any portion of it, may not be compiled for use on any
//  operating system OTHER than FreeDOS without written permission from Rex Conn
//  <rconn@jpsoft.com>
//
//  (3) The Software, or any portion of it, may not be used in any commercial
//  product without written permission from Rex Conn <rconn@jpsoft.com>
//
//  (4) THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
//  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
//  SOFTWARE.


// INIFILE.H -- Include file for INI file and key parsing structures, macros,
// constants, and strings
//    Copyright 1992-2002 JP Software Inc., All Rights Reserved


// Property pages
#define STARTUP_PP	1
#define WINDOWS_PP	2
#define EDITING_PP	3
#define COLORS_PP	4
#define HISTORY_PP	5
#define SYNTAX_PP	6
#define INTERNET_PP 7
#define MISC_PP		8
#define FONT_PP		9
#define CAVEMAN_PP	10


// Control types for controls in INI file dialog boxes
#define INI_CTL_NULL     0
#define INI_CTL_INT      1
#define INI_CTL_UINT     2
#define INI_CTL_TEXT     3
#define INI_CTL_CHECK    4
#define INI_CTL_RADIO    5
#define INI_CTL_COLOR    6
#define INI_CTL_BOX      7
#define INI_CTL_TAB      8
#define INI_CTL_STATIC   9
#define INI_CTL_BUTTON   10
#define INI_CTL_COMBO    11

#define MAX_INI_NEST 3

// INI file item list
typedef struct {
	LPTSTR pszItemName;
	unsigned int cParseType;
	unsigned int uDefValue;
	void *pValidate;
	void *pItemData;

	unsigned int nPPage;
	unsigned int uControlID;
	unsigned char cControlType;
	unsigned char cRadioCnt;
} INI_ITEM;

// ASCII and scan codes for all keys

#define K_SCAN    EXTENDED_KEY
#define K_CtlAt   0x00
#define K_Null    K_CtlAt
#define K_CtlA    0x01
#define K_CtlB    0x02
#define K_CtlC    0x03
#define K_CtlD    0x04
#define K_CtlE    0x05
#define K_CtlF    0x06
#define K_CtlG    0x07
#define K_CtlH    0x08
#define K_Bksp    K_CtlH
#define K_CtlI    0x09
#define K_Tab     K_CtlI
#define K_CtlJ    0x0A
#define K_LF      K_CtlJ
#define K_CtlEnt  K_CtlJ
#define K_CtlK    0x0B
#define K_CtlL    0x0C
#define K_CtlM    0x0D
#define K_Enter   K_CtlM
#define K_CR      K_CtlM
#define K_CtlN    0x0E
#define K_CtlO    0x0F
#define K_CtlP    0x10
#define K_CtlQ    0x11
#define K_CtlR    0x12
#define K_CtlS    0x13
#define K_CtlT    0x14
#define K_CtlU    0x15
#define K_CtlV    0x16
#define K_CtlW    0x17
#define K_CtlX    0x18
#define K_CtlY    0x19
#define K_CtlZ    0x1A
#define K_Esc     0x1B
#define K_FS      0x1C
#define K_GS      0x1D
#define K_RS      0x1E
#define K_US      0x1F
#define K_Space   0x20
#define K_Excl    0x21
#define K_DQuote  0x22
#define K_Pound   0x23
#define K_Dollar  0x24
#define K_Pct     0x25
#define K_And     0x26
#define K_SQuote  0x27
#define K_LPar    0x28
#define K_RPar    0x29
#define K_Star    0x2A
#define K_Plus    0x2B
#define K_Comma   0x2C
#define K_Dash    0x2D
#define K_Period  0x2E
#define K_Slash   0x2F
#define K_0       0x30
#define K_1       0x31
#define K_2       0x32
#define K_3       0x33
#define K_4       0x34
#define K_5       0x35
#define K_6       0x36
#define K_7       0x37
#define K_8       0x38
#define K_9       0x39
#define K_Colon   0x3A
#define K_Semi    0x3B
#define K_LAngle  0x3C
#define K_Equal   0x3D
#define K_RAngle  0x3E
#define K_Quest   0x3F
#define K_AtSign  0x40
#define K_A       0x41
#define K_B       0x42
#define K_C       0x43
#define K_D       0x44
#define K_E       0x45
#define K_F       0x46
#define K_G       0x47
#define K_H       0x48
#define K_I       0x49
#define K_J       0x4A
#define K_K       0x4B
#define K_L       0x4C
#define K_M       0x4D
#define K_N       0x4E
#define K_O       0x4F
#define K_P       0x50
#define K_Q       0x51
#define K_R       0x52
#define K_S       0x53
#define K_T       0x54
#define K_U       0x55
#define K_V       0x56
#define K_W       0x57
#define K_X       0x58
#define K_Y       0x59
#define K_Z       0x5A
#define K_LSqr    0x5B
#define K_BSlash  0x5C
#define K_RSqr    0x5D
#define K_Caret   0x5E
#define K_Under   0x5F
#define K_BQuote  0x60
#define K_a       0x61
#define K_b       0x62
#define K_c       0x63
#define K_d       0x64
#define K_e       0x65
#define K_f       0x66
#define K_g       0x67
#define K_h       0x68
#define K_i       0x69
#define K_j       0x6A
#define K_k       0x6B
#define K_l       0x6C
#define K_m       0x6D
#define K_n       0x6E
#define K_o       0x6F
#define K_p       0x70
#define K_q       0x71
#define K_r       0x72
#define K_s       0x73
#define K_t       0x74
#define K_u       0x75
#define K_v       0x76
#define K_w       0x77
#define K_x       0x78
#define K_y       0x79
#define K_z       0x7A
#define K_LBrace  0x7B
#define K_Bar     0x7C
#define K_RBrace  0x7D
#define K_Tilde   0x7E
#define K_CtlBS   0x7F
#define K_255     0xFF
#define K_AltA    0x1E + K_SCAN
#define K_AltB    0x30 + K_SCAN
#define K_AltC    0x2E + K_SCAN
#define K_AltD    0x20 + K_SCAN
#define K_AltE    0x12 + K_SCAN
#define K_AltF    0x21 + K_SCAN
#define K_AltG    0x22 + K_SCAN
#define K_AltH    0x23 + K_SCAN
#define K_AltI    0x17 + K_SCAN
#define K_AltJ    0x24 + K_SCAN
#define K_AltK    0x25 + K_SCAN
#define K_AltL    0x26 + K_SCAN
#define K_AltM    0x32 + K_SCAN
#define K_AltN    0x31 + K_SCAN
#define K_AltO    0x18 + K_SCAN
#define K_AltP    0x19 + K_SCAN
#define K_AltQ    0x10 + K_SCAN
#define K_AltR    0x13 + K_SCAN
#define K_AltS    0x1F + K_SCAN
#define K_AltT    0x14 + K_SCAN
#define K_AltU    0x16 + K_SCAN
#define K_AltV    0x2F + K_SCAN
#define K_AltW    0x11 + K_SCAN
#define K_AltX    0x2D + K_SCAN
#define K_AltY    0x15 + K_SCAN
#define K_AltZ    0x2C + K_SCAN

#define K_Alt0    0x81 + K_SCAN
#define K_Alt1    0x78 + K_SCAN
#define K_Alt2    0x79 + K_SCAN
#define K_Alt3    0x7A + K_SCAN
#define K_Alt4    0x7B + K_SCAN
#define K_Alt5    0x7C + K_SCAN
#define K_Alt6    0x7D + K_SCAN
#define K_Alt7    0x7E + K_SCAN
#define K_Alt8    0x7F + K_SCAN
#define K_Alt9    0x80 + K_SCAN

#define K_AltBS   0x0E + K_SCAN
#define K_AltTab  0xA5 + K_SCAN
#define K_ShfTab  0x0F + K_SCAN
#define K_CtlTab  0x94 + K_SCAN
#define K_Left    0x4B + K_SCAN
#define K_CtlLft  0x73 + K_SCAN
#define K_Right   0x4D + K_SCAN
#define K_CtlRt   0x74 + K_SCAN
#define K_Up      0x48 + K_SCAN
#define K_CtlUp   0x8D + K_SCAN
#define K_Down    0x50 + K_SCAN
#define K_CtlDn   0x91 + K_SCAN
#define K_Home    0x47 + K_SCAN
#define K_CtlHm   0x77 + K_SCAN
#define K_End     0x4F + K_SCAN
#define K_CtlEnd  0x75 + K_SCAN
#define K_PgUp    0x49 + K_SCAN
#define K_CtlPgU  0x84 + K_SCAN
#define K_PgDn    0x51 + K_SCAN
#define K_CtlPgD  0x76 + K_SCAN
#define K_Ins     0x52 + K_SCAN
#define K_Del     0x53 + K_SCAN
#define K_CtlIns  0x92 + K_SCAN
#define K_CtlDel  0x93 + K_SCAN
#define K_F1      0x3B + K_SCAN
#define K_F2      0x3C + K_SCAN
#define K_F3      0x3D + K_SCAN
#define K_F4      0x3E + K_SCAN
#define K_F5      0x3F + K_SCAN
#define K_F6      0x40 + K_SCAN
#define K_F7      0x41 + K_SCAN
#define K_F8      0x42 + K_SCAN
#define K_F9      0x43 + K_SCAN
#define K_F10     0x44 + K_SCAN
#define K_F11     0x85 + K_SCAN
#define K_F12     0x86 + K_SCAN
#define K_ShfF1   0x54 + K_SCAN
#define K_ShfF2   0x55 + K_SCAN
#define K_ShfF3   0x56 + K_SCAN
#define K_ShfF4   0x57 + K_SCAN
#define K_ShfF5   0x58 + K_SCAN
#define K_ShfF6   0x59 + K_SCAN
#define K_ShfF7   0x5A + K_SCAN
#define K_ShfF8   0x5B + K_SCAN
#define K_ShfF9   0x5C + K_SCAN
#define K_ShfF10  0x5D + K_SCAN
#define K_ShfF11  0x87 + K_SCAN
#define K_ShfF12  0x88 + K_SCAN
#define K_CtlF1   0x5E + K_SCAN
#define K_CtlF2   0x5F + K_SCAN
#define K_CtlF3   0x60 + K_SCAN
#define K_CtlF4   0x61 + K_SCAN
#define K_CtlF5   0x62 + K_SCAN
#define K_CtlF6   0x63 + K_SCAN
#define K_CtlF7   0x64 + K_SCAN
#define K_CtlF8   0x65 + K_SCAN
#define K_CtlF9   0x66 + K_SCAN
#define K_CtlF10  0x67 + K_SCAN
#define K_CtlF11  0x89 + K_SCAN
#define K_CtlF12  0x8A + K_SCAN
#define K_AltF1   0x68 + K_SCAN
#define K_AltF2   0x69 + K_SCAN
#define K_AltF3   0x6A + K_SCAN
#define K_AltF4   0x6B + K_SCAN
#define K_AltF5   0x6C + K_SCAN
#define K_AltF6   0x6D + K_SCAN
#define K_AltF7   0x6E + K_SCAN
#define K_AltF8   0x6F + K_SCAN
#define K_AltF9   0x70 + K_SCAN
#define K_AltF10  0x71 + K_SCAN
#define K_AltF11  0x8B + K_SCAN
#define K_AltF12  0x8C + K_SCAN


// Definitions for INI file parsing (INIPARSE.C)

// INI file parsing types
#define INI_CHAR      0
#define INI_INT       1
#define INI_UINT      2
#define INI_CHOICE    3
#define INI_COLOR     4
#define INI_KEY       5
#define INI_STR       6
#define INI_PATH      7
#define INI_KEY_MAP   8
#define INI_INCLUDE   9

#define INI_PTMASK   0x7F
#define INI_NOMOD		0x80

#define VNULL (void *)0


// Data for INI file parsing (INIPARSE.C)

#ifdef INIPARSE

// Validation ranges, token lists, and functions for INI file items

static int V_HstRng[] = {256, 8192, 128};       // history range
static int V_DirHstRng[] = {256, 2048, 128};    // directory history range
static int V_CurRng[] = {0, 100, 1};            // cursor shape range
static int V_DesRng[] = {20, 512, 10};          // description max range
static int V_AliasRng[] = {256, 32767, 128};    // Alias range
static int V_EnvRng[] = {160, 32767, 128};      // Environment range
static int V_EnvFreeRng[] = {128, 32767, 128};  // Environment free range
static int V_HMRng[] = {0, 256, 1};             // history minimum save range
static int V_StackRng[] = {8192, 16384, 128};   // stack size range

static int V_FuzzyCD[] = {0, 3};				// Fuzzy completion style
static int V_BaseRng[] = {0, 1, 1};             // Base 0 or 1 for 1st element
static int V_EvalRng[] = {0, 10, 1};            // @EVAL precision range
static int V_BFRng[] = {0, 20000, 1000};        // beep frequency range
static int V_BLRng[] = {0, 54, 1};              // beep length range
static int V_RCRng[] = {0, 2048, 1};            // row / column count range
static int V_Tabs[] = {1, 32, 1};	 	        // tabstops


static TCHAR *YNList[] = {_TEXT("No"), _TEXT("Yes")};      // yes or no
TOKEN_LIST(V_YesNo, YNList);

static TCHAR *YNAList[] = {_TEXT("Auto"), _TEXT("Yes"), _TEXT("No")};      // yes, no, or auto
TOKEN_LIST(V_YesNoAuto, YNAList);

static TCHAR *OnOff[] = {_TEXT("Off"), _TEXT("On")};      // off or on
TOKEN_LIST(V_OnOff, OnOff);
                                    // edit modes
static TCHAR *EMList[] = {_TEXT("Overstrike"), _TEXT("Insert"), _TEXT("InitOverstrike"), _TEXT("InitInsert")};
TOKEN_LIST(V_EMList, EMList);
                                    // window states
static TCHAR *WState[] = {_TEXT("Standard"), _TEXT("Maximize"), _TEXT("Minimize"), _TEXT("Custom")};
TOKEN_LIST(V_WState, WState);

static TCHAR *HDups[] = {_TEXT("Off"), _TEXT("First"), _TEXT("Last")};
TOKEN_LIST(V_HistDups, HDups);

static TCHAR *DTList[] = {_TEXT("Auto"), _TEXT("."), _TEXT(",")};      // decimal / thousands chars
TOKEN_LIST(V_DTChar, DTList);

static TCHAR *ServComp[] = { _TEXT("None"), _TEXT("Local"), _TEXT("Global")};
TOKEN_LIST(V_SComp, ServComp);


static TCHAR *SecNames[] = {_TEXT("4DOS"), _TEXT("Primary"), _TEXT("Secondary")};  // section names

// FIXME - Is this stuff used in OPTION???
#if _NT || _OPTION
TOKEN_LIST(SectionNames, SecNames);
#endif

static TCHAR *UMBOpts[] = {"No", "Yes", "1", "2", "3", "4", "5", "6", "7", "8"};      
TOKEN_LIST(V_UMBOpts, UMBOpts);


// Take Command dialog box control number macros
//    Embed Dialog ID in high byte, control type in third nibble, and
//    ID count for radio buttons in fourth nibble.
//#define DLG_DATA(CtlID, CtlType, RadioCnt) ((unsigned int)(((CtlID - IDI_BASE) << 8) | (CtlType << 4) | RadioCnt)), 
#define DLG_DATA(CtlID, CtlType, RadioCnt) (unsigned int)CtlID,  CtlType, RadioCnt,
#define DLG_NULL 0, 0, 0,


INI_ITEM gaINIItemList[] = {

	// All products
	_TEXT("AmPm"), INI_CHOICE, 2, &V_YesNoAuto, &gaInifile.TimeFmt, SYNTAX_PP, DLG_DATA(IDI_AmPm, INI_CTL_RADIO, 3)

	"ANSI", INI_CHOICE, 0, &V_YesNoAuto, &gaInifile.ANSI, 0, DLG_DATA(IDI_ANSI, INI_CTL_RADIO, 3)

	_TEXT("AppendToDir"), INI_CHOICE, 0, &V_YesNo, &gaInifile.AppendDir, EDITING_PP, DLG_DATA(IDI_AppendToDir, INI_CTL_CHECK, 0)
	//"Base"), INI_UINT, 0, &V_BaseRng, &gaInifile.Base, DLG_NULL
	_TEXT("BatchEcho"), INI_CHOICE, 1, &V_YesNo, &gaInifile.BatEcho, STARTUP_PP, DLG_DATA(IDI_BatchEcho, INI_CTL_CHECK, 0)
	_TEXT("BeepFreq"), INI_UINT, 440, &V_BFRng, &gaInifile.BeepFreq, SYNTAX_PP, DLG_DATA(IDI_BeepFreq, INI_CTL_INT, 0)
	_TEXT("BeepLength"), INI_UINT, 2, &V_BLRng, &gaInifile.BeepDur, SYNTAX_PP, DLG_DATA(IDI_BeepLength, INI_CTL_INT, 0)

	_TEXT("CDDWinLeft"), INI_UINT, 3, &V_RCRng, &gaInifile.CDDLeft, HISTORY_PP, DLG_DATA(IDI_CDDWinLeft, INI_CTL_INT, 0)
	_TEXT("CDDWinTop"), INI_UINT, 3, &V_RCRng, &gaInifile.CDDTop, HISTORY_PP, DLG_DATA(IDI_CDDWinTop, INI_CTL_INT, 0)
	_TEXT("CDDWinWidth"), INI_UINT, 72, &V_RCRng, &gaInifile.CDDWidth, HISTORY_PP, DLG_DATA(IDI_CDDWinWidth, INI_CTL_INT, 0)
	_TEXT("CDDWinHeight"), INI_UINT, 16, &V_RCRng, &gaInifile.CDDHeight, HISTORY_PP, DLG_DATA(IDI_CDDWinHeight, INI_CTL_INT, 0)
	_TEXT("ClearKeyMap"), INI_KEY_MAP, 0, VNULL, NULL, 0, DLG_NULL
	_TEXT("ColorDir"), INI_STR, 0, VNULL, &gaInifile.DirColor, COLORS_PP, DLG_DATA(IDI_ColorDir, INI_CTL_TEXT, 0)
	_TEXT("CompleteHidden"), INI_CHOICE, 0, &V_YesNo, &gaInifile.CompleteHidden, EDITING_PP, DLG_DATA(IDI_CompleteHidden,INI_CTL_CHECK,0)
	_TEXT("CopyPrompt"), INI_CHOICE, 0, &V_YesNo, &gaInifile.CopyPrompt, STARTUP_PP, DLG_DATA(IDI_CopyPrompt, INI_CTL_CHECK,0)

	_TEXT("Debug"), INI_UINT, 0, VNULL, &gaInifile.INIDebug, 0, DLG_NULL
	_TEXT("DecimalChar"), INI_CHOICE, 0, &V_DTChar, &gaInifile.DecimalChar, SYNTAX_PP, DLG_DATA(IDI_DecimalChar, INI_CTL_RADIO, 3)
	_TEXT("DescriptionMax"), INI_UINT, 512, &V_DesRng, &gaInifile.DescriptMax, MISC_PP, DLG_DATA(IDI_DescriptionMax, INI_CTL_INT, 0)
	_TEXT("DescriptionName"), INI_STR, 0, VNULL, &gaInifile.DescriptName, MISC_PP, DLG_NULL
	_TEXT("Descriptions"), INI_CHOICE, 1, &V_YesNo, &gaInifile.Descriptions, MISC_PP, DLG_DATA(IDI_Descriptions, INI_CTL_CHECK, 0)
	// Kludge - we use two entries here so that changes made by OPTION
	// get into the .INI file with the correct name, but do not affect the
	// values for the current session
	_TEXT("DirHistory"), (INI_UINT | INI_NOMOD), 1024, &V_DirHstRng, &gaInifile.DirHistorySize, HISTORY_PP, DLG_NULL
	_TEXT("DirHistory"), INI_UINT, 1024, &V_DirHstRng, &gaInifile.DirHistoryNew, HISTORY_PP, DLG_DATA(IDI_DirHistory, INI_CTL_INT, 0)
	//"ErrorColors"), INI_COLOR, 0, VNULL, &gaInifile.ErrorColor, DLG_NULL
	_TEXT("EvalMax"), INI_UINT, 10, &V_EvalRng, &gaInifile.EvalMax, MISC_PP, DLG_DATA(IDI_EvalMax, INI_CTL_INT, 0)
	_TEXT("EvalMin"), INI_UINT, 0, &V_EvalRng, &gaInifile.EvalMin, MISC_PP, DLG_DATA(IDI_EvalMin, INI_CTL_INT, 0)
	_TEXT("FileCompletion"), INI_STR, 0, VNULL, &gaInifile.FC, EDITING_PP, DLG_DATA(IDI_FileCompletion, INI_CTL_TEXT, 0)
	_TEXT("FuzzyCD"), INI_UINT, 0, &V_FuzzyCD, &gaInifile.FuzzyCD, MISC_PP, DLG_DATA(IDI_FuzzyCD, INI_CTL_RADIO, 4)
	_TEXT("HistMin"), INI_UINT, 0, &V_HMRng, &gaInifile.HistMin, HISTORY_PP, DLG_DATA(IDI_HistMin, INI_CTL_INT, 0)
	// Kludge - see note under DirHistory directive above
	_TEXT("History"), (INI_UINT | INI_NOMOD), 2048, &V_HstRng, &gaInifile.HistorySize, HISTORY_PP, DLG_NULL
	_TEXT("History"), INI_UINT, 2048, &V_HstRng, &gaInifile.HistoryNew, HISTORY_PP, DLG_DATA(IDI_History, INI_CTL_INT, 0)
	_TEXT("HistCopy"), INI_CHOICE, 0, &V_YesNo, &gaInifile.HistoryCopy, HISTORY_PP, DLG_DATA(IDI_HistCopy, INI_CTL_CHECK, 0)
	_TEXT("HistDups"), INI_CHOICE, 0, &V_HistDups, &gaInifile.HistoryDups, HISTORY_PP, DLG_DATA(IDI_HistDups, INI_CTL_RADIO, 3)
	_TEXT("HistMove"), INI_CHOICE, 0, &V_YesNo, &gaInifile.HistoryMove, HISTORY_PP, DLG_DATA(IDI_HistMove, INI_CTL_CHECK, 0)
	_TEXT("HistLogName"), INI_PATH, 0, (void *)0x4000, &gaInifile.HistLogName, STARTUP_PP, DLG_DATA(IDI_HistLogName, INI_CTL_TEXT, 0)
	_TEXT("HistLogOn"), INI_CHOICE, 0, &V_YesNo, &gaInifile.HistLogOn, STARTUP_PP, DLG_DATA(IDI_HistLogOn, INI_CTL_CHECK, 0)
	_TEXT("HistWrap"), INI_CHOICE, 1, &V_YesNo, &gaInifile.HistoryWrap, HISTORY_PP, DLG_DATA(IDI_HistWrap, INI_CTL_CHECK, 0)
	_TEXT("Include"), INI_INCLUDE, 0, VNULL, NULL, 0, DLG_NULL
	_TEXT("INIQuery"), INI_CHOICE, 0, &V_YesNo, &gaInifile.INIQuery, 0, DLG_NULL
	_TEXT("InputColors"), INI_COLOR, 0, VNULL, &gaInifile.InputColor, COLORS_PP, DLG_DATA(IDI_InputColors, INI_CTL_COLOR, 0)
	_TEXT("ListColors"), INI_COLOR, 0, VNULL, &gaInifile.ListColor, COLORS_PP, DLG_DATA(IDI_ListColors, INI_CTL_COLOR, 0)
	_TEXT("ListRowStart"), INI_UINT, 1, &V_BaseRng, &gaInifile.ListRowStart, MISC_PP, DLG_NULL
	_TEXT("LogName"), INI_PATH, 0, (void *)0x4000, &gaInifile.LogName, STARTUP_PP, DLG_DATA(IDI_LogName, INI_CTL_TEXT, 0)
	_TEXT("LogOn"), INI_CHOICE, 0, &V_YesNo, &gaInifile.LogOn, STARTUP_PP, DLG_DATA(IDI_LogOn, INI_CTL_CHECK, 0)
	_TEXT("LogErrors"), INI_CHOICE, 0, &V_YesNo, &gaInifile.LogErrors, STARTUP_PP, DLG_DATA(IDI_LogErrors, INI_CTL_CHECK, 0)
	_TEXT("NoClobber"), INI_CHOICE, 0, &V_YesNo, &gaInifile.NoClobber, STARTUP_PP, DLG_DATA(IDI_NoClobber, INI_CTL_CHECK, 0)
	_TEXT("PathExt"), INI_CHOICE, 0, &V_YesNo, &gaInifile.PathExt, STARTUP_PP, DLG_DATA(IDI_PathExt, INI_CTL_CHECK, 0)
	_TEXT("PopupWinHeight"), INI_UINT, 12, &V_RCRng, &gaInifile.PWHeight, HISTORY_PP, DLG_DATA(IDI_PopupWinHeight, INI_CTL_INT, 0)
	_TEXT("PopupWinTop"), INI_UINT, 1, &V_RCRng, &gaInifile.PWTop, HISTORY_PP, DLG_DATA(IDI_PopupWinTop, INI_CTL_INT, 0)

	"ScreenRows", INI_UINT, 0, &V_RCRng, &gaInifile.Rows, 0, DLG_DATA(IDI_ScreenRows, INI_CTL_INT, 0)

	_TEXT("SelectColors"), INI_COLOR, 0, VNULL, &gaInifile.SelectColor, COLORS_PP, DLG_DATA(IDI_SelectColors, INI_CTL_COLOR, 0)
	_TEXT("StdColors"), INI_COLOR, 0, VNULL, &gaInifile.StdColor, COLORS_PP, DLG_DATA(IDI_StdColors, INI_CTL_COLOR, 0)
	_TEXT("TabStops"), INI_UINT, 8, &V_Tabs, &gaInifile.Tabs, WINDOWS_PP, DLG_DATA(IDI_Tabs, INI_CTL_INT, 0)
	_TEXT("ThousandsChar"), INI_CHOICE, 0, &V_DTChar, &gaInifile.ThousandsChar, SYNTAX_PP, DLG_DATA(IDI_ThousandsChar, INI_CTL_RADIO, 3)
	_TEXT("TreePath"), INI_PATH, 0, VNULL, &gaInifile.TreePath, MISC_PP, DLG_DATA(IDI_TreePath, INI_CTL_TEXT, 0)
	_TEXT("SwitchChar"), INI_CHAR, '/', VNULL, &gaInifile.SwChr, SYNTAX_PP, DLG_NULL
	_TEXT("UnixPaths"), INI_CHOICE, 0, &V_YesNo, &gaInifile.UnixPaths, STARTUP_PP, DLG_DATA(IDI_UnixPaths, INI_CTL_CHECK, 0)
	_TEXT("UpperCase"), INI_CHOICE, 0, &V_YesNo, &gaInifile.Upper, STARTUP_PP, DLG_DATA(IDI_UpperCase, INI_CTL_CHECK, 0)

	// DOS only
	"CommandSep", INI_CHAR, '^', VNULL, &gaInifile.CmdSep, 0, DLG_DATA(IDI_CommandSep, INI_CTL_TEXT, 0)
	"EscapeChar", INI_CHAR, 24, VNULL, &gaInifile.EscChr, 0, DLG_DATA(IDI_EscapeChar, INI_CTL_TEXT, 0)
	"ParameterChar", INI_CHAR, '&', VNULL, &gaInifile.ParamChr, 0, DLG_DATA(IDI_ParameterChar, INI_CTL_TEXT, 0)
	"Win95LFN", INI_CHOICE, 1, &V_YesNo, &gaInifile.Win95LFN, 0, DLG_NULL
	"Win95SFNSearch", INI_CHOICE, 1, &V_YesNo, &gaInifile.Win95SFN, 0, DLG_DATA(IDI_Win95SFNSearch, INI_CTL_CHECK, 0)

	// Everything but Windows
	_TEXT("4StartPath"), INI_PATH, 0, VNULL, &gaInifile.FSPath, STARTUP_PP, DLG_DATA(IDI_4StartPath, INI_CTL_TEXT, 0)
	_TEXT("CursorIns"), INI_INT, 100, &V_CurRng, &gaInifile.CursI, EDITING_PP, DLG_DATA(IDI_CursorIns, INI_CTL_INT, 0)
	_TEXT("CursorOver"), INI_INT, 15, &V_CurRng, &gaInifile.CursO, EDITING_PP, DLG_DATA(IDI_CursorOver, INI_CTL_INT, 0)
	_TEXT("EditMode"), INI_CHOICE, 0, &V_EMList, &gaInifile.EditMode, EDITING_PP, DLG_DATA(IDI_EditMode, INI_CTL_RADIO, 4)
	_TEXT("CDDWinColors"), INI_COLOR, 0, VNULL, &gaInifile.CDDColor, 0, DLG_DATA(IDI_CDDWinColors, INI_CTL_COLOR, 0)
	_TEXT("ListboxBarColors"), INI_COLOR, 0, VNULL, &gaInifile.LBBar, 0, DLG_NULL
	_TEXT("PopupWinColors"), INI_COLOR, 0, VNULL, &gaInifile.PWColor, 0, DLG_DATA(IDI_PopupWinColors, INI_CTL_COLOR, 0)
	_TEXT("PopupWinLeft"), INI_UINT, 40, &V_RCRng, &gaInifile.PWLeft, HISTORY_PP, DLG_DATA(IDI_PopupWinLeft, INI_CTL_INT, 0)
	_TEXT("PopupWinWidth"), INI_UINT, 36, &V_RCRng, &gaInifile.PWWidth, HISTORY_PP, DLG_DATA(IDI_PopupWinWidth, INI_CTL_INT, 0)
	_TEXT("Printer"), INI_STR, 0, VNULL, &gaInifile.Printer, 0, DLG_DATA(IDI_Printer, INI_CTL_TEXT, 0)

	// 4DOS only
	// Kludge - see note under DirHistory directive above
	"Alias", (INI_UINT | INI_NOMOD), 1024, &V_AliasRng, &gaInifile.AliasSize, 0, DLG_NULL
	"Alias", INI_UINT, 1024, &V_AliasRng, &gaInifile.AliasNew, 0, DLG_DATA(IDI_Alias, INI_CTL_INT, 0)
	"AutoExecPath", INI_PATH, 0, VNULL, &gaInifile.AEPath, 0, DLG_DATA(IDI_AutoExecPath, INI_CTL_TEXT, 0)
	"AutoExecParms", INI_STR, 0, VNULL, &gaInifile.AEParms, 0, DLG_DATA(IDI_AutoExecParms, INI_CTL_TEXT, 0)
	"BrightBG", INI_CHOICE, 2, &V_YesNo, &gaInifile.BrightBG, 0, DLG_DATA(IDI_BrightBG, INI_CTL_CHECK, 0)
	"EnvFree", INI_UINT, 128, &V_EnvFreeRng, &gaInifile.EnvFree, 0, DLG_DATA(IDI_EnvFree, INI_CTL_INT, 0)
	// Kludge - see note under DirHistory directive above
	"Environment", (INI_UINT | INI_NOMOD), 512, &V_EnvRng, &gaInifile.EnvSize, 0, DLG_NULL
	"Environment", INI_UINT, 512, &V_EnvRng, &gaInifile.EnvNew, 0, DLG_DATA(IDI_Environment, INI_CTL_INT, 0)
	"Function", (INI_UINT | INI_NOMOD), 1024, &V_AliasRng, &gaInifile.FunctionSize, 0, DLG_NULL
	"Function", INI_UINT, 1024, &V_AliasRng, &gaInifile.FunctionNew, 0, DLG_DATA(IDI_Function, INI_CTL_INT, 0)
	"HelpOptions", INI_STR, 0, VNULL, &gaInifile.HOptions, 0, DLG_DATA(IDI_HelpOptions, INI_CTL_TEXT, 0)
	" ", INI_PATH, 0, VNULL, &gaInifile.HPath, 0, DLG_NULL
	"InstallPath", INI_PATH, 0, VNULL, &gaInifile.InstallPath, 0, DLG_DATA(IDI_InstallPath, INI_CTL_TEXT, 0)
	"LineInput", INI_CHOICE, 0, &V_YesNo, &gaInifile.LineIn, 0, DLG_NULL
	"Mouse", INI_CHOICE, 0, &V_YesNoAuto, &gaInifile.Mouse, SYNTAX_PP, DLG_DATA(IDI_Mouse, INI_CTL_RADIO, 3)
	"REXXPath", INI_PATH, 0, VNULL, &gaInifile.RexxPath, 0, DLG_DATA(IDI_REXXPath, INI_CTL_TEXT, 0)
	"ScreenColumns", INI_UINT, 0, &V_RCRng, &gaInifile.Columns, 0, DLG_DATA(IDI_ScreenColumns, INI_CTL_INT, 0)
	"StackSize", INI_UINT, 8192, &V_StackRng, &gaInifile.StackSize, 0, DLG_DATA(IDI_StackSize, INI_CTL_INT, 0)
	"Swapping", INI_STR, 0, VNULL, &gaInifile.Swap, 0, DLG_DATA(IDI_Swapping, INI_CTL_TEXT, 0)
	"UMBLoad", INI_CHOICE, 0, &V_UMBOpts, &gaInifile.UMBLd, 0, DLG_DATA(IDI_UMBLoad, INI_CTL_COMBO, 0)
	"UMBAlias", INI_CHOICE, 0, &V_UMBOpts, &gaInifile.UMBAlias, 0, DLG_DATA(IDI_UMBAlias, INI_CTL_COMBO, 0)
	"UMBEnvironment", INI_CHOICE, 0, &V_UMBOpts, &gaInifile.UMBEnv, 0, DLG_DATA(IDI_UMBEnvironment, INI_CTL_COMBO, 0)
	"UMBFunction", INI_CHOICE, 0, &V_UMBOpts, &gaInifile.UMBFunction, 0, DLG_DATA(IDI_UMBFunction, INI_CTL_COMBO, 0)
	"UMBHistory", INI_CHOICE, 0, &V_UMBOpts, &gaInifile.UMBHistory, 0, DLG_DATA(IDI_UMBHistory, INI_CTL_COMBO, 0)
	"UMBDirHistory", INI_CHOICE, 0, &V_UMBOpts, &gaInifile.UMBDirHistory, 0, DLG_DATA(IDI_UMBDirHistory, INI_CTL_COMBO, 0)

	// Everything but Windows
	_TEXT("ListStatBarColors"), INI_COLOR, 0, VNULL, &gaInifile.ListStatusColor, 0, DLG_DATA(IDI_ListStatBarColors, INI_CTL_COLOR, 0)
	_TEXT("PauseOnError"), INI_CHOICE, 1, &V_YesNo, &gaInifile.PauseErr, STARTUP_PP, DLG_NULL
	_TEXT("SelectStatBarColors"), INI_COLOR, 0, VNULL, &gaInifile.SelectStatusColor, 0, DLG_DATA(IDI_SelectStatBarColors, INI_CTL_COLOR, 0)

	_TEXT("DuplicateBugs"), INI_CHOICE, 0, &V_YesNo, &gaInifile.DupBugs, 0, DLG_NULL
	_TEXT("LocalAliases"), INI_CHOICE, 0, &V_YesNo, &gaInifile.LocalAliases, STARTUP_PP, DLG_DATA(IDI_LocalAliases, INI_CTL_CHECK, 0)
	_TEXT("LocalDirHistory"), INI_CHOICE, 0, &V_YesNo, &gaInifile.LocalDirHistory, STARTUP_PP, DLG_DATA(IDI_LocalDirHistory, INI_CTL_CHECK, 0)
	_TEXT("LocalFunctions"), INI_CHOICE, 0, &V_YesNo, &gaInifile.LocalFunctions, STARTUP_PP, DLG_DATA(IDI_LocalFunctions, INI_CTL_CHECK, 0)
	_TEXT("LocalHistory"), INI_CHOICE, 0, &V_YesNo, &gaInifile.LocalHistory, STARTUP_PP, DLG_DATA(IDI_LocalHistory, INI_CTL_CHECK, 0)
	_TEXT("NextINIFile"), INI_STR, 0, VNULL, &gaInifile.NextININame, 0, DLG_NULL

	// Dummy item to hold name of primary INI file, set up here so
	// INIStr will move it around as needed, but with blank name so user
	// can't modify it
	_TEXT("PrimaryINIFile"), INI_STR, 0, VNULL, &gaInifile.PrimaryININame, 0, DLG_NULL

	// Key mapping items (same in all products)
	_TEXT("AddFile"), INI_KEY, (K_F10 + MAP_EDIT), VNULL, NULL, 0, DLG_NULL
	_TEXT("AliasExpand"), INI_KEY, (K_CtlF + MAP_GEN), VNULL, NULL, 0, DLG_NULL
	_TEXT("Backspace"), INI_KEY, (K_Bksp + MAP_GEN), VNULL, NULL, 0, DLG_NULL
	_TEXT("BeginLine"), INI_KEY, (K_Home + MAP_GEN), VNULL, NULL, 0, DLG_NULL
	_TEXT("CommandEscape"), INI_KEY, (K_255 + MAP_EDIT), VNULL, NULL, 0, DLG_NULL
	_TEXT("Copy"), INI_KEY, (K_CtlY + MAP_GEN), VNULL, NULL, 0, DLG_NULL
	_TEXT("Del"), INI_KEY, (K_Del + MAP_GEN), VNULL, NULL, 0, DLG_NULL
	_TEXT("DelHistory"), INI_KEY, (K_CtlD + MAP_EDIT), VNULL, NULL, 0, DLG_NULL
	_TEXT("DelToBeginning"), INI_KEY, (K_CtlHm + MAP_GEN), VNULL, NULL, 0, DLG_NULL
	_TEXT("DelToEnd"), INI_KEY, (K_CtlEnd + MAP_GEN), VNULL, NULL, 0, DLG_NULL
	_TEXT("DelWordLeft"), INI_KEY, (K_CtlL + MAP_GEN), VNULL, NULL, 0, DLG_NULL
	_TEXT("DelWordRight"), INI_KEY, (K_CtlR + MAP_GEN), VNULL, NULL, 0, DLG_NULL
	_TEXT("Down"), INI_KEY, (K_Down + MAP_GEN), VNULL, NULL, 0, DLG_NULL
	_TEXT("EndHistory"), INI_KEY, (K_CtlE + MAP_EDIT), VNULL, NULL, 0, DLG_NULL
	_TEXT("EndLine"), INI_KEY, (K_End + MAP_GEN), VNULL, NULL, 0, DLG_NULL
	_TEXT("EraseLine"), INI_KEY, (K_Esc + MAP_GEN), VNULL, NULL, 0, DLG_NULL
	_TEXT("ExecLine"), INI_KEY, (K_Enter + MAP_GEN), VNULL, NULL, 0, DLG_NULL
	_TEXT("Help"), INI_KEY, (K_F1 + MAP_EDIT), VNULL, NULL, 0, DLG_NULL
	_TEXT("HelpWord"), INI_KEY, (K_CtlF1 + MAP_EDIT), VNULL, NULL, 0, DLG_NULL
	_TEXT("Paste"), INI_KEY, (K_CtlV + MAP_GEN), VNULL, NULL, 0, DLG_NULL
	_TEXT("PopupWinBegin"), INI_KEY, (K_CtlPgU + MAP_HWIN), VNULL, NULL, 0, DLG_NULL
	_TEXT("PopupWinDel"), INI_KEY, (K_CtlD + MAP_HWIN), VNULL, NULL, 0, DLG_NULL
	_TEXT("PopupWinEdit"), INI_KEY, (K_CtlEnt + MAP_HWIN), VNULL, NULL, 0, DLG_NULL
	_TEXT("PopupWinEnd"), INI_KEY, (K_CtlPgD + MAP_HWIN), VNULL, NULL, 0, DLG_NULL
	_TEXT("PopupWinExec"), INI_KEY, (K_Enter + MAP_HWIN), VNULL, NULL, 0, DLG_NULL
	_TEXT("Ins"), INI_KEY, (K_Ins + MAP_GEN), VNULL, NULL, 0, DLG_NULL
	_TEXT("Left"), INI_KEY, (K_Left + MAP_GEN), VNULL, NULL, 0, DLG_NULL
	_TEXT("LFNToggle"), INI_KEY, (K_CtlA + MAP_EDIT), VNULL, NULL, 0, DLG_NULL
	_TEXT("LineToEnd"), INI_KEY, (K_CtlEnt + MAP_EDIT), VNULL, NULL, 0, DLG_NULL
	_TEXT("ListContinue"), INI_KEY, (K_C + MAP_LIST), VNULL, NULL, 0, DLG_NULL
	_TEXT("ListExit"), INI_KEY, (K_Esc + MAP_LIST), VNULL, NULL, 0, DLG_NULL
	_TEXT("ListFind"), INI_KEY, (K_F + MAP_LIST), VNULL, NULL, 0, DLG_NULL
	_TEXT("ListFindReverse"), INI_KEY, (K_CtlF + MAP_LIST), VNULL, NULL, 0, DLG_NULL
	_TEXT("ListHex"), INI_KEY, (K_X + MAP_LIST), VNULL, NULL, 0, DLG_NULL
	_TEXT("ListHighBit"), INI_KEY, (K_H + MAP_LIST), VNULL, NULL, 0, DLG_NULL
	_TEXT("ListInfo"), INI_KEY, (K_I + MAP_LIST), VNULL, NULL, 0, DLG_NULL
	_TEXT("ListNext"), INI_KEY, (K_N + MAP_LIST), VNULL, NULL, 0, DLG_NULL
	_TEXT("ListPrevious"), INI_KEY, (K_CtlN + MAP_LIST), VNULL, NULL, 0, DLG_NULL
	_TEXT("ListPrint"), INI_KEY, (K_P + MAP_LIST), VNULL, NULL, 0, DLG_NULL
	_TEXT("ListWrap"), INI_KEY, (K_W + MAP_LIST), VNULL, NULL, 0, DLG_NULL
	_TEXT("NextFile"), INI_KEY, (K_F9 + MAP_EDIT), VNULL, NULL, 0, DLG_NULL
	_TEXT("NormalEditKey"), INI_KEY, (NORMAL_KEY + MAP_EDIT), VNULL, NULL, 0, DLG_NULL
	_TEXT("NormalPopupKey"), INI_KEY, (NORMAL_KEY + MAP_HWIN), VNULL, NULL, 0, DLG_NULL
	_TEXT("NormalKey"), INI_KEY, (NORMAL_KEY + MAP_GEN), VNULL, NULL, 0, DLG_NULL
	_TEXT("NormalListKey"), INI_KEY, (NORMAL_KEY + MAP_LIST), VNULL, NULL, 0, DLG_NULL
	_TEXT("NormalPopupKey"), INI_KEY, (NORMAL_KEY + MAP_HWIN), VNULL, NULL, 0, DLG_NULL
	_TEXT("PopFile"), INI_KEY, (K_F7 + MAP_EDIT), VNULL, NULL, 0, DLG_NULL
	_TEXT("PrevFile"), INI_KEY, (K_F8 + MAP_EDIT), VNULL, NULL, 0, DLG_NULL
	_TEXT("RepeatFile"), INI_KEY, (K_F12 + MAP_EDIT), VNULL, NULL, 0, DLG_NULL
	_TEXT("Right"), INI_KEY, (K_Right + MAP_GEN), VNULL, NULL, 0, DLG_NULL
	_TEXT("SaveHistory"), INI_KEY, (K_CtlK + MAP_EDIT), VNULL, NULL, 0, DLG_NULL
	_TEXT("Up"), INI_KEY, (K_Up + MAP_GEN), VNULL, NULL, 0, DLG_NULL
	_TEXT("WordLeft"), INI_KEY, (K_CtlLft + MAP_GEN), VNULL, NULL, 0, DLG_NULL
	_TEXT("WordRight"), INI_KEY, (K_CtlRt + MAP_GEN), VNULL, NULL, 0, DLG_NULL

	_TEXT("DirWinOpen"), INI_KEY, (K_CtlPgU + MAP_EDIT), VNULL, NULL, 0, DLG_NULL
	_TEXT("HistWinOpen"), INI_KEY, (K_PgUp + MAP_EDIT), VNULL, NULL, 0, DLG_NULL
	_TEXT("NextHistory"), INI_KEY, (K_Down + MAP_EDIT), VNULL, NULL, 0, DLG_NULL
	_TEXT("PrevHistory"), INI_KEY, (K_Up + MAP_EDIT), VNULL, NULL, 0, DLG_NULL
};

TOKEN_LIST(INIItems, gaINIItemList);

unsigned int guINIItemCount = (sizeof(gaINIItemList) / sizeof(gaINIItemList[0]));

#else

extern INI_ITEM gaINIItemList[];
extern unsigned int guINIItemCount;

#endif


// INI file parsing errors
#define E_SECNAM  _TEXT("Invalid section name")
#define E_BADNAM  _TEXT("Invalid item name")
#define E_BADNUM  _TEXT("Invalid numeric value for")
#define E_BADCHR  _TEXT("Invalid character value for")
#define E_BADCHC  _TEXT("Invalid choice value for")
#define E_BADKEY  _TEXT("Invalid key substitution for")
#define E_KEYFUL  _TEXT("Keystroke substitution table full")
#define E_BADCOL  _TEXT("Invalid color for")
#define E_BADPTH  _TEXT("Invalid path or file name for")
#define E_STROVR  _TEXT("String area overflow")
#define E_INCL	  _TEXT("Include file not found")
#define E_NEST	  _TEXT("Include files nested too deep")
#define E_LIVEMOD _TEXT("Value can only be changed at startup")

// strip character value & turn off extended key & normalized key flags
#define CONTEXT_BITS(keyval) (keyval & 0xFF00 & (~(EXTENDED_KEY | NORMAL_KEY)))



#ifdef KEYPARSE

// Data for key parsing (KEYPARSE.C)

// key prefixes with function key base scan codes
struct {
	LPTSTR prefixstr;
	unsigned char F1Pref;      // scan code base for F1 - F10
	unsigned char F11Pref;     // scan code base for F11 - F12, minus 10
} KeyPrefixList[] = {
	_TEXT("@"), 0x3B, (0x85 - 10),       // no prefix ("@" is checked before prefix),
                                 // F1, F11
	_TEXT("Alt"), 0x68, (0x8B - 10),     // Alt-F1, Alt-F11
	_TEXT("Ctrl"), 0x5E, (0x89 - 10),    // Ctrl-F1, Ctrl-F11
	_TEXT("Shift"), 0x54, (0x87 - 10),   // Shift-F1, Shift-F11
};
TOKEN_LIST(KeyPrefixes, KeyPrefixList);

// names and codes for non-printing keys 
struct {
	LPTSTR namestr;
	unsigned int NPStd;      // standard ASCII or scan code
	unsigned int NPSecond;   // secondary ASCII or scan code
} KeyNameList[] = {           // second key:
	_TEXT("Esc"), K_Esc, 0,              // none
	_TEXT("Bksp"), K_Bksp, K_CtlBS,      // Ctrl-Bksp
	_TEXT("Tab"), K_Tab, K_ShfTab,       // Shift-Tab
	_TEXT("Enter"), K_Enter, K_CtlEnt,   // Ctrl-Enter
	_TEXT("Up"), K_Up, K_CtlUp,          // Ctrl-Up
	_TEXT("Down"), K_Down, K_CtlDn,      // Ctrl-Down
	_TEXT("Left"), K_Left, K_CtlLft,     // Ctrl-Left
	_TEXT("Right"), K_Right, K_CtlRt,    // Ctrl-Right
	_TEXT("PgUp"), K_PgUp, K_CtlPgU,     // Ctrl-PgUp
	_TEXT("PgDn"), K_PgDn, K_CtlPgD,     // Ctrl-PgDn
	_TEXT("Home"), K_Home, K_CtlHm,      // Ctrl-Home
	_TEXT("End"), K_End, K_CtlEnd,       // Ctrl-End
	_TEXT("Ins"), K_Ins, K_CtlIns,       // Ctrl-Ins
	_TEXT("Del"), K_Del, K_CtlDel,       // Ctrl-Del
};
TOKEN_LIST(KeyNames, KeyNameList);

static TCHAR ALT_ALPHA_KEYS[] = {
   K_AltA - K_SCAN,
   K_AltB - K_SCAN,
   K_AltC - K_SCAN,
   K_AltD - K_SCAN,
   K_AltE - K_SCAN,
   K_AltF - K_SCAN,
   K_AltG - K_SCAN,
   K_AltH - K_SCAN,
   K_AltI - K_SCAN,
   K_AltJ - K_SCAN,
   K_AltK - K_SCAN,
   K_AltL - K_SCAN,
   K_AltM - K_SCAN,
   K_AltN - K_SCAN,
   K_AltO - K_SCAN,
   K_AltP - K_SCAN,
   K_AltQ - K_SCAN,
   K_AltR - K_SCAN,
   K_AltS - K_SCAN,
   K_AltT - K_SCAN,
   K_AltU - K_SCAN,
   K_AltV - K_SCAN,
   K_AltW - K_SCAN,
   K_AltX - K_SCAN,
   K_AltY - K_SCAN,
   K_AltZ - K_SCAN
};

static TCHAR ALT_DIGIT_KEYS[] = {
   K_Alt0 - K_SCAN,
   K_Alt1 - K_SCAN,
   K_Alt2 - K_SCAN,
   K_Alt3 - K_SCAN,
   K_Alt4 - K_SCAN,
   K_Alt5 - K_SCAN,
   K_Alt6 - K_SCAN,
   K_Alt7 - K_SCAN,
   K_Alt8 - K_SCAN,
   K_Alt9 - K_SCAN
};

#endif
