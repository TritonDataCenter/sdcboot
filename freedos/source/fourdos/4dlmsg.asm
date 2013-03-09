

;  Permission is hereby granted, free of charge, to any person obtaining a copy
;  of this software and associated documentation files (the "Software"), to deal
;  in the Software without restriction, including without limitation the rights
;  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
;  copies of the Software, and to permit persons to whom the Software is
;  furnished to do so, subject to the following conditions:
;
;  (1) The above copyright notice and this permission notice shall be included in all
;  copies or substantial portions of the Software.
;
;  (2) The Software, or any portion of it, may not be compiled for use on any
;  operating system OTHER than FreeDOS without written permission from Rex Conn
;  <rconn@jpsoft.com>
;
;  (3) The Software, or any portion of it, may not be used in any commercial
;  product without written permission from Rex Conn <rconn@jpsoft.com>
;
;  (4) THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
;  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
;  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
;  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
;  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
;  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
;  SOFTWARE.


          ;
          ; 4DLMSG.ASM -- Include file for 4DOS.ASM, 4DLINIT.ASM, and
          ; SERVER.ASM
          ;
          ; Copyright 1989 - 1999, JP Software Inc.
          ; All Rights Reserved
          ;
          ; This message file contains all the 4DOS loader and server
          ; messages.  It is set up as a separate file for ease of
          ; translation.  Conditional assembly is used to determine
          ; which messages to assemble.
          ;
          ;
          ;         Messages for 4DOS.ASM
          ;
          if        _MsgType eq 0       ;assemble resident messages
RespTab   db        "IRAF"              ;table of allowed critical error
                                        ;  responses, in order by DOS
                                        ;  response code (0=ignore, 1=retry,
                                        ;  2=abort, 3=fail)
RespCnt   equ       $ - RespTab         ;number of allowed responses
LdError   db        BELL, CR, LF, "4DOS unrecoverable error "
ErrCode   db        "  "                ;space for code
          db        CR, LF, "$"         ;rest of message
          ;
          ;         Critical error handler messages
          ;
DevErr    db        " on device "
DevName   db        8 dup (?), 0
          endif                         ;end resident portion
          ;
          ;
          ;         Messages for 4DLINIT.ASM
          ;
          if        _MsgType eq 1       ;assemble initialization messages
          ;
          if        DEBUG ge 2          ;debugging messages
DBPrefix  db        "Debug: $"
          else                          ;different version if not debugging
DBPrefix  db        "Pt "
DBMCode   dw        ?
          db        CR, LF, "$"
          endif
          ;
INIStrOv  db        "No room for INI file name", BELL, CR, LF, "$"
TailMsg   db        "4DOS tail is:  [", 0
TailEnd   db        "]", CR, LF, "Press any key to continue ...", 0
TLongMsg  db        "Command tail too long, truncated", BELL, CR, LF, "$"
BadSwMsg  db        'Invalid startup switch "$'
BadSwIgn  db        '", ignored', BELL, CR, LF, "$"
IniNFMsg  db        "Specified INI file not found", BELL, CR, LF, "$"
BadAEFil  db        "Invalid AUTOEXEC filename"
          db        BELL, CR, LF, "$"
AELngMsg  db        "AUTOEXEC parameters too long, discarded", BELL, CR, LF, "$"
INIDMsg1  db        'Error in command-line directive "', 0
INIDMsg2  db        '":', CR, LF, "  ", 0
EMSMsg    db        "4DOS EMS swapping initialized", "$"
XMSMsg    db        "4DOS XMS swapping initialized", "$"
DiskMsg   db        "4DOS disk swapping initialized, swapping to $"
SwBad1    db        'Invalid Swapping option or path "', 0
SwBad2    db        '", ignored.', CR, LF, 0
NoneMsg   db        "4DOS loading in non-swapping mode", CR, LF, "$"
FailMsg   db        BELL, "4DOS swapping failed, loading in"
          db        " non-swapping mode", CR, LF, "$"
SizeMsg   db        " ($"               ;start of swap size
SizeBuf   db        8 dup (?)           ;room for "nnnK)<cr><lf>$"
NoDatUMB  db        "No upper memory available, low memory will be used$"
NoDatReg  db        "Region unavailable, using first available region$"
RPUText   db        " for resident portion", CR, LF, "$"
MEUText   db        " for master environment", CR, LF, "$"
GAUText   db        " for global aliases", CR, LF, "$"
GFUText   db        " for global functions", CR, LF, "$"
GDUText   db        " for global directory history", CR, LF, "$"
GHUText   db        " for global history", CR, LF, "$"
DLMsg	db	"Data segment too large, truncating alias / function / history lists", BELL, CR, LF
	db	"Press any key to continue ...$"
WinOMsg   db        "4DOS running in a virtual DOS machine "
          db        "under OS/2 version 2", CR, LF, "$"
WinEMsg   db        "4DOS running under Windows 3 in 386 "
          db        "enhanced mode", CR, LF, "$"
WinSMsg   db        "4DOS running under Windows 3 in standard or "
          db        "real mode", CR, LF, "$"
Win9XMsg  db        "4DOS running under Windows $"
Win95Msg	db	"95", CR, LF, "$"
Win98Msg	db	"98", CR, LF, "$"
WinMEMsg	db	"ME", CR, LF, "$"
W95DSMsg  db        "Primary shell uses disk swapping, settings "
	db	"will not be inherited", CR, LF, "$"
DVMsg     db        "4DOS running under DESQview", CR, LF, "$"
GDREMsg   db        "Too many SETs in CONFIG.SYS -- all CONFIG.SYS "
          db        "SETs ignored", CR, LF, "$"
InitEMsg  db        CR, LF, "4DOS initialization error -- $"
FatalMsg  db        BELL, BELL, "Fatal error -- reboot the system or restart the session"
CrLf      db        CR, LF, "$"         ;cr/lf for after error number, also
                                        ;  part of above message
          ;
          ;
          ;         Critical error handler prompts
          ;
          ; Critical error handler prompts, to be moved into critical error
          ; handler prompt data area (which one is moved depends on DOS
          ; version).  CAUTION:  The "Prompt" variable in INT24.ASM holds
          ; one of these prompts; it must be long enough to do so!  If you 
          ; modify these you may need to adjust the length of Prompt.
          ;
Prompt2   db        Pr2Len, CR, LF
          db        "R(etry), I(gnore), or A(bort)? ", 0
Pr2Len    equ       ($ - Prompt2)
Prompt3   db        Pr3Len, CR, LF
          db        "R(etry), I(gnore), F(ail), or A(bort)? ", 0
Pr3Len    equ       ($ - Prompt3)
          ;
          ;
          ; Module names for debug messages during load.  Byte before
          ; each name is debug level.
          ;
          if        DEBUG ge 2
MT_XMS    db        2, "XMS Swap$"
MT_EMS    db        2, "EMS Swap$"
MT_Disk   db        2, "Disk Swap$"
MT_RODat  db        2, "Disk Swap Reopen Data$"
MT_Reopn  db        2, "Disk Swap File Reopen$"
MT_ShDat  db        2, "Shell Data$"
MT_ShMan  db        2, "Shell Number Manager$"
MT_I2ES   db        2, "Standard Int 2E Handler$"
MT_I2EF   db        2, "Full Int 2E Handler$"
MT_Crit   db        2, "Critical Error Text$"
MT_CMIni  db        2, "COMMAND.COM Message Handler Init$"
MT_CMHdl  db        2, "COMMAND.COM Message Handler$"
MT_PErr   db        2, "Parse error table$"
MT_NName  db        2, "Netware Names$"
          endif
          ;
          endif                         ;end initialization messages
          ;
          ;
          ;         Messages for SERVER.ASM
          ;
          if        _MsgType eq 2       ;assemble server messages
          ;
SrvError  db        CR, LF, "4DOS server error -- $"
ExitMsg	db	CR, LF, "Press any key to exit ...$"
FatalMsg  db        BELL, BELL, "Fatal error -- reboot the system or restart the session"
CrLf      db        CR, LF, "$"         ;cr/lf for after error number, also
                                        ;  part of above message
          endif                         ;end server messages

