

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
          ; SERVDATA - Server data include file
          ;
          comment   }

          Copyright 1989, 1990, J.P. Software, All Rights Reserved

          Author:  Tom Rawson  12/10/89

          This module contains the data used by the 4DOS high-memory server
          (SERVER.ASM) and also accessed by the low-memory initialization
          code (4DLINIT.ASM).  It is designed as an include file for use in
          both modules.

          } end description
          ;
SDStart   equ       $                   ;start point for server data
LMExePtr  dw        0                   ;EXEC code offset in loader
LMSeg     dw        0                   ;loader segment
          ;
DosMajor  db        ?                   ;DOS major version
DosMinor  db        ?                   ;DOS minor version
PrevVer   db        0                   ;4DOS version of previous shell
TranLenP  dw        ?                   ;transient total paragraph count
TRelBase  dw        ?                   ;transient relocation base seg
TCodeSeg  dw        ?                   ;transient final code seg address
TDataSeg  dw        ?                   ;transient final code seg address
INISeg    dw        ?                   ;INI file data location (== _end),
                                        ;  as a segment
FreePtr   dw        0                   ;pointer for free list
          ;
SrvPSP    dw        0                   ;server copy of current PSP
PassEnv   dw        ?                   ;passed environment segment
SavChain  dw        0                   ;saved PSP chain pointer
CritTAdr  farptr    <0,0>               ;critical error message text address
                                        ;  ** Following 2 bytes are PUSHed
                                        ;  together by INT 2E code and must
                                        ;  stay together!
SrvFlags  db        0                   ;server copy of low memory flags
SrvFlag2  db        0                   ;server copy of low memory flags 2
InitFlag  db        0                   ;server copy of initialization flags
                                        ;  ** Following 2 bytes are PUSHed
                                        ;  together by INT 2E code and must
                                        ;  stay together!
Flags     db        0                   ;server local flags
IntFlags  db        0                   ;interrupt flags
OldInt2F  farptr    <0,0>               ;saved Int 2F vector
OldInt23  farptr    <0,0>               ;saved Int 23 vector
OldInt24  farptr    <0,0>               ;saved Int 24 vector
OldInt2E  farptr    <0,0>               ;saved Int 2E vector
OldSav22  farptr    <0,0>               ;saved old INT 22 vector (PSP:0A-0D)
LocInt23  dw        0                   ;local INT 23 offset
LocInt24  farptr    <0,0>               ;local INT 24 offset and segment
SrvXDrvr  farptr    <>                  ;XMS driver address
SrvFrame  dw        0                   ;EMS page frame segment address
          ;
EnvSizeP  dw        0                   ;4DOS environment size, paragraphs
          ;
URCount   dw        ?                   ;UMB region count
URInfo    label     byte                ;UMB region table
          repeat    MAXREG              ;count regions
          RegInfo   <>                  ;define data for one region
          endm
          ;
SrvSwap   db        0                   ;4dos swap control
SwapPath  path      <>                  ;swap file path and name
Region1   RegSCB    <>                  ;info for region 1 swap
Region2   RegSCB    <>                  ;info for region 2 swap
Region3   RegSCB    <>                  ;info for region 3 swap
          ;
          ife       INSERVER            ;use regular names outside server
LocDev    DevSCB    <>                  ;local swapping device
LocCDev   DevSCB    <>                  ;local code swap device when reduced
                                        ;  swapping is used (copied from
                                        ;  previous shell)
          else                          ;use server names inside server
SLocDev   DevSCB    <>                  ;server local swapping device
SLocCDev  DevSCB    <>                  ;server local code swap device
          endif
          ;
MFreeLst  label     byte                ;list of memory blocks to free
          repeat    MAXFREE             ;count entries
          FreeItem  <0,0>               ;define data for one entry
          endm
          dw        0                   ;terminate free list
          ;
LRList    label     byte                ;list of low memory blocks to reserve
          repeat    MAXLR               ;count entries
          LRItem    <0,0,0,0>           ;define data for one entry
          endm
          dw        0                   ;terminate LR list
          ;
OutStart  db        0                   ;swap out start block
OutCnt    db        0                   ;swap out block count
InStart   db        0                   ;swap in start block
InCnt     db        0                   ;swap in block count
RelocCnt  dw        0                   ;number of relocation items
OldRBase  dw        0                   ;old relocation base
RelocTab  db        RELOCMAX dup (0)    ;relocation table
SDataLen  equ       $ - SDStart         ;server data length (bytes)


