#define UNZIP_INTERNAL
#include "unzip.h"
#include "version.h"

#include <Traps.h>
#include <Values.h>

char UnzipVersion[32], ZipinfoVersion[32];

void MacFSTest (int);
void ResolveMacVol (short, short *, long *, StringPtr);

void screenOpen(char *);
void screenControl(char *, int);
void screenClose(void);

#define aboutAlert      128

#define selectDialog    129
#define okItem          1
#define cancelItem      2
#define editItem        3
#define staticItem      4

#define unzipMenuBar    128

#define appleMenu       128
#define aboutItem       1

#define fileMenu        129
#define extractItem     1
#define infoItem        2
#define listItem        3
#define testItem        4
#define commentItem     6
#define freshenItem     8
#define updateItem      9
#define quitItem        11

#define editMenu        130
#define cutItem         1
#define copyItem        2
#define pasteItem       3

#define modifierMenu    131
#define excludeItem     1
#define selectItem      2
#define quietItem       9
#define verboseItem     10

#define screenMenu      132
#define pauseItem       1
#define scrollItem      2

#define extractMenu     133
#define screenItem      3
#define junkItem        5

#define caseMenu        134
#define insensitiveItem 1
#define lowercaseItem   2

#define convertMenu     135
#define autoItem        1
#define binaryItem      2
#define textItem        3

#define overwriteMenu   136
#define alwaysItem      1
#define neverItem       2
#define promptItem      3

#define infoMenu        137
#define prtCommentItem  2
#define prtHeaderItem   3
#define prtTotalsItem   4

#define formatMenu      138
#define filenameItem    1
#define longItem        2
#define mediumItem      3
#define shortItem       4

long modifiers, modifierMask;

#define allFlags        0x000FFFFF

#define quietFlag       0x00000001
#define verboseFlag     0x00000002

#define pauseFlag       0x00080000
#define scrollFlag      0x00040000

#define screenFlag      0x00000004
#define junkFlag        0x00000008

#define insensitiveFlag 0x00000010
#define lowercaseFlag   0x00000020

#define autoFlag        0x00000040
#define textFlag        0x00000080

#define neverFlag       0x00000100
#define overwriteFlag   0x00000200

#define prtCommentFlag  0x00000400
#define prtHeaderFlag   0x00000800
#define prtTotalsFlag   0x00001000

#define filenameFlag    0x00002000
#define longFlag        0x00004000
#define mediumFlag      0x00008000
#define shortFlag       0x00010000

#define extractMask     0x000003FD
#define infoMask        0x0001FE02
#define listMask        0x00000001
#define testMask        0x00000001
#define commentMask     0x00000000
#define freshenMask     0x000003FD
#define updateMask      0x000003FD

EventRecord myevent;
MenuHandle appleHandle, modifierHandle, screenHandle, extractHandle;
MenuHandle caseHandle, convertHandle, overwriteHandle, infoHandle;
MenuHandle formatHandle;
Handle menubar, itemHandle;
short itemType;
Rect itemRect;

char command, fileList[256];

Boolean stop;

SysEnvRec sysRec;

char *macgetenv(s) char *s; {
    if (s == NULL) return(fileList);
    return(NULL);
}

Boolean TrapAvailable(machineType, trapNumber, trapType)
short machineType;
short trapNumber;
TrapType trapType;
{
    if (machineType < 0)
        return (false);

    if ((trapType == ToolTrap) &&
        (machineType > envMachUnknown) &&
        (machineType < envMacII)) {
        if ((trapNumber &= 0x03FF) > 0x01FF)
            trapNumber = _Unimplemented;
    }
    return (NGetTrapAddress(trapNumber, trapType) !=
        GetTrapAddress(_Unimplemented));
}

void domenu(menucommand) long menucommand;
{
    short themenu, theitem;
    DialogPtr thedialog;
    Str255 name;
    long check;

    themenu = HiWord(menucommand);
    theitem = LoWord(menucommand);

    switch (themenu) {

    case appleMenu:
        if (theitem == aboutItem) {
            ParamText((StringPtr)UnzipVersion, (StringPtr)ZipinfoVersion, nil, nil);
            Alert(aboutAlert, nil);
        } else {
            GetItem(appleHandle, theitem, name);
            theitem = OpenDeskAcc(name);
        }
        break;

    case fileMenu:
        switch (theitem) {
        case extractItem:
            if (modifiers & screenFlag)
                command = 'c';
            else
                command = 'x';
            modifierMask = extractMask;
            break;
        case infoItem:
            command = 'Z';
            modifierMask = infoMask;
            break;
        case listItem:
            if (modifiers & verboseFlag)
                command = 'v';
            else
                command = 'l';
            modifierMask = listMask;
            break;
        case testItem:
            command = 't';
            modifierMask = testMask;
            break;
        case commentItem:
            command = 'z';
            modifierMask = commentMask;
            break;
        case freshenItem:
            command = 'f';
            modifierMask = freshenMask;
            break;
        case updateItem:
            command = 'u';
            modifierMask = updateMask;
            break;
        case quitItem:
            stop = true;
            break;
        default:
            break;
        }
        break;

    case editMenu:
        break;

    case modifierMenu:
        switch (theitem) {
        case excludeItem:
            check = -1;
            break;
        case selectItem:
            thedialog = GetNewDialog(selectDialog, nil, (WindowPtr)(-1));
            SetPort(thedialog);
            do
                ModalDialog(nil, &theitem);
            while ((theitem != okItem) && (theitem != cancelItem));
            if (theitem == okItem) {
                GetDItem(thedialog, editItem, &itemType, &itemHandle, &itemRect);
                GetIText(itemHandle, (StringPtr)&fileList);
                p2cstr(fileList);
            }
            DisposDialog(thedialog);
            check = -1;
            break;
        case quietItem:
            check = (modifiers ^= quietFlag) & quietFlag;
            break;
        case verboseItem:
            check = (modifiers ^= verboseFlag) & verboseFlag;
            break;
        default:
            break;
        }
        if (check == 0)
            CheckItem(modifierHandle, theitem, false);
        else if (check > 0)
            CheckItem(modifierHandle, theitem, true);
        break;

    case screenMenu:
        switch (theitem) {
        case pauseItem:
            check = (modifiers ^= pauseFlag) & pauseFlag;
            screenControl("p", check);
            break;
        case scrollItem:
            check = (modifiers ^= scrollFlag) & scrollFlag;
            screenControl("s", check);
            break;
        default:
            break;
        }
        if (check == 0)
            CheckItem(screenHandle, theitem, false);
        else if (check > 0)
            CheckItem(screenHandle, theitem, true);
        break;

    case extractMenu:
        switch (theitem) {
        case screenItem:
            check = (modifiers ^= screenFlag) & screenFlag;
            break;
        case junkItem:
            check = (modifiers ^= junkFlag) & junkFlag;
            break;
        default:
            break;
        }
        if (check == 0)
            CheckItem(extractHandle, theitem, false);
        else if (check > 0)
            CheckItem(extractHandle, theitem, true);
        break;

    case caseMenu:
        switch (theitem) {
        case insensitiveItem:
            check = (modifiers ^= insensitiveFlag) & insensitiveFlag;
            break;
        case lowercaseItem:
            check = (modifiers ^= lowercaseFlag) & lowercaseFlag;
            break;
        default:
            break;
        }
        if (check == 0)
            CheckItem(caseHandle, theitem, false);
        else if (check > 0)
            CheckItem(caseHandle, theitem, true);
        break;

    case convertMenu:
        switch (theitem) {
        case autoItem:
            CheckItem(convertHandle, autoItem, true);
            CheckItem(convertHandle, binaryItem, false);
            CheckItem(convertHandle, textItem, false);
            modifiers &= (allFlags ^ textFlag);
            modifiers |= autoFlag;
            break;
        case binaryItem:
            CheckItem(convertHandle, autoItem, false);
            CheckItem(convertHandle, binaryItem, true);
            CheckItem(convertHandle, textItem, false);
            modifiers &= (allFlags ^ (autoFlag | textFlag));
            break;
        case textItem:
            CheckItem(convertHandle, autoItem, false);
            CheckItem(convertHandle, binaryItem, false);
            CheckItem(convertHandle, textItem, true);
            modifiers &= (allFlags ^ autoFlag);
            modifiers |= textFlag;
            break;
        default:
            break;
        }
        break;

    case overwriteMenu:
        switch (theitem) {
        case alwaysItem:
            CheckItem(overwriteHandle, alwaysItem, true);
            CheckItem(overwriteHandle, neverItem, false);
            CheckItem(overwriteHandle, promptItem, false);
            modifiers &= (allFlags ^ neverFlag);
            modifiers |= overwriteFlag;
            break;
        case neverItem:
            CheckItem(overwriteHandle, alwaysItem, false);
            CheckItem(overwriteHandle, neverItem, true);
            CheckItem(overwriteHandle, promptItem, false);
            modifiers &= (allFlags ^ overwriteFlag);
            modifiers |= neverFlag;
            break;
        case promptItem:
            CheckItem(overwriteHandle, alwaysItem, false);
            CheckItem(overwriteHandle, neverItem, false);
            CheckItem(overwriteHandle, promptItem, true);
            modifiers &= (allFlags ^ (neverFlag | overwriteFlag));
            break;
        default:
            break;
        }
        break;

    case infoMenu:
        switch (theitem) {
        case prtCommentItem:
            check = (modifiers ^= prtCommentFlag) & prtCommentFlag;
            break;
        case prtHeaderItem:
            check = (modifiers ^= prtHeaderFlag) & prtHeaderFlag;
            break;
        case prtTotalsItem:
            check = (modifiers ^= prtTotalsFlag) & prtTotalsFlag;
            break;
        default:
            break;
        }
        if (check == 0)
            CheckItem(infoHandle, theitem, false);
        else if (check > 0)
            CheckItem(infoHandle, theitem, true);
        break;

    case formatMenu:
        switch (theitem) {
        case filenameItem:
            CheckItem(formatHandle, filenameItem, true);
            CheckItem(formatHandle, longItem, false);
            CheckItem(formatHandle, mediumItem, false);
            CheckItem(formatHandle, shortItem, false);
            modifiers &= (allFlags ^ (longFlag | mediumFlag | shortFlag));
            modifiers |= filenameFlag;
            break;
        case longItem:
            CheckItem(formatHandle, filenameItem, false);
            CheckItem(formatHandle, longItem, true);
            CheckItem(formatHandle, mediumItem, false);
            CheckItem(formatHandle, shortItem, false);
            modifiers &= (allFlags ^ (filenameFlag | mediumFlag | shortFlag));
            modifiers |= longFlag;
            break;
        case mediumItem:
            CheckItem(formatHandle, filenameItem, false);
            CheckItem(formatHandle, longItem, false);
            CheckItem(formatHandle, mediumItem, true);
            CheckItem(formatHandle, shortItem, false);
            modifiers &= (allFlags ^ (filenameFlag | longFlag | shortFlag));
            modifiers |= mediumFlag;
            break;
        case shortItem:
            CheckItem(formatHandle, filenameItem, false);
            CheckItem(formatHandle, longItem, false);
            CheckItem(formatHandle, mediumItem, false);
            CheckItem(formatHandle, shortItem, true);
            modifiers &= (allFlags ^ (filenameFlag | longFlag | mediumFlag));
            modifiers |= shortFlag;
            break;
        default:
            break;
        }
        break;

    default:
        break;

    }

    HiliteMenu(0);
    return;
}

void dokey(myevent) EventRecord *myevent;
{
    char code;

    code = (char)(myevent->message & charCodeMask);

    if (myevent->modifiers & cmdKey) {
        if (myevent->what != autoKey) {
            domenu(MenuKey(code));
        }
    }

    return;
}

void domousedown(myevent) EventRecord *myevent;
{
    WindowPtr whichwindow;
    long code;

    code = FindWindow(myevent->where, &whichwindow);

    switch (code) {

    case inSysWindow:
        SystemClick(myevent, whichwindow);
        break;

    case inMenuBar:
        domenu(MenuSelect(myevent->where));
        break;

    }

    return;
}

int main(argc, argv) int argc; char *argv[];
{
    struct Globals saveGlobals;
    Boolean haveEvent, useWNE;
    short markChar;
    FILE *fp;

    FlushEvents(everyEvent, 0);
    InitGraf(&qd.thePort);
    InitFonts();
    InitWindows();
    InitMenus();
    TEInit();
    InitDialogs(nil);
    InitCursor();

    CONSTRUCTGLOBALS();

    sprintf(UnzipVersion, "%d.%d%d%s of %s", UZ_MAJORVER, UZ_MINORVER,
        PATCHLEVEL, BETALEVEL, VERSION_DATE);
    sprintf(ZipinfoVersion, "%d.%d%d%s of %s", ZI_MAJORVER, ZI_MINORVER,
        PATCHLEVEL, BETALEVEL, VERSION_DATE);

    c2pstr(UnzipVersion);
    c2pstr(ZipinfoVersion);

    SysEnvirons(1, &sysRec);
    useWNE = TrapAvailable(sysRec.machineType, _WaitNextEvent, ToolTrap);

    SetMenuBar(menubar = GetNewMBar(unzipMenuBar));
    DisposeHandle(menubar);
    InsertMenu(GetMenu(screenMenu), -1);
    InsertMenu(GetMenu(extractMenu), -1);
    InsertMenu(GetMenu(caseMenu), -1);
    InsertMenu(GetMenu(convertMenu), -1);
    InsertMenu(GetMenu(overwriteMenu), -1);
    InsertMenu(GetMenu(infoMenu), -1);
    InsertMenu(GetMenu(formatMenu), -1);
    AddResMenu(appleHandle = GetMHandle(appleMenu), 'DRVR');
    modifierHandle = GetMHandle(modifierMenu);
    screenHandle = GetMHandle(screenMenu);
    extractHandle = GetMHandle(extractMenu);
    caseHandle = GetMHandle(caseMenu);
    convertHandle = GetMHandle(convertMenu);
    overwriteHandle = GetMHandle(overwriteMenu);
    infoHandle = GetMHandle(infoMenu);
    formatHandle = GetMHandle(formatMenu);
    DrawMenuBar();

    screenOpen("Unzip");

    modifiers = 0;

    GetItemMark(modifierHandle, quietItem, &markChar);
    if (markChar) modifiers ^= quietFlag;
    GetItemMark(modifierHandle, verboseItem, &markChar);
    if (markChar) modifiers ^= verboseFlag;

    GetItemMark(screenHandle, pauseItem, &markChar);
    if (markChar) modifiers ^= pauseFlag;
    screenControl("p", markChar);
    GetItemMark(screenHandle, scrollItem, &markChar);
    if (markChar) modifiers ^= scrollFlag;
    screenControl("s", markChar);

    GetItemMark(extractHandle, screenItem, &markChar);
    if (markChar) modifiers ^= screenFlag;
    GetItemMark(extractHandle, junkItem, &markChar);
    if (markChar) modifiers ^= junkFlag;

    GetItemMark(caseHandle, insensitiveItem, &markChar);
    if (markChar) modifiers ^= insensitiveFlag;
    GetItemMark(caseHandle, lowercaseItem, &markChar);
    if (markChar) modifiers ^= lowercaseFlag;

    GetItemMark(convertHandle, autoItem, &markChar);
    if (markChar) modifiers ^= autoFlag;
    GetItemMark(convertHandle, textItem, &markChar);
    if (markChar) modifiers ^= textFlag;

    if ((modifiers & (autoFlag | textFlag)) == (autoFlag | textFlag)) {
        CheckItem(convertHandle, textItem, false);
        modifiers &= (allFlags ^ textFlag);
    } else if (modifiers & (autoFlag | textFlag))
        CheckItem(convertHandle, binaryItem, false);
    else
        CheckItem(convertHandle, binaryItem, true);

    GetItemMark(overwriteHandle, alwaysItem, &markChar);
    if (markChar) modifiers ^= overwriteFlag;
    GetItemMark(overwriteHandle, neverItem, &markChar);
    if (markChar) modifiers ^= neverFlag;

    if ((modifiers & (neverFlag | overwriteFlag)) == (neverFlag | overwriteFlag)) {
        CheckItem(overwriteHandle, alwaysItem, false);
        CheckItem(overwriteHandle, neverItem, false);
        CheckItem(overwriteHandle, promptItem, true);
        modifiers &= (allFlags ^ (neverFlag | overwriteFlag));
    } else if (modifiers & (neverFlag | overwriteFlag))
        CheckItem(overwriteHandle, promptItem, false);
    else
        CheckItem(overwriteHandle, promptItem, true);

    GetItemMark(infoHandle, prtCommentItem, &markChar);
    if (markChar) modifiers ^= prtCommentFlag;
    GetItemMark(infoHandle, prtHeaderItem, &markChar);
    if (markChar) modifiers ^= prtHeaderFlag;
    GetItemMark(infoHandle, prtTotalsItem, &markChar);
    if (markChar) modifiers ^= prtTotalsFlag;

    GetItemMark(formatHandle, filenameItem, &markChar);
    if (markChar) modifiers ^= filenameFlag;
    GetItemMark(formatHandle, longItem, &markChar);
    if (markChar) modifiers ^= longFlag;
    GetItemMark(formatHandle, mediumItem, &markChar);
    if (markChar) modifiers ^= mediumFlag;
    GetItemMark(formatHandle, shortItem, &markChar);
    if (markChar) modifiers ^= shortFlag;

    if (modifiers & longFlag) {
        CheckItem(formatHandle, filenameItem, false);
        CheckItem(formatHandle, mediumItem, false);
        CheckItem(formatHandle, shortItem, false);
        modifiers &= (allFlags ^ (filenameFlag | mediumFlag | shortFlag));
    } else if (modifiers & mediumFlag) {
        CheckItem(formatHandle, filenameItem, false);
        CheckItem(formatHandle, shortItem, false);
        modifiers &= (allFlags ^ (filenameFlag | shortFlag));
    } else if (modifiers & shortFlag) {
        CheckItem(formatHandle, filenameItem, false);
        modifiers &= (allFlags ^ filenameFlag);
    } else if (! (modifiers & filenameFlag)) {
        CheckItem(formatHandle, shortItem, false);
        modifiers |= shortFlag;
    }

    command = ' ';

    stop = false;
    while (!stop) {
        SetCursor(&qd.arrow);

        if (useWNE) {
            haveEvent = WaitNextEvent(everyEvent, &myevent, MAXLONG, NULL);
        } else {
            SystemTask();
            haveEvent = GetNextEvent(everyEvent, &myevent);
        }

        if (haveEvent) {
            switch (myevent.what) {

            case activateEvt:
                break;

            case keyDown:
            case autoKey:
                dokey(&myevent);
                break;

            case mouseDown:
                domousedown(&myevent);
                break;

            case updateEvt:
                screenUpdate(myevent.message);
                break;

            case mouseUp:
            case keyUp:
                break;

            default:
                break;

            }
        }

        if (command != ' ') {
            char *s, **v, modifierString[32];
            SFReply fileRep;
            Point p;
            int m, n;

            SetPt(&p, 40, 40);

            SFGetFile(p, "\pSpecify ZIP file:", 0L, -1, nil, 0L, &fileRep);
            if (fileRep.good) {
                MacFSTest(fileRep.vRefNum);
                ResolveMacVol(fileRep.vRefNum, &G.gnVRefNum, &G.glDirID, NULL);

                p2cstr((Ptr)fileRep.fName);

                modifierMask &= modifiers;

                s = modifierString;

                *s++ = '-';
                if ((command != 'x') && (command != 'Z')) *s++ = command;

                if (modifierMask) {
                    if (modifierMask & (autoFlag | textFlag)) *s++ = 'a';
                    if (modifierMask & textFlag) *s++ = 'a';
                    if (modifierMask & insensitiveFlag) *s++ = 'C';
                    if (!G.HFSFlag || (modifierMask & junkFlag)) *s++ = 'j';
                    if (modifierMask & lowercaseFlag) *s++ = 'L';
                    if (modifierMask & neverFlag) *s++ = 'n';
                    if (modifierMask & overwriteFlag) *s++ = 'o';
                    if (modifierMask & quietFlag) *s++ = 'q';
                    if (modifierMask & verboseFlag) *s++ = 'v';

                    if (modifierMask & prtCommentFlag) *s++ = 'z';
                    if (modifierMask & prtHeaderFlag) *s++ = 'h';
                    if (modifierMask & prtTotalsFlag) *s++ = 't';
                    if (modifierMask & filenameFlag) *s++ = '2';
                    if (modifierMask & longFlag) *s++ = 'l';
                    if (modifierMask & mediumFlag) *s++ = 'm';
                    if (modifierMask & shortFlag) *s++ = 's';
                }

                if (*(s - 1) == '-') s -= 1;

                *s = '\0';

                v = (char **)malloc(sizeof(char *));
                *v = "unzip";
                argc = 1;

                envargs(&argc, &v, NULL, NULL);

                argv = (char **)malloc((argc + 3) * sizeof(char *));

                argv[m = 0] = (command == 'Z') ? "zipinfo" : "unzip";
                if (*modifierString) argv[++m] = modifierString;
                argv[++m] = (char *)fileRep.fName;
                for (n = 1; n < argc; n++) argv[n + m] = v[n];
                argv[argc += m] = NULL;

                free(v);

                for (n = 0; argv[n] != NULL; n++) printf("%s ", argv[n]);
                printf("...\n\n");

                memcpy(&saveGlobals, &G, sizeof(struct Globals));

                unzip(__G__ argc, argv);
                
                memcpy(&G, &saveGlobals, sizeof(struct Globals));

                printf("\nDone\n");
            }

            fileList[0] = '\0';
            command = ' ';
        }
    }

    screenClose();

    DESTROYGLOBALS()

    ExitToShell();
}
