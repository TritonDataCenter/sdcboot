//--------------------------------------------------------------------------
// This is written in Turbo C++ for DOS version 3.0.
// Only standard Libraries are needed.  All "special" code is contained
//   directly in this source file.
//--------------------------------------------------------------------------

#pragma option -mt
               //Tiny Memory Model

// Need to process Break, Ctrl-C, and Critical Error Interrupts ourselves
//   while reading/writing in DOS.  On Chris' computer, a failure reading
//   or writing to the Epson printer causes multiple Abort/Retry/Fail
//   messages to appear (we try multiple times and they all fail).
// Either that, or we just need to use the BIOS.


//--------------------------------------------------------------------------
// This program MUST be compiled with the Tiny Memory Model, since some
//   functions require CS = DS!!
//--------------------------------------------------------------------------

//--------------------------------------------------------------------------
// After compiling, the file is in .EXE format and is named with a .EXE
//   extension.  We rename to a .COM extension.  This does not affect the
//   operation of the program at all, but simply allows our other USB
//   programs to be able to find it.  We would normally have TurboCPP
//   compile directly to a .COM format (by using the -lt command-line switch),
//   but it won't compile correctly that way (even though it should).
//--------------------------------------------------------------------------

//--------------------------------------------------------------------------
// This program attempts to determine and display the Level of Ink(s)
//   left in a printer.  At least for now, the only printers that are
//   compatible with this program are certain ones made by
//   HP (Hewlett-Packard) and Epson.  Also, as far as I know, it will
//   only work with InkJet printers, and not with Dot-Matrix or Laser
//   or other types of Printers (though I don't know that for sure).
//
// In addition, this program requires certain "extended" functions to be
//   provided by the BIOS in order to talk to the Printer.  The
//   extended functions do not exist in any standard BIOS available
//   today, though, but are only provided by an external program.  Today,
//   the only program available that provides those functions is my
//   USBPRINT program (whose main purpose in life is to allow DOS printing
//   to USB-attached printers).  You will need to install the USBPRINT
//   program into memory (it's a TSR) before you try to run this INKLEVEL
//   program, or it won't work.
//
// The "extended" functions provided by USBPRINT are both at the BIOS level
//   and at the DOS level.  It is good programming practice to use the
//   highest level functions that work properly (first DOS, then
//   BIOS, then direct hardware only as a last resort when nothing else
//   works).  Unfortunately, depending which version of DOS (and which
//   manufacturer of DOS) the user is currently using, the DOS functions
//   may not always work properly.  The BIOS services always work, no
//   matter what the shortcoming of the current version of DOS happen
//   to be.  In this program, we try to do everything through DOS services
//   and use the BIOS services only if the DOS services don't work.  This
//   is done to prove that the DOS services can and do work when DOS does
//   what it's supposed to.  This program is therefore larger and slower
//   than it needs to be to do the job, but shows what it is possible to
//   do with plain DOS.
//
// This program simply displays the Ink Levels as reported by the
//   printer, which are not always accurate.  Almost every printer I've
//   seen still has at least some Ink remaining even when it reports
//   that the Ink cartridge is empty.  One reason they do this is to try
//   and sell you more Ink than you actually need, by "scaring" you into
//   thinking you're out of Ink when you really aren't.  Epson has actually
//   been sued over such false reporting, and I've seen HP printers that
//   will successfully print hundreds of pages after it says an
//   Ink Cartridge is empty.  So, take the reported Ink Levels with a
//   grain of salt, and just use them as a general guideline rather than
//   as an absolute fact.
//--------------------------------------------------------------------------


//±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±
// Includes
//±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±

#include <conio.h>
#include <ctype.h>
#include <dos.h>
#include <fcntl.h>
#include <io.h>
#include <iomanip.h>
#include <iostream.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


//±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±
// Function Prototypes
//±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±

void GetCmdLine(void);

int  ParseCmdLine(char *Pointer, int Testing);
int  SkipSpaces(char **Pointer);
int  SkipDashSlash(char **Pointer);

int  WriteInkLevel(int LPTIndex, int SpecificRequest);
int  GetOldHPInk(int LPTIndex);
int  GetNewHPInk(int LPTIndex);
int  GetEpsonInk(int LPTIndex);
int  GetEpsonStatus(int LPTIndex, int Retry);

void FixDeviceIDString(void);
int  TestPrinterClass(void);
int  TestCompatibility(void);
int  GetHexPercent(char *HexCharsIn);
int  GetDecPercent(char *DecCharsIn);

void WriteHeaderStart(int LPTIndex);
void WriteGraphHeader(int LPTIndex, char *InkStatusMsg, int SpacesInFront);
void WriteColorLine(char ForeColor, char *InkColorMsg, int Percent);
void WriteHeaderErrNoDvcID(int LPTIndex, char *ErrorMsg);
void WriteHeaderErrWithDvcID(int LPTIndex, char *ErrorMsg);
void WriteHeaderErrWithIQ(int LPTIndex, char *ErrorMsg);
void WriteHeaderErrWithTag(int LPTIndex, char *TagMsg);
void WriteDeviceID(void);
int  WriteDvcDescr(void);
void WriteEpsonInkQuantity(void);

int  I17XTestInstall(void);
int  I17XTestPort(int LPTIndex);
int  TestPrinter(int LPTIndex);
int  I17XGetDeviceID(int Port, int BufferSeg, int BufferOffset, int Size);
int  I17XGetData(int Port, int BufferSeg, int BufferOffset, int Size);
int  I17XSendData(int Port, int BufferSeg, int BufferOffset, int Size);

void GetScreenRows(void);
void WriteQuery(char *String);
void WriteString(char *String);
void WriteStringColor(char *String);
void WritePercentColor(int Percent);
void RepeatChar(char *Character, int Count);
void RepeatCharColor(char *Character, int Count);

void WriteCopyright(void);
void WindowsNTTest(void);
void ResetString(char *Pointer, int Size);
void BeepSpeaker(void);


//±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±
// CONSTANTS
//±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±

//--------------------------------------------------------------------------
// Our Program name
//--------------------------------------------------------------------------
#define ProgramName "INKLEVEL" //Progra Name

//--------------------------------------------------------------------------
// ErrorLevels returned to DOS Prompt
//--------------------------------------------------------------------------
#define ErLvlNoError   0 //No Error
#define ErLvlEnviron   1 //Environment variable error
#define ErLvlCmdLine   2 //Command-line error
#define ErLvlWindowsNT 3 //Running under Windows NT
#define ErLvlNoExtSvc  4 //Extended Printer Services not installed


//--------------------------------------------------------------------------
// Printer Types we can handle
//--------------------------------------------------------------------------
#define PrinterTypeEpson 1 //Epson-compatible Printer
#define PrinterTypeOldHP 2 //Old HP-compatible (contains VSTATUS: Tag)
#define PrinterTypeNewHP 3 //New HP-compatible (contains S: Tag)


//---------------------------------------------------------------------------
// Status byte returned by Parallel Port BIOS (Int 17h, Function 02h)
//---------------------------------------------------------------------------
#define PStsTimeout     0x01 //Timeout
#define PStsIOError     0x08 //I/O Error
#define PStsSelected    0x10 //Selected
#define PStsNoPaper     0x20 //Out of Paper
#define PStsACKnowledge 0x40 //ACK received
#define PStsNotBusy     0x80 //Not Busy (Ready)


//--------------------------------------------------------------------------
// Extended INT 17h Functions
//--------------------------------------------------------------------------
#define I17BXInput 0x4c50 //BX = 'LP'
#define I17CXInput 0x5421 //CX = 'T!'
#define I17DXInput 0xffff //DX = -1

#define I17AXInstallCheck    0x1b00 //Installation Check
#define I17AXJobStart        0x1b01 //Start a New Print Job
#define I17AXJobEnd          0x1b02 //End an Existing Print Job
#define I17AXJobCancel       0x1b03 //Cancel an Existing Print Job
#define I17AXRedirGoingAway  0x1b0F //Pgm containing Redir Tbl is disappearing
#define I17AXGetRedirTblAddr 0x1b10 //Get Address of LPT Redirection Table
#define I17AXGetPortRedir    0x1b11 //Get Port Redirect Status
#define I17AXSetPortRedir    0x1b12 //Set Port Redirect Status
#define I17AXGetDvcIDString  0x1b20 //Get Device ID String
#define I17AXTxBlock         0x1b21 //Transmit (Send) Block of Data
#define I17AXRxBlock         0x1b22 //Receive Block of Data


//--------------------------------------------------------------------------
// Extended INT 17h Error Codes
//--------------------------------------------------------------------------
#define I17ErrBlockTimeout   0x0008 //Timeout during Block Transfer
#define I17ErrIncompleteXfer 0x0009 //Incomplete Block Transfer (not Timeout)


//--------------------------------------------------------------------------
// Buffer Sizes
//--------------------------------------------------------------------------
#define DeviceIDSize    1024 //Size of Buffer for Device ID String
#define InkQuantitySize  512 //Size of Buffer for Epson Ink Quantity String


//--------------------------------------------------------------------------
// Standard DOS "hard-coded" Device Handles
//--------------------------------------------------------------------------
#define StdInHandle  0 //Standard Input
#define StdOutHandle 1 //Standard Output
#define StdErrHandle 2 //Standard Error
#define StdAuxHandle 3 //Standard Auxiliary (COM1)
#define StdPrnHandle 4 //Standard Printer (LPT1)


//--------------------------------------------------------------------------
// Codes returned by INT 21h, Function 4400h (Get Device Information)
//--------------------------------------------------------------------------
  //If bit 7 is Set (is a Device):
#define DvcInfoIsStdIn    0x0001 //STDIN
#define DvcInfoIsStdOut   0x0002 //STDOUT
#define DvcInfoIsNUL      0x0004 //NUL
#define DvcInfoIsClock    0x0008 //CLOCK$
#define DvcInfoInt29h     0x0010 //Uses Int 29h (Special Device)
#define DvcInfoBinary     0x0020 //Binary (Raw) Mode
#define DvcInfoEOF        0x0040 //EOF on Input
#define DvcInfoIsDevice   0x0080 //Is a Device (not a File)
#define DvcInfoKEYB       0x0100 //Unknown (set by DOS 6.2x KEYB program)
#define DvcInfoOpenClose  0x0800 //Supports Open/Close calls
#define DvcInfoOutputBusy 0x2000 //Supports Output until Busy
#define DvcInfoIOCTL      0x4000 //Can process IOCTL requests (Func 4402h)

  //If bit 7 is Clear (is a File):
#define DvcInfoDriveMask    0x003f //Drive number (0 = A:)
#define DvcInfoNotWritten   0x0040 //File not been written (Dirty Buffer)
#define DvcInfoInt24h       0x0100 //Generate Int 24h on Errors (DOS 4 only)
#define DvcInfoNotRemovable 0x0800 //Media Not Removable
#define DvcInfoNoDateTime   0x4000 //Don't update File Date/Time (DOS 3.0+)
#define DvcInfoRemote       0x8000 //File is Remote (DOS 3.0+)


//--------------------------------------------------------------------------
// Category Codes for DOS Generic IOCTL Functions (goes in CH)
//--------------------------------------------------------------------------
#define IOCatUnknown      0x00 //Unknown (DOS 3.3+)
#define IOCatCOMx         0x01 //COMx
#define IOCatTermReserved 0x02 //Reserved for Terminal Control
#define IOCatCON          0x03 //CON
#define IOCatKeybReserved 0x04 //Reserved for Keyboard Control
#define IOCatLPTx         0x05 //LPTx
#define IOCatEuroMouse    0x07 //Mouse Control (European DOS 4.0)
#define IOCatDisk         0x08 //Disk Control (INT 21/440Dh)
#define IOCatFAT32Disk    0x48 //FAT32 Disk Control (INT 21/440Dh)


//--------------------------------------------------------------------------
// Functions Codes for DOS Generic IOCTL Functions (goes in CL)
//--------------------------------------------------------------------------
#define IOCmdSetScreenMode 0x40 //Set Screen Mode (DOS 3 only)
#define IOCmdGetScreenMode 0x60 //Get Screen Mode (DOS 3 only)
#define IOCmdGetDeviceID   0x60 //Get Printer Device ID (the one we've added!)
#define IOCmdSetRetry      0x45 //Set Printer Iteration (retry) count
#define IOCmdGetRetry      0x65 //Get Printer Iteration (retry) count
#define IOCmdSetCodePage   0x4A //Set (Select) Code Page
#define IOCmdGetCodePage   0x6A //Get Code Page
#define IOCmdGetCodePagePr 0x6B //Get Code Page Prepare List (DOS 4+)
#define IOCmdStartCodePage 0x4C //Start Code Page Preparation
#define IOCmdEndCodePage   0x4D //End Code page Preparation
#define IOCmdSetDisplay    0x5F //Set Display Info (DOS 4+)
#define IOCmdGetDisplay    0x7F //Get Display Info (DOS 4+)


//±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±
// STRUCTURES
//±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±

//--------------------------------------------------------------------------
// STRUCTURE FOR DOS GENERIC IOCTL GETDEVICEID FUNCTION
//--------------------------------------------------------------------------

struct GetDeviceIDStruc
 {
  unsigned char IOCtlFlags;       //Flags
                                  //  Bit 0 = No short packet error
                                  //  All other bits reserved
  unsigned char IOCtlReserved;    //Reserved
  unsigned int  IOCtlBuffSize;    //Maximum Buffer Size (bytes)
  unsigned int  IOCtlBuffOffset;  //Buffer Offset
  unsigned int  IOCtlBuffSegment; //Buffer Segment
 };


//±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±
// GLOBAL VARIABLES
//±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±

char DeviceIDRaw[DeviceIDSize];    //Buffer for Device ID String
char InkQuantity[InkQuantitySize]; //Buffer for Ink Epson Quantity String
char CommandLine[128];             //Original Command Line
char TextAttrib;                   //Original Text Attribute
int  ScreenRows;                   //Number of screen rows
int  RowCount;                     //Row Counter for Auto-Pausing
int  IsColor;                      //1 if Color Screen, 0 if Monochrome
int  PrinterType;                  //Printer Type (Epson, OldHP, NewHP)


//±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±
// PROGRAM CODE
//±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±

//--------------------------------------------------------------------------
// THE MAIN PROGRAM CODE
//--------------------------------------------------------------------------

int main(void)

 {
  int   LPTIndex;       //LPT Index Number
  int   WriteCount;     //Number of printers written
  char *EnvironmentVar; //Environment Variable(s) Pointer
  int   EnvReturn;      //Environment Parse Return Value
  int   CmdLineReturn;  //Command Line Parse Return Value

  GetScreenRows();  //Get ScreenRows and Color/Mono Attributes
  WriteCopyright(); //Display Copyright Message
  GetCmdLine();     //Copy command line tail to local string

  EnvironmentVar = getenv(ProgramName);
  if (EnvironmentVar != NULL)                    //If Environment exists
   {
    EnvReturn = ParseCmdLine(EnvironmentVar,1);  //Test Environment
    if (EnvReturn == -1)                         //If Environment Error
     {                                           //Write error message
      cerr << "\n"
           << "Error in the Environment variable:\n"
           << "  "
           << ProgramName
           << "="
           << EnvironmentVar
           << "\n\n"
           << "Reset the Environment variable (type 'SET "
           << ProgramName
           << "=') and then\n"
           << "  type '"
           << ProgramName
           << " ?' for Help."
           << "\n\n";
      exit(ErLvlEnviron);                        //Quit program
     }
   }

  CmdLineReturn = ParseCmdLine(CommandLine,1);   //Test Command Line
  if (CmdLineReturn == -1)                       //If Error in Command-line
    {                                            //Write error message
     cerr << "\n"
          << "SYNTAX: "
          << ProgramName
          << " [LPTx] [LPTx] ...\n"
          << "          where [LPTx] is a Printer Port number between LPT1 and LPT9.\n"
          << "\n"
          << "        "
          << ProgramName
          << "\n"
          << "           with no LPT numbers after it will display\n"
          << "           Ink Quantity information for all Epson- and HP-compatible Printers\n"
          << "           that it can find."
          << "\n\n";
     exit(ErLvlCmdLine);                         //Quit program
    }

  WindowsNTTest();             //Test for Windows NT
                               //  Exits program with Error Message if so
  if (I17XTestInstall() == -1) //If no Extended Printer Services
   {                           //Write Error Message
    cerr << "\n"
         << "Extended Printer Services are not installed.\n"
         << "Install a program like USBPRINT into memory and try again."
         << "\n";
    BeepSpeaker();             //Beep the speaker
    exit(ErLvlNoExtSvc);       //Quit program
   }

  if ((EnvironmentVar != NULL) &&   //If valid Environment
      (EnvReturn != 0))
   {                                //Write environment string
    WriteString("\n");
    WriteString("Environment: ");
    WriteString(ProgramName);
    WriteString("=");
    WriteString(EnvironmentVar);
    WriteString("\n");
    ParseCmdLine(EnvironmentVar,0); //Process Environment
    ParseCmdLine(CommandLine,0);    //Process Command Line
    WriteString("\n");
    return(ErLvlNoError);           //Quit program
   }

  if (CmdLineReturn != 0)                         //If values on Cmd Line,
    ParseCmdLine(CommandLine,0);                  //  process them
   else                                           //If no values on Cmd Line
   {
    WriteCount = 0;                               //Initialize Counter
    for (LPTIndex = 0; LPTIndex <= 8; LPTIndex++) //Do all possible LPTx's
      if (WriteInkLevel(LPTIndex,0) != 0)         //Write InkLevel
        WriteCount++;                             //If valid, increment Counter
    if (WriteCount == 0)                          //Any valid printers?
      WriteString("\nNo Epson- or HP-compatible printers were found.\n");
   }
  WriteString("\n");                              //Move down
  return(ErLvlNoError);                           //Done
 }


//±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±
// PARSE THE COMMAND LINE AND/OR ENVIRONMENT VARIABLE(S) FOR OPTIONS
//±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±

//--------------------------------------------------------------------------
// Copy the original command line contents to our local buffer,
//   converting CR at end of command line to ASCII 0.
// We do this rather than letting the C compiler parse it for us into
//   separate "parameters", since we want to allow maximum flexibility
//   for the user in how they enter command-line Options (including
//   the possibility of no spaces and no switch characters between the
//   different Options).
// In addition, we want to use the same parsing routines to work
//   with both the Command-line and the Environment variable.  The C
//   compiler does not separate out the Environment variable into distinct
//   "parameters" the way it does the command-line,
//--------------------------------------------------------------------------
void GetCmdLine(void)

 {
  char *Pointer; //Temporary Pointer

  movedata(_psp,0x81,_DS,(unsigned)CommandLine,127); //Copy PSP cmd line
  Pointer = CommandLine;                             //Point at Command Line
  while (*Pointer++ != 13);                          //Convert to
  *(Pointer - 1) = 0;                                //  ASCIIZ
 }


//--------------------------------------------------------------------------
// PARSE THE COMMAND-LINE FOR OPTIONS.
// If Error, returns -1.
// Returns number of valid command-line Arguments (LPT's) processed.
//   If Return >= 1, and Testing = 0, this routine wrote the Output.
//   If Return = 0, calling routine is responsible for writing the Output.
//--------------------------------------------------------------------------
int ParseCmdLine
     (
      char *Pointer,
      int  Testing
     )

 {
  int   LPTIndex;  //LPT Index
  int   Counter;   //Switch Counter
  int   SawLPTMsg; //Port Number started with LPT
  int   HaveError; //Error on Command Line

  Counter = 0;                                    //Initialize Counter
  HaveError = 0;                                  //No Error

  while (SkipSpaces(&Pointer) != 0)               //Until end of Cmd Line
   {
    if (HaveError != 0)                           //If Error,
      return(-1);                                 //  return -1
    SawLPTMsg = 0;                                //Default = LPT message
    if (SkipDashSlash(&Pointer) == 0)             //Skip over Dash/Slash
      HaveError++;                                //If end-of-line, error
     else                                         //Not end-of-line
     {
      if (strnicmp(Pointer,"LPT",3) == 0)         //If "LPT",
       {
        SawLPTMsg++;                              //  mark as seeing the LPT
        Pointer += 3;                             //  and skip over the LPT
       }                                          //
      if ((*Pointer < '1') || (*Pointer > '9'))   //If not a Number,
        HaveError++;                              //  Error
       else                                       //Is a Number
       {
        LPTIndex = atoi(Pointer);                 //Get the number
        if ((LPTIndex < 1) || (LPTIndex > 9))     //If invalid LPT Index,
          HaveError++;                            //  Error
         else                                     //Valid LPT Index
         {
          while ((*Pointer >= '0') && (*Pointer <= '9'))
            Pointer++;                            //Skip over the Number
          if (SawLPTMsg != 0)                     //If we saw LPT,
            if (*Pointer == ':')                  //  skip over the Colon
              Pointer++;                          //  if it's there
          if (Testing == 0)                       //If for real,
            WriteInkLevel((LPTIndex-1),1);        //  Write the Percentages
          Counter++;                              //Increment Counter
         }
       }
     }
   }
  return(Counter);
 }


//--------------------------------------------------------------------------
// SKIP OVER THE SPACES IN A STRING.
// Return = 0 if end-of-line was found.
//        = 1 if not at end-of-line.
//--------------------------------------------------------------------------
int SkipSpaces
     (
      char **Pointer
     )

 {
  while (**Pointer == ' ') //Skip over
    (*Pointer)++;          //  Spaces
  if (**Pointer == 0)      //If end-of-string,
    return(0);             //  return error
  return(1);               //Return OK
 }


//--------------------------------------------------------------------------
// Skip over the Dash or Slash in a String (if there), and point at the
//   next character after that.
// Return = 0 if end-of-line was found.
//        = 1 if not at end-of-line.
//--------------------------------------------------------------------------
int SkipDashSlash
     (
      char **Pointer
     )

 {
  if ((**Pointer == '-') ||           //If Dash
      (**Pointer == '/'))             //  or slash
   {
    (*Pointer)++;                     //Skip over it
    return (SkipSpaces(&(*Pointer))); //Look for next character
   }
  return(1);                          //Return OK
 }


//±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±
// DOWNLOAD AND WRITE THE INK QUANTITY DETAILS FOR A PRINTER.
//±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±

//--------------------------------------------------------------------------
// WRITE THE INK LEVEL PERCENTAGES FOR A PRINTER.
// Return = 1 if SpecificRequest = 0 and Printer was valid
//            (we wrote something)
//        = 0 if invalid printer
//--------------------------------------------------------------------------
int WriteInkLevel
     (
      int LPTIndex,
      int SpecificRequest
     )

 {
  ResetString(DeviceIDRaw,DeviceIDSize);      //Reset Device ID Buffer

  if (I17XTestPort(LPTIndex) == 0)            //If no Ext Int 17 on Port
   {
    if (SpecificRequest != 0)                 //If Specific Request
     WriteHeaderErrNoDvcID(LPTIndex,          //Write Error Message
       "Extended Int 17 Functions are not installed on this Port.");
    return(0);                                //Error
   }

//Just do this if Index <=2 (to avoid NT problems??)

  if (TestPrinter(LPTIndex) == 0)             //If no Printer Attached
   {
    if (SpecificRequest != 0)                 //If Specific Request
     WriteHeaderErrNoDvcID(LPTIndex,          //Write Error Message
       "No Printer is currently attached to this Port.");
    return(0);                                //Error
   }

  if (I17XGetDeviceID(LPTIndex,               //If Device ID Request
                      FP_SEG(DeviceIDRaw),    //  doesn't work,
                      FP_OFF(DeviceIDRaw),
                      (DeviceIDSize-2)) == 0)
   {
    if (SpecificRequest != 0)                 //If Specific Request
     WriteHeaderErrNoDvcID(LPTIndex,          //Write Error Message
       "This Device did not respond to the Device ID Request.");
    return(0);                                //Error
   }

  FixDeviceIDString();                        //Verify validity of Device ID

  if (TestPrinterClass() == 0)                //If it's not a Printer
   {
    if (SpecificRequest != 0)                 //If Specific Request
     WriteHeaderErrWithDvcID(LPTIndex,        //Write Error Message
       "This Device does not appear to be a compatible Printer.");
    return(0);                                //Error
   }

  PrinterType = TestCompatibility();          //Get the Printer Type
  if (PrinterType == 0)                       //If not compatible,
   {
    if (SpecificRequest != 0)                 //If Specific Request
     WriteHeaderErrWithDvcID(LPTIndex,        //Write Error Message
       "This Printer is not compatible with INKLEVEL.");
    return(0);                                //Error
   }

  switch (PrinterType)                      //Handle appropriate PrinterType
   {
    case PrinterTypeEpson:
      if (GetEpsonInk(LPTIndex) == 0)
       return(0);
     break;
    case PrinterTypeOldHP:
     if (GetOldHPInk(LPTIndex) == 0)
       return(0);
     break;
    case PrinterTypeNewHP:
     if (GetNewHPInk(LPTIndex) == 0)
       return(0);
     break;
   }
  return(1);
 }


//--------------------------------------------------------------------------
// PARSE THE DEVICE ID STRING AND GET THE INK PERCENTAGES FOR AN OLD HP
// Return = 0 if Error (Invalid Percentage(s))
//        = 1 if OK (Percentages were written)
// NOTES: If we got to this point, we have already verified that there
//          is a VSTATUS: Tag, so we don't need to test for that errors.
//          However, there is no guarantee that the KP and CP will be there!
//        In here, we test for a KP Status, and if it's there, simply assume
//          that there is a CP status as well.
//--------------------------------------------------------------------------
int GetOldHPInk
     (
      int LPTIndex
     )

 {
  char *Pointer;
  char *Pointer2;
  char *StatusMsg;
  int   Percent;

  Pointer  = (DeviceIDRaw + 1);                   //Point at String
  StatusMsg = strstr(Pointer,";VSTATUS:");        //Locate VSTATUS: Tag
  StatusMsg++;
  Pointer2 = strstr(StatusMsg,";");               //Locate end of Tag
  Pointer = strstr(StatusMsg,",KP");              //Locate ",KP" Status
  if ((Pointer == NULL) ||                        //If not there
      (Pointer >= Pointer2))
   {
    WriteHeaderErrWithTag(LPTIndex,StatusMsg);    //Write Error Message
    return(0);                                    //Return error
   }

  WriteGraphHeader(LPTIndex,StatusMsg,10);        //Write the Grah Header
  Percent = GetDecPercent(Pointer + 3);           //Get the Black Percent
  WriteColorLine(LIGHTGRAY,"Black:    ",Percent); //Write it
  Pointer = strstr(StatusMsg,",CP");              //Locate ",CP" Status
  if ((Pointer == NULL) ||                        //If not there
      (Pointer >= Pointer2))
    Percent = -1;                                 //Make Percent Illegal
   else
    Percent = GetDecPercent(Pointer + 3);         //If OK, get Color Percent
  WriteColorLine(RED,      "Tri-Color:",Percent); //Write it
  return(1);                                      //OK
 }


//--------------------------------------------------------------------------
// PARSE THE DEVICE ID STRING AND GET THE INK PERCENTAGES FOR A NEW HP
// Return = 0 if Error (Invalid Percentage(s))
//        = 1 if OK (Percentages were written)
// NOTES: If we got to this point, we have already verified that there
//          is an S: Tag, so we don't need to test for that again
//        There are several dilemmas we have with regard to "decoding"
//          and HP S: Tag to determine the Ink Percentages.  I've sent
//          an e-mail asking HP the "proper" way to decode things, but
//          they have never responded.  So, we are left to our own devices
//          to try and discern the patterns and try to figure out what
//          HP is up to.
//        I have seen example Device ID strings for several different
//          kinds of HP printers, though I definitely have not seen all
//          of them.  In addition, HP keeps coming out with new models
//          on a fairly regular basis, and it's hard to say how they
//          might change things in the future.
//        For all of the example strings I've seen, if the first two
//          characters are "00" or "01", the number of ink cartridges
//          is in character 16.  If the first two characters are "03",
//          the number of ink cartridges is in character 18.  If the
//          first two characters are "04", the number of ink cartridges
//          is in character 22.  Conspicuously absent from my examples
//          are the first two characters being "02" (as well as, probably,
//          some others).  I don't know if HP never made any 02's,
//          or if I've just never seen an example that does, or even if the
//          pattern I've seen is truly universal and will work on all
//          printers.  So, I don't rely on that pattern.
//        Instead, I use an algorithm to find the number of ink
//          cartridges depends on another pattern.  It also assumes
//          a few things, which may or may not be true for all HP
//          printers.  A few things it assumes are that there are
//          at least 16 characters at the beginning of the S: tag
//          before the number of ink cartridges, that the ink cartridge
//          information is the last thing in the S: tag, that the
//          character at the beginning of each individual ink cartridge
//          status is either a 4, 8 or C, and that the ink cartridge type
//          only needs two characters to be identified.
//          If these assumptions are correct, the algorithm should be able
//          to handle even "unusal" printers that I haven't seen before.
//        One potentially major "problem" with the algorithm is that
//          we need to maintain a comprehensive list of all codes
//          that HP uses to identify the different kinds of Ink
//          Cartridges.
//--------------------------------------------------------------------------
int GetNewHPInk
     (
      int LPTIndex
     )

 {
  char *Pointer;
  char *Pointer2;
  char *StatusMsg;


  char *InkTypeString = "0x0";   //Change this to two characters!!


  int   Test;
  int   Test2;
  int   NumInks;
  int   Counter;
  int   InkType;
  int   Percent;

  Pointer2 = (DeviceIDRaw + 1);                  //Point at String
  StatusMsg = strstr(Pointer2,";S:");            //Locate S: Tag
  StatusMsg++;                                   // and save it
  Pointer = (StatusMsg + 12);                    //Point at possible Ink Percent
  Test = 0;                                      //Initialize Test to False
  while ((Test == 0) && (Pointer[0] != ';'))     //Keep going until we find it
   {
    if ((*Pointer >= '2') && (*Pointer <= '6'))
     {
      NumInks = int(*Pointer - '0');             //If valid, get the # of Inks
      Pointer++;                                 //Point at First Cartridge
      Test2 = 0;                                 //Initialize Loop Tester
      for (Counter = 0; Counter < NumInks; Counter++)
        if (*(Pointer + Counter*8) == '4' ||     //Test for valid Ink Type
            *(Pointer + Counter*8) == '8' ||     //  Numbers in all
            *(Pointer + Counter*8) == 'c' ||     //  Positions
            *(Pointer + Counter*8) == 'C')
          Test2++;                               //If valid Ink Type, mark it
      if (*(Pointer + Counter*8) == ';' &&       //If at end of Tag, and
            Test2 == NumInks)                    //  Valid # of Ink Types
        Test++;                                  //Mark as OK
     }
     else
      Pointer++;                                 //If invalid, go to next char
   }
  if (Test == 0)                                 //If failure,
   {
    WriteHeaderErrWithTag(LPTIndex,StatusMsg);   //Write Error Message
    return(0);                                   //Return error
   }

  Pointer++;                                      //Point at the Ink Cart Type
  WriteGraphHeader(LPTIndex,StatusMsg,14);        //Write the Graph Header
  for (Counter = 1;Counter <= NumInks; Counter++) //For each Cartridge
   {
    *(InkTypeString + 2) = *Pointer;              //Copy the Cartridge Type
    InkType = int(strtol(InkTypeString,0,16));    //Get the Cartridge Type
    Percent = GetHexPercent(Pointer + 5);         //Get the Percent
    Pointer += 8;                                 //Point at next Cartridge
    switch (InkType)
     {
      case 0:                                     //No Cartridge Installed
        WriteColorLine(LIGHTGRAY,"Empty:        ",Percent);
        break;
      case 1:                                     //Black
        WriteColorLine(LIGHTGRAY,"Black:        ",Percent);
        break;
      case 2:                                     //Tri-Color (CMY)
        WriteColorLine(RED,      "Tri-Color:    ",Percent);
        break;
      case 3:                                     //Photo (KCM)
        WriteColorLine(GREEN,    "Photo:        ",Percent);
        break;
      case 4:                                     //Cyan
        WriteColorLine(CYAN,     "Cyan:         ",Percent);
        break;
      case 5:                                     //Magenta
        WriteColorLine(MAGENTA,  "Magenta:      ",Percent);
        break;
      case 6:                                     //Yellow
        WriteColorLine(YELLOW,   "Yellow:       ",Percent);
        break;
      case 7:                                     //Photo Cyan
        WriteColorLine(CYAN,     "Cyan Photo:   ",Percent);
        break;
      case 8:                                     //Photo Magenta
        WriteColorLine(MAGENTA,  "Magenta Photo:",Percent);
        break;
      case 9:                                     //Photo Yellow
        WriteColorLine(YELLOW,   "Yellow Photo: ",Percent);
        break;
      case 10:                                    //Grey Photo (GGK)
        WriteColorLine(LIGHTGRAY,"Grey Photo:   ",Percent);
        break;
      case 11:                                    //Blue Photo
        WriteColorLine(LIGHTBLUE,"Blue Photo:   ",Percent);
        break;
      case 12:                                    //Quad-Color (KCMY)
        WriteColorLine(RED,      "Quad-Color:   ",Percent);
        break;
//      case 13:                                    //LCLM (Light Cyan/Magenta?)
//        WriteColorLine(LIGHTGRAY,"LCLM:          ",Percent);
//        break;
//      case 14:                                    //Yellow/Magenta (YM)
//        WriteColorLine(YELLOW,   "Yellow/Magenta:",Percent);
//        break;
//      case 15:                                    //Cyan/Black (CK)
//        WriteColorLine(LIGHTGRAY,"Cyan/Black:    ",Percent);
//        break;
//      case 16:                                    //LGPK (Lt Gry, Photo Blk?)
//        WriteColorLine(LIGHTGRAY,"Gray/Black:    ",Percent);
//        break;
//      case 17:                                    //LG (Light Gray?)
//        WriteColorLine(LIGHTGRAY,"Light Gray:    ",Percent);
//        break;
//      case 18:                                    //G (Gray?)
//        WriteColorLine(LIGHTGRAY,"Gray?:         ",Percent);
//        break;
//      case 19:                                    //PG (Photo Gray?)
//        WriteColorLine(LIGHTGRAY,"Photo Gray?:   ",Percent);
//        break;
//      case 32:                                    //White Cartridge
//        WriteColorLine(LIGHTGRAY,"White:         ",Percent);
//        break;
//      case 33:                                    //Red Cartridge
//        WriteColorLine(RED,      "Red:           ",Percent);
//        break;
      default:                                    //Unrecognized Cartridge
        WriteColorLine(LIGHTGRAY,"Unknown:       ",Percent);
     }
   }
  return(1);
 }


//--------------------------------------------------------------------------
// GET THE INK PERCENTAGES FOR AN EPSON PRINTER
// Return = 0 if Error
//        = 1 if OK (Percentages were written)
// NOTE: Unlike the HP Printers where the Ink Levels are embedded in the
//         Device ID string, we actually have to "talk" to an Epson Printer
//         to find out the Ink Quantities.  This is far more involved
//         and complicated than it is on an HP Printer.
//       In addition, some Epson printers will respond to an "ST" (Status)
//         Request, some will respond to an "IQ" (Ink Quantity) Request,
//         some will respond to both, and some will respond to neither.
//       Also, the responses are unreliable (at best), so we need to try
//         several times before totally giving up.
//       For example, the only Epson printer I have ever actually tried
//         this program on (an Epson Stylus C84) has both a parallel and a USB
//         interface.  It responds to the "IQ" Request when it is hooked
//         up to the parallel interface, but doesn't when it is hooked up to
//         the USB interface.  Go figure.
//       We send the "Exit Packet Mode" request to all Printers, no matter
//         what kind of port they're hooked up to.  In theory, we only
//         need to send it to USB printers, but don't need to send it
//         to parallel printers.  As things stand today, we can't tell
//         whether a printer is USB or parallel (and, IMHO, it shouldn't
//         matter), so we send it to all printers.  It doesn't seem to do
//         anything to adversely affect a non-USB printer, so we're
//         probably OK, at least for now.  If this later turns out to
//         be a problem, we will need to update the Extended Int 17h
//         functions to include returning the type of printer we have
//         (USB, parallel, ???).  If this happens, we will need to keep
//         in mind the possibility of a parallel printer attached to a
//         computer's USB port through one of those "USB-to-Parallel"
//         adapter cables.  In such a case, the printer itself is actually
//         a parallel printer, and will respond as such (not needing
//         the "Exit Packet Mode" request), even though the computer
//         may think it's USB.
//       I have absolutely no idea what "Packet Mode" is, why a
//         USB printer comes up in "Packet Mode" by default, why
//         it needs to be "exited" before the printer will do anything
//         useful, or why a parallel printer doesn't have it, or whether
//         an Epson-compatible printer (not actually made by Epson) might
//         have it.
//--------------------------------------------------------------------------
int GetEpsonInk
     (
      int LPTIndex
     )

 {
  int   Retry;
  int   Test = 0;
  int   Percent;
  int   ColorLine = 0;
  int   Counter;
  char *IQMessage;
  char  Keystroke;
  char  ResetMsg[] = "\x1b@"                     //Reset Printer
                     "\x1b(R\x08\x00\x00REMOTE1" //Enter Remote Mode
                     "RS\x01\x00\x01"            //Hard Reset
                     "\x1b\x00\x00\x00"          //Exit Remote Mode
                     "\x1b\x00\x1b\x00\x1b\x00"  //Reinitialize?
                     "\x1b@";                    //Reset Printer

  Retry = 0;
  do                                          //Try to get IQ Message
   {
    if (GetEpsonStatus(LPTIndex, Retry) != 0)
      Test++;
   } while ((Test == 0) && (++Retry < 4));    //Try it 4 times (2 IQ, 2 ST)
  if (Test == 0)
   {
    WriteQuery("The Epson Printer on LPT");
    cerr << setw(1) << (LPTIndex + 1);
    WriteQuery(" did not respond to the Ink Quantity Request.");
    WriteQuery("\n");
    WriteQuery("Would you like this program to Reset the Printer");
    WriteQuery(" and try again? ");
    while (kbhit() == 0);                       //Wait for a keystroke
    Keystroke = toupper(getch());               //Get the keystroke
    if (Keystroke == 'Y')                       //If Yes
     {
      WriteQuery("Y\n\n");                      //Write a Y
      I17XSendData(LPTIndex,                    //Hard Reset
        FP_SEG(ResetMsg),
        FP_OFF(ResetMsg),
        sizeof(ResetMsg) - 1);
      WriteQuery("Waiting 60 seconds for Epson Printer to Reset.");
      WriteQuery("  Press a key to stop waiting.");
      WriteQuery("\n");
      Counter = 60;
      while ((kbhit() == 0) && (Counter-- > 0)) //Wait 60 seconds or keypress
       {
        delay(1000);                            //Wait 1 second
        cerr << "\r" << setw(2) << Counter;     //Write the Counter
       }
      WriteQuery("\n");                         //Move down
      if (kbhit() != 0)                         //If user pressed a key,
        getch();                                //  absorb it
      Retry = 0;
      do                                        //Try to get IQ Message again
       {
        if (GetEpsonStatus(LPTIndex, Retry) != 0)
          Test++;
       } while ((Test == 0) && (++Retry < 4));  //Try it 4 times (2 IQ, 2 ST)
      if (Test == 0)
       {
        WriteHeaderErrWithDvcID(LPTIndex,       //  write Error Message
          "This printer did not respond to an Epson Ink Quantity Request.");
        return(0);                              //Return Error
       }
     }
     else                                       //If N (anything except Y),
      {
       WriteQuery("N\n");                       //  write an N
       WriteHeaderErrWithDvcID(LPTIndex,        //  write Error Message
         "This printer did not respond to an Epson Ink Quantity Request.");
       return(0);                               //Return Error
      }
   }

//SUCCESS!!

  IQMessage = strstr(InkQuantity,"IQ:");        //Point at Ink Quantity Tag
  WriteGraphHeader(LPTIndex,IQMessage,14);      //Write the Graph Header
  IQMessage += 3;                               //Point at the Black Percent
  while (*IQMessage != ';')                     //Go until semicolon
   {
    Percent = GetHexPercent(IQMessage);         //Get the Percent
    switch (ColorLine)
     {
      case 0:
        WriteColorLine(LIGHTGRAY,   "Black:        ",Percent);
        break;
      case 1:
        WriteColorLine(CYAN,        "Cyan:         ",Percent);
        break;
      case 2:
        WriteColorLine(MAGENTA,     "Magenta:      ",Percent);
        break;
      case 3:
        WriteColorLine(YELLOW,      "Yellow:       ",Percent);
        break;
      case 4:
        WriteColorLine(LIGHTCYAN,   "Photo Cyan:   ",Percent);
        break;
      case 5:
        WriteColorLine(LIGHTMAGENTA,"Photo Magenta:",Percent);
        break;
      case 6:
        WriteColorLine(YELLOW,      "Photo Yellow: ",Percent);
        break;
      default:
        WriteColorLine(LIGHTGRAY,   "Unknown:      ",Percent);
     }
    IQMessage += 2;                        //Point at next Color Percent
    ColorLine++;                           //Increment Color Value
   }
  return(1);
 }


//--------------------------------------------------------------------------
// DOWNLOAD THE EPSON INK QUANTITY STRING FROM THE PRINTER
// Return = 0 if error (Printer did not respond,
//            or STatus Message does not contain IQ: Tag)
//        = 1 if OK (String was downloaded and contains IQ: Tag)
//--------------------------------------------------------------------------
int GetEpsonStatus
     (
      int LPTIndex, //Printer Index for INT 17h Extended Functions
      int Retry     //Retry Counter (do different things for 0, even, & odd)
     )

 {
  int  ReturnValue;

  char ExitPktMsg[] = ""
                      "\x1@"                      //Reset Printer
                      "\x00\x00\x00"              //Always start with 0-0-0
                      "\x1b\x01@EJL 1284.4\n"     //Exit Packet Mode
                      "@EJL     \n"               //  (whatever that is)
                      "\x1b@";                    //Reset Printer
  char ReqIQMsg[]   = ""
                      "\x1b@"                     //Reset Printer
                      "\x1b(R\x08\x00\x00REMOTE1" //Enter Remote Mode
                      "IQ\x01\x00\x01"            //Request Ink Quantity
                      "\x1b\x00\x00\x00"          //Exit Remote Mode
                      "\x1b\x00\x1b\x00\x1b\x00"  //Reinitialize?
                      "\x1b@";                    //Reset Printer
  char ReqSTMsg[]   = ""
                      "\x1b@"                     //Reset Printer
                      "\x1b(R\x08\x00\x00REMOTE1" //Enter Remote Mode
                      "ST\x02\x00\x00\x01"        //Enable Status Requests
                      "\x1b\x00\x00\x00"          //Exit Remote Mode
                      "\x1b\x00\x1b\x00\x1b\x00"  //Reinitialize?
                      "\x1b@";                    //Reset Printer

  ResetString(InkQuantity,InkQuantitySize);       //Reset Epson IQ Buffer
  if (Retry == 0)                                 //If first Retry
   {
    if (I17XSendData(LPTIndex,                    //  Exit Packet Mode
          FP_SEG(ExitPktMsg),
          FP_OFF(ExitPktMsg),
         (sizeof(ExitPktMsg) - 1)) == 0)          //If we couldn't send it
     return(0);                                   //  Error
    }
  if ((Retry & 1) == 0)                           //If Even Retry
    ReturnValue = I17XSendData(LPTIndex,          //  Request Ink Quantity
                        FP_SEG(ReqIQMsg),
                        FP_OFF(ReqIQMsg),
                       (sizeof(ReqIQMsg) - 1));
   else                                           //If Odd Retry
    ReturnValue = I17XSendData(LPTIndex,          //  Request STatus
                        FP_SEG(ReqSTMsg),
                        FP_OFF(ReqSTMsg),
                       (sizeof(ReqSTMsg) - 1));
  if (ReturnValue != 0)                           //If Data was sent OK
   {
    delay(20);                                    //Wait 20 ms
    if (I17XGetData(LPTIndex,                     //Get Printer Response
          FP_SEG(InkQuantity),
          FP_OFF(InkQuantity),
          (InkQuantitySize - 1)) != 0)            //If we got a Response
     {
      if (strstr(InkQuantity,"IQ:") != NULL)      //Look for an IQ: Tag
        return(1);                                //If there, we're OK
     }
   }
  return(0);                                      //If no IQ: Tag, Error
 }


//±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±
// SUPPORT FUNCTIONS TO DECODE AND WORK WITH DEVICE ID & STATUS STRINGS
//±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±

//--------------------------------------------------------------------------
// Verify that there is a semicolon at the end of the Device ID String
//   (Some Devices do not put the semicolon at the end like they should).
// In addition, we replace the second byte of the Device ID string (normally
//   the LSB of the String Size) with a semicolon.  This aids in our
//   Tag (string) searches later on in some of the routines.  We never
//   use the string size for anything anyway, so we should be OK.
// If we did end up needing to use the string size (the first two bytes
//   of the string), we would need to verify it here (some
//   Devices return an incorrect size in the string).  If we end up needing
//   the String Size for something, keep in mind that it is in
//   Big-Endian format, not Little-Endian as is normal for Intel-based
//   computers.
//--------------------------------------------------------------------------
void FixDeviceIDString(void)

 {
  char *Pointer;

  Pointer = (DeviceIDRaw + 1);                 //Skip over the first size byte
  *Pointer = ';';                              //Put semicolon at the beginning
  if (*(Pointer + strlen(Pointer) - 1) != ';') //If no semicolon at the end,
      *(Pointer + strlen(Pointer))      = ';'; //  put one there as well
 }


//--------------------------------------------------------------------------
// PARSE DEVICE ID STRING TO SEE IF THE DEVICE IS EVEN A PRINTER
// Return = 1 if Device Class = PRINTER.
//        = 0 if not.
//--------------------------------------------------------------------------
int TestPrinterClass(void)

 {
  char *Pointer;
  char *Pointer2;
  char *Pointer3;

  Pointer2 = (DeviceIDRaw+1);                      //Point at the String
  Pointer = strstr(Pointer2,";CLASS:");            //Look for CLASS:
  if (Pointer != NULL)                             //If Found,
    Pointer += 7;                                  //  point at Class
   else                                            //If not Found,
   {
    Pointer = strstr(Pointer2,";CLS:");            //  Look for CLS:
    if (Pointer != NULL)                           //If Found,
      Pointer +=5;                                 //  point at Class
     else                                          //If not Found,
      return(0);                                   // Error
   }
  Pointer3 = (strstr(Pointer,";"));                //Find end of Class
  Pointer2 = (strstr(Pointer,"PRINTER"));          //Look for "PRINTER"
  if ((Pointer2 != NULL) && (Pointer2 < Pointer3)) //If Found,
    return(1);                                     //  we're OK
  Pointer2 = (strstr(Pointer,"Printer"));          //Look for "Printer"
  if ((Pointer2 != NULL) && (Pointer2 < Pointer3)) //If Found,
    return(1);                                     //  we're OK
  Pointer2 = (strstr(Pointer,"printer"));          //Look for "printer"
  if ((Pointer2 != NULL) && (Pointer2 < Pointer3)) //If Found,
    return(1);                                     //  we're OK
  return(0);                                       //If none of these, Error
 }


//--------------------------------------------------------------------------
// PARSE DEVICE ID STRING TO SEE IF THE PRINTER IS HP- OR EPSON- COMPATIBLE
// Return = PrinterTypeOldHP if COMMAND SET contains "PCL", "PML", or "LDL"
//             and also has a "VSTATUS:" Tag (Old HP)
//        = PrinterTypeNewHP if COMMAND SET contains "PCL", "PML", or "LDL"
//             and also has an "S:" Tag (New HP)
//        = PrinterTypeEpson if COMMAND SET contains "ESCP" (Epson)
//        = 0 if none of these.
//--------------------------------------------------------------------------
int TestCompatibility(void)

 {
  char *Pointer;
  char *Pointer2;
  char *Pointer3;
  int   HP;

  Pointer2 = (DeviceIDRaw+1);                      //Point at the String
  Pointer = strstr(Pointer2,";COMMAND SET:");      //Look for COMMAND SET:
  if (Pointer != NULL)                             //If Found,
    Pointer += 13;                                 //  point at Protocols
   else                                            //If not Found,
   {
    Pointer = strstr(Pointer2,";CMD:");            //  Look for CMD:
    if (Pointer != NULL)                           //If Found,
      Pointer +=5;                                 //  point at Protocols
     else                                          //If not Found,
      return(0);                                   // Error
   }

  Pointer3 = (strstr(Pointer,";"));                //Find end of Protocols
  Pointer2 = (strstr(Pointer,"ESCP"));             //Look for ESCPx Protocol
  if ((Pointer2 != NULL) && (Pointer2 < Pointer3)) //If Found,
    return(PrinterTypeEpson);                      //  it's Epson-compatible

  HP = 0;                                          //Assume no HP
  Pointer2 = (strstr(Pointer,"PCL"));              //Look for PCL Protocol
  if ((Pointer2 != NULL) && (Pointer2 < Pointer3)) //If Found,
    HP++;                                          //  it's HP-compatible
  Pointer2 = (strstr(Pointer,"PML"));              //Look for PML Protocol
  if ((Pointer2 != NULL) && (Pointer2 < Pointer3)) //If Found,
    HP++;                                          //  it's HP-compatible
  Pointer2 = (strstr(Pointer,"LDL"));              //Look for LDL Protocol
  if ((Pointer2 != NULL) && (Pointer2 < Pointer3)) //If Found,
    HP++;                                          //  it's HP-compatible
  if (HP == 0)                                     //If it's not HP-compatible,
    return(0);                                     //  Error

  Pointer2 = (DeviceIDRaw + 1);                    //Point at the String
  if (strstr(Pointer2,";S:") != NULL)              //If we have an S: Tag,
    return(PrinterTypeNewHP);                      //  it's a New HP
  if (strstr(Pointer2,";VSTATUS:") != NULL)        //If we have a VSTATUS: Tag,
    return(PrinterTypeOldHP);                      //  it's an Old HP

  return(0);                                       //If none of these, error
 }


//--------------------------------------------------------------------------
// CONVERT THE NEXT TWO HEX CHARACTERS OF A STRING TO A NUMBER (PERCENT)
// If the two characters are invalid for some reason, we return a -1
//--------------------------------------------------------------------------
int GetHexPercent
     (char *HexCharsIn //2 Hex characters
     )

 {
  char *HexMsg = "0x00";
  int   Percent;

  if ((*(HexCharsIn + 0) >= '0' && *(HexCharsIn + 0) <= '9') ||
      (*(HexCharsIn + 0) >= 'a' && *(HexCharsIn + 0) <= 'f') ||
      (*(HexCharsIn + 0) >= 'A' && *(HexCharsIn + 0) <= 'F'))
    *(HexMsg + 2) = *(HexCharsIn + 0); //If first char is valid, store it
   else                                //If not,
    return(-1);                        //  return an error value
  if ((*(HexCharsIn + 1) >= '0' && *(HexCharsIn + 1) <= '9') ||
      (*(HexCharsIn + 1) >= 'a' && *(HexCharsIn + 1) <= 'f') ||
      (*(HexCharsIn + 1) >= 'A' && *(HexCharsIn + 1) <= 'F'))
    *(HexMsg + 3) = *(HexCharsIn + 1); //If second char is valid, store it
   else                                //If not,
    return(-1);                        //  return an error value

  Percent = int(strtol(HexMsg,0,16));  //Convert the String to a Number
  if (Percent > 100)                   //If an invalid Percent,
    Percent = -1;                      //  make it the error value
    return(Percent);                   //Return our Percentage value
 }


//--------------------------------------------------------------------------
// CONVERT THE NEXT THREE DECIMAL CHARACTERS OF A STRING TO A NUMBER (PERCENT)
// If the three characters are invalid for some reason, we return a -1
//--------------------------------------------------------------------------
int GetDecPercent
     (char *DecCharsIn //3 Decimal characters
     )

 {
  char *DecMsg = "000";
  int   Percent;

  if (*(DecCharsIn + 0) >= '0' && *(DecCharsIn + 0) <= '9')
    *(DecMsg + 0) = *(DecCharsIn + 0); //If first char is valid, store it
   else                                //If not,
    return(-1);                        //  return an error value
  if (*(DecCharsIn + 1) >= '0' && *(DecCharsIn + 1) <= '9')
    *(DecMsg + 1) = *(DecCharsIn + 1); //If second char is valid, store it
   else                                //If not,
    return(-1);                        //  return an error value
  if (*(DecCharsIn + 2) >= '0' && *(DecCharsIn + 2) <= '9')
    *(DecMsg + 2) = *(DecCharsIn + 2); //If third char is valid, store it
   else                                //If not,
    return(-1);                        //  return an error value

 Percent = int(strtol(DecMsg,0,10));   //Convert the String to a Number
 if (Percent > 100)                    //If an invalid Percent,
   Percent = -1;                       //  make it the error value
   return(Percent);                    //Return our Percentage value
 }


//±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±
// Support functions to write Program, Device ID, & Status Strings
//±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±

//--------------------------------------------------------------------------
// WRITE THE FIRST PART OF THE HEADER (THE LPT NUMBER) FOR A PRINTER
//--------------------------------------------------------------------------
void WriteHeaderStart
      (
       int LPTIndex
      )

 {
  WriteString("\n\n");             //Move down
  WriteString("LPT");              //Write LPT
  cout << setw(1) << (LPTIndex+1); //Write Number
  WriteString(":  ");              //Write Colon, move over
 }


//--------------------------------------------------------------------------
// WRITE THE GRAPH HEADER LINE
//--------------------------------------------------------------------------
void WriteGraphHeader
      (
       int   LPTIndex,
       char *InkStatusMsg, //Message containing the Ink Quantity
                           //  Message must terminate with a semicolon
                           //  We write the entire message for troubleshooting
       int   SpacesInFront //Number of Spaces to put in front of Graph Line
                           //  Does NOT account for our Text Percents
      )

 {
  char *Pointer;
  char  TempChar;

  WriteHeaderStart(LPTIndex);             //Write first part of Header
  WriteDvcDescr();                        //Write Printer Descr
  WriteString("\n");                      //Go to next line
  RepeatChar("Í",75);                     //Write a
  WriteString("\n");                      //  Separator Line

  textcolor(LIGHTGRAY);                   //Write as Light Grey
  textbackground(BLACK);                  //  on Black
  Pointer = InkStatusMsg;                 //Find the end of the
  while (*Pointer++ != ';');              //  Ink Status Msg
  TempChar = *Pointer;                    //Save the character after the ;
  *Pointer = 0;                           //Convert it to a String
  WriteString(InkStatusMsg);              //Write it
  WriteString("\n");                      //Move down
  RepeatChar("Ä",75);                     //Write a
  WriteString("\n");                      //  Separator Line
  WriteString("\n");                      //Move down one more line
  *Pointer = TempChar;                    //Restore original Ink Status Msg
  RepeatCharColor(" ",SpacesInFront + 6); //Write Graph Header
  WriteStringColor("0   10   20   30   40   50   60   70   80   90  100");
  WriteString("\n");
  RepeatCharColor(" ",SpacesInFront + 6);
  WriteStringColor("ÚÄÄÄÄÂÄÄÄÄÂÄÄÄÄÂÄÄÄÄÂÄÄÄÄÂÄÄÄÄÂÄÄÄÄÂÄÄÄÄÂÄÄÄÄÂÄÄÄÄ¿");
  WriteString("\n");
 }


//--------------------------------------------------------------------------
// WRITE A LINE OF THE INK DISPLAY GRAPH
//--------------------------------------------------------------------------
void WriteColorLine
      (
       char  ForeColor,   //Foreground Color to write graph with
       char *InkColorMsg, //Message to write at beginning of line (Ink Color)
       int   Percent      //Percent of Ink
      )

 {
  textcolor(ForeColor);                       //Set Foreground Color
  textbackground(BLACK);                      //Set Background Color
  WriteStringColor(InkColorMsg);              //Write Ink Color Message
  WriteStringColor(" ");                      //Write a Space
  if ((Percent < 0) || (Percent > 100))       //If Illegal Percent,
    WriteStringColor("      Illegal %");      //  write Illegal
   else
   {
    WritePercentColor(Percent);               //Write the Percent
    WriteStringColor("% ");                   //Write a Percent sign
    if (Percent >= 1)                         //If Percent is >= 1,
     {
      WriteStringColor("Þ");                  //  Write the 1% character
      Percent--;                              //Modify Percent
     }
     else                                     //If Percent = 0,
      WriteStringColor(" ");                  //  Write a space
    RepeatCharColor("Û",(Percent/2));         //Write 2% characters
    if ((Percent-((Percent/2)*2)) == 1)       //If Percent is odd,
      WriteStringColor("Ý");                  //  write last character
    RepeatCharColor(" ",((100 - Percent)/2)); //Finish graph line
   }
  WriteString("\n");                          //Go to the next line
  textattr(TextAttrib);                       //Reset Color
  WriteStringColor("\ra\r \r");               //Write to screen so Color
                                              //  Reset takes effect
 }


//--------------------------------------------------------------------------
// WRITE THE HEADER FOR A "BAD" PRINTER
//  (One where we could never download the Device ID, for whatever reason)
//--------------------------------------------------------------------------
void WriteHeaderErrNoDvcID
      (
       int   LPTIndex,
       char *ErrorMsg
      )

 {
  WriteHeaderStart(LPTIndex); //Write the first part of the Header
  WriteString("\n");          //Write
  RepeatChar("Í",75);         //  Separator
  WriteString("\n");          //  Line
  WriteString(ErrorMsg);      //Write Error Message
  WriteString("\n");          //Move down
 }


//--------------------------------------------------------------------------
// WRITE THE HEADER FOR A "BAD" PRINTER
//  (One where we downloaded the Device ID,
//     but couldn't decode it for some reason)
//--------------------------------------------------------------------------
void WriteHeaderErrWithDvcID
      (
       int   LPTIndex,
       char *ErrorMsg
      )

 {
  WriteHeaderStart(LPTIndex); //Write first part of Header
  WriteDvcDescr();            //Write Printer Descr
  WriteString("\n");          //Go to next line
  RepeatChar("Ä",75);         //Write
  WriteString("\n");          //  Separator Line
  WriteDeviceID();            //Write the entire Device ID String
  RepeatChar("Ä",75);         //Write
  WriteString("\n");          //  Separator Line
  WriteString(ErrorMsg);      //Write Error Message
  WriteString("\n");          //Move down
 }


//--------------------------------------------------------------------------
// WRITE THE HEADER FOR A "BAD" EPSON PRINTER
//  (One where we downloaded the IQ String,
//     but couldn't decode it for some reason)
//--------------------------------------------------------------------------
void WriteHeaderErrWithIQ
      (
       int   LPTIndex,
       char *ErrorMsg
      )

 {
  WriteHeaderStart(LPTIndex); //Write first part of Header
  WriteDvcDescr();            //Write Printer Descr
  WriteString("\n");          //Go to next line
  RepeatChar("Ä",75);         //Write
  WriteString("\n");          //  Separator Line
  WriteEpsonInkQuantity();    //Write the entire Epson Status/IQ String
  RepeatChar("Ä",75);         //Write
  WriteString("\n");          //  Separator Line
  WriteString(ErrorMsg);      //Write Error Message
  WriteString("\n");          //Move down
 }


//--------------------------------------------------------------------------
// WRITE THE HEADER FOR AN "UNDECODABLE" PRINTER
//  (One where we downloaded the Device ID, found the appropriate Tag,
//     but couldn't decode it for some reason)
// This assumes that the Tag message ends in a semicolon!
//--------------------------------------------------------------------------
void WriteHeaderErrWithTag
      (
       int   LPTIndex,
       char *TagMsg
      )

 {
  char *Pointer;
  char TempChar;

  WriteHeaderStart(LPTIndex); //Write first part of Header
  WriteDvcDescr();            //Write Printer Descr
  WriteString("\n");          //Go to next line
  RepeatChar("Ä",75);         //Write
  WriteString("\n");          //  Separator Line
  Pointer = TagMsg;           //Convert
  while (*Pointer++ != ';');  //  Tag Message
  TempChar = *Pointer;        //  to a
  *Pointer = 0;               //  String
  WriteString(TagMsg);        //Write Tag Message
  *Pointer = TempChar;        //Restore original Tag Message
  WriteString("\n");          //Move down
  RepeatChar("Ä",75);         //Write a
  WriteString("\n");          //  separator line
  WriteString("\n");          //Move down
  WriteString("The Status String could not be decoded correctly.");
  WriteString("\n");
 }


//--------------------------------------------------------------------------
// Write the entire Device ID String, with Carriage returns after the
//   semicolons.
// NOTE: This assumes that the Device ID String has been fixed (so that
//         it always ends with a semicolon).
//--------------------------------------------------------------------------
void WriteDeviceID(void)

 {
  char *Pointer;

  Pointer = (DeviceIDRaw + 2); //Point at Device ID String
  while (*Pointer != 0)        //Go until the end-of-string
   {
    while (*Pointer != ';')    //Go until end-of-Tag
      cout << *Pointer++;      //Write the character, point at the next one
    cout << *Pointer++;        //Write the semicolon
    WriteString("\n");         //Move to the next line
   }
 }


//--------------------------------------------------------------------------
// PARSE DEVICE ID STRING AND WRITE A DEVICE DESCRIPTION
// Writes DESCRIPTION if it exists, else writes MODEL & MANUFACTURER.
// Return always = 0 (We just use Return to get out of routine early)
//--------------------------------------------------------------------------
int WriteDvcDescr(void)

 {
  char *Pointer;
  char *Pointer2;

  Pointer2 = (DeviceIDRaw + 1);                //Point at the String

  Pointer = strstr(Pointer2,";DESCRIPTION:");  //Look for DESCRIPTION:
  if (Pointer != NULL)                         //If Found,
    Pointer += 13;                             //  Point at Descr
   else                                        //If not Found,
   {
    Pointer = strstr(Pointer2,";DES:");        //  look for DES:
    if (Pointer != NULL)                       //If Found,
      Pointer += 5;                            //  Point at Descr
   }
  if (Pointer != NULL)                         //If we have Descr,
   {
    while (*Pointer != ';')                    //Write until we get to
     cout << *Pointer++;                       //  the semicolon
    return(0);                                 //Done
   }

  Pointer = strstr(Pointer2,";MANUFACTURER:"); //Look for MANUFACTURER:
  if (Pointer != NULL)                         //If Found,
    Pointer += 14;                             //  Point at Manufacturer
   else                                        //If not Found,
   {
    Pointer = strstr(Pointer2,";MFG:");        //  look for MFG:
    if (Pointer != NULL)                       //If Found,
      Pointer += 5;                            //  Point at Manufacturer
   }
  if (Pointer != NULL)                         //If we have Mfg,
   {
    while (*Pointer != ';')                    //Write until we get to
     cout << *Pointer++;                       //  the semicolon
   }
   else                                        //If no Manufacturer,
   {
    cout << "Unknown Printer";                 //  Write Unknown Printer
    return(0);                                 //Error
   }

  Pointer = strstr(Pointer2,";MODEL:");        //Look for MODEL:
  if (Pointer != NULL)                         //If Found,
    Pointer += 7;                              //  Point at Model
   else                                        //If not Found,
   {
    Pointer = strstr(Pointer2,";MDL:");        //  look for MDL:
    if (Pointer != NULL)                       //If Found,
      Pointer += 5;                            //  Point at Model
   }
  if (Pointer != NULL)                         //If we have Model,
   {
    cout << " ";                               //  Write a Space
    while (*Pointer != ';')                    //Write until we get to
     cout << *Pointer++;                       //  the semicolon
   }
  return(0);                                   //Done
 }


//--------------------------------------------------------------------------
// Write the entire Epson Ink Quantity String, without the Form Feed
//   at the end.
//--------------------------------------------------------------------------
void WriteEpsonInkQuantity(void)

 {
  char *Pointer;

  Pointer = (InkQuantity); //Point at Ink Quantity String
  while (*Pointer != 12)   //Go until the Form-Feed character at the end
    cout << *Pointer++;    //Write the character, point at the next one
  WriteString("\n");       //Move to the next line
 }


//±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±
// EXTENDED PRINTER FUNCTIONS
//±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±

//--------------------------------------------------------------------------
// TEST TO SEE IF EXTENDED PRINTER FUNCTIONS ARE INSTALLED.
// Returns 0 if OK (Extended Services are installed)
//        -1 if not
//--------------------------------------------------------------------------
int I17XTestInstall(void)

 {
  static union REGS Regs; //CPU Registers
  int  FileHandle;        //File Handle

   FileHandle = _open("LPT9",O_RDWR);      //Try to open LPT9 Device
   if (FileHandle == -1)                   //If Error
     return(-1);                           //No Extended Services Installed
   Regs.x.ax = 0x4400;                     //AX = Function = Get Dvc Info
   Regs.x.bx = FileHandle;                 //BX = Handle
   intdos(&Regs,&Regs);                    //Do it
   if ((Regs.h.dl & DvcInfoIsDevice) == 0) //If a File (not a Device)
    {
     close(FileHandle);                    //Close the File
     return(-1);                           //No Extended Services Installed
    }

  //------------------------------------------------------------------------
  // Test using DOS Services.
  // Only DOS 5+ supports this particular call (Generic IOCTL Check), so
  //   an error here could simply mean that DOS does not support the call.
  // An error at the DOS level does not necessarily mean that the
  //   Extended Printer Services are not installed.
  //------------------------------------------------------------------------
  Regs.x.ax = 0x4410;           //AX = Func = 4410h (Generic IOCTL Check)
  Regs.x.bx = FileHandle;       //BX = File Handle
  Regs.h.ch = IOCatLPTx;        //CH = Category = Printer
  Regs.h.cl = IOCmdGetDeviceID; //CL = Command = Get Device ID
  intdos(&Regs, &Regs);         //Do it
  close(FileHandle);            //Close the File
  if (Regs.x.cflag == 0)        //If no error,
    return(0);                  //  Extended Services Installed

  //------------------------------------------------------------------------
  // Test using BIOS Services.
  // Only do this if the DOS test failed.
  //------------------------------------------------------------------------


  cout << "\n"
       << "Test Extended Functions BIOS"
       << "\n";


  Regs.x.ax = I17AXInstallCheck;   //AX = Function (Install Check)
  Regs.x.bx = I17BXInput;          //BX = 'LP'
  Regs.x.cx = I17CXInput;          //CX = 'T!'
  Regs.x.dx = I17DXInput;          //DX = -1 (No specific Port)
  int86(0x17,&Regs,&Regs);         //Do Int 17h (LPT BIOS)
  if ((Regs.x.ax != 0x0000)     || //If returned
      (Regs.x.bx != I17CXInput) || //  registers
      (Regs.x.cx != I17BXInput) || //  are
      (Regs.x.dx != I17DXInput))   //  incorrect,
    return(-1);                    //  Extended Services Not Installed
  return(0);                       //Extended Services Installed
 }


//--------------------------------------------------------------------------
// Test to see if Int 17h Extended Functions are installed on a
//   particular LPT Port.
// Return = 1 if Functions are installed.
//        = 0 if not.
//--------------------------------------------------------------------------
int I17XTestPort
     (
      int LPTIndex
     )

 {
  static union REGS Regs;      //CPU General Registers
  char   *DeviceName = "LPTx"; //Name of Device to Open
  int    FileHandle;           //File Handle

  //------------------------------------------------------------------------
  // Test Port using DOS Services.
  // Only DOS 5+ supports this particular call (Generic IOCTL Check), so
  //   an error here could simply mean that DOS does not support the call.
  // An error at the DOS level does not necessarily mean that the
  //   Extended Printer Services are not installed for this Port.
  //------------------------------------------------------------------------
  *(DeviceName+3) = (unsigned char) LPTIndex + '1'; //Fill in Device Name
  FileHandle = _open(DeviceName, O_RDWR);           //Attempt to Open Device
  if (FileHandle != -1)                             //If No Error
   {
    Regs.x.ax = 0x4410;           //AX = Func = 4410h (Generic IOCTL Check)
    Regs.x.bx = FileHandle;       //BX = File Handle
    Regs.h.ch = IOCatLPTx;        //CH = Category = Printer
    Regs.h.cl = IOCmdGetDeviceID; //CL = Command = Get Device ID
    intdos(&Regs, &Regs);         //Do it
    close(FileHandle);            //Close the File
    if (Regs.x.cflag == 0)        //If no error,
      return(1);                  //  Port has Extended Printer Services
   }

  //------------------------------------------------------------------------
  // Test Port using BIOS Services.
  // Do this only if DOS services fail.
  //------------------------------------------------------------------------


  cout << "\n"
       << "Test Port BIOS"
       << "\n";


  Regs.x.ax = I17AXInstallCheck; //AX = Function (Installation Check)
  Regs.x.bx = I17BXInput;        //BX = 'LP'
  Regs.x.cx = I17CXInput;        //CX = 'T!'
  Regs.x.dx = LPTIndex;          //DX = LPT Index
  int86(0x17,&Regs,&Regs);       //Do Int 17h (LPT BIOS)
  if ((Regs.x.ax != 0x0000) ||
      (Regs.x.bx == I17BXInput))
    return(0);                   //If invalid registers returned, Error
  return(1);                     //No error
 }


//--------------------------------------------------------------------------
// TEST TO SEE IF A PRINTER IS ATTACHED TO A PORT OR NOT.
// Return = 1 if Printer installed.
//        = 0 if not.
//--------------------------------------------------------------------------
int TestPrinter
     (
      int LPTIndex
     )

 {
  static union REGS Regs;

  Regs.h.ah = 0x02;                            //AH = Function (Get Status)
  Regs.x.dx = LPTIndex;                        //DX = LPT Index
  int86(0x17,&Regs,&Regs);                     //Do It
  if (Regs.h.ah == (PStsSelected+PStsNotBusy)) //If Printer attached,
    return(1);                                 //  return OK
  return(0);                                   //Return Error
 }


//--------------------------------------------------------------------------
// DOWNLOAD THE DEVICE ID FROM A PRINTER.
// Return = 1 if Device ID was downloaded.
//        = 0 if not.
//--------------------------------------------------------------------------
int I17XGetDeviceID
     (
      int Port,
      int BufferSeg,
      int BufferOffset,
      int Size
     )

 {
  static union REGS Regs;           //CPU General Registers
  struct SREGS Segs;                //CPU Segment Registers
  struct GetDeviceIDStruc GetDvcID; //Get Device ID Data Structure
  char   *DeviceName = "LPTx";      //Name of Device to Open
  int    FileHandle;                //File Handle

  //------------------------------------------------------------------------
  // Get Device ID String using DOS Services.
  // Only DOS 3.2+ supports this particular call (Generic IOCTL), so
  //   an error here could mean that DOS does not support the call.
  // It's pretty rare for people not to be using at least DOS 3.3
  //   these days, but it can happen.
  //------------------------------------------------------------------------


  cout << "\n"
       << "Get Device ID DOS"
       << "\n";


  *(DeviceName+3) = (unsigned char) Port + '1'; //Fill in Device Name
  FileHandle = _open(DeviceName, O_RDWR);       //Attempt ot Open Device
  if (FileHandle != -1)                         //If OK
   {
    GetDvcID.IOCtlFlags = 1;                    //No Short Packet Error
    GetDvcID.IOCtlBuffSize = Size;              //Maximum String Size
    GetDvcID.IOCtlBuffOffset = BufferOffset;    //Buffer Offset
    GetDvcID.IOCtlBuffSegment = BufferSeg;      //Buffer Segment
    Regs.x.ax = 0x440C;                         //AX = Generic Char IOCTL
    Regs.x.bx = FileHandle;                     //BX = File Handle
    Regs.h.ch = IOCatLPTx;                      //CH = Category = Printer
    Regs.h.cl = IOCmdGetDeviceID;               //CL = Command = Get Device ID
    Regs.x.dx = (unsigned) &GetDvcID;           //DS:[DX] = Data Structure
    intdos(&Regs,&Regs);                        //Do it
    close(FileHandle);                          //Close the File
    if (Regs.x.cflag == 0)                      //If no error,
      return(1);                                //  we're done
   }

  //------------------------------------------------------------------------
  // Get Device ID String using BIOS Services.
  // Do this only if DOS services fail.
  //------------------------------------------------------------------------


  cout << "\n"
       << "Get Device ID BIOS"
       << "\n";


  Regs.x.ax = I17AXGetDvcIDString; //AX = Function (Get Device ID)
  Regs.x.bx = BufferOffset;        //BX = Buffer Offset
  Regs.x.cx = Size;                //CX = Buffer Size
  Regs.x.dx = Port;                //DX = Port
  Segs.ds   = BufferSeg;           //DS = Buffer Segment
  int86x(0x17,&Regs,&Regs,&Segs);  //Do Int 17h (LPT BIOS)
  if (((Regs.x.ax == 0x0000) ||
       (Regs.x.ax == I17ErrIncompleteXfer)) &&
       (Regs.x.cx > 10))           //If valid registers returned,
    return(1);                     //  no error
  return(0);                       //Error
 }


//--------------------------------------------------------------------------
// DOWNLOAD SOME DATA FROM A PRINTER.
// Return = Number of Bytes if Data was downloaded.
//        = 0 if not.
//--------------------------------------------------------------------------
int I17XGetData
     (
      int Port,
      int BufferSeg,
      int BufferOffset,
      int Size
     )

 {
  static union REGS Regs;      //CPU General Registers
  struct SREGS Segs;           //CPU Segment Registers
  char   *DeviceName = "LPTx"; //Name of Device to Open
  int    FileHandle;           //File Handle

  //------------------------------------------------------------------------
  // Get Data using DOS Services.
  //------------------------------------------------------------------------


  cout << "\n"
       << "Read DOS"
       << "\n";


  *(DeviceName+3) = (unsigned char) Port + '1'; //Fill in Device Name
  FileHandle = _open(DeviceName, O_RDWR);       //Attempt to Open Device
  if (FileHandle != -1)                         //If OK
   {
    Regs.h.ah = 0x3F;                           //AH = Function = Read
    Regs.x.bx = FileHandle;                     //BX = File Handle;
    Regs.x.cx = Size;                           //CX = Number of Bytes
    Segs.ds = BufferOffset;                     //DS:[DX] =
    Regs.x.dx = BufferOffset;                   //  Buffer
    intdosx(&Regs,&Regs,&Segs);                 //Do it
    close(FileHandle);                          //Close the Device
    if (Regs.x.cflag == 0)                      //If no error,
      return(Regs.x.ax);                        //  Done
   }

  //------------------------------------------------------------------------
  // Get Data using BIOS Services.
  // Do this only if DOS services fail.
  //------------------------------------------------------------------------


  cout << "\n"
       << "Read BIOS"
       << "\n";


  Regs.x.ax = I17AXRxBlock;       //AX = Function (Receive Data)
  Regs.x.bx = BufferOffset;       //BX = Buffer Offset
  Regs.x.cx = Size;               //CX = Buffer Size
  Regs.x.dx = Port;               //DX = Port
  Segs.ds   = BufferSeg;          //DS = Buffer Segment
  int86x(0x17,&Regs,&Regs,&Segs); //Do Int 17h (LPT BIOS)
  if ((Regs.x.ax == 0x0000) ||
      (Regs.x.ax == I17ErrIncompleteXfer))
    return(Regs.x.cx);            //If valid Data, return bytes Xfered
  return(0);                      //Error
 }


//--------------------------------------------------------------------------
// SEND SOME DATA TO A PRINTER.
// Return = 1 if Data was sent.
//        = 0 if not.
//--------------------------------------------------------------------------
int I17XSendData
     (
      int Port,
      int BufferSeg,
      int BufferOffset,
      int Size
     )

 {
  static union REGS Regs;      //CPU General Registers
  struct SREGS Segs;           //CPU Segment Registers
  char   *DeviceName = "LPTx"; //Name of Device to Open
  int    FileHandle;           //File Handle

  //------------------------------------------------------------------------
  // Write Data using DOS Services.
  //------------------------------------------------------------------------


  cout << "\n"
       << "Write DOS"
       << "\n";


  *(DeviceName+3) = (unsigned char) Port + '1'; //Fill in Device Name
  FileHandle = _open(DeviceName, O_RDWR);       //Attempt to Open Device
  if (FileHandle != -1)                         //If OK
   {
    Regs.h.ah = 0x40;                           //AH = Function = Write
    Regs.x.bx = FileHandle;                     //BX = File Handle;
    Regs.x.cx = Size;                           //CX = Number of Bytes
    Segs.ds = BufferOffset;                     //DS:[DX] =
    Regs.x.dx = BufferOffset;                   //  Buffer
    intdosx(&Regs,&Regs,&Segs);                 //Do it
    close(FileHandle);                          //Close the Device
    if ((Regs.x.cflag == 0) &&                  //If no error
        (Regs.x.ax == Size))                    //  and all bytes sent,
      return(1);                                //  Done
   }

  //------------------------------------------------------------------------
  // Write Data using BIOS Services.
  // Do this only if DOS services fail.
  //------------------------------------------------------------------------


  cout << "\n"
       << "Write BIOS"
       << "\n";


  Regs.x.ax = I17AXTxBlock;       //AX = Function (Send Data)
  Regs.x.bx = BufferOffset;       //BX = Buffer Offset
  Regs.x.cx = Size;               //CX = Buffer Size
  Regs.x.dx = Port;               //DX = Port
  Segs.ds   = BufferSeg;          //DS = Buffer Segment
  int86x(0x17,&Regs,&Regs,&Segs); //Do Int 17h (LPT BIOS)
  if (Regs.x.ax == 0x0000)        //If it worked,
    return(1);                    //  no Error
  return(0);                      //Error
 }


//±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±
// SCREEN-WRITING FUNCTIONS (INCLUDES AUTO-PAUSING FUNCTIONS)
//±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±

//--------------------------------------------------------------------------
// Calculate and store the number of screen rows,
//   and Mono/Color Screen attributes.
// If Screen Output has been redirected (e.g., INKLEVEL | MORE),
//   this sets ScreenRows = 0.
//--------------------------------------------------------------------------
void GetScreenRows(void)

 {
  static union REGS Regs;
  struct text_info TextInfo;

  gettextinfo(&TextInfo);               //Get Screen Info
  if (TextInfo.currmode == 7)           //If monochrome screen,
    IsColor = 0;                        //  mark it as monochrome
   else                                 //If color screen,
    IsColor = 1;                        //  mark it as color
  TextAttrib = TextInfo.attribute;      //Save text attribute
  Regs.x.ax = 0x4400;                   //Function 4400h (Get Device Info)
  Regs.x.bx = StdOutHandle;             //Device = STDOUT
  intdos(&Regs,&Regs);                  //Do it
  if ((Regs.h.dl & (DvcInfoIsDevice+DvcInfoIsStdOut)) == //If not Redirected
                   (DvcInfoIsDevice+DvcInfoIsStdOut))
    ScreenRows = TextInfo.screenheight; //  Store ScreenRows
   else                                 //If Redirected,
    ScreenRows = 0;                     //  ScreenRows = 0
 }


//--------------------------------------------------------------------------
// WRITE A USER QUERY STRING OF CHARACTERS TO THE SCREEN.
// If output is redirected, write to STDERR.
// If output is not redirected, write using the Auto-Pause routines
//--------------------------------------------------------------------------
void WriteQuery
      (
       char *String
      )

 {
  if (ScreenRows == 0)   //If Output is redirected
    cerr << String;      //Write to STDERR
   else                  //If Output is not redirected
    WriteString(String); //Write to Pause
 }


//--------------------------------------------------------------------------
// WRITE A STRING OF CHARACTERS TO THE SCREEN, PAUSING WHEN SCREEN IS FULL.
// ALL Messages written to the screen that involve a new line character
//   MUST use this routine to write things, rather than using
//   printf or cout.  If a routine does not use this routine, the
//   Auto-Pause feature will not work correctly!
//--------------------------------------------------------------------------
void WriteString
      (
       char *String
      )

 {
  char *PauseMsg    = "\n\004\004\004 MORE \004\004\004";
  char *PauseDelMsg = "\r\031\031\031 MORE \031\031\031\n";

  if (ScreenRows == 0)                   //If Output is Redirected,
    cout << String;                      //  just write the String
   else                                  //If Output is not Redirected
   {
    while (*String != 0)                 //Search until End-Of-String
     {
      cout << *String;                   //Write the next character
      if (strncmp(String++,"\n",1) == 0) //If New Line,
       {
        RowCount++;                      //  increment Row Count
        if (RowCount >= (ScreenRows-2))  //If at end-of screen
         {
          cout << PauseMsg;              //Write our Pause Message
          while (kbhit() == 0);          //Wait for a keystroke
          getch();                       //Get the keystroke
          cout << PauseDelMsg;           //Delete the Pause Message
          RowCount = 0;                  //Reset Row Count
         }
       }
     }
   }
 }


//--------------------------------------------------------------------------
// WRITE A STRING OF CHARACTERS TO THE SCREEN IN COLOR (DIRECT VIDEO).
// If STDOUT is redirected (ScreenRows = 0), or if screen is monochrome
//   (IsColor = 0), this simply sends the screen to STDOUT rather than
//   writing directly to the screen.  This allows the redirection to
//   work like it should, or allows a monochrome screen to not get
//   messed up.
// This assumes that the Color Parameters are already set (with textcolor
//   and textbackground calls).
//--------------------------------------------------------------------------
void WriteStringColor
      (
       char *String
      )

 {
  if ((ScreenRows == 0) || //If Output is Redirected,
      (IsColor    == 0))   //  or screen is monochrome,
    WriteString(String);   //  write to STDOUT with Pause
   else                    //If not Redirected and screen is color,
    cprintf(String);       //  write directly to Video Memory
 }


//--------------------------------------------------------------------------
// WRITE A PERCENTAGE (THREE CHARACTERS) IN COLOR TO THE SCREEN.
// As far as I can tell, there is no easy way to convert an integer
//   to a string exactly three characters long using the
//   standard library functions, so we'll do it ourselves "the hard way".
//--------------------------------------------------------------------------
void WritePercentColor
      (
       int Percent
      )

 {
  int   Hundreds;
  int   Tens;
  int   Ones;
  char *Character = "0";

  if (Percent == 100)             //Write Hundreds
   {
    WriteStringColor("1");
    Percent -= 100;
    Hundreds = 1;
   }
   else
   {
    WriteStringColor(" ");
    Hundreds = 0;
   }
  Tens = (Percent / 10);          //Calculate Tens
  Ones = (Percent % 10);          //Calculate Ones
  if ((Hundreds == 1) ||          //Write Tens
      (Tens != 0))
   {
    *Character = (Tens + '0');
    WriteStringColor(Character);
   }
   else
    WriteStringColor(" ");
  *Character = (Ones + '0');      //Write Ones
  WriteStringColor(Character);
 }


//--------------------------------------------------------------------------
// WRITE A REPEATING CHARACTER TO THE SCREEN.
//--------------------------------------------------------------------------
void RepeatChar
      (
       char *Character,
       int   Count
      )

 {
  while (Count-- > 0)      //Write the character
   WriteString(Character); //  the specified number of times
 }


//--------------------------------------------------------------------------
// WRITE A REPEATING CHARACTER TO THE SCREEN, USING DIRECT VIDEO OUTPUT.
//--------------------------------------------------------------------------
void RepeatCharColor
      (
       char *Character,
       int   Count
      )

 {
  while (Count-- > 0)           //Write the character
   WriteStringColor(Character); // the specified number of times
 }


//±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±
// MISCELLANEOUS SUPPORT FUNCTIONS
//±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±

//--------------------------------------------------------------------------
// WRITE OUR COPYRIGHT MESSAGE TO THE SCREEN.
//--------------------------------------------------------------------------
void WriteCopyright(void)

 {
  WriteString(ProgramName);
  WriteString(" 0.01, (C) 2007, Bret E. Johnson.\n");
  WriteString("Printer Ink Fill Percentage Display Program for DOS.\n");
  WriteString("Works with many Epson- and HP-compatible Printers.\n");
 }


//--------------------------------------------------------------------------
// TEST TO SEE IF WE ARE RUNNING UNDER WINDOWS NT.
//   If so, quit the program with an Error Message and an ErrorLevel.
// NOTE: This simply tests for an Environment variable called "OS", which
//         is set to "Windows_NT", at least for XP.  I assume (but do not
//         know for sure) that this test will also work under other
//         versions of NT.  By a long shot, this is neither a definitive
//         or foolproof test for Windows NT, but will hopefully be good
//         enough for our purposes.
//       The reason we specifically test for NT and not for other (non-NT)
//         versions of Windows is because this program crashes an NT
//         DOS Box.  I haven't test it thoroughly, but under DOS-based
//         versions of Windows it doesn't seem to crash -- it just doesn't
//         work (at least with USB-attached Printers).  Only when it crashes
//         under a particular OS is there a reason to perform a test
//         (in my opinion).
//       In case you're interested, the reason it doesn't work under
//         Windows is because Windows doesn't like to give DOS programs
//         direct access to the hardware, but direct access is the only way
//         to work with printers at this level.
//--------------------------------------------------------------------------
void WindowsNTTest(void)

 {
  static union REGS Regs;  //CPU Registers

  Regs.x.ax = 0x3306;      //Function 3306h (Get True DOS Version)
  Regs.x.bx = 0;           //Preset Error Flag
  intdos(&Regs, &Regs);    //Do it
  if (Regs.x.bx == 0x3205) //Version 5.50 (NT DOS Box)?
   {                       //If so, print error message
    cerr << "\n"
         << "This program does not work correctly underneath Windows NT."
         << "\n";
    BeepSpeaker();         //Beep the speaker
    exit(ErLvlWindowsNT);  //Quit program
   }
 }


//--------------------------------------------------------------------------
// FILL A STRING BUFFER WITH ALL ZEROES
//--------------------------------------------------------------------------
void ResetString
      (
       char *Pointer,
       int   Size
      )

 {
  while (Size-- > 0) //Fill String
   *Pointer++ = 0;   //  with zeroes
 }

//--------------------------------------------------------------------------
// BEEP THE SPEAKER
//--------------------------------------------------------------------------
void BeepSpeaker(void)

 {
  sound(1000); //Beep Speaker at 1000 Hz
  delay(200);  //Hold it for 200 milliseconds
  nosound();   //Turn off the Speaker
 }