' * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
' * FreeDOS Updater                                                         *
' * Written by Mateusz Viste using FreeBASIC v0.18.3                        *
' * License: GNU/GPL v3                                                     *
' *                                                                         *
' * Note: FDUPDATE needs WGET or CURL to work.                              *
' * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *

#INCLUDE ONCE "getlsm.bi"    ' Needed to extract informations from LSM files.
#INCLUDE ONCE "readcfg.bi"   ' Needed to read the configuration file.
#INCLUDE ONCE "file.bi"      ' Needed to use the 'FileExists' function
#INCLUDE ONCE "bascubs.bi"   ' NLS Support - "BAS-cubs"

DECLARE SUB CheckForUpdates()
DECLARE SUB InstallNewSoft()
DECLARE SUB LoadPackageLists()
DECLARE SUB KeybFlush()
DECLARE SUB About()
DECLARE SUB CheckRequirements()
DECLARE SUB GetConfig()
DECLARE SUB LetsInstallPkg(MyPkg AS STRING, RemoveExistent AS BYTE = 1)
DECLARE FUNCTION GetCSV(RawCSV AS STRING, n AS INTEGER) AS STRING
DECLARE FUNCTION ShowMsg(Komunikat1 AS STRING, Komunikat2 AS STRING, Komunikat3 AS STRING, Komunikat4 AS STRING, Komunikat5 AS STRING) AS STRING

TYPE BOOL AS BYTE        ' Creating the BOOL type, as it
CONST TRUE AS BOOL = 1   ' is not supported natively by
CONST FALSE AS BOOL = 0  ' the FreeBASIC compiler.

CONST pVer AS STRING = "v0.54"
CONST pDate AS STRING = "2008"

CONST MaxPackages AS INTEGER = 2048  ' Defines the maximum number of packages which can be stored in arrays.

REM *** Initialising all shared variables ***
DIM SHARED AS INTEGER LocalPkg, RemotePkg
DIM SHARED AS STRING UpdateChannel, Downloader
DIM SHARED LocalPkgName(1 TO MaxPackages) AS STRING
DIM SHARED LocalPkgVer(1 TO MaxPackages) AS STRING
DIM SHARED LocalPkgDate(1 TO MaxPackages) AS STRING
DIM SHARED LocalPkgDesc(1 TO MaxPackages) AS STRING
DIM SHARED LocalPkgFull(1 TO MaxPackages) AS STRING
DIM SHARED RemotePkgName(1 TO MaxPackages) AS STRING
DIM SHARED RemotePkgVer(1 TO MaxPackages) AS STRING
DIM SHARED RemotePkgDate(1 TO MaxPackages) AS STRING
DIM SHARED RemotePkgDesc(1 TO MaxPackages) AS STRING
DIM SHARED RemotePkgFull(1 TO MaxPackages) AS STRING

REM *** Initialising private variables ***
DIM AS BOOL NewSoftware = FALSE

PRINT
PRINT "FreeDOS Updater "; pVer; " Copyright (C) Mateusz Viste "; pDate
PRINT
IF INSTR(LCASE(COMMAND(1)+COMMAND(2)), "/h") <> 0 OR INSTR(LCASE(COMMAND(1)+COMMAND(2)), "/?") THEN About
IF LCASE(COMMAND(1)) = "/new" THEN NewSoftware = TRUE
GetConfig
CheckRequirements

LoadPackageLists
IF NewSoftware = TRUE THEN
  InstallNewSoft
 ELSE
  CheckForUpdates
END IF

END


REM ***** END OF THE MAIN PROGRAM *** BELOW ARE ALL SUBS AND FUNCTIONS *****


SUB LoadPackageLists()
  DIM AS INTEGER DelimiterPos, ErrorLevel
  DIM AS STRING LineTmp, LSMfilename

 REM *** Compile the local packages list ***
  LocalPkg = 0
  PRINT NLSMessage(1, 1, "Checking installed packages...");
  LineTmp = DIR(ENVIRON("DOSDIR")+"\packages\*.*", &h10)
  DO
    LineTmp = TRIM(DIR)
    IF LineTmp <> "." AND LineTmp <> ".." AND LineTmp <> "" THEN
      LocalPkg += 1
      LocalPkgName(LocalPkg) = LineTmp
    END IF
  LOOP UNTIL LineTmp = ""

  FOR DelimiterPos = 1 TO LocalPkg
    LSMfilename = ENVIRON("DOSDIR") + "\appinfo\" + MID(LocalPkgName(DelimiterPos), 1, LEN(LocalPkgName(DelimiterPos)) - 1) + ".lsm"
    LocalPkgVer(DelimiterPos) = GetLSM(LSMfilename, "version")
    LocalPkgDate(DelimiterPos) = GetLSM(LSMfilename, "entered-date")
    LocalPkgDesc(DelimiterPos) = GetLSM(LSMfilename, "description")
    LocalPkgFull(DelimiterPos) = GetLSM(LSMfilename, "name")
  NEXT DelimiterPos
  PRINT " "; NLSMessage(1, 2, "Done.");
  PRINT

 REM *** Retrieve the remote packages list ***

  PRINT NLSMessage(1, 3, "Retrieving the remote packages list...")

  SELECT CASE LCASE(Downloader)
    CASE "curl"
      SHELL Downloader + " " + UpdateChannel + "/fdupdate.tab -o " + ENVIRON("TEMP") + "\fdupdate.tab"
    CASE "wget"
      SHELL Downloader + " " + UpdateChannel + "/fdupdate.tab -O " + ENVIRON("TEMP") + "\fdupdate.tab"
  END SELECT
  IF FileExists(ENVIRON("TEMP") + "\fdupdate.tab") = 0 THEN END
  RemotePkg = 0
  OPEN ENVIRON("TEMP") + "\fdupdate.tab" FOR INPUT AS #1
  DO
    RemotePkg += 1
    LINE INPUT #1, LineTmp
    RemotePkgName(RemotePkg) = GetCSV(LineTmp, 1)
    RemotePkgFull(RemotePkg) = GetCSV(LineTmp, 2)
    RemotePkgVer(RemotePkg) = GetCSV(LineTmp, 3)
    RemotePkgDate(RemotePkg) = GetCSV(LineTmp, 4)
    RemotePkgDesc(RemotePkg) = GetCSV(LineTmp, 5)
  LOOP UNTIL EOF(1)
  CLOSE #1
  KILL ENVIRON("TEMP") + "\fdupdate.tab"
END SUB


SUB CheckRequirements()
REM Here I check for all FDUPDATE's requirements. If any of them isn't
REM fullfilled, then the program stops with an ERRORLEVEL 9.

REM ** Checking for FDPKG **
'  WriteMe!

REM ** Checking for FDPKG **
'  WriteMe!

REM ** Checking for %DOSDIR% **
IF ENVIRON("DOSDIR") = "" THEN
  PRINT NLSMessage(2, 1, "You need to have a 'DOSDIR' environment variable! Sorry.")
  END(9)
END IF

REM ** Checking for %TEMP% **
IF ENVIRON("TEMP") = "" THEN
  PRINT NLSMessage(2, 2, "You need to have a 'TEMP' environment variable! Sorry.")
  END(9)
END IF

REM ** Checking for the downloader **
'IF DIR(ENVIRON("DOSDIR")+"\packages\wgetx", &h10) = "" THEN
'  END(9)
'END IF

END SUB


SUB GetConfig()
  DIM AS STRING CFGfile, TempString
  CFGfile = EXEPATH + "\" + "fdupdate.cfg"
  Downloader = ReadCFG(CFGfile, "Downloader")
  UpdateChannel = ReadCFG(CFGfile, "Repository")
  IF LCASE(Downloader) = "wget" OR LCASE(Downloader) = "curl" THEN
      PRINT NLSMessage(3, 5, "Downloader:"); " "; Downloader
    ELSE
      Downloader = "wget"
      PRINT NLSMessage(3, 6, "Using the default download command:"); " "; Downloader
  END IF
  IF UpdateChannel <> "" THEN
      PRINT NLSMessage(3, 1, "Update server:"); " "; UpdateChannel
    ELSE
      UpdateChannel = "http://www.freedos.org/fdupdate"
      PRINT NLSMessage(3, 4, "Using the default update server at"); " "; UpdateChannel
  END IF
END SUB


FUNCTION ShowMsg(Komunikat1 AS STRING, Komunikat2 AS STRING, Komunikat3 AS STRING, Komunikat4 AS STRING, Komunikat5 AS STRING) AS STRING
  DIM AS STRING LastKey
  DIM AS INTEGER OldCol, OldRow
  OldRow = CSRLIN
  OldCol = POS
  PCOPY 0, 1
  COLOR 14, 1
  LOCATE  9, 25: PRINT "ษออออออออออออออออออออออออออออออออออออออป";
  LOCATE 10, 25: PRINT "บ                                      บ";
  LOCATE 11, 25: PRINT "บ                                      บ";
  LOCATE 12, 25: PRINT "บ                                      บ";
  LOCATE 13, 25: PRINT "บ                                      บ";
  LOCATE 14, 25: PRINT "บ                                      บ";
  LOCATE 15, 25: PRINT "ศออออออออออออออออออออออออออออออออออออออผ";

  COLOR 7
  LOCATE 10, 27: PRINT MID(Komunikat1, 1, 36);
  LOCATE 11, 27: PRINT MID(Komunikat2, 1, 36);
  LOCATE 12, 27: PRINT MID(Komunikat3, 1, 36);
  LOCATE 13, 27: PRINT MID(Komunikat4, 1, 36);
  LOCATE 14, 27: PRINT MID(Komunikat5, 1, 36);

  DO : SLEEP : LastKey = INKEY : LOOP UNTIL LastKey <> ""
  KeybFlush
  COLOR 7, 0
  PCOPY 1, 0
  LOCATE OldRow, OldCol
  RETURN LastKey
END FUNCTION


SUB About()
  PRINT NLSMessage(0, 1, "FreeDOS Updater allows you to easily maintain your FreeDOS system up to date.")
  PRINT NLSMessage(0, 2, "It requires the following applications to work:")
  PRINT "    "; NLSMessage(0, 3, "WGET or CURL - For downloading tasks")
  PRINT "    "; NLSMessage(0, 4, "FDPKG        - For installing/deleting tasks")
  PRINT
  PRINT NLSMessage(0, 5, "Usage: FDUPDATE [/new] [/h]")
  PRINT " "; NLSMessage(0, 6, "/new - Allows to install new packages from the repository")
  PRINT " "; NLSMessage(0, 7, "/h   - Displays the help screen")
  END(3)
END SUB


SUB CheckForUpdates()
  DIM AS INTEGER LocalPkgCounter, RemotePkgCounter, UpdatedCounter
  DIM AS BOOL UpdateProposition = FALSE

  FOR RemotePkgCounter = 1 TO RemotePkg
   LocalPkgCounter = 0
   DO
     LocalPkgCounter += 1
   LOOP UNTIL UCASE(LocalPkgName(LocalPkgCounter)) = UCASE(RemotePkgName(RemotePkgCounter)) OR LocalPkgCounter = LocalPkg
   IF LocalPkgVer(LocalPkgCounter) <> RemotePkgVer(RemotePkgCounter) AND UCASE(LocalPkgName(LocalPkgCounter)) = UCASE(RemotePkgName(RemotePkgCounter)) THEN
      UpdateProposition = TRUE
      IF UCASE(ShowMsg(NLSMessage(4, 1, "Different package version detected!"), MID(NLSMessage(4, 2, "Local package:") + SPACE(17), 1, 17) + LocalPkgName(LocalPkgCounter) + " " + LocalPkgVer(LocalPkgCounter), MID(NLSMessage(4, 3, "Remote package:") + SPACE(17), 1, 17) + RemotePkgName(RemotePkgCounter) + " " + RemotePkgVer(RemotePkgCounter),  "", SPACE(18 - LEN(NLSMessage(4, 4, "Update it? [Y/N]")) / 2) + NLSMessage(4, 4, "Update it? [Y/N]") + " ")) = NLSMessage(4, 5, "Y") THEN
          UpdatedCounter += 1
          LetsInstallPkg(RemotePkgName(RemotePkgCounter))
      END IF
   END IF
  NEXT RemotePkgCounter

  PRINT
  IF UpdateProposition = TRUE THEN
      PRINT NLSMessage(4, 6, "Finished."); " "; UpdatedCounter; " "; NLSMessage(4, 7, "package(s) have been updated.")
    ELSE
      PRINT NLSMessage(4, 8, "No updates available!")
  END IF
END SUB


SUB InstallNewSoft()
  DIM AS INTEGER LocalPkgCounter, RemotePkgCounter, AvSoftware, OldRow, OldCol, Choice, Counter, DispPackage, CurPage
  DIM AS STRING Pozycja, LastKey
  DIM AvSoftwareID(1 TO MaxPackages) AS INTEGER

  OldRow = CSRLIN
  OldCol = POS
  PCOPY 0, 1
  Choice = 1

  FOR RemotePkgCounter = 1 TO RemotePkg
   LocalPkgCounter = 0
   DO
     LocalPkgCounter += 1
   LOOP UNTIL UCASE(LocalPkgName(LocalPkgCounter)) = UCASE(RemotePkgName(RemotePkgCounter)) OR LocalPkgCounter = LocalPkg
   IF UCASE(LocalPkgName(LocalPkgCounter)) <> UCASE(RemotePkgName(RemotePkgCounter)) THEN
     AvSoftware += 1
     AvSoftwareID(AvSoftware) = RemotePkgCounter
   END IF
  NEXT RemotePkgCounter

  COLOR 14, 1
  CLS
  LOCATE  1, 1: PRINT "ฺ"; STRING(29, "ฤ"); "[ FreeDOS Updater ]"; STRING(30, "ฤ"); "ฟ";
  LOCATE  2, 1: PRINT "ณ"; SPACE(78); "ณ";
  LOCATE  3, 1: PRINT "ณ"; SPACE(78); "ณ";
  LOCATE  4, 1: PRINT "ณ"; SPACE(78); "ณ";
  LOCATE  5, 1: PRINT "ณ"; SPACE(78); "ณ";
  LOCATE  6, 1: PRINT "ณ"; SPACE(78); "ณ";
  LOCATE  7, 1: PRINT "ณ"; SPACE(78); "ณ";
  LOCATE  8, 1: PRINT "ณ"; SPACE(78); "ณ";
  LOCATE  9, 1: PRINT "ณ"; SPACE(78); "ณ";
  LOCATE 10, 1: PRINT "ณ"; SPACE(78); "ณ";
  LOCATE 11, 1: PRINT "ณ"; SPACE(78); "ณ";
  LOCATE 12, 1: PRINT "ณ"; SPACE(78); "ณ";
  LOCATE 13, 1: PRINT "ณ"; SPACE(78); "ณ";
  LOCATE 14, 1: PRINT "ณ"; SPACE(78); "ณ";
  LOCATE 15, 1: PRINT "ณ"; SPACE(78); "ณ";
  LOCATE 16, 1: PRINT "ณ"; SPACE(78); "ณ";
  LOCATE 17, 1: PRINT "ณ"; SPACE(78); "ณ";
  LOCATE 18, 1: PRINT "ณ"; SPACE(78); "ณ";
  LOCATE 19, 1: PRINT "ณ"; SPACE(78); "ณ";
  LOCATE 20, 1: PRINT "ภ"; STRING(78, "ฤ"); "ู";
  LOCATE 21, 1: PRINT "ฺ"; STRING(78, "ฤ"); "ฟ";
  LOCATE 22, 1: PRINT "ณ"; SPACE(78); "ณ";
  LOCATE 23, 1: PRINT "ณ"; SPACE(78); "ณ";
  LOCATE 24, 1: PRINT "ภ"; STRING(78, "ฤ"); "ู";
  COLOR 0, 3
  LOCATE 25, 1: PRINT MID(" " + NLSMessage(5, 1, "Up/Down & PgUp/PgDn: Browse packages   Enter: Install the package   Esc: Quit") + SPACE(80), 1, 80);

  DO
    CurPage = INT((Choice - 1) / 18) + 1
    FOR Counter = 1 TO 18
      IF Choice MOD 18 = Counter OR Choice = Counter THEN COLOR 15, 2 ELSE COLOR 7, 1
      LOCATE Counter + 1, 2, 0
      DispPackage = (CurPage - 1) * 18 + Counter
      IF DispPackage <= AvSoftware THEN
          Pozycja = MID(" " + RemotePkgFull(AvSoftwareID(DispPackage)) + " " + RemotePkgVer(AvSoftwareID(DispPackage)) + " [" + RemotePkgDate(AvSoftwareID(DispPackage)) + "]" + SPACE(78), 1, 78)
        ELSE
          Pozycja = SPACE(78)
      END IF
      PRINT Pozycja;
    NEXT Counter

    COLOR 14, 1: LOCATE 20, 1: PRINT "ภ"; RIGHT(STRING(80, "ฤ") + "[ " + NLSMessage(5, 2, "Page") + ": " + STR(CurPage), 75); " ]ฤู";
    COLOR 7, 1
    LOCATE 22, 3
    PRINT MID(RemotePkgDesc(AvSoftwareID(Choice)) + SPACE(76), 1, 76);
    LOCATE 23, 3
    IF LEN(MID(RemotePkgDesc(AvSoftwareID(Choice)), 77)) > 76 THEN
        PRINT MID(RemotePkgDesc(AvSoftwareID(Choice)), 77, 73); "...";
      ELSE
        PRINT MID(RemotePkgDesc(AvSoftwareID(Choice)) + SPACE(76 * 2), 77, 76);
    END IF

    DO : SLEEP : LastKey = INKEY : LOOP UNTIL LastKey <> ""
    KeybFlush
    SELECT CASE LastKey
      CASE CHR(255) + "H"  ' Gora
        IF Choice > 1 THEN Choice -= 1
      CASE CHR(255) + "P"  ' Dol
        IF Choice < AvSoftware THEN Choice += 1
      CASE CHR(255) + "I"  ' PgUp
        IF Choice > 18 THEN Choice -= 18 ELSE Choice = 1
      CASE CHR(255) + "Q"  ' PgDown
        IF Choice < (AvSoftware - 18) THEN Choice += 18 ELSE Choice = AvSoftware
    END SELECT
  LOOP UNTIL LastKey = CHR(27) OR LastKey = CHR(13)

  COLOR 7, 0
  PCOPY 1, 0
  LOCATE OldRow, OldCol, 1

  IF LastKey = CHR(13) THEN
      LetsInstallPkg(RemotePkgName(AvSoftwareID(Choice)), 0)
      PRINT RemotePkgFull(AvSoftwareID(Choice)); " "; NLSMessage(5, 3, "has been installed!")
    ELSE
      PRINT NLSMessage(5, 4, "No new package installed!")
  END IF
  END(0)
END SUB


FUNCTION GetCSV(RawCSV AS STRING,n AS INTEGER) AS STRING
    ' GetCSV analizes a string in CSV format (comma or tab delimited), and
    ' returns the value of the n-th field. If the field doesn't exist, it
    ' returns a null string.
    DIM AS STRING FieldValue
    DIM AS INTEGER Counter, CommaPos, ValueLen
    CommaPos = 0
    FOR Counter = 1 TO n - 1
        DO
            CommaPos += 1
        LOOP UNTIL MID(RawCSV, CommaPos, 1) = CHR(9) OR CommaPos = LEN(RawCSV)
    NEXT Counter
    ValueLen = -1
    DO
        ValueLen += 1
    LOOP UNTIL MID(RawCSV, CommaPos + ValueLen + 1, 1) = CHR(9) OR CommaPos + ValueLen = LEN(RawCSV)
    FieldValue = MID(RawCSV, CommaPos + 1, ValueLen)
    RETURN FieldValue
END FUNCTION


SUB KeybFlush
  DO: LOOP UNTIL INKEY = ""
END SUB


SUB LetsInstallPkg(MyPkg AS STRING, RemoveExistent AS BYTE = 1)
  SELECT CASE LCASE(Downloader)
    CASE "curl"
      SHELL Downloader + " " + UpdateChannel + "/" + MyPkg + ".zip -o " + ENVIRON("TEMP") + "\" + LCASE(MyPkg) + ".zip"
    CASE "wget"
      SHELL Downloader + " " + UpdateChannel + "/" + MyPkg + ".zip -O " + ENVIRON("TEMP") + "\" + LCASE(MyPkg) + ".zip"
  END SELECT
  IF FileExists(ENVIRON("TEMP") + "\" + LCASE(MyPkg) + ".zip") = 0 THEN END
  IF RemoveExistent = 1 THEN SHELL "FDPKG /REMOVE /Y " + MyPkg
  SHELL "FDPKG /INSTALL " + ENVIRON("TEMP") + "\" + LCASE(MyPkg) + ".zip"
  KILL ENVIRON("TEMP") + "\" + LCASE(MyPkg) + ".zip"
END SUB
