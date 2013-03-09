//--------------------------------------------------------------------------
// This is written in Turbo C++ for DOS version 3.0.
// Only standard Libraries are needed.  All "special" code is contained
//   directly in this source file.
//--------------------------------------------------------------------------

//Interfaces Numbers should be increasing, but do not necessarily need
//  to be contiguous (according to later Rev's of MS Drivers).  Make
//  sure we test all Interface (& Alt Interface?) Numbers.

//Does not show HID Interface at end of Plantronics Device, though it does
//  respond to INT 14h Requests from USBJStik.  Find out why.


#pragma option -a -d -mt
               //Word-alignment
                  //Merge duplicate strings
                     //Tiny Memory Model

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
// This program will display information about all of the different
//   Devices that are attached to the USB Host(s) on the Computer.
//   It can only do this if the software controlling the USB Host(s)
//   is compatible, however.  The only combatible software written
//   at this time is made by me (Bret Johnson), and is called
//   USBUHCI.  If you don't have any USB Controllers on your computer
//   (just about impossible on a newer computer), or if they are
//   not controlled by a compatible program (USBUHCI), this program
//   will not do you any good.  This program will also NOT work
//   with the USB in Legacy Mode (that is, when it is being controlled
//   by BIOS software).
//--------------------------------------------------------------------------


//±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±
// Includes
//±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±

#include <conio.h>
#include <ctype.h>
#include <dir.h>
#include <dos.h>
#include <iomanip.h>
#include <process.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


//±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±
// Function Prototypes
//±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±


void GetCmdLine        (void);
int  ParseCmdLine      (char *Pointer,      int Testing);
int  SkipSpaces        (char **Pointer);
int  SkipDashSlash     (char **Pointer);
int  GetSegOff         (char **Pointer,     int *Segment,
                        int  *Offset);
int  GetHex            (char **Pointer,     int *HexNumber);
char GetAlias          (char **TestString,  char *AliasList);
void WriteHelp         (void);
void WriteAliases      (void);
void WriteErrorLevels  (void);

void WriteNewDeviceTbl (void);

int  WriteHostInfo     (int HostIndex,      int SpecificRequest);
int  WriteDeviceInfo   (int HostIndex,      int DeviceAddress);
int  WriteAltIntfInfo  (int HostIndex,      int DeviceAddress,
                        int InterfaceNum,   int AltInterface,
                        int DeviceClass,    int DeviceSubClass,
                        int DeviceProtocol,
                        int BadDeviceFlag,  int BadDeviceErr,
                        int SpacesToWrite);
void WriteDivider      (void);

//void far FarWrite      (void);
void WritePCIVendor    (int PCIVendor);
void WriteUSBVendor    (int USBVendor);
void WriteStageError   (int ErrorStage,     int ErrorCode);
void WriteDescription  (int DeviceClass,    int DeviceSubClass,
                        int DeviceProtocol, int IntfClass,
                        int IntfSubClass,   int IntfProtocol);
int  DoExec            (char *SuptFile);
int  GetOurExecPath    (void);

void Int14TestInstall  (void);
int  Int14SendRequest  (int *AXRegister,    int *BXRegister,
                        int *CXRegister,    int *DXRegister);

void GetScreenRows     (void);
void WriteString       (char *String);
void RepeatChar        (char Character,     int Count);
void WriteNewLines     (int NumLines);
void WriteDecimal      (int Number,         int Width);
void WriteHex          (int  Number,        int Spaces,
                        char Suffix);

void WriteCopyright    (void);
void ParentIsShell     (void);
void CopyNameToMCB     (void);
void BeepSpeaker       (void);


//±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±
// Constants
//±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±

//--------------------------------------------------------------------------
// ErrorLevels returned to DOS Prompt
//--------------------------------------------------------------------------
#define ErLvlNoError 0 //No Error
#define ErLvlEnviron 1 //Environment variable error
#define ErLvlCmdLine 2 //Command-line error
#define ErLvlNoInt14 3 //Compatible USB Driver not installed


//--------------------------------------------------------------------------
// Standard DOS "hard-coded" Device Handles
//--------------------------------------------------------------------------
#define StdInHandle  0 //Standard Input
#define StdOutHandle 1 //Standard Output
#define StdErrHandle 2 //Standard Error
#define StdAuxHandle 3 //Standard Auxiliary (COM1)
#define StdPrnHandle 4 //Standard Printer (LPT1)


//--------------------------------------------------------------------------
// PSP Segment Offsets
//--------------------------------------------------------------------------
#define PSPParentPSP   0x16 //Offset in PSP of Parent PSP Segment
#define PSPEnvironment 0x2C //Offset in PSP of Environment Segment


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


//±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±
// Int 14 Request Constants
// They go in the various Int 14h Request Structure Parameters,
//   or are sent as return values (error codes & such).
//±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±

//--------------------------------------------------------------------------
// INT 14h USB Functions
//--------------------------------------------------------------------------
#define I14BXInput        0x5553 //'US'
#define I14CXInput        0x4221 //'B!'

#define I14AXInstallCheck 0x5000 //Installation Check Function
#define I14AXDoFunction   0x5001 //All other functions

//--------------------------------------------------------------------------
//Int 14h Request Types
//--------------------------------------------------------------------------
#define I14RRTHostClass         0x00 //Host/System/OS Class
#define   I14RRTGetHostSWInfo   0x01 //Get Host Software Info
#define   I14RRTGetHostHWInfo   0x02 //Get Host Hardware Info
#define   I14RRTGetHostVendInfo 0x03 //Get Host Vendor Info
#define   I14RRTGetHostStatus   0x04 //Get Current Host Status
#define   I14RRTHostRun         0x08 //Start/Run/Resume Host
#define   I14RRTHostStop        0x09 //Stop Host
#define   I14RRTHostReset       0x0A //Reset Host
#define   I14RRTHostSuspend     0x0B //Global Suspend Host
#define   I14RRTHostResume      0x0C //Force Global Resume on Host
#define I14RRTTimingClass       0x10 //Frame Timing Class
#define   I14RRTRegTmgOwner     0x11 //Register as Timing Owner
#define   I14RRTUnRegTmgOwner   0x12 //UnRegister as Timing Owner
#define   I14RRTIncTiming       0x13 //Increment (Slow Down) Frame Timing
#define   I14RRTDecTiming       0x14 //Decrement (Speed Up) Frame Timing
#define   I14RRTChangeTiming    0x15 //Change Frame Timing (by Large Amount)
#define I14RRTHubClass          0x20 //Hub Class
#define   I14RRTGetDvcHubInfo   0x21 //Get Hub Info for Device
#define   I14RRTNewDvcConn      0x24 //Hub has Detected new Device
#define   I14RRTDvcDisc         0x25 //Device has been Disconnected
#define   I14RRTSendHubChar     0x27 //Send Hub Characteristics to Host
#define   I14RRTEnableHubPort   0x28 //Enable Device given Hub & Port
#define   I14RRTDisableHubPort  0x29 //Disable Device given Hub & Port
#define   I14RRTResetHubPort    0x2A //Reset Device given Hub & Port
#define   I14RRTSuspendHubPort  0x2B //Suspend Device given Hub & Port
#define   I14RRTResumeHubPort   0x2C //Resume Device given Hub & Port
#define   I14RRTPwrOnHubPort    0x2D //Power On Device given Hub & Port
#define   I14RRTPwrOffHubPort   0x2E //Power Off Device given Hub & Port
#define   I14RRTPwrResetHubPort 0x2F //Power Reset Device given Hub & Port
#define I14RRTTPowerClass       0x30 //Power Class
#define   I14RRTGetDvcPowerInfo 0x31 //Get Power Info for Device
#define   I14RRTPwrOnDevice     0x3D //Power On Device given Dvc Addr
#define   I14RRTPwrOffDevice    0x3E //Power Off Device given Dvc Addr
#define   I14RRTPwrResetDevice  0x3F //Power Reset Device given Dvc Addr
#define I14RRTDeviceClass       0x40 //Device Class
#define   I14RRTGetDvcClassInfo 0x41 //Get Device Class Info
#define   I14RRTGetDvcVendInfo  0x42 //Get Device Vendor Info
#define   I14RRTGetDvcStatus    0x43 //Get Device Status
#define   I14RRTEnableDevice    0x48 //Enable/Resume Device given Dvc Addr
#define   I14RRTDisableDevice   0x49 //Disable Device given Dvc Addr
#define   I14RRTResetDevice     0x4A //Reset Device given Dvc Addr
#define   I14RRTSuspendDevice   0x4B //Suspend Device given Dvc Addr
#define   I14RRTResumeDevice    0x4C //Resume Device given Dvc Addr
#define I14RRTConfigClass       0x50 //Configuration Class
#define   I14RRTSetNewConfig    0x58 //Set/Change Device Config Value
#define I14RRTInterfaceClass    0x60 //Interface Class
#define   I14RRTFindRegIntf     0x62 //Look for Registered Interface
#define   I14RRTFindUnRegIntf   0x63 //Look for Unregistered Interface
#define   I14RRTRegIntfOwner    0x64 //Register as Interface Owner
#define   I14RRTUnRegIntfOwner  0x65 //Unregister as Interface Owner
#define   I14RRTIntfDontLook    0x68 //Existing Interface Owner Don't Look
#define I14RRTAltIntfClass      0x70 //Alternate Interface Class
#define   I14RRTGetAltIntfInfo  0x71 //Get Alternate Interface Info
#define I14RRTEndPointClass     0x80 //End Point Class
#define I14RRTPacketClass       0x90 //Packet Class
#define   I14RRTDoIsoch         0x94 //Schedule Isochronous Transaction
#define   I14RRTDoInterruptPer  0x95 //Schedule Periodic Interrupt
#define   I14RRTDoControl       0x96 //Schedule Control/Setup Request
#define   I14RRTDoBulk          0x97 //Schedule Bulk Transaction
#define   I14RRTDoInterrupt1T   0x98 //Schedule One-Time Interrupt
#define   I14RRTCloseHandle     0x9C //Close/Remove Scheduled Transaction
#define   I14RRTChangeIntPer    0x9D //Change Periodicity of Interrupt
#define   I14RRTGetTransStatus  0x9F //Get Status of Packet Transaction
#define I14RRTMiscClass         0xA0 //Miscellaneous Class
#define   I14RRTLargeCallDone   0xA1 //Large (Complicated) Call Complete
#define   I14RRTBeepSpeaker     0xAF //Beep the Speaker
#define I14RRTInterHostClass    0xE0 //Inter-Host Communication Class
#define   I14RRTCopyNDTable     0xE5 //Copy New Device Ownership Table
#define I14RRTInternalClass     0xF0 //Host Internal/Troubleshooting Class

//--------------------------------------------------------------------------
//Int 14h Flags
//--------------------------------------------------------------------------
#define I14RFlagIn            0x01 //In Direction
#define I14RFlagLowSpeed      0x02 //Low-Speed Device
#define I14RFlagHiSpeed       0x04 //High-Speed Device
#define I14RFlagNoRetries     0x10 //No Auto Retries for Control
#define I14RFlagShortPktOK    0x20 //No Retries for Short Packets
#define I14RFlagSpecificFrame 0x40 //Use Specific Frame Number
#define I14RFlagAddrIsPhys    0x80 //Data Address is Physical

//--------------------------------------------------------------------------
//Error Codes Returned on Completion of Transfer Descriptor (AX)
//--------------------------------------------------------------------------
#define TDStsOK             0x0000 //ACK Received (TD completed OK / no errors)
#define TDStsNAKReceived    0x0001 //NAK Received
#define TDStsStalled        0x0002 //TD is Stalled
#define TDStsTimeout        0x0004 //TD has timed out (Bulk/Control)
#define TDStsOverDue        0x0008 //TD is OverDue (Int/Isoch)
#define TDStsShortPacket    0x0010 //TD Short Packet Detected (w/ SPD Enabled)
#define TDStsBabbleDetected 0x0020 //Babble Detected
#define TDStsCRCTOReceived  0x0040 //CRC/TimeOut Error Received
#define TDStsBitStuffError  0x0080 //Rx Data contained > 6 ones in a row
#define TDStsDataBuffErr    0x0100 //Data Buffer Error
#define TDStsControlSetup   0x1000 //Error occurred during Control Setup Stage
#define TDStsLargeCallErr   0x8000 //Error during Large Call

//--------------------------------------------------------------------------
//Codes Returned by Int 14h Get Host Hardware Info Request (BL)
//--------------------------------------------------------------------------
  //Low Nibble (not bit-mapped)
#define USBHostTypeUHCI 0x01 //Universal Host Controller Interface
#define USBHostTypeOHCI 0x02 //Open Host Controller Interface
#define USBHostTypeEHCI 0x03 //Enhanced Host Controller Interface (USB 2.0)

  //High Nibble (not bit-mapped)
#define USBBusTypePCI   0x10 //Peripheral Component Interconnect

//--------------------------------------------------------------------------
//Codes Returned by Int 14h Get Device Info Request (DL)
//--------------------------------------------------------------------------
#define LowSpeedFlag 0x01 //Device is Low Speed

//--------------------------------------------------------------------------
//Codes Returned by Int 14h Get Device Status Request (CH)
//--------------------------------------------------------------------------
#define BadDevice        0x01 //Bad Device
#define RWakeupSupported 0x02 //Remote Wakeup Supported
#define RWakeupEnabled   0x04 //Remote Wakeup Enabled
#define TestModeEnabled  0x08 //Test Mode Enabled
#define BeingConfiged    0x80 //Device is currently being Configured

//--------------------------------------------------------------------------
//Codes Returned by Int 14h Get Alternate Interface Info Request (CL)
//--------------------------------------------------------------------------
#define AltIntfSelected 0x01 //Alternate Interface is Selected
#define AltIntfOwned    0x02 //(Alternate) Interface is Owned

//--------------------------------------------------------------------------
//Codes Returned by Int 14h Get Device Power Info Request (CL)
//--------------------------------------------------------------------------
#define DvcPwrInfoSelfPwr  0x01 //Device is Self-Powered
#define DvcPwrInfoHubPPP   0x40 //Device is Per-Port-Powered Hub
#define DvcPwrInfoDvcIsHub 0x80 //Device is a Hub

//--------------------------------------------------------------------------
//Number of Entries in the New Device Notification table
//--------------------------------------------------------------------------
#define NDEntries 128


//±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±
// Structures
//±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±

//--------------------------------------------------------------------------
// Int 14h USB Request Structure
//--------------------------------------------------------------------------

struct Int14RequestStruc
 {
  unsigned char I14RRequestType;     //Request Type
  unsigned char I14RFlags;           //Bit-level Flags
  unsigned char I14RHostIndex;       //Host Index
  unsigned char I14RDeviceAddress;   //USB Device Address
                                     //  Also I14RHubAddress
  unsigned char I14REndPoint;        //End Point
                                     //  Also I14RHubPort
                                     //  Also I14RAltInterface
  unsigned char I14RConfigValue;     //Configuration Value
                                     //  Also I14RCloseID
  unsigned char I14RInterfaceNum;    //Interface Number
  unsigned char I14RSearchIndex;     //Search Index
  unsigned int  I14RVendorID;        //Vendor ID
  unsigned int  I14RProductID;       //Product ID
  unsigned char I14RDvcClass;        //Device Class
  unsigned char I14RDvcSubClass;     //Device SubClass
  unsigned char I14RDvcProtocol;     //Device Protocol
  unsigned char I14RIntfClass;       //Interface Class
  unsigned char I14RIntfSubClass;    //Interface SubClass
  unsigned char I14RIntfProtocol;    //Interface Protocol
  unsigned int  I14RRequestHandle;   //Request Handle Number
  unsigned int  I14RPeriodicity;     //Interrupt Periodicity/Duration
                                     //  Also I14RBeepFrequency
  unsigned int  I14RTimeout;         //Transaction Timeout Value
  unsigned int  I14RDataAddrOff;     //Data Address Offset
  unsigned int  I14RDataAddrSeg;     //Data Address Segment
  unsigned int  I14RDataSize;        //Size of Data (bytes)
  unsigned int  I14RCallBackAddrOff; //Call Back Address Offset (IP)
                                     //  Also I14RLargeCallRtnCode
  unsigned int  I14RCallBackAddrSeg; //Call Back Address Segment (CS)
  unsigned int  I14RUserPktID;       //User Packet ID
  unsigned char I14RSetupReqType;    //Setup Packet Request Type
  unsigned char I14RSetupReq;        //Setup Packet Request Code
  unsigned int  I14RSetupValue;      //Setup Packet Value
  unsigned int  I14RSetupIndex;      //Setup Packet Index
  unsigned int  I14RSetupLength;     //Setup Packet Length
  unsigned int  I14RFrameTiming;     //Frame Timing Value (def = 12000)
                                     //  Also I14RFrameIndex
  unsigned int  I14RIsochSchedOff;   //Isochronous Schedule Table Offset
  unsigned int  I14RIsochSchedSeg;   //Isochronous Schedule Table Segment
  unsigned int  I14RFiller1;         //Fillers to equal 64 bytes
  unsigned int  I14RFiller2;         //  "     "    "   "    "
  unsigned int  I14RFiller3;         //  "     "    "   "    "
  unsigned int  I14RFiller4;         //  "     "    "   "    "
  unsigned int  I14RFiller5;         //  "     "    "   "    "
  unsigned int  I14RFiller6;         //  "     "    "   "    "
  unsigned int  I14RFiller7;         //  "     "    "   "    "
 }
   Int14Request,
  *Int14RequestPtr = &Int14Request;


//--------------------------------------------------------------------------
// New Device (Device 0) Notification Structure
//--------------------------------------------------------------------------

struct NewDvcNotifyStruc
 {
  unsigned int  NDVendorID;     //Vendor ID
  unsigned int  NDProductID;    //Product ID

  unsigned char NDDvcClass;     //Device Class
  unsigned char NDDvcSubClass;  //Device SubClass
  unsigned char NDDvcProtocol;  //Device Protocol
  unsigned char NDIntfClass;    //Interface Class
  unsigned char NDIntfSubClass; //Interface SubClass
  unsigned char NDIntfProtocol; //Interface Protocol

  unsigned int  NDUserPktID;    //User Packet ID
  unsigned int  NDCallBackOff;  //Call Back Address Offset
  unsigned int  NDCallBackSeg;  //Call Back Address Segment (0 = Empty Entry)
 };


//±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±
// Global Variables
// These are used by multiple subroutines to pass information back and
//   forth, so we put them here instead of messing around trying to pass
//   pointers back and forth between all of the various subroutines.
// This may not be the "elegant" way to do things, but makes life easier.
//±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±

char CommandLine[128]; //Original Command Line
char ExecString[70];   //EXEC Program Path & Name String
char ExecParams[126];  //EXEC Parameter String
int  ExecWriteOff = 0; //EXEC program's String Writing Routine Offset
int  ExecWriteSeg = 0; //EXEC program's String Writing Routine Segment
char TextAttrib;       //Original Text Attribute
int  ScreenRows;       //Number of screen rows
int  RowCount = 0;     //Row Counter for Auto-Pausing
int  IsColor;          //1 if Color Screen, 0 if Monochrome
int  AtCommandLine;    //0 if called from another Program, 1 if at Cmd Line
int  DoHelp = 0;       //Write Help Message
int  DoAlias = 0;      //Write Command-Line Alias Table
int  DoErrorLevel = 0; //Write ErrorLevel Table
char *ErrorPtr;        //Pointer on command-line where error occured


//--------------------------------------------------------------------------
//NOTE: This Table must be arranged in reverse order.  Since we search
//        the table from top to bottom, a string that begins with a
//        substring that corresponds to another table entry must come
//        BEFORE the substring entry does, or we will find the substring
//        entry first which causes errors.
//      Format for a Table Entry is an Alias Word, followed by a 0 ("\0"),
//        followed by a single character that the Alias corresponds to,
//        followed by another 0.
//      The last entry in the table must be a Nul String.
//--------------------------------------------------------------------------

char *AliasTbl =                //Command-line Option Aliases
  "OptionAliases"               "\0" "A" "\0"
  "OptionAlias"                 "\0" "A" "\0"

  "NewTbl"                      "\0" "N" "\0"
  "NewTable"                    "\0" "N" "\0"

  "NewDvcTbl"                   "\0" "N" "\0"
  "NewDvcTable"                 "\0" "N" "\0"
  "NewDvcNotifyTbl"             "\0" "N" "\0"
  "NewDvcNotifyTable"           "\0" "N" "\0"
  "NewDvcNotify"                "\0" "N" "\0"
  "NewDvcNotificationTbl"       "\0" "N" "\0"
  "NewDvcNotificationTable"     "\0" "N" "\0"
  "NewDvcNotification"          "\0" "N" "\0"

  "NewDeviceTbl"                "\0" "N" "\0"
  "NewDeviceTable"              "\0" "N" "\0"
  "NewDeviceNotifyTbl"          "\0" "N" "\0"
  "NewDeviceNotifyTable"        "\0" "N" "\0"
  "NewDeviceNotify"             "\0" "N" "\0"
  "NewDeviceNotificationTbl"    "\0" "N" "\0"
  "NewDeviceNotificationTable"  "\0" "N" "\0"
  "NewDeviceNotification"       "\0" "N" "\0"

  "New"                         "\0" "N" "\0"

  "Help"                        "\0" "?" "\0"
  "H"                           "\0" "?" "\0"

  "ErrorLvlTbl"                 "\0" "V" "\0"
  "ErrorLvlTable"               "\0" "V" "\0"
  "ErrorLvls"                   "\0" "V" "\0"
  "ErrorLvl"                    "\0" "V" "\0"
  "ErrorLevelTbl"               "\0" "V" "\0"
  "ErrorLevelTable"             "\0" "V" "\0"
  "ErrorLevels"                 "\0" "V" "\0"
  "ErrorLevel"                  "\0" "V" "\0"

  "ErrLvlTbl"                   "\0" "V" "\0"
  "ErrLvlTable"                 "\0" "V" "\0"
  "ErrLvls"                     "\0" "V" "\0"
  "ErrLvl"                      "\0" "V" "\0"
  "ErrLevelTbl"                 "\0" "V" "\0"
  "ErrLevelTable"               "\0" "V" "\0"
  "ErrLevels"                   "\0" "V" "\0"
  "ErrLevel"                    "\0" "V" "\0"

  "ErLvlTbl"                    "\0" "V" "\0"
  "ErLvlTable"                  "\0" "V" "\0"
  "ErLvls"                      "\0" "V" "\0"
  "ErLvl"                       "\0" "V" "\0"
  "ErLevelTbl"                  "\0" "V" "\0"
  "ErLevelTable"                "\0" "V" "\0"
  "ErLevels"                    "\0" "V" "\0"
  "ErLevel"                     "\0" "V" "\0"

  "CommandLineAliases"          "\0" "A" "\0"
  "CommandLineAlias"            "\0" "A" "\0"

  "CmdLineAliases"              "\0" "A" "\0"
  "CmdLineAlias"                "\0" "A" "\0"

  "Aliases"                     "\0" "A" "\0"
  "Alias"                       "\0" "A" "\0"

  ""; //End of Table


//±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±
// Program Code
//±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±

//==========================================================================
// Our Main Program Code
//==========================================================================
 int main(void)

 {
  char *EnvironmentVar; //Environment Variable Pointer
  int   HostIndex;      //Host Index Number
  int   WriteCount;     //Number of Hosts written
  int   EnvReturn;      //Environment Parse Return Value
  int   CmdLineReturn;  //Command Line Parse Return Value
  int   NumSpaces;      //Number of Spaces to write for Error Messages

//--------------------------------------------------------------------------
//Perform preliminary houskeeping setup
//--------------------------------------------------------------------------

  CopyNameToMCB();  //Make sure the PSP MCB contains our program name
  GetScreenRows();  //Get ScreenRows and Color/Mono Attributes
  ParentIsShell();  //Test whether we're at the Command line or not
  GetCmdLine();     //Copy command line tail to local string
  WriteCopyright(); //Display Copyright Message

//--------------------------------------------------------------------------
//Test environment for errors
//--------------------------------------------------------------------------

  EnvironmentVar = getenv("USBDEVIC");
  if (EnvironmentVar != NULL)                    //If Environment exists
   {
    EnvReturn = ParseCmdLine (EnvironmentVar,1); //Test Environment
    if (EnvReturn == -1)                         //If Environment Error
     {
      BeepSpeaker();                             //Beep the Speaker
      cerr << "\n"                               //Write Environment Variable
           << "Error in the Environment variable \"USBDEVIC\":\n"
           << "USBDEVIC="
           << EnvironmentVar
           << "\n";
      while ((*ErrorPtr == 0) ||                 //Go back to non-space
             (*ErrorPtr == ' '))
        ErrorPtr--;
      NumSpaces = (strlen("USBDEVIC=")) +        //Calc # of Spaces to write
                  (ErrorPtr - EnvironmentVar);
      while (NumSpaces -- >0)                    //Write the Spaces
        cerr << " ";
      cerr << "^ Error"                          //Write the Error Pointer
           << "\n\n"
           << "Reset the Environment variable (type \"SET USBDEVIC=\") and then\n"
           << "Type \"USBDEVIC ?\" for Help."
           << "\n";
      exit(ErLvlEnviron);                        //Quit program
     }
   }

//--------------------------------------------------------------------------
//Test command-line for errors
//--------------------------------------------------------------------------

  CmdLineReturn = ParseCmdLine (CommandLine,1);  //Test Command Line
  if (CmdLineReturn == -1)                        //If Error in Command-line
    {
      BeepSpeaker();                             //Beep the Speaker
      cerr << "\n"                               //Write Environment Variable
           << "Error in the command-line Options for USBDEVIC:\n"
           << "USBDEVIC "
           << CommandLine
           << "\n";
      while ((*ErrorPtr == 0) ||                 //Go back to non-space
             (*ErrorPtr == ' '))
        ErrorPtr--;
      NumSpaces = (strlen("USBDEVIC ")) +        //Calc # of Spaces to write
                  (ErrorPtr - CommandLine);
      while (NumSpaces -- >0)                    //Write the Spaces
        cerr << " ";
      cerr << "^ Error"                          //Write the Error Pointer
           << "\n\n"
           << "Type \"USBDEVIC ?\" for Help."
           << "\n";
     exit(ErLvlCmdLine);                         //Quit program
    }

//--------------------------------------------------------------------------
//Perform "housekeeping" requests that don't really have anything to do
//  with the main purpose of the program
//--------------------------------------------------------------------------

  if (DoHelp != 0) //Write Help Message, if requested
   {
    WriteHelp();
    exit(ErLvlNoError);
   }

  if (DoAlias != 0) //Write Alias Table, if Requested
   {
    WriteAliases();
    exit(ErLvlNoError);
   }

  if (DoErrorLevel != 0) //Write ErrorLevel Table, if Requested
   {
    WriteErrorLevels();
    exit(ErLvlNoError);
   }

//--------------------------------------------------------------------------
//Test for compatibility to make sure our program can work properly
//--------------------------------------------------------------------------

  Int14TestInstall(); //Test Int 14h USB Functions installed
                      //  Exits program with Error Message if not

//--------------------------------------------------------------------------
//Process environment
//--------------------------------------------------------------------------

  if ((EnvironmentVar != NULL) &&           //If valid Environment
      (EnvReturn != 0))
   {
    WriteNewLines(1);
    WriteString ("Environment: USBDEVIC=");
    WriteString (EnvironmentVar);
    WriteNewLines(1);
    ParseCmdLine (EnvironmentVar,0);        //Process Environment
    ParseCmdLine (CommandLine,0);           //Process Command Line
    WriteNewLines(1);
    return (ErLvlNoError);                  //Quit program
   }

//--------------------------------------------------------------------------
//Process command-line
//--------------------------------------------------------------------------

  if (CmdLineReturn != 0)         //If values on Cmd Line,
    ParseCmdLine (CommandLine,0); //  process them
   else                           //If no values on Cmd Line
   {
    WriteCount = 0;               //Initialize Counter
    for (HostIndex = 0; HostIndex <= 15; HostIndex++) //Do all possible Hosts
      if (WriteHostInfo(HostIndex,0) != 0)
        WriteCount++;             //If valid Host, increment Counter
    if (WriteCount == 0)          //Write Message if no Hosts were written
      WriteString("\nNo compatible USB Host Drivers were found.\n");
   }

//--------------------------------------------------------------------------
//Done
//--------------------------------------------------------------------------

  return (ErLvlNoError);          //Done
 }


//±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±
// Parse the Command Line and/or Environment Variable(s) for Options
//±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±

//==========================================================================
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
//==========================================================================
void GetCmdLine(void)

 {
  char *Pointer;            //Temporary Pointer

  movedata(_psp,0x81,_DS,(unsigned)CommandLine,127); //Copy PSP cmd line
  Pointer = CommandLine;    //Point at Command Line
  while (*Pointer++ != 13); //Replace the Carriage Return
  *(Pointer - 1) = 0;       //  with a 0
 }


//==========================================================================
// Parse the command-line for Options.
// If Error, returns -1, and Sets ErrorPtr
// Returns number of valid command-line Arguments (LPT's) processed.
//   If Return >= 1, and Testing = 0, this routine wrote the Output.
//   If Return = 0, calling routine is responsible for writing the Output.
//==========================================================================
int ParseCmdLine
     (
      char *Pointer, //Command-line string to test (could be Environment)
      int  Testing   //Parsing for errors or for real?
     )

 {
  int   HostIndex;   //Host Index
  int   Counter = 0; //Valid Switch Counter
  char  Char;        //Character from command-line

  while (SkipSpaces(&Pointer) != 0)      //Until end of Cmd Line
   {
//--------------------------------------------------------------------------
// Test for Segment:Offset (indicates end-of-line)
//--------------------------------------------------------------------------
    if (AtCommandLine == 0)              //If not at command line,
      if (GetSegOff(&Pointer,            //If Segment:Offset followed by EOL,
                    &ExecWriteSeg,
                    &ExecWriteOff) != 0)
        return(Counter);                 //  we're done
//--------------------------------------------------------------------------
// Test for Dash or Slash in front of an Option
//--------------------------------------------------------------------------
    if (SkipDashSlash(&Pointer) == 0) //Skip over Dash/Slash
     {                                //If Dash/Slash followed by EOL,
      ErrorPtr = Pointer;             //  Error
      return (-1);
     }
//--------------------------------------------------------------------------
// Get the Option
//--------------------------------------------------------------------------
    Char = GetAlias(&Pointer,AliasTbl); //Look for an alias
    if (Char == 0)                      //If no Alias,
      Char = toupper(*Pointer++);       //  just get the next character
    switch (Char)                       //Handle Options
     {
      case '?':                         //Help
        DoHelp = 1;                     //Mark flag
       break;

      case 'A':                         //Alias List
        DoAlias = 1;                    //Mark flag
       break;

      case 'N':                         //New Device Notification Table
        if (Testing == 0)               //If for real,
          WriteNewDeviceTbl();          //  Write the New Device Table
        Counter++;                      //Increment Counter
       break;

      case '0':                         //A Host Index Number
      case '1':
      case '2':
      case '3':
      case '4':
      case '5':
      case '6':
      case '7':
      case '8':
      case '9':
        HostIndex = atoi(Pointer - 1);  //Get the number
        if ((HostIndex <  0) ||         //If invalid number,
            (HostIndex > 15))
         {
          ErrorPtr = --Pointer;         //Error
          return (-1);
         }
        while ((*Pointer >= '0') &&     //Move past the number
               (*Pointer <= '9'))
          Pointer++;                    //Skip over the Number
        if (Testing == 0)               //If for real,
          WriteHostInfo(HostIndex,1);   //  Write the Host Info
        Counter++;                      //Increment Counter
       break;

      case 'V':                         //ErrorLevel Table
        DoErrorLevel = 1;               //Mark flag
       break;

      default:                          //If none of these, error
        ErrorPtr = --Pointer;
        return (-1);
     }
   }
  return (Counter);
 }


//==========================================================================
// Skip over the Spaces in a String.
// Return = 0 if end-of-line was found.
//        = 1 if not at end-of-line.
//==========================================================================
int SkipSpaces
     (
      char **Pointer //String to work with
     )

 {
  while (**Pointer == ' ') //Skip over
    (*Pointer)++;          //  Spaces
  if (**Pointer == 0)      //If end-of-string,
    return (0);            //  return error
  return (1);              //Return OK
 }


//==========================================================================
// Skip over the Dash or Slash in a String (if there), and point at the
//   next character after that.
// Return = 0 if end-of-line was found.
//        = 1 if not at end-of-line.
//==========================================================================
int SkipDashSlash
     (
      char **Pointer //String to work with
     )

 {
  if ((**Pointer == '-') ||           //If Dash
      (**Pointer == '/'))             //  or slash
   {
    (*Pointer)++;                     //Skip over it
    return (SkipSpaces(&(*Pointer))); //Look for next character
   }
  return (1);                         //Return OK
 }


//==========================================================================
// Get a Segment:Offset from the Command Line (assumed to be Hex)
// Return = 1 if Segment:Offset was found
//            (data stored)
//        = 0 if not found
//            or if anything follows Segment:Offset on the command line
//            (data unchanged)
//==========================================================================
int GetSegOff
     (char **Pointer,
      int   *Segment,
      int   *Offset
     )

 {
  char *StringPtr;                  //Our Pointer
  int   OurSegment;                 //Our Segment
  int   OurOffset;                  //Our Offset

  StringPtr = *Pointer;             //Point at the String
  if (GetHex (&StringPtr,&OurSegment) == 0) //If no Hex Number,
    return(0);                      //  Error
  if (SkipSpaces (&StringPtr) == 0) //If end-of-line,
    return(0);                      //  Error
  if (*StringPtr != ':')            //If not a colon,
    return(0);                      //  Error
  StringPtr++;                      //If a colon, move over
  if (SkipSpaces (&StringPtr) == 0) //If end-of-line,
    return(0);                      //  Error
  if (GetHex (&StringPtr,&OurOffset) == 0) //If no Hex Number,
    return(0);                      //  Error
  if (SkipSpaces (&StringPtr) != 0) //If not end-of-line,
    return(0);                      //  Error

  *Segment = OurSegment;            //Store data
  *Offset = OurOffset;
  *Pointer = StringPtr;
  return(1);                        //Done
 }


//==========================================================================
// Get a Hex Number from the command-line.
// Unlike the standard C convention, we do not want the number to start
//   with "0x", and it can optionally end with an "h".
// Return = 1 if Hex Number was found (stored in HexNumber)
//            Pointer is updated
//        = 0 if invalid number (HexNumber = unchanged)
//==========================================================================
int GetHex
     (
      char **Pointer,
      int   *HexNumber
     )

 {
  char *StringPtr;                     //Pointer we will use
  char NextChar;                       //Next character of command line
  int  GotNumeral = 0;                 //We got at least one numeral?
  int  OurNumber = 0;                  //Our Number as we're going

  StringPtr = *Pointer;                //Point at the String
  if (SkipSpaces (&StringPtr) == 0)    //If end-of-line,
    return(0);                         //  error
  do
   {
    NextChar = toupper(*StringPtr++);  //Get the next character, capitalized
    switch (NextChar)
     {
      case '0':                        //Numbers 0 - 9
      case '1':
      case '2':
      case '3':
      case '4':
      case '5':
      case '6':
      case '7':
      case '8':
      case '9':
        if ((OurNumber & 0xF000) != 0) //If this will make the number too big,
          return(0);                   //  Error
        GotNumeral++;                  //If not, we have a valid numeral
        OurNumber = (OurNumber * 16) + (NextChar - '0');
       break;

      case 'A':                        //Letters A-F
      case 'B':
      case 'C':
      case 'D':
      case 'E':
      case 'F':
        if ((OurNumber & 0xF000) != 0) //If this will make the number too big,
          return(0);                   //  Error
        GotNumeral++;                  //If not, we have a valid numeral
        OurNumber = (OurNumber * 16) + (NextChar - 'A' + 10);
       break;

      case 'H':                        //End-of-Hex String Marker
        StringPtr++;                   //Adjust StringPtr

      default:                         //If none of these
        if (GotNumeral == 0)           //If no numerals,
          return(0);                   //  Error
        NextChar = 0;                  //If OK, we're done with the number
        StringPtr--;                   //Point back at the non-hex character
     }
   } while (NextChar != 0);            //Do until end-of-string is found

  SkipSpaces (&StringPtr);             //Skip over any trailing spaces
  *Pointer = StringPtr;                //Update command-line Pointer
  *HexNumber = OurNumber;              //Store the Number
  return(1);                           //Done
 }


//==========================================================================
// Look for a matching string from a list of Aliases
// Return = 0 no match was found
//        = the character from the Alias table if a match was found
//          If found, the TestString pointer is updated
//==========================================================================
char GetAlias
      (
       char **TestString,
       char *AliasList
      )

 {
  char *StrPtr;                       //String Pointer
  char *AliasPtr;                     //Alias Pointer
  int   AliasLength;                  //Length of Alias String

  AliasPtr = AliasList;               //Initialize Alias Pointer
  while (*AliasPtr != 0)              //Until end-of-table
   {
    StrPtr = *TestString;             //Point at Test String
    AliasLength = strlen(AliasPtr);   //Store Alias String Length
    while ((*AliasPtr != 0) &&        //Until end-of-alias string
           (toupper(*AliasPtr) ==     //  and while characters
            toupper(*StrPtr)))        //  keep matching
     {
      AliasPtr++;                     //Go to
      StrPtr++;                       //  next character
     }
    if (*AliasPtr == 0)               //If the entire string matched
     {
      *TestString += AliasLength;     //Update the TestString pointer
      return(*(AliasPtr+1));          //  return the Alias character
     }
    AliasPtr += (strlen(AliasPtr)+3); //If not, point at next Alias entry
   }
  return(0);                          //If no match, return 0
 }


//±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±
// Write "housekeeping" outputs to the screen (Help, Alias, etc.)
//±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±

//==========================================================================
// Write the Help (syntax) Message to the screen
//==========================================================================
void WriteHelp(void)

 {
  char *HelpMsg =
   "\n"
   "SYNTAX: USBDEVIC [Options]\n"
   "\n"
   "  ? Í Show this HELP screen\n"
   "  A Í Show all ALIASES for these command line Options\n"
   "  V Í Show all ErrorLevels (DOS Return Codes)\n"
   "  # Í Show all devices attached to USB Host Index # (0-15)\n"
   "  N Í Show New Device Notification Table for USB Hosts\n"
   "\n"
   "You can combine more than one # (Host Index) and N (New Device Table)\n"
   "  on the same command-line.  The following, for example, is OK:\n"
   "\n"
   "  USBDEVIC 0 1 12 N\n"
   "\n"
   "If running from inside another program (if not running from the command-line),\n"
   "  the Options can be followed with a hex call-back address (Segment:Offset)\n"
   "  to which the output will be written.  See USBDEVIC.DOC for details.\n"
   "\n"
   "USBDEVIC with no Options after it will display details about the USB Devices\n"
   "  attached to all compatible USB Hosts.\n";

  WriteString (HelpMsg);
 }


//==========================================================================
// Write the entire list of Aliases to the screen
//==========================================================================
void WriteAliases(void)

 {
  void *Array[50];  //Array of Alias Pointers
  char *AliasHdr = "ALIASES FOR COMMAND-LINE OPTIONS";
  char *Pointer;    //Current Pointer to Alias Table Entry
  char TestChar;    //Current test character
  int  Index;       //Array Index
  int  LineLength;  //Length of Line (in characters)

  WriteNewLines(1);                             //Move down
  RepeatChar (' ',(79 - strlen(AliasHdr)) / 2); //Center Header Line
  WriteString (AliasHdr);                       //Write Header
  WriteNewLines(1);                             //Move down
  RepeatChar ('Í',79);                          //Write Divider Line
  WriteNewLines(2);                             //Move down

  for (TestChar = '?'; TestChar <= 'Z'; TestChar++) //For all Aliases
   {
    Pointer = AliasTbl;                         //Point at the Alias Table
    Index = 0;                                  //Initialize Index
    while (*Pointer != 0)                       //Until end-of-table
     {
      if (*(Pointer+strlen(Pointer)+1) == TestChar) //If it matches,
        Array[Index++] = Pointer;               //Store the String Pointer
      Pointer += (strlen(Pointer) + 3);         //Point at next table entry
     }
    if (Index != 0)                             //If any matches at all,
     {
      RepeatChar (TestChar,1);                  //Write the Character
      LineLength = 1;                           //Initialize Line Length
      for (Index--; Index >= 0; Index--)        //For all Aliases
       {
        if (LineLength + strlen((char *)Array[Index]) + 1 > 79)
         {                                      //If it won't fit on line
          WriteNewLines(1);                     //Move down
          RepeatChar (' ',1);                   //Move over
          LineLength = 1;                       //Reset LineLength
         }
        RepeatChar (' ',1);                     //Move over
        WriteString((char *)Array[Index]);      //Write the Alias
        LineLength += (strlen((char *)Array[Index]) + 1);
       }                                        //Adjust Line Length
      WriteNewLines(2);                         //Move down
     }
   }
 }


//==========================================================================
// Write ErrorLevel Tabe tot eh screen
//==========================================================================
void WriteErrorLevels(void)

 {
  char *ErrLvlMsg =
   "\n"
   "            ERRORLEVELS (DOS RETURN CODES)\n"
   "ÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍ\n"
   "0 No Error\n"
   "1 Bad Option in the USBDEVIC environment variable\n"
   "2 Bad Option on the USBDEVIC command-line\n"
   "3 No compatible USB Host Driver is installed in memory\n";

  WriteString (ErrLvlMsg);
 }


//±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±
// Process the main program Options
//±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±

//==========================================================================
// Write Contents of New Device Notification Table
// There is ony one table that all USB Hosts share.
//==========================================================================
void WriteNewDeviceTbl(void)

 {
  struct NewDvcNotifyStruc NewDeviceTable[NDEntries]; //Complete Table
  int    Handle;         //Table Handle (Index Counter)
  int    NumEntries;     //Number of valid table entries found
//  int    ReturnValue;    //Return value from Subroutine Calls
  int    AXRegister;     //Return AX Register
  int    BXRegister;     //Return BX Register
  int    CXRegister;     //Return CX Register
  int    DXRegister;     //Return DX Register

  WriteNewLines(1);
  WriteString ("                        NEW DEVICE NOTIFICATION TABLE                        \n");
  RepeatChar ('Í',77);
  WriteNewLines(2);
  WriteString ("       ÍÍDEVICEÍÍÍ  ÍINTERFACEÍ                                         USER \n");
  WriteString ("           Sub Pro      Sub Pro                 VENDR PROD    NOTIFY    PACKT\n");
  WriteString ("Handl  Cls Cls col  Cls Cls col  DEVICE TYPE     ID    ID     ADDRESS    ID  \n");
  WriteString ("ÍÍÍÍÍ  ÍÍÍ ÍÍÍ ÍÍÍ  ÍÍÍ ÍÍÍ ÍÍÍ ÍÍÍÍÍÍÍÍÍÍÍÍÍÍ  ÍÍÍÍÍ ÍÍÍÍÍ  ÍÍÍÍÍÍÍÍÍÍ ÍÍÍÍÍ\n");

//Copy New Device Table into our Data Area

  Int14RequestPtr->I14RRequestType = I14RRTCopyNDTable; //Request Type
  Int14RequestPtr->I14RHostIndex = 0xFF;                //Host Index
  Int14RequestPtr->I14RDataAddrOff = unsigned(&NewDeviceTable); //Data Offset
  Int14RequestPtr->I14RDataAddrSeg = _DS;               //Data Segment
  Int14RequestPtr->I14RDataSize = (128*sizeof(NewDvcNotifyStruc)); //Data Size
  Int14SendRequest                                      //Issue Request
    (&AXRegister, &BXRegister,                          //Assume no Error!
     &CXRegister, &DXRegister);

//Write the contents of the Table

  NumEntries = 0;       //Initialize Valid Entries Counter
  for (Handle = 0; Handle < NDEntries; Handle++)        //For all Entries
   {
    if (NewDeviceTable[Handle].NDCallBackSeg != 0)      //If valid entry
     {                                                  //  write the data
      NumEntries++;
      WriteDecimal (Handle,5);
      WriteDecimal ((int)NewDeviceTable[Handle].NDDvcClass,5);
      WriteDecimal ((int)NewDeviceTable[Handle].NDDvcSubClass,4);
      WriteDecimal ((int)NewDeviceTable[Handle].NDDvcProtocol,4);
      WriteDecimal ((int)NewDeviceTable[Handle].NDIntfClass,5);
      WriteDecimal ((int)NewDeviceTable[Handle].NDIntfSubClass,4);
      WriteDecimal ((int)NewDeviceTable[Handle].NDIntfProtocol,4);
      RepeatChar (' ',1);
      WriteDescription ((int)NewDeviceTable[Handle].NDDvcClass,
                        (int)NewDeviceTable[Handle].NDDvcSubClass,
                        (int)NewDeviceTable[Handle].NDDvcProtocol,
                        (int)NewDeviceTable[Handle].NDIntfClass,
                        (int)NewDeviceTable[Handle].NDIntfSubClass,
                        (int)NewDeviceTable[Handle].NDIntfProtocol
                       );
      WriteHex (NewDeviceTable[Handle].NDVendorID,2,'h');
      WriteHex (NewDeviceTable[Handle].NDProductID,1,'h');
      WriteHex (NewDeviceTable[Handle].NDCallBackSeg,2,':');
      WriteHex (NewDeviceTable[Handle].NDCallBackOff,0,'h');
      WriteHex (NewDeviceTable[Handle].NDUserPktID,1,'h');
      WriteNewLines(1);
     }
   }
  if (NumEntries == 0) //If no valid Entries
    WriteString ("#ÄÄÄ#  #Ä# #Ä# #Ä#  #Ä# #Ä# #Ä#   No Entries    #ÄÄÄ# #ÄÄÄ#  #ÄÄÄÄÄÄÄÄ# #ÄÄÄ#\n");
  WriteNewLines(1);
 }


//==========================================================================
// Write the Info for an individual USB Host
// Return = 1 if SpecificRequest = 0 and Host Index was valid
//            (we wrote something)
//        = 0 if Host Index was invalid
//==========================================================================
int WriteHostInfo
     (
      int HostIndex,      //Host Index
      int SpecificRequest //A specifically requested Host Index?
     )

 {
  int  ReturnValue;   //Return value from Subroutine Calls
  int  AXRegister;    //Return AX Register
  int  BXRegister;    //Return BX Register
  int  CXRegister;    //Return CX Register
  int  DXRegister;    //Return DX Register
  int  DeviceAddress; //Device Address
  char *MainHeader = "DEVICE ADDRESSES"; //Main Table Header

//--------------------------------------------------------------------------
//Issue Get Host Hardware Info Request
//--------------------------------------------------------------------------

  Int14RequestPtr->I14RRequestType = I14RRTGetHostHWInfo; //Request Type
  Int14RequestPtr->I14RHostIndex = HostIndex; //Host Index
  ReturnValue = Int14SendRequest              //Issue Request
                 (&AXRegister, &BXRegister,
                  &CXRegister, &DXRegister);
  if (ReturnValue != 0)                       //If Error,
   {
    if (SpecificRequest != 0)                 //  & Specific Req,
     {                                        //Write Error Msg
      WriteNewLines(1);
      RepeatChar ('Í',29);
      if (HostIndex >= 10)
        WriteString ("Í");
      WriteNewLines(1);
      WriteString ("Host Index ");
      WriteDecimal (HostIndex,1);
      WriteString (" does not exist!!");
      WriteNewLines(1);
      RepeatChar ('Í',29);
      if (HostIndex >= 10)
        WriteString ("Í");
      WriteNewLines(1);
     }
    return (0);                               //If Just Err, Quit
   }

//--------------------------------------------------------------------------
//OK - Write Header
//--------------------------------------------------------------------------

  WriteNewLines(1);
  RepeatChar (' ',((79-strlen(MainHeader))/2));             //Write Header                        //Write a
  WriteString (MainHeader);
  WriteNewLines(1);
  RepeatChar ('Í',79);
  WriteNewLines(1);
  WriteString (" Host Index: ");                            //Write Host Index
  WriteDecimal (HostIndex,2);
  WriteString ("  Host Type: ");                            //Write Host Type
  if ((BXRegister & 0xF) == USBHostTypeUHCI)
    WriteString ("UHCI");
   else
    if ((BXRegister & 0xF) == USBHostTypeOHCI)
      WriteString ("OHCI");
     else
      if ((BXRegister & 0xF) == USBHostTypeEHCI)
        WriteString("EHCI");
       else
        WriteString ("Unkn");
  WriteString ("  Bus Type: ");                             //Write Bus Type
  if ((BXRegister & USBBusTypePCI) != 0)
    WriteString ("PCI ");
   else
    WriteString ("Unkn");
  WriteString ("  IRQ#: ");                                 //Write IRQ#
  WriteDecimal ((CXRegister & 0xFF),2);
  WriteString ("  Root Hub Ports: ");                       //Write Root Ports
  WriteDecimal (((CXRegister >> 8)& 0xFF),1);

  WriteNewLines(1);                                         //Move down

  Int14RequestPtr->I14RRequestType = I14RRTGetHostVendInfo; //Request Type
  Int14SendRequest (&AXRegister, &BXRegister,               //Issue Request
                    &CXRegister, &DXRegister);
  WriteString (" Vendor:");                                 //Write Vendor
  WriteHex (BXRegister,1,'h');
  WriteString (" = ");
  WritePCIVendor (BXRegister);
  WriteString ("   Product:");                              //Write Product
  WriteHex (CXRegister,1,'h');

  WriteNewLines(1);                                         //Move down
  RepeatChar ('Í',79);                                      //Write Divider
  WriteNewLines(1);                                         //Move down

  WriteString ("                DEVICES                                   INTERFACES           \n");
  WriteString ("ÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍ  ÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍ\n");
  WriteString ("                           L                C  I A                O            \n");
  WriteString ("ADRS                       o         P      o  n l                w            \n");
  WriteString ("ÍÍÍÍ   (hex)               S         o BUS  n  t t                n            \n");
  WriteString ("Test VEND PROD     Sub Pro p USB HUB r POWR f  f I                e     Sub Pro\n");
  WriteString ("RWak  ID   ID  Cls Cls col d VER ADR t (mA) g  c n  DESCRIPTION   d Cls Cls col\n");
  WriteString ("ÍÍÍÍ ÍÍÍÍ ÍÍÍÍ ÍÍÍ ÍÍÍ ÍÍÍ Í ÍÍÍ ÍÍÍ Í ÍÍÍÍ Í  Í Í ÍÍÍÍÍÍÍÍÍÍÍÍÍÍ Í ÍÍÍ ÍÍÍ ÍÍÍ\n");


//--------------------------------------------------------------------------
//Every Host will have at least one Device:  The Root Hub at Address 1,
//  so we don't need to worry about the possibility of 0 attached Devices.
//We will not look for Address 0, which would be a Device currently in the
//  middle of the Configuration process.  Such a Device will not have
//  complete details available yet, which could cause us a lot of grief
//  while trying to display things.
//--------------------------------------------------------------------------

  for (DeviceAddress = 1; DeviceAddress <= 127; DeviceAddress++) //For All Dvcs,
    WriteDeviceInfo (HostIndex, DeviceAddress);                  //  write Info
  WriteNewLines(1);                                              //Move down

  return (1);
 }


//==========================================================================
// Write a Divider Line for the Devices Description
//==========================================================================
void WriteDivider(void)

 {
  WriteString ("ÄÄÄÄ ÄÄÄÄ ÄÄÄÄ ÄÄÄ ÄÄÄ ÄÄÄ Ä ÄÄÄ ÄÄÄ Ä ÄÄÄÄ Ä");
  WriteString ("  Ä Ä ÄÄÄÄÄÄÄÄÄÄÄÄÄÄ Ä ÄÄÄ ÄÄÄ ÄÄÄ\n");
 }


//==========================================================================
// Write the Info for A Single USB Device (Address)
// Return = 1 if Host Index and Device Address are valid
//            (we wrote something)
//        = 0 if Host Index and/or Device Address are invalid
//==========================================================================
int WriteDeviceInfo
     (
      int  HostIndex,    //Host Index
      int  DeviceAddress //Device Address
     )

 {
  int  ReturnValue;         //Return value from Subroutine Calls
  int  AXRegister;          //Return AX Register
  int  BXRegister;          //Return BX Register
  int  CXRegister;          //Return CX Register
  int  DXRegister;          //Return DX Register
  int  BadDeviceFlag;       //Bad Device Stage
  int  BadDeviceErr;        //Bad Device Error Code
  int  InterfaceNum;        //Interface Number
  int  AltInterface;        //Alternate Interface Number
  int  DeviceClass;         //Device Class
  int  DeviceSubClass;      //Device SubClass
  int  DeviceProtocol;      //Device Protocol
  int  ConfigValue;         //Current Configuration Value
  int  VendorID;            //Vendor ID
  int  NumSpaces;           //Nubmer of Spaces to write at beginnings of lines
  char *HexString = "xxxx"; //Hex String

//--------------------------------------------------------------------------
//Get Device Status
//--------------------------------------------------------------------------

  Int14RequestPtr->I14RRequestType = I14RRTGetDvcStatus;    //Request Type
  Int14RequestPtr->I14RHostIndex = (char)HostIndex;         //Host Index
  Int14RequestPtr->I14RDeviceAddress = (char)DeviceAddress; //Device Address
  ReturnValue = Int14SendRequest                            //Issue Request
                 (&AXRegister, &BXRegister,
                  &CXRegister, &DXRegister);
  if (ReturnValue != 0)                                     //If Error,
    return (0);                                             //  Quit

//--------------------------------------------------------------------------
//OK - Start Writing Status
//--------------------------------------------------------------------------

  if (DeviceAddress > 1)                                    //If not Root Hub,
    WriteDivider();                                         //  Write Divider
  WriteDecimal (DeviceAddress,3);                           //Write Address


//If being configed, Hub Address & Port & Speed are valid -- Write them here.
//  All other values are could be incorrect, and should not be written.


  if (((BXRegister >> 8) & BeingConfiged) != 0)             //If Under Config,
   {                                                        //  Write it
    WriteString ("  .... .... ... ... ... . ... ... . .... .");
    WriteString ("  . . Enumerating    . ... ... ...");
    WriteNewLines(1);                                       //Move down
    return(1);                                              //Quit
   }

  if (((BXRegister >> 8) & BadDevice) != 0)                 //If Bad Device,
   {
    BadDeviceFlag = (CXRegister & 0xFF);                    //  set BadDvcFlag
    BadDeviceErr  = DXRegister;                             //  and BadDvcErr
   }
   else                                                     //If OK,
   {
    BadDeviceFlag = 0;                                      //  BadDvcFlag = 0
    BadDeviceErr = 0;                                       //  BadDvcErr = 0
   }

  if (((BXRegister >> 8) & TestModeEnabled) != 0)           //If Test Mode,
    WriteString ("T");                                      //  Write "T"
   else
    if (((BXRegister >> 8) & RWakeupEnabled) != 0)          //If Rmt Wkup On,
      WriteString ("R");                                    //  Write "R"
     else
      if (((BXRegister >> 8) & RWakeupSupported) != 0)      //If Rmt Wkup Spt,
        WriteString ("r");                                  //  Write "r"
       else
        RepeatChar (' ',1);                                 //If none, " "
  ConfigValue = (BXRegister & 0xFF);                        //Save Config Value
  RepeatChar (' ',1);                                       //Move Over

//--------------------------------------------------------------------------
//Write Vendor & Product ID
//--------------------------------------------------------------------------

  Int14RequestPtr->I14RRequestType = I14RRTGetDvcVendInfo; //Request Type
  ReturnValue = Int14SendRequest             //Issue Request
                 (&AXRegister, &BXRegister,
                  &CXRegister, &DXRegister);

  VendorID = BXRegister;                     //Save Vendor ID
  sprintf(HexString,"%04X",VendorID);        //Write Vendor ID
  WriteString (HexString);
  RepeatChar(' ',1);
  sprintf(HexString,"%04X",CXRegister);      //Write Product ID
  WriteString (HexString);

//--------------------------------------------------------------------------
//Write Device Class Info, Speed, & USB Version
//--------------------------------------------------------------------------

  Int14RequestPtr->I14RRequestType = I14RRTGetDvcClassInfo; //Request Type
  ReturnValue = Int14SendRequest              //Issue Request
                 (&AXRegister, &BXRegister,
                  &CXRegister, &DXRegister);

  DeviceClass    =  BXRegister       & 0xFF;  //Save Dvc Class
  DeviceSubClass = (BXRegister >> 8) & 0xFF;  //Save Dvc SubCls
  DeviceProtocol =  CXRegister       & 0xFF;  //Save Dvc Protocol
  WriteDecimal (DeviceClass,4);               //Write Dvc Cls
  WriteDecimal (DeviceSubClass,4);            //Write Dvc SubCls
  WriteDecimal (DeviceProtocol,4);            //Write Dvc Procol
  RepeatChar (' ',1);                         //Move over
  if ((DXRegister & LowSpeedFlag) != 0)       //If Low Speed,
    WriteString ("Y");                        //  write "Y"
   else                                       //If Full Speed,
    WriteString (".");                        //  write "."
  WriteDecimal ((CXRegister >> 12),2);        //Write Maj Ver
  WriteString (".");                          //Write Ver Sep
  WriteDecimal (((CXRegister >> 8) & 0xF),1); //Write Min Ver

//--------------------------------------------------------------------------
//Write Device Hub Info
//--------------------------------------------------------------------------

  Int14RequestPtr->I14RRequestType = I14RRTGetDvcHubInfo; //Request Type
  ReturnValue = Int14SendRequest                  //Issue Request
                 (&AXRegister, &BXRegister,
                  &CXRegister, &DXRegister);

  if ((BXRegister & 0xFF) == 0xFF)                //If Root Hub,
    WriteString (" ... .");                       //  write dots
   else                                           //If not Root Hub,
    {
     WriteDecimal ((BXRegister & 0xFF),4);        //  write Hub Addr
     WriteDecimal (((BXRegister >> 8) & 0xFF),2); //  and Port Num
    }

//--------------------------------------------------------------------------
//Write Device Power Info
//--------------------------------------------------------------------------

  Int14RequestPtr->I14RRequestType = I14RRTGetDvcPowerInfo; //Request Type
  ReturnValue = Int14SendRequest             //Issue Request
                 (&AXRegister, &BXRegister,
                  &CXRegister, &DXRegister);

  RepeatChar (' ',1);                        //Move over
  if ((CXRegister & DvcPwrInfoSelfPwr) == 0) //If not Self Pwr,
    RepeatChar (' ',1);                      //  write space
   else                                      //If Self Pwr,
    WriteString ("s");                       //  write "s"
  WriteDecimal (BXRegister,3);               //Write Power Draw

//--------------------------------------------------------------------------
//Write Configuration Value
//--------------------------------------------------------------------------

  WriteDecimal (ConfigValue,2); //Write Config Value

//--------------------------------------------------------------------------
//Write Interface 0, Alternate Interface 0 Info
//--------------------------------------------------------------------------

  NumSpaces = 2;                                  //Write 2 spaces
  WriteAltIntfInfo (HostIndex, DeviceAddress,     //Write I0/A0 Info
                    0,
                    0,
                    DeviceClass,
                    DeviceSubClass,
                    DeviceProtocol,
                    BadDeviceFlag,
                    BadDeviceErr,
                    NumSpaces);
  RepeatChar (' ',5);                             //Move over

  if (BadDeviceFlag != 0)                         //If Bad Device,
   {
    WriteStageError (BadDeviceFlag,BadDeviceErr); //Write Err Descr
    WriteNewLines(1);                             //Move down
    return(1);                                    //Done
   }

  if (DeviceAddress == 1)                         //Write Vendor Name
    WritePCIVendor (VendorID);
   else
    WriteUSBVendor (VendorID);

//--------------------------------------------------------------------------
//Write Interface 0, Alternate Interface 1+ Info
//--------------------------------------------------------------------------

  for (AltInterface = 1; AltInterface <= 255; AltInterface++) //For all possible
   {                                       //  Alt Interfaces,
    if (WriteAltIntfInfo (HostIndex,       //Attempt to write
                          DeviceAddress,
                          0,
                          AltInterface,
                          DeviceClass,
                          DeviceSubClass,
                          DeviceProtocol,
                          BadDeviceFlag,
                          BadDeviceErr,
                          NumSpaces) != 0)
      if (NumSpaces == 2)                  //If valid, update
        NumSpaces += 45;                   //  # of Spaces
   }

//--------------------------------------------------------------------------
//Write Interface 1+ Info
//--------------------------------------------------------------------------

  for (InterfaceNum = 1; InterfaceNum <= 255; InterfaceNum++)   //For all Intf
    for (AltInterface = 0; AltInterface <= 255; AltInterface++) //For all AIntf
      if (WriteAltIntfInfo (HostIndex,       //Attempt to write
                            DeviceAddress,
                            InterfaceNum,
                            AltInterface,
                            DeviceClass,
                            DeviceSubClass,
                            DeviceProtocol,
                            BadDeviceFlag,
                            BadDeviceErr,
                            NumSpaces) != 0)
       {
        if (NumSpaces == 2)                  //If valid, update
          NumSpaces += 45;                   //  # of Spaces
       }
       else                                  //If invalid Intf/
        {                                    //  AltIntf combo
         if (AltInterface == 0)              //If invalid Intf
           InterfaceNum = 256;               //  Exit outer loop
         AltInterface = 256;                 //Exit inner loop
        }
  if (NumSpaces == 2)                        //If no Alt Intfs,
    WriteNewLines(1);                        //  move down

  return (1);
 }


//==========================================================================
// Write the Info for A Single Alternate Interface
// Return = 1 if All Input Values are valid
//            (we wrote something)
//        = 0 if input(s) are invalid
//==========================================================================
int WriteAltIntfInfo
     (
      int HostIndex,      //Host Index
      int DeviceAddress,  //Device Address
      int InterfaceNum,   //Interface Number
      int AltInterface,   //Alternate Interface Number
      int DeviceClass,    //Device Class
      int DeviceSubClass, //Device SubClass
      int DeviceProtocol, //Device Protocol
      int BadDeviceFlag,  //Device is Bad?
      int BadDeviceErr,   //If Bad, Device Error Code
      int SpacesToWrite   //Spaces to write at beginning of line
     )

 {
  int ReturnValue;  //Return value from Subroutine Calls
  int AXRegister;   //Return AX Register
  int BXRegister;   //Return BX Register
  int CXRegister;   //Return CX Register
  int DXRegister;   //Return DX Register
  int IntfClass;    //Interface Class
  int IntfSubClass; //Interface SubClass
  int IntfProtocol; //Interface Protocol

//--------------------------------------------------------------------------
//Get Interface Info
//--------------------------------------------------------------------------

  Int14RequestPtr->I14RRequestType = I14RRTGetAltIntfInfo;  //Request Type
  Int14RequestPtr->I14RHostIndex = (char)HostIndex;         //Host Index
  Int14RequestPtr->I14RDeviceAddress = (char)DeviceAddress; //Device Address
  Int14RequestPtr->I14RInterfaceNum = (char)InterfaceNum;   //Set Interface
  Int14RequestPtr->I14REndPoint = (char)AltInterface;       //Set Alt Intf
  ReturnValue = Int14SendRequest                            //Issue Request
                 (&AXRegister, &BXRegister,
                  &CXRegister, &DXRegister);
  if (ReturnValue != 0)                                     //If Error,
    return (0);                                             //  Quit

//--------------------------------------------------------------------------
//Store Interface Info
//--------------------------------------------------------------------------

  IntfClass    =  BXRegister       & 0xff; //Store Intf Cls
  IntfSubClass = (BXRegister >> 8) & 0xff; //Store Intf SubCls
  IntfProtocol =  CXRegister       & 0xff; //Store Intf Prot

//--------------------------------------------------------------------------
//Write Description
//--------------------------------------------------------------------------

  RepeatChar (' ', SpacesToWrite);                //Move Over
  if (AltInterface == 0)                          //If new Intf,
    WriteDecimal (InterfaceNum,1);                //  write Intf Num
   else                                           //If old Intf,
    RepeatChar (' ',1);                           //  write Space
  WriteDecimal (AltInterface,2);                  //Write AltIntf
  if ((CXRegister & (AltIntfSelected << 8)) != 0) //If Selected,
    WriteString ("*");                            //  write "*"
   else                                           //If Unselected,
    RepeatChar (' ',1);                           //  write " "
  if (DeviceAddress == 1)                         //If Root Hub,
    WriteString ("Root Hub      ");               //  Write it
   else
    if (BadDeviceFlag != 0)                       //If Bad Device,
     {
      WriteString ("Bad!");                       //  Write "Bad"
      WriteDecimal (BadDeviceFlag,4);             //  and NewDvcStage
      WriteHex (BadDeviceErr,1,'h');              //  and NewDvcErr
     }
     else                                         //If not Root or Bad
      WriteDescription (DeviceClass,              //  Write Descr
                        DeviceSubClass,
                        DeviceProtocol,
                        IntfClass,
                        IntfSubClass,
                        IntfProtocol);

//--------------------------------------------------------------------------
//Write rest of Interface Info
//--------------------------------------------------------------------------

  RepeatChar (' ',1);                          //Move Over
  if ((CXRegister & (AltIntfOwned << 8)) != 0) //If Owned,
    WriteString ("Y");                         //  write "Y"
   else                                        //If Unowned,
    WriteString (".");                         //  write "."
  WriteDecimal (IntfClass,4);                  //Write Intf Cls
  WriteDecimal (IntfSubClass,4);               //Write Intf SubCls
  WriteDecimal (IntfProtocol,4);               //Write Intf Prot
  WriteNewLines (1);                           //Move down

  return (1);
 }


//±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±
// Functions which need to EXEC another program
//±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±

////==========================================================================
//// Process a far call from another (EXEC'd) program.
//// This is called wiht DS:[DX] pointed at an ASCIIZ string, which
////   we need to write.  We write it ourselves instead of letting the
////   other program do it so that we can control screen pauses correctly,
////   and also to make sure the strings are written using exactly the same
////   DOS/BIOS routines (to make sure the colors are consistent).
////  This requires we use the Tiny Memory Model (CS = DS)!!
////  Also, this will only write a string a maximum of 79 characters long.
////==========================================================================
//void far _saveregs FarWrite()
//
//  {
//   char *String = "1234567890123456789012345678901234567890"
//                  "123456789012345678901234567890123456789"; //String to write
//
//   asm {  //On input to this function, DS:[DX] = String to write
//       PUSH DS; PUSH ES  //Save used registers
//       PUSH CS; POP  ES  //Point ES at our local data area
//       CLD               //Go forward with string functions
//       MOV  SI,DX        //Point DS:[SI] at the string to copy
//       MOV  DI,&String   //Point ES:[DI] at our local string
//       MOV  CX,79        //Copy 79 bytes
//       REP  MOVSB        //Copy the string into our data area
//       PUSH CS; POP  DS  //Point DS at the local data area
//    }
//   WriteString (String);
//   asm {
//       POP  ES; POP  DS  //Restore used registers
//    }
//  }


//==========================================================================
// Write a PCI Vendor Name
//==========================================================================
void WritePCIVendor
      (
       int PCIVendor //PCI Vendor
      )

 {
  char *SuptFile = "VENDORID.COM";           //Support File Name to Call

  sprintf(ExecParams,"PCI %04Xh",PCIVendor); //Complete Parameter String
  DoExec (SuptFile);                         //Do it
 }


//==========================================================================
// Write a USB Vendor Name
//==========================================================================
void WriteUSBVendor
      (
       int USBVendor //USB Vendor
      )

 {
  char *SuptFile = "VENDORID.COM";           //Support File Name to Call

  sprintf(ExecParams,"USB %04Xh",USBVendor); //Complete Parameter String
  DoExec (SuptFile);                         //Do it
 }


//==========================================================================
// Write an Error Stage and Code Description
//==========================================================================
void WriteStageError
      (
       int ErrorStage, //Error Stage
       int ErrorCode   //Error Code
      )

 {
  char *SuptFile = "USBSUPT1.COM";         //Support File Name to Call
  char  Integer[7];                        //Integer & Hex String

  WriteString ("Error at Reset/Enumeration Stage "); //Write Stage Number
  WriteDecimal (ErrorStage,1);
  WriteNewLines (1);                       //Move down
  RepeatChar (' ',5);                      //Move over
  WriteString ("Error");                   //Write Error Code
  WriteHex (ErrorCode,1,'h');
  WriteString (" = ");                     //Write Error Description
  if (ErrorStage >= 200)                   //If Stage is more than 199,
    WriteString ("Internal/Undefined");    //  it's not a USB Error Code
   else
   {
    if ((ErrorStage & 1) != 0)             //If Error Code is Odd,
      strcpy(ExecParams,"Int14ErrorCode"); //  it's an INT 14h Request Error
     else                                  //If Error Code is Even,
      strcpy(ExecParams,"TDStatusCode");   //  it's a Transfer Descriptor Error
    sprintf(Integer," %04Xh",ErrorCode);   //Append the Error Code
    strcat(ExecParams,Integer);
    DoExec (SuptFile);                     //Perform the EXEC function
   }
 }


//==========================================================================
// Write an Interface Description
//==========================================================================
void WriteDescription
      (
       int DeviceClass,    //Device Class
       int DeviceSubClass, //Device SubClass
       int DeviceProtocol, //Device Protocol
       int IntfClass,      //Interface Class
       int IntfSubClass,   //Interface SubClass
       int IntfProtocol    //Interface Protocol
      )

 {
  char  *SuptFile = "USBSUPT1.COM";        //Support File Name to Call
  char   Integer[6];                       //Integer String

  strcpy (ExecParams,"DeviceDescription"); //Complete Parameter String
  sprintf(Integer," %1d",DeviceClass);
  strcat(ExecParams,Integer);
  sprintf(Integer," %1d",DeviceSubClass);
  strcat(ExecParams,Integer);
  sprintf(Integer," %1d",DeviceProtocol);
  strcat(ExecParams,Integer);
  sprintf(Integer," %1d",IntfClass);
  strcat(ExecParams,Integer);
  sprintf(Integer," %1d",IntfSubClass);
  strcat(ExecParams,Integer);
  sprintf(Integer," %1d",IntfProtocol);
  strcat(ExecParams,Integer);
  DoExec (SuptFile);                       //Perform the EXEC function
 }


//±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±
// Functions needed to perform EXEC's to other programs
// All of these put a particular Path at the beginning of
//  the global variable ExecString.
//±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±

//==========================================================================
// Perform an EXEC call to another program
//
// NOTE: We originally tried to have Child Programs indirectly use our
//         screen-writing code (via the FarWrite routine), but cannot get
//         it to work correctly.  When calling an internal routine from
//         an external program, we need to either have CS and DS be the
//         same, or be able to know what each one is (and must be able
//         to store the value of DS in somewhere in CS, since CS is the
//         only thing that is known when internal routine is called).
//         That seems to be impossible to do with TurboCPP.
//       If we compile the program from the IDE, we can get the .EXE
//         file to generate CS = DS = ES, but the file is way too big.  If
//         we do it from the command line, the generated .EXE file is
//         much smaller, but CS != DS, and I have no idea why or if
//         there's a way to fix it.  I also cannot get TurboCPP to
//         directly generate a .COM file (where CS=DS=ES automatically),
//         even though it's supposed to be able to.  This is very
//         frustrating.  I've given up and now let the child program write
//         to the screen itself.  Having the file be small is far more
//         important to me than having the output possibly be inconsistent.
//       There are two reasons for wanting the child program to use our
//         screen-writing routine.  The first is so that we can correctly
//         control screen pauses.  Since all of the child programs we call
//         just write a few characters (never enough to generate a new line),
//         we can live with that shortcoming (even though it's not the
//         "right" way to do it).
//       The second reason is so that the screen writing appears consistent
//         (colors, speed, etc.).  Different versions of DOS are funny
//         with different methods of writing, so the output may not be
//         consistent.  TurboCPP's screen writing routines are probably
//         very different than what I use in my .ASM programs.  Hopefully,
//         this won't be a noticeable issue (but you never know with
//         Microsoft -- a command prompt under Windows XP doesn't understand
//         how the screen works very well at all).
//==========================================================================

int DoExec
     (
      char *SuptFile //Filename to call
     )

 {
  struct ffblk FFBlock;                   //File Find Block Storage
  char   Integer[6];                      //Integer String

  cout.flush();                           //Flush the screen output buffer
                                          //If we don't do this, and stdout
                                          //  is redirected, our output gets
                                          //  screwed up

  if (ExecWriteSeg != 0)                  //If being EXEC'd
   {
    sprintf(Integer," %4X",ExecWriteSeg); //  write EXEC Code Segment
    strcat(ExecParams,Integer);
    strcat(ExecParams,":");               //  write ":"
    sprintf(Integer,"%4X",ExecWriteOff);  //  write EXEC Code Offset
    strcat(ExecParams,Integer);
   }

//  if (ExecWriteSeg == 0)                  //If writing ourselves,
//    sprintf(Integer," %4X",_CS);          //  write our Code Segment
//   else                                   //If being EXEC'd,
//    sprintf(Integer," %4X",ExecWriteSeg); //  write EXEC Code Segment
//  strcat(ExecParams,Integer);
//  strcat(ExecParams,":");
//  if (ExecWriteSeg == 0)                  //If writing ourselves,
//    sprintf(Integer,"%4X",FarWrite);      //  write our Code Offset
//   else                                   //If being EXEC'd,
//    sprintf(Integer,"%4X",ExecWriteOff);  //  write EXEC Code Offset
//  strcat(ExecParams,Integer);

  if (GetOurExecPath() != 0)               //If we know where we are,
   {
    if (*(ExecString +                     //Make sure our Path
         (strlen(ExecString) - 1))         //  ends with a backslash
          != '\\')
      strcat(ExecString,"\\");
    strcat(ExecString, SuptFile);          //Add the FileName to the Path

    if (findfirst(ExecString,              //If file exists,
                  &FFBlock, 0) == 0)
     {
      spawnl(P_WAIT,                       //Execute the File
             ExecString,
             ExecString,
             ExecParams,
             NULL);
      return (0);                          //Done
     }
   }

  if (spawnlp(P_WAIT,                      //Search the PATH, starting with
             SuptFile,                     //  the current directory,
             SuptFile,                     //  and execute the file
             ExecParams,
             NULL) != 0)
   {
    WriteString (SuptFile);                //If not found,
    WriteString ("!!");                    //  Write Error
   }

  return(0);                              //Done
 }


//==========================================================================
// Get the path where our executable file (USBDEVIC.COM) is located.
//==========================================================================
int GetOurExecPath (void)

 {
  char *Pointer;        //Path Pointer
  char *EndPointer;     //End-of-Path Pointer
  unsigned int EnvSeg;  //Environment Segment
  char far *FarPointer; //Environment Pointer

  if (_osmajor < 3)                  //If DOS version < 3.00,
    return (0);                      //  Error

  movedata(_psp,PSPEnvironment,      //Copy Environment Segment Pointer
           _DS,(unsigned)&EnvSeg,    //  from PSP
           sizeof(EnvSeg));
  (long int)*(&FarPointer) =         //Put EnvSeg:0 into FarPointer
    (((long int)EnvSeg) << 16) + 0;  //NOTE: I tried to do this with MK_FP
                                     //      (the supposedly "correct" way
                                     //      to do it) but it didn't work,
                                     //      so I needed to do it in
                                     //      a roundabout way.

  while (*FarPointer != 0)           //Find end of initial environment
    while (*FarPointer++ != 0);      //  (look for double-zero)
  FarPointer += 3;                   //Skip over double zero & word after that
  movedata(FP_SEG(FarPointer),       //Copy Contents (Path where our
           FP_OFF(FarPointer),       //  executable file, USBDEVIC.EXE,
           _DS,                      //  is located) to the beginning of
           (unsigned)&ExecString,    //  ExecString
           70);

  Pointer = ExecString;              //Point at Environment Contents
  EndPointer = Pointer +
               strlen(Pointer)-1;    //Point at last character of Path
  while (( EndPointer >= Pointer) && //Go backwards to get rid of
         (*EndPointer != ':')     && //  the filename and just keep
         (*EndPointer != '\\'))      //  the Path
    EndPointer--;
  *(EndPointer + 1) = 0;             //Convert it to a string

  return (1);
 }


//±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±
// Int 14h USB Functions
//±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±

//==========================================================================
// Test to see if any compatible USB Host Controller Drivers are installed.
//   If not, quit the program with an Error Message and an ErrorLevel.
//==========================================================================
void Int14TestInstall(void)

 {
  static union REGS Regs; //CPU Registers

  Regs.x.ax = I14AXInstallCheck;   //AX = Function (Installation Check)
  Regs.x.bx = I14BXInput;          //BX = Correct Value
  Regs.x.cx = I14CXInput;          //CX = Correct Value
  int86(0x14,&Regs,&Regs);         //Do Int 17h (LPT BIOS)
  if ((Regs.x.ax != 0x0000)     || //If returned registers
      (Regs.x.bx != I14CXInput) || //  are incorrect,
      (Regs.x.cx != I14BXInput))   //  write error message
   {
    cerr << "\n"
         << "No compatible USB Host Drivers are installed in memory.\n"
         << "Install a program like USBUHCI into memory and try again."
         << "\n";
    BeepSpeaker();                 //Beep the speaker
    exit(ErLvlNoInt14);            //Quit program
   }
 }


//==========================================================================
// Send a "standard" Int 14h Request to a USB Driver
//==========================================================================
int Int14SendRequest
     (
      int *AXRegister, //CPU AX Register
      int *BXRegister, //CPU BX Register
      int *CXRegister, //CPU CX Register
      int *DXRegister  //CPU DX Register
     )

 {
  static union REGS Regs;

  Regs.x.ax = I14AXDoFunction;          //AX = Function (Do Function)
  Regs.x.bx = I14BXInput;               //BX = Correct Value
  Regs.x.cx = I14CXInput;               //CX = Correct Value
  Regs.x.dx = FP_OFF (Int14RequestPtr); //DS:[DX] = Request Structure

  int86(0x14,&Regs,&Regs); //Do Int 14h (USB Request)
  *AXRegister = Regs.x.ax; //Set return AX
  *BXRegister = Regs.x.bx; //Set return BX
  *CXRegister = Regs.x.cx; //Set return CX
  *DXRegister = Regs.x.dx; //Set return DX
  return (Regs.x.ax);      //Set return function value
 }


//±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±
// Screen-writing Functions (includes Auto-Pausing functions)
//±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±

//==========================================================================
// Calculate and store the number of screen rows,
//   and Mono/Color Screen attributes.
// If Screen Output has been redirected (e.g., INKLEVEL | MORE),
//   this sets ScreenRows = 0.
//==========================================================================
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
  int86(0x21,&Regs,&Regs);              //Do it
  if ((Regs.h.dl & (DvcInfoIsDevice+DvcInfoIsStdOut)) == //If not Redirected
                   (DvcInfoIsDevice+DvcInfoIsStdOut))
    ScreenRows = TextInfo.screenheight; //  Store ScreenRows
   else                                 //If Redirected,
    ScreenRows = 0;                     //  ScreenRows = 0
 }


//==========================================================================
// Write a String of characters to the screen, Pausing when screen is full.
// ALL Messages written to the screen that involve a new line character
//   MUST use this routine to write things, rather than using
//   printf or cout.  If a routine does not use this routine, the
//   Auto-Pause feature will not work correctly!
//
// If we are being EXEC'd by another program, and therefore issue our
//   string writes to the EXECing program, we modify our strings
//   "on the fly".  C strings are stored with end-of-line as a simple
//   LF (ASCII 10) character, but what actually gets written to the screen
//   is a CR/LF combination.  Therefore, we modify the strings appropriately
//   before we pass them on to the EXEC program.
//==========================================================================
void WriteString
      (
       char *String //String to Write
      )

 {
  char *PauseMsg    = "\n\004\004\004 MORE \004\004\004";
  char *PauseDelMsg = "\r\031\031\031 MORE \031\031\031\n";
  char *SubString   = "1234567890123456789012345678901234567890"
                      "1234567890123456789012345678901234567890"
                      "12345" ; //Substring to write
  char *SubStringBegin;

  if (ExecWriteSeg != 0)                   //If we're being EXEC'd,
   {
    while (*String != 0)                  //Until end-of-string
     {
      SubStringBegin = String;            //Remember beginning-of-string
      while ((*String != 0) &&            //Find end-of-string or
             (*String != 10))             //  end-of-line
        String++;
      if (*String == 10)                  //If end-of-line
       {
        *String = 0;                      //Copy the
        strcpy(SubString,SubStringBegin); //  Substring
        *String++ = 10;                   //Restore end-of-line marker
        strcat(SubString,"\r\n");         //Append a CR/LF
       }
       else                               //If end-of-string,
        strcpy(SubString,SubStringBegin); //Copy the SubString
      asm {
        PUSH BX; PUSH DX                  //Save used registers
        MOV  DX,&SubString                //Point DS:[DX] at the string
        XOR  BX,BX                        //Point DS:[BX] at the
        LEA  BX,[BX+&ExecWriteOff]        //  Code Address
        CALL DWORD PTR [BX]               //Call the EXEC programs Writer
        POP  DX; POP  BX                  //Restore used registers
       }
     }
   }
   else
   {
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
 }


//==========================================================================
// Write a Repeating Character to the screen.
// If Input Count = 0, this writes nothing
//==========================================================================
void RepeatChar
      (
       char Character, //Character to Write
       int  Count      //Number of Times to Write it
      )

 {
  char *String = "C";    //Character converted to a string

  String[0] = Character; //Store the character in the string
  while (Count-- != 0)   //Write the character
   WriteString(String);  //  the specified number of times
 }


//==========================================================================
// Write some NewLines to the screen.
//==========================================================================
void WriteNewLines(int NumLines)

 {
  while (NumLines-- > 0) //Do it NumLine times
    WriteString ("\n");  //  write a New Line
 }


//==========================================================================
// Write a decimal number (Integer) to the screen.
//==========================================================================
void WriteDecimal
      (
       int Number,  //Number to write
       int Width    //Width of space to take up
      )

 {
  char TempString[6]; //Temporary String
  int  Counter;       //Size counter

  sprintf(TempString,"%1d",Number);       //Write Number to string
  Counter = (Width - strlen(TempString)); //Calc number of spaces to add
  if (Counter > 0)                        //If > 0,
    RepeatChar (' ',Counter);             //Write the spaces
  WriteString (TempString);               //Write the Number
 }


//==========================================================================
// Write a hexadecimal number (Integer) to the screen.
//==========================================================================
void WriteHex
      (
       int  Number, //Number to write
       int  Spaces, //Number of Spaces to write at beginning
       char Suffix  //Suffix Character to write at end of string (usually 'h')
                    //  (if 0, write nothing)
      )

 {
  char TempString[5]; //Temporary String

  RepeatChar (' ',Spaces);           //Write the Spaces
  sprintf(TempString,"%04X",Number); //Write Number to string
  WriteString (TempString);          //Write the Number
  if (Suffix != 0)                   //Write the Suffix
   {
    sprintf(TempString,"%c",Suffix);
    WriteString(TempString);
   }
 }


//==========================================================================
// Write our Copyright message to the screen.
// Don't write anything if we're not a DOS Command Line.
//==========================================================================
void WriteCopyright(void)

 {
  if (AtCommandLine == 1) //If we're at a DOS command line,
   {                      //  write our Copyright Message
    WriteString("USBDEVIC 0.05, (C) 2008, Bret E. Johnson.\n");
    WriteString("Program to display information about Devices");
    WriteString(" attached to the USB Host(s).\n");
   }
 }


//±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±
// Miscellaneous Support Functions
//±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±

//==========================================================================
// Test and see if our Parent is a DOS Command Shell
// The test we perform is to simply check the Parent's name, to see if
//   it's the same as one of the commonly-recognized DOS Command shell
//   names (COMMAND, CMD, NDOS, etc.).  This is not a fool-proof test,
//   but will hopefully be good enough for our purposes.  I don't know
//   of any way to test and see if a program is actually a Command Shell
//   or not other than by its name.  If anybody has a better way,
//   I'd be interested in hearing about it.
//==========================================================================
void ParentIsShell (void)

 {
  char *ParentName = "ParentNm"; //Parent's Name

  // I tried to do the following with some movedata commands, but after
  //   compiling the program would always crash (as far as I can tell,
  //   the Stack was getting screwed up somehow).  So, I ended up resorting
  //   to inline Assembly.  I like Assembly better anyway.
  // So much for higher level languages being "better".

  // I'm not sure exactly which CPU registers I actually need to save,
  //   so I save all of them that I use.

  // This ASM code gets the PSP of this programs Parent (stored in the PSP),
  //   decrements it by one (to point at the Memory Control Block of the
  //   Parents PSP), and then copies the Parents Name (stored in the MCB)
  //   to our local ParentName variable.

  asm {
       PUSH AX; PUSH CX       //Save used registers
       PUSH DI; PUSH SI
       PUSH DS; PUSH ES
       PUSH DS; POP  ES       //Make sure ES points at our local data
       MOV  DS,_psp           //Point DS
       MOV  AX,[PSPParentPSP] //  at our
       DEC  AX                //  Parents
       MOV  DS,AX             //  MCB Segment
       MOV  SI,8              //Point DS:[SI] at the MCB Name
       MOV  DI,&ParentName    //Point ES:[DI] at our Parent Name Storage
       MOV  CX,4              //Copy the
       CLD                    //  Parent's Name
       REP  MOVSW             //  to our data area
       POP  ES; POP  DS       //Restore used registers
       POP  SI; POP  DI
       POP  CX; POP  AX
      }

  if ((strcmpi(ParentName,"COMMAND") == 0) || //If Parent's Name matches
      (strcmpi(ParentName,"CMD")     == 0) || //  a Command Shell Name,
      (strcmpi(ParentName,"NDOS")    == 0) ||
      (strcmpi(ParentName,"4DOS")    == 0) ||
      (strcmpi(ParentName,"4NT")     == 0) ||
      (strcmpi(ParentName,"4OS2")    == 0) ||
      (strcmpi(ParentName,"")        == 0))   //  (DOSBox parent is empty)
    AtCommandLine = 1;                        //  mark the flag as Yes
   else
    AtCommandLine = 0;                        //If not, mark the flag as No
 }


//==========================================================================
// Fill the MCB for our Program's PSP with our Program Name
// This happens automatically in DOS 4+, but does not in previous versions
//   of DOS.  We need this to be correct so that when we EXEC other programs
//   they know that they are not being called from the command line.
//==========================================================================

void CopyNameToMCB (void)

 {
  char *OurName = "USBDEVIC"; //Our Program Name, ASCIIZ

  asm {
       PUSH AX; PUSH CX       //Save used registers
       PUSH DI; PUSH SI
       PUSH ES
       MOV  AX,_psp           //Point ES:[DI]
       DEC  AX                //  at the Program name
       MOV  ES,AX             //  in the MCB
       MOV  DI,8              //  of our PSP
       MOV  SI,&OurName       //Point DS:[SI] at our program name
       MOV  CX,4              //Copy our
       CLD                    //  Program Name
       REP  MOVSW             //  to the MCB
       POP  ES                //Restore used registers
       POP  SI; POP  DI
       POP  CX; POP  AX
      }
 }


//==========================================================================
// Beep the Speaker
//==========================================================================
void BeepSpeaker(void)

 {
  sound(1000); //Beep Speaker at 1000 Hz
  delay(200);  //Hold it for 200 milliseconds
  nosound();   //Turn off the Speaker
 }
