#ifndef CATDEFS_H_INCLUDED
#define CATDEFS_H_INCLUDED

#include "cats\catgets.h"

#ifdef HTML_HELP
nl_catd cat;
#else
extern nl_catd cat;
#endif

/***********************************************************************/

/* Program Usage Screen Strings */
#define hcatProgramName CATDEF_ENTRY(1, 0, "FreeDOS HTML Help Viewer")
#define hcatBasicOptions CATDEF_ENTRY(1, 1, "Basic Options")
#define hcatUsageTopic CATDEF_ENTRY(1, 2, "Show help on this topic")
#define hcatUsageQMark CATDEF_ENTRY(1, 3, "Displays this help message")
#define hcatUsageMono CATDEF_ENTRY(1, 4, "Force monochrome display")
#define hcatUsageFancy CATDEF_ENTRY(1, 5, "Use fancy color scheme")
#define hcatUsageASCII CATDEF_ENTRY(1, 6, "Uses ASCII instead of extended characters")
#define hcatUsageCP CATDEF_ENTRY(1, 7, "Tell HELP the codepage is nnn, rather than let detect")
#define hcatAdvancedOptions CATDEF_ENTRY(1, 8, "Advanced Options")
#define hcatUsageOverride1 CATDEF_ENTRY(1, 9, "Overrides the help path")
#define hcatUsageOverride2 CATDEF_ENTRY(1, 10, "(if no file is specified, index.htm is assumed)")
#define hcatUsageLoad CATDEF_ENTRY(1, 11, "Loads a file other than index.htm relative to the help path")
#define hcatUsageHelp1 CATDEF_ENTRY(1, 12, "When the user presses F1 or clicks on \"help on help\"")
#define hcatUsageHelp2 CATDEF_ENTRY(1, 13, "help will load this file. Default is help.htm")
#define hcatEnvironVar CATDEF_ENTRY(1, 14, "Environment Variables")
#define hcatUsageHelpPath CATDEF_ENTRY(1, 15, "Directory that contains your help files")
#define hcatUsageHelpCmd CATDEF_ENTRY(1, 16, "Put /M, /A, F1, F2 here to make them default")

/* Error Message Strings */
#define hcatMemErr CATDEF_ENTRY(2, 0, "Could not allocate memory")
#define hcatInvArg CATDEF_ENTRY(2, 1, "Invalid Argument")
#define hcat2ManyTopics CATDEF_ENTRY(2, 2,"Please specify only one topic")
#define hcatFwithN CATDEF_ENTRY(2, 3, "Cannot use /f with /m")
#define hcatHowGetUsage CATDEF_ENTRY(2, 4,"Type \"HELP /?\" for usage.")
#define hcatNoExactFound CATDEF_ENTRY(2, 5, "No exact match for topic found")
#define hcatCouldntFind CATDEF_ENTRY(2, 6, "Could not find topic")
#define hcatResizeFail CATDEF_ENTRY(2, 7, "Internal Error: Resize Fail")
#define hcatReadCompressFail CATDEF_ENTRY(2, 8, "Could not read the compressed file")
#define hcatOpenErr CATDEF_ENTRY(2, 9, "Couldn't Open")
#define hcatZipEmpty CATDEF_ENTRY(2, 10, "Zip file empty or corrupt")
#define hcatFirstFile CATDEF_ENTRY(2, 11, "Could not find this file. Loading first html file in zipfile")
#define hcatHTMLinZipErr CATDEF_ENTRY(2, 12, "Could not find html file in zip file")
#define hcatCodepagePlease CATDEF_ENTRY(2, 13, "Please specify a valid codepage")
#define hcatCodepageNotSupported CATDEF_ENTRY(2, 14, "Specified codepage not supported")
#define hcatCodepagesSupported CATDEF_ENTRY(2, 15, "Supported codepages are")

/* Menu Strings */
#define hcatMenuExit CATDEF_ENTRY(3, 0,    "Exit")
#define hcatMenuHelp CATDEF_ENTRY(3, 1,    "Help on Help")
#define hcatMenuBack CATDEF_ENTRY(3, 2,    "Back")
#define hcatMenuForward CATDEF_ENTRY(3, 3, "Forward")
#define hcatMenuContents CATDEF_ENTRY(3, 4,"Contents")
#define hcatMenuSearch CATDEF_ENTRY(3, 5,  "Search")

/* Button Strings */
#define hcatButtonOK CATDEF_ENTRY(4, 0, "OK")
#define hcatButtonCancel CATDEF_ENTRY(4, 1, "Cancel")
#define hcatButtonHelp CATDEF_ENTRY(4, 2, "Help")

/* Status Bar Strings */
#define hcatStatusLooking CATDEF_ENTRY(5, 0, "Looking for topic...")
#define hcatStatusSearching CATDEF_ENTRY(5, 1, "Full search in progress...")
#define hcatStatusEscape CATDEF_ENTRY(5, 2, "(PRESS ESC TO ABORT)...")

/* Search Box Strings */
#define hcatSearchboxTitle CATDEF_ENTRY(6, 0, "Search Help")
#define hcatSearchboxPrompt CATDEF_ENTRY(6, 1, "Text to Find:")
#define hcatSearchboxCase CATDEF_ENTRY(6, 2, "case sensitive")
#define hcatSearchboxWhole CATDEF_ENTRY(6, 3, "whole word only")
#define hcatSearchboxFull CATDEF_ENTRY(6, 4, "full search")
#define hcatSearchboxPage CATDEF_ENTRY(6, 5, "on this page")
#define hcatSearchhlpTitle CATDEF_ENTRY(6, 6, "What it Does")
#define hcatSearchhlpLine1 CATDEF_ENTRY(6, 7, "full search:")
#define hcatSearchhlpLine2 CATDEF_ENTRY(6, 8, "Looks through all files")
#define hcatSearchhlpLine3 CATDEF_ENTRY(6, 9, "listed on the contents page.")
#define hcatSearchhlpLine4 CATDEF_ENTRY(6, 10, " ")
#define hcatSearchhlpLine5 CATDEF_ENTRY(6, 11, "on this page:")
#define hcatSearchhlpLine6	CATDEF_ENTRY(6, 12, "Looks in the open document")
#define hcatSearchhlpLine7 CATDEF_ENTRY(6, 13, "only.")

/* Search Results Strings */
#define hcatSearchResults CATDEF_ENTRY(7, 0, "Search Results")
#define hcatSearchFileSet CATDEF_ENTRY(7, 1, "Searched all files listed on contents page")
#define hcatSearched CATDEF_ENTRY(7, 2, "Searched")
#define hcatSearchFor CATDEF_ENTRY(7, 3, "for")
#define hcatCantSrchSrch CATDEF_ENTRY(7, 4,"Sorry, can't search the search results.")
#define hcatUserAborted CATDEF_ENTRY(7, 5, "USER ABORTED (pressed escape)")
#define hcatNoResults CATDEF_ENTRY(7, 6, "No results found")

/***********************************************************************/

/* Macro definition to read from catalogue */
#define CATDEF_ENTRY(a, b, default) (const char *) \
                                    catgets(cat, a, b, default)

#endif