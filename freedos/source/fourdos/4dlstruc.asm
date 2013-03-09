

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
          ; 4DLSTRUC.ASM - Loader / server data structures
          ;
          ; This include file is used in 4DOS.ASM, 4DLINIT.ASM, and
          ; SERVER.ASM.
          ;
          ; Copyright 1989-1999, J.P. Software, All Rights Reserved
          ;
          ;
          ;         RegInfo:  Used by UMBREG to define regions in
          ;         upper memory.  Must match UMBREGINFO structure
          ;         in 4DOS.H
          ;
RegInfo   struc
BegReg    dw        ?                   ;region start paragraph
EndReg    dw        ?                   ;region end paragraph
MinFree   dw        ?                   ;min free space in paragraphs
FreeList  dw        ?                   ;first MCB in free list
RegInfo   ends
          ;
          ;         Swap Control Blocks:  These blocks are used by the
          ;         server code to provide a uniform structure for
          ;         controlling transfers between memory and EMS, XMS,
          ;         or disk swapping space.  There are two types of blocks:
          ;         RegSCB controls swapping for a region of memory, and
          ;         DevSCB provides the "device" information on how to access
          ;         the swap area (memory or disk).
          ;
RegSCB    struc                         ;region swapping info
DevPtr    dw        ?                   ;offset of corresponding device SCB
SwapFlag  db        ?                   ;block control flags
SwapOff   dw        ?                   ;segment offset of this block in
                                        ;  the swap area
SwapMSeg  dw        ?                   ;base memory segment of this block
SwapLen   dw        ?                   ;length of this block in paragraphs
SwapChk   dd        ?                   ;checksum
RegSCB    ends                          ;end region swapping info
          ;
          ;
DevSCB    struc                         ;device swapping info
SwapType  db        0FFh                ;swapping type, default to none
SwapHdl   dw        ?                   ;swapping handle
SwapSig   db        SIGBYTES dup (?)    ;disk signature
SwapSum   dw        ?                   ;checksum shift / add counts
DevSCB    ends                          ;end device swapping info
          ;
SwapSeg   equ       SwapSig             ;disk signature area also used for
                                        ;  EMS frame segment
SwapDrvr  equ       SwapSig             ;disk signature area also used for
                                        ;  XMS driver address
          ;
          ;
          ;         AHData:  This structure contains alias, function, & history
          ;         data used in the process of reserving space for the
          ;         alias, function, and history lists and inheriting old 
          ;         alias, function and history data.
          ;
AHData    struc                         ;alias / function / history data
AHType    db        0                   ;block type
AHPType   db        0                   ;previous shell's type
AHSeg     dw        0                   ;block segment address (absolute if
                                        ;  global, DGROUP-relative if local)
AHLenB    dw        0                   ;block length, bytes
AHLenP    dw        0                   ;block length, paragraphs
AHLocB    dw        0                   ;local storage length, bytes
AHPLenB   dw        0                   ;previous block length, bytes
AHPSeg    dw        0                   ;previous block segment addr (same
                                        ;  format as AHSeg)
AHData    ends
          ;
          ;
          ;         ShellDat:  This structure passes information between
          ;         shells.
          ;
          ;         CAUTION:  The alias, func, and history sizes below are se
          ;         to default to 0 so that 4DLINIT can use them as the "old"
          ;         size even in the primary shell.  Do not change these
          ;         defaults or the sizing code in 4DLINIT may fail in the 
          ;         primary shell!
          ;
          ;         CAUTION:  Elements of SwapAH are referenced by absolute
          ;         offsets rather than labels, due to MASM restrictions.
          ;         Check the SwDatAH and SetupAH routines in 4DLINIT.ASM if
          ;         you move either of those labels within the SwapAH
          ;         structure.
          ;
SwapAH    struc                         ;alias/function/history swap data
                                        ;  (used in ShellDat)
AHSwType  db        0                   ;block type
AHSwLen   dw        0                   ;list size in bytes
AHSwSeg   dw        0                   ;segment (absolute if global,
                                        ;  DGROUP-relative if local)
SwapAH    ends
          ;
ShellDat  struc                         ;shell data -- all defaults to 0
ShIVer    dw        INTVER              ;this shell's internal version (build)
				;  number
ShCETAdr  dd        0                   ;critical error text address
ShWMode	db	0		;this shell's windows mode, def none
DSwpType  db        0FFh                ;inherited data swap type, def. none
DSwpFlag  db        0                   ;inherited data swap flags
DSwpHdl   dw        ?                   ;inherited data swap handle
DSwpMLoc  dw        ?                   ;inherited data relative paragraph
                                        ;  address in memory, if currently
                                        ;  memory resident
DSwpLoc   dw        ?                   ;inherited data relative paragraph
                                        ;  address in swap area
DSwpLenP  dw        0                   ;total length in paragraphs
DSwpILen  dw        0                   ;INI data size in bytes
DSwpAls   SwapAH    <0, 0, 0>           ;alias swap data
DSwpFunc  SwapAH    <0, 0, 0>           ;function swap data
DSwpHist  SwapAH    <0, 0, 0>           ;history swap data
DSwpDir   SwapAH    <0, 0, 0>           ;directory history swap data
CSwpRBas  dw        ?                   ;code relocation base segment
ShINILoc  dw        2 dup (0)           ;INI file data location in memory
;SFStdErr  db        0FFh                ;system file handle for primary
;                                        ;  shell's STDERR device
ShellDat  ends                          ;shell data
          ;
          ;
          ;         ErrTable:  Gives the header structure for an error
          ;         message table.
          ;
ErrTable  struc                         ;error message table
          ife       _FLAT
BadMsg    dw        0                   ;offset of message to display when
                                        ;  error message is not found
          else
BadMsg    dd        0                   ;offset of message to display when
                                        ;  error message is not found
          endif
TransTab  db        30 dup (?)          ;decompression translate table
TxtStart  db        ?                   ;first byte of text
ErrTable  ends                          ;error message table
          ;
          ;
          ;         ModEntry:  Used internally in 4DLINIT.ASM to maintain
          ;         information on loadable modules added to resident code
          ;
ModEntry  struc                         ;module information
ModStart  dw        ?                   ;start of module in 4DLINIT
ModLen    dw        ?                   ;module length in bytes
ModPtr    dw        ?                   ;offset of pointer to module
          if        DEBUG ge 2          ;include display name if debugging
ModText   dw        ?                   ;pointer to debug text
          endif
ModFlags  db        ?                   ;module flags
ModEntry  ends                          ;module information
          ;
          ;         Macros for loadable modules
          ;
StartMod  macro     Name                ;;define start of a module
MB_&Name  equ       $                   ;;remember starting location
          endm
          ;
EndMod    macro     Name                ;;define end of a module
ML_&Name  equ       $ - MB_&Name        ;;remember length
          endm
          ;
DefMod    macro     Name, PtrName       ;;define a module table entry
          if        DEBUG ge 2          ;;include display name if debugging
Mod&Name  ModEntry  <MB_&Name, ML_&Name, PtrName, MT_&Name, 0>
          else                          ;;not debugging, no name
Mod&Name  ModEntry  <MB_&Name, ML_&Name, PtrName, 0>
          endif
ModCount  =         ModCount + 1        ;;count the table entry
          endm
          ;
          ;
          ;         IniBit:  Used internally in 4DLINIT.ASM to transfer
          ;         data between INI file choice items and internal bit
          ;         flags.
          ;
IBit      struc                         ;INI file bit flags data
IBMask    db        ?                   ;reversed flag bit mask
IBFlag    db        ?                   ;flags for this bit
IBLoc     dw        ?                   ;location of item in INIData
IBFlAddr  dw        ?                   ;flag byte address
IBit      ends                          ;INI file bit flags data
          ;
          ; Bits in IBFlag
          ;
IBREV     equ       80h                 ;reverse sense of this bit
IBNOCLR   equ       40h                 ;do not clear this bit
          ;
          ;         Macro for INI file bit data
          ;
IniBit    macro     Mask, IBFlags, FlagLoc, DLoc  ;;define bit data
          IBit      <(Not Mask), IBFlags, I_&DLoc, FlagLoc>  ;;use structure
IBitCnt   =         IBitCnt + 1         ;;count the flag
          endm
          ;
          ;
          ;         FreeItem:  Used to build a list of free blocks to
          ;         reserve during cleanup.
          ;
          ;         CAUTION:  Must be organized so that first word = zero
          ;         indicates last item; this allows a terminating zero.
          ;
FreeItem  struc                         ;item in free list
FreeSeg   dw        ?                   ;segment to free
FreeType  db        ?                   ;block type (0 = DOS, 1 = DOS UMB,
                                        ;  2 = XMS UMB)
FreeItem  ends
          ;
          ;
          ;         LRItem:  Used to reserve blocks of low memory at startup.
          ;         For reserving low-memory blocks for the environment,
          ;         global aliases, global functions, and global history.
          ;
          ;         CAUTION:  Must be organized so that first word = zero
          ;         indicates last item; this allows a terminating zero.
          ;
LRItem    struc                         ;low memory reservation data
LRRSize   dw        ?                   ;size to reserve (bytes)
LRCSize   dw        ?                   ;size to copy (bytes)
LRSource  dw        ?                   ;source segment
LRSavLoc  dw        ?                   ;offset in INI file data area where
                                        ;  we should store segment address
                                        ;  of block
LRLowLoc  dw        ?                   ;offset in low memory where we should
                                        ;  store segment address of block
                                        ;  (for inheritance)
LRItem    ends

