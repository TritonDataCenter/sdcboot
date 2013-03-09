

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
          ; LOADDATA - Loader data include file
          ;
          comment   }

          Copyright 1989, 1990, J.P. Software, All Rights Reserved

          Author:  Tom Rawson  12/10/89

          This module contains the data used by the 4DOS low-memory server
          (4DOS.ASM) and also accessed by the high-memory server interface
          code (SERVER.ASM).  It is designed as an include file for use in
          both modules.

          } end description
          ;
          ;         CAUTION:  Code in 4DLINIT may overlay the first 112 bytes
          ;         defined here with Netware Names (if they are enabled).
          ;         The code that does so is run after the stack has been
          ;         switched to the transient portion's stack and assumes
          ;         that it will be overlaying LocStack which is not in use
          ;         at the time.  Any change in the structure below could
          ;         make this assumption invalid -- alter with extreme
          ;         caution!
          ;
LocStack  db        STACKSIZ dup (?)    ;local stack
StackTop  equ       $                   ;top of local stack
          ;
ShellNum  db        ?                   ;our shell number
LMEParms  EXECPARM  <>                  ;EXEC parameter block
                                        ;  ** Following 2 bytes are PUSHed
                                        ;  together by INT 2E code and must
                                        ;  stay together!
LMFlags   db        0                   ;low-memory flags
LMFlag2   db        0                   ;low-memory flags 2
LMHandle  dw        ?                   ;local swap handle (EMS / XMS / disk)
DiskSig   db        "4DOS"              ;disk swap signature
DSigTime  db        (SIGBYTES-4) dup (0)  ;swap signature time area
OldInt    farptr    <>                  ;old interrupt 2F pointer
PSPSeg    dw        ?                   ;loader PSP segment
ExecSig   db        0                   ;EXEC active signature
ServLenW  dw        ?                   ;high memory server length (words)
ServChk   dd        ?                   ;server checksum for disk swapping
ReOpenP   farptr    <>                  ;disk swap file reopen pointer
Err24Hdl  dw        ?                   ;system input / output file handles
                                        ;  for use by INT 24 handler
          ;
          ; CAUTION:  The following three blocks are copied from one shell
          ; to another in a single copy operation, and hence must appear
          ; together and in this order.
          ;
ShellInf  ShellDat  <>                  ;local shell data
CodeReg   RegSCB    <>                  ;code swap region
CodeDev   DevSCB    <>                  ;code swap device
          ;

