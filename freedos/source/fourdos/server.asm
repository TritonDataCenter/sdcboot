

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


          title     SERVER - 4DOS Interface to Low-Memory Server
          ;
          page      ,132                ;make wide listing
          ;
          comment   }

          Copyright 1989, 1990, J.P. Software, All Rights Reserved

          Author:  Tom Rawson  10/28/88
                   substantial swapping mods 3/13/89
                   Entirely rewritten as high-memory interface 12/8/89
                   Substantial mods for version 4.0 12/90

          This routine provides the interface between the 4DOS low-memory 
          server and the transient portion of 4DOS.  It is designed to
          handle as much of the interface work as possible in order to
          minimize low-memory requirements.  As a result, it is coupled
          relatively tightly to the low-memory code in 4DOS.ASM.

          This code must NOT contain relocatable items which point to any
          segment other than _TEXT.  When the transient portion of 4DOS is
          moved into place by the loader all relocatable items are adjusted
          for the new location, but subsequent relocations when 4DOS is
          swapped in may adjust these items again if reduced swapping is
          used, and this will destroy the originally correct relocations
          within the server.  To access the transient data segment from
          within the server, use TDataSeg, which is properly initialized by
          the loader; do not use _DATA, which will generate relocations and
          cause trouble.

          PLEASE NOTE:  The use of segment registers in this code is very
          involved.  They must be changed at exactly the right time or 
          there will be BIG trouble!  Here's a guide to the various 
          segments used:

             Server Segment:  The segment this code is in.  Within the
             code CS always points to this segment (of course!).  Often
             DS does also, for access to local variables.  The 4DOS.COM
             code locates this segment via the variable ServSeg, stored 
             in the loader segment.

             PSP Segment:  This is the segment where the 4DOS.COM or
             4DOSxx.EXE PSP is stored.  Its address can be found in the
             variable PSPSeg, defined in LOADDATA.ASM and stored in the
             loader segment, and also in SrvPSP, defined in SERVDATA.ASM
             and stored in the server segment.  ES is used to address the 
             PSP segment.

             Resident Segment:  This is the segment which contains
             the 4DOS.COM resident code.  In general it is the same as the
             PSP segment, but if 4DOS.COM is moved to a UMB in the 640K to
             1M range on a system with XMS memory, then the two can be
             different.  Its address can be found in the variable LMSeg,
             defined in SERVDATA.ASM and stored in the server segment.  ES
             is generally used to address this segment.

             Caller's Data Segment:  This is the value of DS passed in
             on a call to one of the server routines.  It is the DGROUP
             segment address for 4DOSxx.EXE.  Its address can be found
             in the variable SaveDS, defined in this module and stored
             in the server segment.  DS is generally used to address this
             segment.

             Environment Segment:  This is the environment passed to the 
             DOS EXEC function; its address can be found in the EnvSeg 
             argument to the EXEC routine below.  DS is generally used 
             to address this segment.
             
             Master Environment Segment:  This is the 4DOS master 
             environment segment.  Its address can be found in the
             INI file data (offset I_ESeg).  ES is generally used
             to address this segment.

          Note that two stacks are used as well.  The "local" stack is the
          one used by 4DOSxx.EXE and is in the "Caller's Data Segment"
          described above.  The loader stack is used when transferring 
          control to the resident portion of 4DOS.COM, and is located in the
          loader segment as defined above.

          } end description
          ;
          ;
          ; Includes
          ;
          include   product.asm         ;product / platform definitions
          include   trmac.asm           ;general macros
          .cseg     SERV                ;set server segment if not defined
                                        ;  externally
          include   model.inc           ;Spontaneous Assembly memory models
          include   4dlparms.asm        ;loader / server parameters
          include   4dlstruc.asm        ;loader / server data structures
          include   inistruc.asm        ;INI file macros and structures
          ;
          ;
          ; Public declarations
          ;
          public    SDataLoc, SFBreak, I23Break, I2EHdlr
          public    SLocDev, SLocCDev
          public    CritTAdr
          ;
          ;
          ; Other segments accessed
          ;
_TEXT     segment   para public 'CODE'  ;transient portion text segment
          extrn     __dataseg:word, DoINT2E:far
          extrn     BreakHandler:far
_TEXT     ends
          ;
_DATA     segment   para public 'DATA'  ;transient portion data segment
          extrn     __nheap_desc:word
_DATA     ends
          ;
DGROUP    group     _DATA               ;define group
          ;
          ;
LOAD_TEXT segment   at 0                ;just an address template
          org       100h                ;start at end of PSP
          include   loaddata.asm        ;include loader data info
LOAD_TEXT ends
          ;
          ;
          .defcode  ,1                  ;set up code segment, no DGROUP
          ;
          assume    cs:@curseg, ds:nothing, es:nothing, ss:nothing
          ;
          ; External references
          ;
          ;         Externals set via Spontaneous Assembly macros
          ;
          .extrnx   ResHigh:near
          .extrnx   DosUMB:near
          .extrnx   Reloc:near
          .extrnx   ErrMsg:far
          ;
          ;         Standard externals
          ;
          extrn     SwapOne:far, CheckSum:near
          extrn     DVSet:near, DVClear:near
          extrn     ServErrs:byte

          ;
          page
          ;
          ;
          errset    ErrHandl            ;set error handler name
          ;
          ;
          ; RESIDENT DATA 
          ;
          ;
SDataLoc  equ       $                   ;server data start
INSERVER  equ       1                   ;show we are in server
          include   servdata.asm        ;common-access server data
          ;
          ;
          ;         local data
          ;
TPAAddr   dw        0                   ;address of reserved transient area
TPASize   dw        0                   ;size of reserved transient area
TmpStack  dw        ?                   ;temp stack offset in 4DOS data seg
SaveDS    dw        ?                   ;saved caller's DS
OldCS     dw        ?                   ;old CS from before swap
INIDP     farptr    <>                  ;INI file data pointer
SavStack  farptr    <>                  ;saved stack during EXEC
I2EStack  farptr    <>                  ;saved INT 2E caller's stack
I2ECall   dw        offset _TEXT:DoInt2E  ;INT 2E call offset
          dw        0                   ;INT 2E call segment
EParms    EXECPARM  <>                  ;local EXEC parameter block
SigBuf    db        SIGBYTES dup (?)    ;disk swap signature buffer
          ;
SigState  dw        SIGDISAB            ;signal state, starts disabled
SigAdr    dw        ?                   ;signal pointer
          ;
WhiteSpc  db        " ,;=+", TAB, CR, LF, 0       ;"white space" characters
WhiteLen  equ       $ - WhiteSpc                  ;  for FCB scan
          ;
          ;
          ; Include message text
          ;
_MsgType  equ       2                   ;assemble server messages
          ifdef     ENGLISH
          include   4DLMSG.ASM
          endif
          ;
          ifdef     GERMAN
          include   4DLMSG.ASD
          endif
          ;
          ;
          ; ServInit - Initialize the server
          ;
          ; On entry:
          ;     Arguments are as follows (I = IN, O = OUT):
          ;      I  char *TStkPtr       pointer to temp stack area we can use
          ;                             when desperate
          ;      I  INIFILE *InitPtr    pointer to structure for INI file
          ;                             data
          ;
          ;         The output arguments other than SwapMode are not modified
          ;         unless 4DOS was loaded in swapping mode.
          ;
          entry     ServInit,varframe,far  ;set up entry point
          ;
          argW      InitPtr             ;address for initialization data
          argW      TStkPtr             ;ptr to temp stack
          ;
          varW      TranDSeg            ;saved DS
          varend
          ;
          ;---------------------------------------------------------------
          ; Seg Registers:  DS = caller's data; ES = ???
          ;---------------------------------------------------------------
          ;
          ;---------------------------------------------------------------
          ; Stack:  local
          ;---------------------------------------------------------------
          ;
          pushm     si,di,ds            ;save registers
          cld                           ;go forward
          mov       TranDSeg,ds         ;save caller's segment
          loadseg   ds,cs               ;set DS to local segment
          assume    ds:@curseg          ;fix assumes
          ;
          ;---------------------------------------------------------------
          ; Seg Registers:  DS = server; ES = ???
          ;---------------------------------------------------------------
          ;
          ; Remember temporary stack location
          ;
          mov       ax,TStkPtr          ;point to temp stack
          mov       TmpStack,ax         ;save location
          ;
          ; Set shell running flag
          ;
          mov       es,LMSeg            ;get loader segment
          ;
          ;---------------------------------------------------------------
          ; Seg Registers:  DS = server; ES = resident
          ;---------------------------------------------------------------
          ;
          bset      es:LMFlags,INSHELL  ;show shell is running
          pop       es                  ;restore caller's segment
          ;
          ;---------------------------------------------------------------
          ; Seg Registers:  DS = server; ES = caller data
          ;---------------------------------------------------------------
          ;
          ; Save pointer to INI data
          ;
          mov       di,InitPtr          ;get initialization data address
          mov       INIDP.foff,di       ;save it locally
          mov       INIDP.fseg,es       ;save segment too
          ;
          ; Reserve any necessary blocks for master environment, global
          ; aliases, global functions, and global cmd and dir history in 
          ; low memory, and copy any passed data into those blocks.  Each 
          ; block reserved is added to the free list.
          ;
          mov       si,offset LRList    ;point to reservation list
          ;
LRLoop:   les       di,INIDP            ;point to INI data
          mov       bx,[si].LRRSize     ;get paragraphs to reserve
          or        bx,bx               ;check it
           jz       SaveMast            ;if done get out
          cmp       bx,0FFFFh           ;special master env copy to UMB?
           jne      LRAlloc             ;if not go allocate data
          mov       es,es:[di].I_ESeg   ;get master environment location
          jmp       LRCopy              ;and go copy it
          ;
LRAlloc:  calldos   ALLOC,NOMEM         ;reserve the block
          mov       bx,[si].LRSavLoc    ;get block addr location in INI data
          ;
          ;---------------------------------------------------------------
          ; Seg Registers:  DS = server; ES = INI data
          ;---------------------------------------------------------------
          ;
          mov       es:[bx][di],ax      ;store block location
          mov       bx,[si].LRLowLoc    ;get low-memory block addr location
          or        bx,bx               ;anything there?
           jz       LRAddFr             ;if not go on
          mov       es,LMSeg            ;point to low-memory segment
          ;
          ;---------------------------------------------------------------
          ; Seg Registers:  DS = server; ES = resident
          ;---------------------------------------------------------------
          ;
          mov       es:[bx],ax          ;store address down there for
                                        ;  alias / function / history inheritance
          ;
LRAddFr:  mov       bx,FreePtr          ;get free list pointer
          mov       MFreeLst[bx].FreeType,0  ;set type (DOS)
          mov       MFreeLst[bx].FreeSeg,ax  ;set block address
          add       FreePtr,(size FreeItem)  ;bump to next entry
          mov       es,ax               ;destination to ES
          ;
          ;---------------------------------------------------------------
          ; Seg Registers:  DS = server; ES = new block
          ;---------------------------------------------------------------
          ;
LRCopy:   xor       di,di               ;destination offset is zero
          mov       cx,[si].LRCSize     ;get words to copy
           jcxz     LRClear             ;if none go clear it
          pushm     si,ds               ;save DS:SI
          mov       ds,[si].LRSource    ;get source segment
          assume    ds:nothing          ;fix assumes
          ;
          ;---------------------------------------------------------------
          ; Seg Registers:  DS = source block; ES = new block
          ;---------------------------------------------------------------
          ;
          xor       si,si               ;source offset is zero
          rep       movsw               ;copy the data
          popm      ds,si               ;restore DS:SI
          assume    ds:@curseg          ;fix assumes
          ;
          ;---------------------------------------------------------------
          ; Seg Registers:  DS = server; ES = new block
          ;---------------------------------------------------------------
          ;
          jmp       LRNext              ;and go on
          ;
LRClear:  mov       wptr es:[di],0      ;store double null to clear block
          ;
LRNext:   add       si,(size LRItem)    ;skip to next item
          jmp       LRLoop              ;and go check it
          ;
          ; Save master environment address
          ;
SaveMast: les       di,INIDP            ;get INI data pointer
          ;
          ;---------------------------------------------------------------
          ; Seg Registers:  DS = server; ES = INI data
          ;---------------------------------------------------------------
          ;
          mov       ax,es:[di].I_ESeg   ;get master environment address
          push      es                  ;save ES
          mov       es,LMSeg            ;get resident segment
          ;
          ;---------------------------------------------------------------
          ; Seg Registers:  DS = server; ES = resident
          ;---------------------------------------------------------------
          ;
          mov       es:PSP_ENV,ax       ;save it in resident PSP
          test      Flags,LOADUMB       ;resident portion in UMB?
           jz       INIMast             ;no, go on
          mov       es,SrvPSP           ;get low PSP segment
          ;
          ;---------------------------------------------------------------
          ; Seg Registers:  DS = server; ES = PSP
          ;---------------------------------------------------------------
          ;
          mov       es:PSP_ENV,ax       ;save master env segment in low PSP
          ;
INIMast:  pop       es                  ;restore ES
          test      SrvFlags,ROOTFLAG   ;are we the root shell?
           jz       SIResTPA            ;if not go on
          mov       es:[di].I_MSeg,ax   ;save this segment as global master
          ;
          ; Reserve the TPA, then set up the exec parameter block
          ;
SIResTPA: call      ResTPA              ;reserve the TPA if necessary
          mov       bx,SrvPSP           ;get PSP segment
          mov       EParms.EXEC_TSG,bx  ;tail segment is PSP
          mov       EParms.EXEC_TOF,PSP_TLEN  ;tail offset is tail in PSP
          ;
          ; Set up so we are notified if DV closes our window
          ;
          test      SrvFlag2,DVCLEAN    ;DV cleanup enabled?
           jz       InitDone            ;if not go on
          call      DVSet               ;set up notification
          ;
InitDone: popm      di,si               ;restore registers
          mov       ds,TranDSeg         ;restore DS
          ;
          ;---------------------------------------------------------------
          ; Seg Registers:  DS = caller's data; ES = ???
          ;---------------------------------------------------------------
          ;
          assume    ds:nothing          ;fix assumes
          exit                          ;all done
          ;
          ;
          ; ServExec - EXEC an external program via the loader
          ;
          entry     ServExec,varframe,far  ;set up entry point
          ;
          argW      ErrNum              ;error number pointer
          argW      ChgTitle            ;change OS/2 session title?
          argW      EnvSeg              ;far pointer to environment
          argW      CmdTail             ;command tail pointer
          argW      ProgName            ;program name pointer
          ;
          var       OldName,PATHBLEN    ;buffer for old name in OS/2
          varend
          ;
          ;---------------------------------------------------------------
          ; Seg Registers:  DS = caller's data; ES = ???
          ;---------------------------------------------------------------
          ;
          ;---------------------------------------------------------------
          ; Stack:  local
          ;---------------------------------------------------------------
          ;
          ; Save registers, point to PSP
          ;
          pushm     si,di               ;save registers
          mov       es,cs:SrvPSP        ;get PSP segment
          ;
          ;---------------------------------------------------------------
          ; Seg Registers:  DS = caller's data; ES = PSP
          ;---------------------------------------------------------------
          ;
          ; Copy command tail
          ;
          mov       si,CmdTail          ;get command tail address
          mov       di,PSP_TLEN         ;point to local command tail area
          mov       cx,80h              ;move whole tail
          rep       movsb               ;copy tail
          ;
          ; Save caller's data segment, set local data segment
          ;
          call      SetDS               ;set DS and ES
          assume    ds:@curseg          ;fix assumes
          ;
          ;---------------------------------------------------------------
          ; Seg Registers:  DS = server; ES = resident
          ;---------------------------------------------------------------
          ;
          ; Set up parameters for DOS EXEC call
          ;
ExecSet:  mov       ax,EnvSeg           ;get environment segment
          mov       EParms.EXEC_ENV,ax  ;store in parameter block
          mov       es,SrvPSP           ;get PSP segment in es
          ;
          ;---------------------------------------------------------------
          ; Seg Registers:  DS = server; ES = PSP
          ;---------------------------------------------------------------
          ;
          mov       EParms.EXEC_F1S,es  ;set fcb1 segment
          mov       EParms.EXEC_F2S,es  ;set fcb2 segment
          ;
          ; Fill the FCBs by using int 21 function 29h (parse filename)
          ; If the first FCB parse does not return with DS:SI pointing to
          ; white space, we have to skip characters until we find some.
          ;
          push      ds                  ;save ds
          mov       ds,SaveDS           ;get back caller's segment
          assume    ds:nothing          ;fix assumes
          ;
          ;---------------------------------------------------------------
          ; Seg Registers:  DS = caller's data; ES = PSP
          ;---------------------------------------------------------------
          ;
          mov       si,CmdTail          ;point to command tail
          inc       si                  ;skip the tail length byte
          mov       di,PSP_FCB1         ;get fcb1 offset
          mov       cs:EParms.EXEC_F1O,di  ;store in parameter block
          mov       al,1                ;skip leading separators in parse
          calldos   PARSE               ;parse first file name
          push      es                  ;save ES
          loadseg   es,cs               ;set ES for scan
          cld                           ;go forward
          ;
          ;---------------------------------------------------------------
          ; Seg Registers:  DS = caller's data; ES = server
          ;---------------------------------------------------------------
          ;
          ; Skip until we find some white space between the arguments
          ;
FCBScan:  lodsb                         ;get a character from tail
          mov       di,offset WhiteSpc  ;point to white space table
          mov       cx,WhiteLen         ;get table length
          repne     scasb               ;look for white space
          jne       FCBScan             ;none found, keep trying
          pop       es                  ;restore es
          ;
          ;---------------------------------------------------------------
          ; Seg Registers:  DS = caller's data; ES = PSP
          ;---------------------------------------------------------------
          ;
FCBRdy:   mov       di,PSP_FCB2         ;get fcb2 offset
          mov       cs:EParms.EXEC_F2O,di  ;store in parameter block
          mov       al,1                ;skip leading separators in parse
          calldos   PARSE               ;parse second file name
          pop       ds                  ;restore ds
          assume    ds:@curseg          ;fix assumes
          ;
          ;---------------------------------------------------------------
          ; Seg Registers:  DS = server; ES = PSP
          ;---------------------------------------------------------------
          ;
          ; Turn off DESQView close window notification
          ;
          test      SrvFlag2,DVCLEAN    ;DV cleanup enabled?
           jz       GetOS2Nm            ;if not go on
          call      DVClear             ;clear notification
          ;
          ; If we are under OS/2 2.0, enable name access and get our 
          ; session name
          ;
GetOS2Nm: cmp       wptr ChgTitle,0     ;should we change the title?
           je       MoveParm            ;if not go on
          xor       dx,dx               ;enable change on EXEC ?? OS/2 
                                        ;  COMMAND.COM does this and code
                                        ;  doesn't work without a DX=0 call
          call      OS2TAcc             ;call OS/2 title access function
          mov       dx,2                ;get our current title
          call      OS2Title            ;get title into buffer
          bset      SrvFlag2,RESTITLE   ;set flag to restore OS/2 title?
          ;
          ; Move EXEC parameters to loader area, then switch stacks
          ;
MoveParm: mov       es,LMSeg            ;set ES
          ;
          ;---------------------------------------------------------------
          ; Seg Registers:  DS = server; ES = resident
          ;---------------------------------------------------------------
          ;
          push      bp                  ;preserve frame pointer
          mov       si,offset EParms    ;point to local EXEC parms
          mov       di,offset LMEParms  ;point to loader EXEC parms
          mov       cx,size EParms      ;get bytes to move
          rep       movsb               ;move EXEC parameters to loader
          mov       dx,ProgName         ;address of file name for EXEC
          push      bp                  ;preserve BP
          call      LowStack            ;switch to loader stack
          ;
          ;---------------------------------------------------------------
          ; Stack:  loader
          ;---------------------------------------------------------------
          ;
          ; Do the swap out
          ;
          push      dx                  ;save file name address
          call      SwapOut             ;do the swap
          pop       dx                  ;get back file name address
          ;
          ; Now call the loader to execute the program
          ;
          cld                           ;DOS 2.x may need this due to a bug
          cli                           ;hold interrupts until INT 21
          bclr      es:LMFlags,INSHELL+BRKOCCUR+IGNORE23  ;show external prog
                                        ;  running, no ^C, allow new ^C
          bset      es:LMFlag2,SWAPPED  ;show we're swapped out
          mov       ax,(D_EXEC shl 8)   ;get EXEC, subfunction 0
          mov       bx,offset LMEParms  ;point to parameter block
          mov       ds,SaveDS           ;change back to caller's DS
          assume    ds:nothing          ;fix assumes
          mov       es:ExecSig,EXECACT  ;set loader's EXEC active flag
          ;
          ;---------------------------------------------------------------
          ; Seg Registers:  DS = caller's data; ES = resident
          ;---------------------------------------------------------------
          ;
; FIXME!
          call      cs:dword ptr LMExePtr   ;call low-memory EXEC code
          ;
          ;---------------------------------------------------------------
          ; Seg Registers:  DS = ???; ES = ???
          ;---------------------------------------------------------------
          ;
          ;
          ; On return from the loader we can assume that the carry flag
          ; and AX registers are as returned from EXEC, the server segment
          ; has been swapped in, and we are running on the loader stack.
          ;
          ; CAUTION:  Flags and AX must not be disturbed until results are
          ; calculated below!
          ;
          loadseg   ds,cs               ;restore ds to point to local segment
          ;
          ;---------------------------------------------------------------
          ; Seg Registers:  DS = server; ES = ???
          ;---------------------------------------------------------------
          ;
          mov       es,LMSeg            ;get loader segment
          ;
          ;---------------------------------------------------------------
          ; Seg Registers:  DS = server; ES = resident
          ;---------------------------------------------------------------
          ;
          ; Calculate result codes based on EXEC returns
          ;
          assume    ds:@curseg          ;fix assumes
          pushf                         ;save flags
          calldos   GETRET              ;get return code in AH
          popf                          ;restore flags
          mov       bl,0                ;clear result code
          jc        ExecErr             ;if any error go on
          test      es:LMFlags,BRKOCCUR  ;was ^C hit?
          jnz       ExecBrk             ;if so handle that
          cmp       ah,1                ;ctrl-C?
          jne       ExecPut             ;if not go store error level
          ;
ExecBrk:  mov       bl,CTRLCVAL         ;set DOS error value for ^C
          jmp       short ExecPut       ;go store codes
          ;
ExecErr:  dec       bl                  ;error, set result to -1
          ;
ExecPut:  push      ax                  ;save DOS error number
          mov       al,bl               ;get our result code
          cbw                           ;make it a word
          push      ax                  ;save that
          ;
          ; Swap 4DOS back in
          ;
          call      SwapIn              ;do the swap
          ;
          ; Turn DESQView close window notification back on
          ;
          test      SrvFlag2,DVCLEAN    ;DV cleanup enabled?
           jz       Get24               ;if not go on
          call      DVSet               ;set notification
          ;
          ; Get the INT24 address and save it locally, in case a TSR just
          ; took it over and we need to restore it later without disabling
          ; the TSR.
          ;
Get24:    mov       al,24h              ;get interrupt number
          calldos   GETINT              ;get the vector
          dstore    LocInt24,bx,es      ;save the current value
          ;
          ;---------------------------------------------------------------
          ; Seg Registers:  DS = server; ES = ???
          ;---------------------------------------------------------------
          ;
          ; Restore the result codes and the local stack
          ;
          pop       ax                  ;restore our result
          pop       dx                  ;restore DOS error number
          cli                           ;hold interrupts
          mov       ss,SavStack.fseg    ;restore ss
          mov       sp,SavStack.foff    ;restore sp
          sti                           ;allow interrupts
          pop       bp                  ;restore BP
          ;
          ; CAUTION:  AX and DX must be preserved as they hold return codes
          ; at this point
          ;
          ;---------------------------------------------------------------
          ; Stack:  local
          ;---------------------------------------------------------------
          ;
          ; If we are under OS/2 2.0, restore our session title
          ;
          call      OS2TRest            ;restore the title
          ;
          ; Restore the frame pointer and return to 4DOS
          ;
          pop       bp                  ;restore the frame pointer
          mov       ds,TDataSeg         ;point back to DGROUP
          assume    ds:nothing          ;fix assumes
          ;
          ;---------------------------------------------------------------
          ; Seg Registers:  DS = caller's data; ES = ???
          ;---------------------------------------------------------------
          ;
          mov       bx,ErrNum           ;get dos error number address
          mov       [bx],dx             ;store DOS error code for caller
          ;
          popm      di,si               ;restore registers
          ;
          ; Code below substitutes for EXIT macro to allow fiddling with
          ; return address
          ;
          mov       sp,bp               ;restore SP
          pop       bp                  ;restore BP
          pop       bx                  ;get back return offset
          pop       cx                  ;and return segment
          push      cs:TCodeSeg         ;put new code segment on stack
          push      bx                  ;and offset
          retf      $$arg - 6           ;and return
          ;
ServExec  endp                          ;that's it
          ;
          ;
          ; ServCtrl -- Server control routine
          ;
          ; On entry:
          ;     Arguments on stack:
          ;         unsigned int SCFunc -- ServCtrl function:
          ;                   0  quit (data = exit code)
          ;                   1  swapping on (data=1) / off (data=0) /
          ;                      status report (data = -1), returns 0 if
          ;                      OK, 1 if error, or returns status
          ;                   2  auto-fail on (data=1) / off (data=0),
          ;                      returns previous value
          ;                   3  free (data=0) / reserve (data=1) the TPA
          ;                   4  console device change notification
          ;                      (captures new INT 24 device)
          ;                   5  return space reserved by transient in
          ;                      paragraphs
          ;                   6  signal handler control (data = SIGENAB to
          ;                      enable, SIGDISAB to disable, or signal
          ;                      handler address)
          ;
          ;         unsigned int SCData -- data for specified function
          ;
          ; On exit:
          ;         AX = return from function
          ;         DS, SI, DI preserved
          ;         Other registers destroyed, interrupt state unchanged
          ;         (except interrupts enabled by signal handler call)
          ;
          ; Call table used for ServCtrl functions:
          ;
SCtrlTab  dw        SCQuit              ;0 = quit
          dw        SCSwap              ;1 = swapping
          dw        SCAFail             ;2 = auto-fail
          dw        SCTPA               ;3 = TPA control
          dw        SCCTTY              ;4 = console device changed
          dw        SCAvail             ;5 = available space
          dw        SCUReg              ;6 = UMB region info
          dw        SCSignal            ;7 = signal handler control
SCMAX     equ       ($ - SCtrlTab - 2) / 2
          ;
          ;
          entry     ServCtrl,argframe,far  ;set up entry point
          ;
          argW      SCData              ;control data
          argW      SCFunc              ;function code
          ;
          call      SetDS               ;set up DS / ES
          assume    es:LOAD_TEXT        ;fix assumes
          mov       ax,SCData           ;get data
          mov       bx,SCFunc           ;get function
          cmp       bx,SCMAX            ;too big?
           ja       SCDone              ;if so get out
          shl       bx,1                ;make it a word offset
          jmp       wptr SCtrlTab[bx]   ;branch to the proper function
          ;
          ;
          ; SCQuit - Clean up any swapping and exit via loader.
          ;
SCQuit:   cmp       DosMajor,20         ;OS/2 2.0 DOS box?
          jae       QClean              ;if so allow exit from root shell
          test      SrvFlags,ROOTFLAG   ;is this the root shell?
          errne     QROOT               ;if so, error
          ;
QClean:   push      ax                  ;save exit code
          call      SrvClean            ;clean everything up
          pop       ax                  ;get exit code
          call      DoQuit              ;that's it, this never returns
          ;
          ;
          ; SCSwap -- turn swapping on/off
          ;
SCSwap:   test      Flags,TRANSHI       ;are we loaded high?
           jz       SwapErr             ;no, call is no good
          or        ax,ax               ;check data value
           jl       SwapInq             ;if < 0, it's inquiry only
           jne      SwapOn              ;if non-zero turn swapping on
          test      es:LMFlags,SWAPENAB  ;is it already off?
           jz       SCDone              ;if so skip it
          bclr      es:LMFlags,SWAPENAB ;zero -- turn off
          mov       bx,INISeg           ;get INI file data segment
          mov       es:ShellInf.DSwpMLoc,bx  ;set for secondary shells
          call      FreeTPA             ;free TPA
          mov       ax,TranLenP         ;get length of high area
          inc       ax                  ;bump for extra paragraph at end
          mov       dx,cs               ;get local segment 
          mov       bx,dx               ;copy as requested segment
          call      ResHigh             ;reserve transient 4DOS memory
           jc       SwResErr            ;if error complain
          cmp       ax,dx               ;does it match?
           jne      SwResErr            ;if not, complain
          call      ResTPA              ;and re-reserve smaller TPA
          jmp       short SwapGood      ;all done
          ;
SwResErr: call      ResTPA              ;re-reserve TPA
          jmp       short SwapErr       ;and return error
          ;
SwapOn:   test      es:LMFlags,SWAPENAB  ;is it already on?
          jnz       SCDone              ;if so skip it
          bset      es:LMFlags,SWAPENAB  ;set swap flag
          mov       es:ShellInf.DSwpMLoc,0  ;clear mem res inheritance
          call      FreeTPA             ;free TPA
          loadseg   es,cs               ;get local segment address
          calldos   FREE                ;free transient 4DOS memory
          pushf                         ;save result
          call      ResTPA              ;re-reserve TPA at full size
          popf                          ;get back result of DOS call
          jc        SwapErr             ;holler if it didn't work
          jmp       short SwapGood      ;all done
          ;
SwapInq:  test      es:LMFlags,SWAPENAB  ;check flag
           jz       SwapGood            ;if swapping off go return zero
                                        ;if swapping on, drop through and
                                        ;  return 1
          ;
SwapErr:  mov       ax,1                ;error, return non-zero
          jmp       SCDone              ;all done
          ;
SwapGood: xor       ax,ax               ;all OK, result is 0
          jmp       SCDone              ;all done
          ;
          ;
          ; SCAFail - Flip the INT 24 AutoFail bit (returns previous value)
          ;
SCAFail:  mov       dl,es:LMFlag2       ;get old flags
          or        ax,ax               ;set or clear?
           jnz      SAFSet              ;if set go on
          bclr      es:LMFlag2,AUTOFAIL ;clear auto fail
          jmp       SAFDone             ;and get out
          ;
SAFSet:   bset      es:LMFlag2,AUTOFAIL ;set auto fail
          ;
SAFDone:  mov       al,dl               ;copy old value
          and       al,AUTOFAIL         ;isolate old bit
          xor       ah,ah               ;clear high byte
          jmp       SCDone              ;and get out
          ;
          ;
          ; SCCTTY - Console device changed, capture new INT 24
          ;          error I/O handles
          ;
SCCTTY:   push      es                  ;save resident segment
          mov       es,es:PSPSeg        ;get PSP segment
          mov       al,es:[PSP_FTAB+STDIN]  ;get SFT handle for STDIN
          mov       ah,es:[PSP_FTAB+STDERR]  ;get SFT handle for STDERR
          pop       es                  ;restore resident segment
          mov       es:Err24Hdl,ax      ;save new INT 24 in/out handles
          jmp       SCDone              ;and get out
          ;
          ;
          ; SCAvail - Get amount of space temporarily reserved by the
          ;           transient code
          ;
SCAvail:  test      Flags,TPARES        ;TPA reserved?
           jnz      SATPASz             ;if so go get size
          mov       bx,0FFFFh           ;get max value
          calldos   ALLOC               ;get available RAM
          mov       ax,bx               ;copy to AX
          jmp       SCDone              ;and go on
          ;
SATPASz:  call      FreeTPA             ;free the TPA
          call      ResTPA              ;reserve it again (cleans things up)
          mov       ax,TPASize          ;get TPA size
          inc       ax                  ;bump for extra MCB
          jmp       SCDone              ;and go on
          ;
          ;
          ; SCTPA - Free or reserve the transient program area
          ;
SCTPA:    or        ax,ax               ;what shall we do?
           jne      STNFree             ;if reserve or reset go do that
          call      FreeTPA             ;free the TPA
          jmp       SCDone              ;and get out
          ;
STNFree:  jg        STReserv            ;if > 0 just reserve
          call      FreeTPA             ;if < 0, free then reserve
          ;
STReserv: call      ResTPA              ;reserve the TPA
          jmp       SCDone              ;and get out
          ;
          ;
          ; SCUReg - Return UMB region data addresses
          ;
SCUReg:	mov	bx,ax		;copy caller's pointer address
	mov	ax,URCount	;get count for return
	lea	dx,URInfo		;get data structure offset
	mov	ds,SaveDS		;get back caller's DS
	dstore	[bx],dx,cs	;store far pointer to data structure
	jmp	SCDone		;all done
          ;
          ;
          ; SCSignal - Implements a ^C handler similar to signal()
          ;
          ;         Argument should be SIGENAB to enable or a greater
          ;         value to disable (these two values should be very high
          ;         so they can't conflict with actual function pointers).
          ;         If the actual value is below SIGENAB it is saved as the
          ;         new function pointer and signals are left DISabled.
          ;
SCSignal: cli                           ;hold interrupts
          cmp       ax,SIGENAB          ;check function ptr against enable
           jae      SigSet              ;if enable or disable go save it
          mov       cs:SigAdr,ax        ;not enable or disable -- must be new
                                        ;  address -- save it
          mov       ax,SIGDISAB         ;set up for disable
          ;
SigSet:   mov       cs:SigState,ax      ;set new signal state
          sti                           ;allow interrupts
          ;
          ;
SCDone:   mov       ds,SaveDS           ;restore DS
          exit                          ;that's all
          ;
          ;
          ; ServTtl - Return OS/2 session title
          ;
          entry     ServTtl,argframe,far  ;set up entry point
          ;
          argD      TitleBuf            ;buffer address
          ;
          pushm     si,di,ds            ;save registers
          assume    ds:nothing	;fix assumes
          les       di,TitleBuf         ;get buffer offset
          mov       dx,2                ;get our current title
          call      OS2Tacc             ;get title into buffer
          popm      ds,di,si            ;restore registers
          exit                          ;that's all
          ;
          ;
          ; SwapOut - Swap 4DOS out (called by ServExec and the INT 2E
          ;           handler)
          ;
          ; On entry:
          ;         DS = server data segment
          ;         ES = resident data segment
          ;
          ; On exit:
          ;         DS, ES preserved
          ;         All other registers destroyed
          ;         Interrupt state unchanged
          ;
          entry     SwapOut,noframe,,local    ;swap 4DOS out
          assume    ds:@curseg          ;set assumes
          ;
          mov       OldCS,cs            ;save our CS for adjustments later
          ;
          ; Check if our swap file has been closed and reopen it if necessary
          ;
          test      SrvFlag2,DREOPEN    ;reopen requested?
           jz       DoSwOut             ;if not go on
          call      dword ptr es:ReopenP  ;call the reopen code
          mov       SLocDev.SwapHdl,bx  ;store (possibly new) handle
          ;
          ; Swap out transient portion if it is loaded high and swapping
          ; is enabled
          ;
DoSwOut:  call      FreeTPA             ;free TPA if it's reserved
          test      es:LMFlags,SWAPENAB  ;swapping enabled? (won't be set
                                         ;  unless we are loaded high)
           jz       SODone              ;if not go on
          ;
          ; Calculate the checksums to be used for swap in.  Note that when
          ; calling the swapper we don't set up ES (PSP segment) as it is
          ; not used by the disk swap code in a checksum-only operation.
          ;
          mov       dl,2                ;set checksum value
          mov       cx,3                ;handle three regions
          mov       si,offset Region1   ;point to first region
          ;
SumCalc:  test      [si].SwapFlag,CHKCALC  ;checksumming this region?
           jz       NextChk             ;if not go on
          call      CSwpOne             ;call the swapper to do the checksum
          ;
NextChk:  add       si,(size RegSCB)    ;move to next region
          loop      SumCalc             ;and go on
          ;
          ; Do the swap out, then calculate server checksum
          ;
          bset      es:LMFlags,IGNORE23 ;ignore ^Break during swap out
          xor       dl,dl               ;set for swap out
          call      DoSwap              ;swap out
          mov       ax,cs               ;get start of this segment
          mov       di,es:ServLenW      ;get server length
          add       di,7                ;add for roundoff
          shrn      di,3                ;make it paragraphs
          mov       cx,SUMALL           ;set shift / add counts to sum all
                                        ;  words in server region
          xor       bx,bx               ;clear low checksum
          xor       dx,dx               ;clear high checksum
          call      CheckSum            ;calculate server checksum
          dstore    <wptr es:ServChk>,bx,dx  ;store checksum
          ;
SODone:   exit                          ;swap out is done
          ;
          ;
          ; SwapIn - Swap 4DOS in (called by ServExec and the INT 2E
          ;          handler)
          ;
          ; On entry:
          ;         DS = server data segment
          ;         ES = resident data segment
          ;
          ; On exit:
          ;         DS, ES preserved
          ;         All other registers destroyed
          ;         Interrupt state unchanged
          ;
          entry     SwapIn,noframe,,local  ;swap 4DOS in
          assume    ds:@curseg          ;set assumes
          ;
          ; Adjust segments in case the transient portion has been moved
          ;
          mov       dx,cs               ;get code segment
          sub       dx,OldCS            ;get adjustment amount
          add       TCodeSeg,dx         ;adjust code segment
          add       TDataSeg,dx         ;adjust data segment
          add       TRelBase,dx         ;adjust relocation base
          add       SavStack.fseg,dx    ;adjust stack segment
          add       INIDP.fseg,dx       ;adjust INI file data segment
          ;
          ; Reserve the TPA if we're loaded high, then swap the rest of
          ; 4DOS back in if swapping is enabled (DX from above must be
          ; preserved)
          ;
          test      Flags,TRANSHI       ;are we loaded high?
           jz       ClrSwap             ;if not just clear flags
          call      ResTPA              ;reserve TPA if necessary
          test      es:LMFlags,SWAPENAB  ;swapping enabled?
           jz       ClrSwap             ;if not just clear flags
          mov       ax,es:LMHandle      ;get swap handle from resident code
                                        ;  (may have changed if swap file was
                                        ;  reopened)
          mov       SLocDev.SwapHdl,ax  ;store for local use
          bclr      SrvFlag2,NORELOC    ;clear no relocation flag
          push      dx                  ;save segment adjustment
          mov       dl,1                ;set for swap in
          call      DoSwap              ;swap in
          pop       dx                  ;restore segment adjustment
          ;
          ; Adjust internal data segment pointers and 
          ;   alias / function / history pointers
          ;
          push      es                  ;save ES
          mov       es,TCodeSeg         ;get new code segment
          assume    es:_TEXT            ;fix assumes
          mov       ax,TDataSeg         ;get new data segment
          mov       es:__dataseg,ax     ;set global DS pointer (don't adjust,
                                        ;  just set -- because it's in code
                                        ;  seg which is swapped out before
                                        ;  this value is set, so it is wrong
                                        ;  after swap in)
          mov       es,ax               ;now change to data seg
          assume    es:_DATA            ;fix assumes
          add       es:__nheap_desc,dx  ;adjust seg in near heap descriptor
          ;
          les       di,INIDP            ;point to INI data
          assume    es:nothing          ;fix assumes
          cmp       es:[di].I_LocalA,0  ;local aliases?
           je       SIChkFunc           ;if not go on
          add       wptr es:[di].I_ALoc+2,dx  ;adjust alias pointer
          ;
SIChkFunc: cmp       es:[di].I_LocalF,0  ;local functions?
           je       SIChkHst            ;if not go on
          add       wptr es:[di].I_FLoc+2,dx  ;adjust function pointer

SIChkHst: cmp       es:[di].I_LocalH,0  ;local history?
           je       SIChkDir            ;if not go on
          add       wptr es:[di].I_HLoc+2,dx  ;adjust history pointer

SIChkDir: cmp       es:[di].I_LocalD,0  ;local directory history?
           je       SIAHDone            ;if not go on
          add       wptr es:[di].I_DLoc+2,dx  ;adjust dir history pointer
          ;
SIAHDone: pop       es                  ;restore ES
          ;
          ; Adjust relocations within 4DOS
          ;
          test      SrvFlag2,NORELOC    ;relocation disabled because no swap
                                        ;  occurred (will happen with disk
                                        ;  swapping if checksum matches)?
           jnz      ClrSwap             ;if so skip relocation
          mov       dx,cs               ;get fixup target
          sub       dx,OldRBase         ;adjust for previous relocation
           je       ClrSwap             ;if no change skip adjustments
          mov       bx,TRelBase         ;point to relocation base address
          mov       si,offset RelocTab  ;point to table
          call      Reloc               ;fix up all relocations
          ;
          ; Clear swapping flags
          ;
ClrSwap:  cli                           ;hold interrupts
          bclr      es:LMFlags,BRKOCCUR+IGNORE23 ;clear loader's break flag,
                                        ;  allow ^Cs
          bclr      es:LMFlag2,SWAPPED  ;no longer swapped out
          ;
          exit                          ;swap in is done
          ;
          ;
          ; DOSWAP - Swap 4DOS in or out
          ;
          ; On entry:
          ;         DL =  0 to copy from base memory to swap area
          ;               1 to copy from swap area to base memory
          ;         DS = server segment
          ;
          ; On exit:
          ;         AX, BX, CX, DX, SI, DI destroyed
          ;         Other registers and interrupt state unchanged
          ;
          entry     DoSwap,noframe,,local  ;swap in/out
          assume    ds:@curseg          ;set assumes
          ;
          push      es                  ;save es
          ;
          or        dl,dl               ;check direction
          jnz       InDir               ;if swapping in go on
          mov       bl,OutStart         ;get swap out start block number
          mov       cl,OutCnt           ;get swap out block count
          jmp       short SetSwap       ;go set up
          ;
InDir:    mov       bl,InStart          ;get swap out start block number
          mov       cl,InCnt            ;get swap out block count
          ;
SetSwap:  mov       al,(size RegSCB)    ;get region SCB size
          dec       bl                  ;make region number 0-based
          mul       bl                  ;get relative block offset in AX
          add       ax,offset Region1   ;get memory offset of first region
          mov       si,ax               ;save offset in SI
          xor       ch,ch               ;clear high byte of count
          ;
SwapLoop: or        dl,dl               ;swap out?
           jnz      SwapGo              ;if not go on
          test      [si].SWAPFLAG,NEWRBAS  ;it's a swap out -- are we
                                           ;  setting a new reloc base?
           jz       SwapGo              ;if not go on
          mov       OldRBase,cs         ;yes, set new base
          mov       es,LMSeg            ;get low segment
          mov       es:ShellInf.CSwpRBas,cs  ;set new base for inheritance

SwapGo:   mov       es,SrvPSP           ;get PSP segment for SwapOne
          call      CSwpOne             ;do the actual swap for one region
           jnc      SwapOK              ;go on if no error
          mov       dx,ax               ;copy error code to DX
          jmp       ErrHandl            ;and go to error handler
          ;
SwapOK:   test      [si].SwapFlag,RLDISABL ;disable relocation if checksum
                                           ;  matches?
           jz       SwapNext            ;no, go on
          or        ax,ax               ;yes, see if it matched
           jz       SwapNext            ;if not, go on
          bset      SrvFlag2,NORELOC    ;segment wasn't swapped, disable
                                        ;  relocation
          ;
SwapNext: add       si,(size RegSCB)    ;move to next block
          loop      SwapLoop            ;swap next block
          ;
          pop       es                  ;restore es
          exit                          ;all done
          ;
          ;
          ; CSwpOne - Call the SwapOne routine.  This routine is used so
          ;           that we don't make a far call to SwapOne, since a far
          ;           call would be relocatable and we can't have relocatable
          ;           items in the server (see paragraph at start of file
          ;           for details).
          ;
          ; On entry:
          ;         All registers set up for SwapOne
          ;
          ; On exit:
          ;         Registers as returned from SwapOne
          ;
          ;
          entry     CSwpOne,varframe,,local  ;set up entry point
          ;
          varD      SOLoc               ;location of SwapOne
          varend                        ;end variables
          ;
          mov       SOLoc.fseg,cs       ;save our segment for CALL
          mov       SOLoc.foff,offset SwapOne  ;set offset
          call      dword ptr SOLoc     ;call SwapOne
          ;
          exit                          ;all done
          ;
          ;
          ; SetDS - Set DS to local data area
          ;
          ; On entry:
          ;         DS = caller's data segment
          ;
          ; On exit:
          ;         DS saved
          ;         DS set to local data segment
          ;         ES set to loader segment
          ;         Other registers and interrupt state unchanged
          ;
          entry     SetDS,noframe,,local    ;set up DS and ES
          ;
          push      ds                  ;save on stack
          loadseg   ds,cs               ;set to local segment
          pop       SaveDS              ;save caller's segment
          mov       es,LMSeg            ;set ES to loader
          ;
          exit                          ;and return
          ;
          ;
          ; OS2TRest - Restore OS/2 2.0 session title
          ;
          ; On entry:
          ;         No requirements
          ;
          ; On exit:
          ;         BX, CX, SI, DI assumed destroyed
          ;         Other registers and interrupt state unchanged
          ;
          ; Notes:
          ;
          entry     OS2TRest,noframe,,local    ;restore OS/2 2.0 title
          ;
          test      SrvFlag2,RESTITLE   ;check if we should restore title
           jz       TRExit              ;if not get out
          pushm     ax,dx               ;save return codes
          mov       dx,1                ;set our title
          call      OS2Title            ;uses what's in the buffer
          bclr      SrvFlag2,RESTITLE   ;clear flag to restore OS/2 title?
          popm      dx,ax               ;restore return codes
          ;
TRExit:   exit                          ;and return
          ;
          ;
          ; OS2Title - Get or set OS/2 2.0 session title
          ;
          ; On entry:
          ;         DX = 1 to set title, 2 to get title
          ;
          ; On exit:
          ;         AX, BX, CX, DX, SI, DI assumed destroyed
          ;         Other registers and interrupt state unchanged
          ;
          ; Notes:
          ;         This proc can't have a frame, it uses the ServExec
          ;         frame (for OldName)!
          ;
          entry     OS2Title,noframe,,local    ;get / set OS/2 2.0 title
          ;
          pushm     ds,es               ;save DS & ES
          loadseg   es,ss               ;get buffer seg
          loadseg   ds,ss               ;get buffer seg
          lea       di,OldName          ;get buffer offset
          call      OS2TAcc             ;and go call OS/2
          popm      es,ds               ;restore DS & ES
          ;
          exit                          ;and return
          ;
          ;
          ; OS2TAcc - Access OS/2 2.0 session title
          ;
          ; On entry:
          ;         DX = 0 is pre-exec notification, 1 to set title,
          ;              2 to get title
          ;         ES:DI = buffer address for get or set title
          ;
          ; On exit:
          ;         AX, BX, CX, DX, SI, DI, ES assumed destroyed
          ;         Other registers and interrupt state unchanged
          ;
          ;
          entry     OS2TAcc,noframe,,local  ;access OS/2 2.0 title
          ;
	cmp       cs:DosMajor,20      ;OS/2 2.0 or above DOS box?
	 jb	TAExit		;if not forget it	
          xor       bx,bx               ;make BX 0
          mov       cx,636Ch            ;get magic value ("cl" in ASCII)
          mov       ax,6400h            ;get DOS function
          calldos                       ;do it
          ;
TAExit:	exit                          ;and return
          ;
          ;
          ; LowStack - Set up loader stack
          ;
          ; On entry:
          ;         DS = local segment
          ;         ES = resident segment
          ;         SS, SP point to caller's stack
          ;
          ; On exit:
          ;         AX, BX destroyed
          ;         old SS, SP saved
          ;         SS, SP point to loader stack
          ;         Other registers unchanged, interrupts on
          ;
          entry     LowStack,noframe,,local    ;set up loader stack
          ;
          pop       bx                  ;get back return address, leave stack
                                        ;  clean
          mov       SavStack.fseg,ss    ;save ss
          mov       SavStack.foff,sp    ;save sp
          mov       ax,es               ;copy loader segment
          cli                           ;hold interrupts
          mov       ss,ax               ;set ss
          mov       sp,offset StackTop  ;set sp
          sti                           ;allow interrupts
          ;
          jmp       bx                  ;return
          ;
LowStack  endp                          ;can't use EXIT, don't want a RET!
          ;
          ;
          ; RESTPA - Reserve the transient program area
          ;
          ; On entry:
          ;         DS = server data segment
          ;
          ; On exit:
          ;         AX, BX destroyed
          ;         Other registers and interrupt state unchanged
          ;
          entry     ResTPA,noframe,,local   ;reserve the TPA
          test      Flags,TPARES        ;should it be reserved?
           jz       RTExit              ;if not go on
          mov       bx,0FFFFh           ;get maximum reserve
          calldos   ALLOC               ;find out how much we have (in BX)
          mov       cs:TPASize,bx       ;save size
          calldos   ALLOC,TALLOC        ;and reserve that
          mov       cs:TPAAddr,ax       ;save address
          ;
RTExit:   exit                          ;all done
          ;
          ;
          ; FREETPA - Free the transient program area
          ;
          ; On entry:
          ;         DS = server data segment
          ;
          ; On exit:
          ;         AX destroyed
          ;         Other registers and interrupt state unchanged
          ;
          entry     FreeTPA,noframe,,local   ;free the TPA
          ;
          ; Free transient program area if it is reserved
          ;
          test      Flags,TPARES        ;should it be reserved?
           jz       FTExit              ;if not get out
          cmp       cs:TPASize,0        ;is it reserved?
           je       FTExit              ;if not get out
          push      es                  ;save ES
          mov       es,cs:TPAAddr       ;get TPA address
          calldos   FREE                ;free transient memory
          pop       es                  ;restore ES
          mov       cs:TPASize,0        ;clear size
          ;
FTExit:   exit                          ;all done
          ;
          ;
          ; Jump table used by SrvClean
          ;
FreeTab   dw        FreeLow             ;0 = low DOS memory
          dw        FreeDUMB            ;1 = DOS UMB
          dw        FreeXUMB            ;2 = XMS UMB
          ;
          ;
          ; SrvClean -- Clean up shell number, swapping, and memory
          ;               allocation
          ;
          ; Used by the Quit code, and by DV.ASM to clean things up when a DV
          ; close window command is executed
          ;
          entry     SrvClean,noframe    ;set up entry point
          ;
          ;---------------------------------------------------------------
          ; Seg Registers:  DS = ???; ES = ???
          ;---------------------------------------------------------------
          ;
          loadseg   ds,cs               ;set DS to local segment
          mov       es,LMSeg            ;set ES to loader
          ;
          ;---------------------------------------------------------------
          ; Seg Registers:  DS = server; ES = ???
          ;---------------------------------------------------------------
          ;
          assume    ds:@curseg          ;fix assumes
          ;
          ; Release shell number
          ;
RelShell: mov       bl,es:ShellNum      ;get shell number
          or        bl,bl               ;are we the root loader?
           je       SwapBye             ;if so go on
          mov       bh,SERVSEC          ;get 4DOS 4 secondary shell function
          cmp       PrevVer,3           ;previous shell version 3 or below?
           ja       RelGo               ;if not go on
          mov       bh,SERVTERM         ;get 4DOS 2/3 shell terminate
          ;
RelGo:    mov       ax,SERVSIG          ;get server signature
          int       SERVINT             ;call server to close down shell num
          mov       es:ShellNum,0       ;show it's closed
          ;
          ; Clean up swapping, release high-memory
          ;
SwapBye:  test      Flags,TRANSHI       ;swapping?
           jz       ClnInt              ;if not skip swapping cleanup
          mov       dx,es:LMHandle      ;get local swap handle
          mov       al,SrvSwap          ;get swapping control
          cmp       al,SWAPEMS          ;EMS swapping?
           je       ClearEMS            ;if so go clear out EMS
          cmp       al,SWAPDISK         ;disk swapping?
           je       ClrDisk             ;if so go clear it
          ;
          ; Deallocate XMS
          ;
          callxms   DALOC,SrvXDrvr      ;deallocate XMS (ignore errors)
          jmp       short ClnInt        ;go on
          ;
          ; Deallocate EMS
          ;
ClearEMS: callems   DALOC               ;deallocate EMS (ignore errors)
          jmp       short ClnInt        ;go on
          ;
          ; Close disk swap file
          ;
ClrDisk:  test      Flags,DISKOPEN      ;is disk swap file open?
           jz       ClnInt              ;if not go on
          mov       bx,dx               ;copy swap file handle
          calldos   CLOSE               ;try to close it (ignore errors)
          mov       dx,offset SwapPath  ;point to swap file path
;;LFN
          calldos   DELETE              ;try to delete file (ignore errors)
          ;
          ; Restore interrupts
          ;
ClnInt:   call      IntClean            ;clean up interrupts
          ;
          ; Free any memory blocks reserved for resident portion in a UMB,
          ; master environment, global aliases & functions, or global cmd 
          ; or dir history
          ;
          mov       si,offset MFreeLst  ;point to free block list
          ;
FreeLoop: mov       cx,[si].FreeSeg     ;get segment address
           jcxz     MemQuit             ;if zero we're done
          mov       bl,[si].FreeType    ;get block type
          xor       bh,bh               ;clear high byte
          shl       bx,1                ;make it word offset
          jmp       wptr FreeTab[bx]    ;jump to block free code
          ;
FreeLow:  mov       es,cx               ;copy segment address
          calldos   FREE                ;free the block (ignore errors)
          jmp       FreeNext            ;and loop
          ;
FreeDUMB: mov       bx,cx               ;get UMB segment
          mov       ah,1                ;get release function
          call      DosUMB              ;release the DOS UMB
          jmp       FreeNext            ;and go on
          ;
FreeXUMB: mov       dx,cx               ;copy segment
          callxms   RELUM,SrvXDrvr      ;deallocate XMS UMB (ignore errors)
          ;
FreeNext: add       si,(size FreeItem)  ;skip to next item
          jmp       FreeLoop            ;and loop
          ;
          ; Free 4DOS memory block
          ;
MemQuit:  call      FreeTPA             ;free the TPA if it's reserved
          test      Flags,TRANSHI       ;transient loaded high?
           je       PSPRest             ;no, go on
          mov       es,LMSeg            ;get low segment
          test      es:LMFlags,SWAPENAB  ;swap enabled?
           jnz      PSPRest             ;yes, go on
          loadseg   es,cs               ;swap disabled so high area is
                                        ;  reserved -- get segment address
          calldos   FREE                ;try to free it
          ;
          ; Restore the old PSP chain pointer
          ;
PSPRest:  mov       es,SrvPSP           ;point to low segment = PSP
          mov       ax,SavChain         ;get old chain pointer
          mov       es:[PSP_CHN],ax     ;restore old value
          dload     ax,bx,OldSav22      ;get old INT 22 vector
          dstore    es:PSP_TERM,ax,bx   ;restore it in PSP
          ;
          exit                          ;and return
          ;
          ;
          ; DOQUIT - Quit from 4DOS due to ServCtrl Quit call or error
          ;
          ; On entry:
          ;         AL = exit code
          ;         DS = server segment
          ;
          ; On exit:
          ;         No exit
          ;
          entry     DoQuit,noframe,,local  ;set up entry
          ;
          ; Use the last 2 bytes of the PSP to stash an INT 21, then branch
          ; to it.  This insures that the terminate call comes from reserved
          ; memory, in the segment that 4DOS was started from.
          ;
          mov       bx,PSP_LAST - 1     ;get address at end of PSP
          mov       es:[bx],21CDh       ;store INT 21 (CD 21) instruction
          mov       ah,D_TERM           ;get terminate code
          push      es                  ;put INT 21 segment on stack
          push      bx                  ;put INT 21 offset on stack
          retf                          ;branch to INT 21 and exit
          ;
DoQuit    endp
          ;
          ;
          ; Include interrupt cleanup routines
          ;
          include   INTCLEAN.ASM
          ;
          ;
          ; Code used to fake a ^Break when an unexpected process termi-
          ; nation occurs.  Resets our INT 23 and 24 vectors as these will
          ; be "restored" from the PSP by DOS when an Abort is done in
          ; response to a critical error and 4DOS is "aborted".
          ;
SFBreak   equ       $                   ;here to fake a ^Break
          ;
          mov       ds,cs:LMSeg         ;get loader segment
          mov       dx,cs:LocInt23      ;get our INT 23 vector
          mov       al,23h              ;get interrupt number
          calldos   SETINT              ;reset the vector
          lds       dx,cs:LocInt24      ;get our INT 24 vector
          mov       al,24h              ;get interrupt number
          calldos   SETINT              ;reset the vector
          loadseg   ds,cs               ;set ds to this segment
          cli                           ;hold interrupts
          mov       ss,TDataSeg         ;set temp stack segment
          mov       sp,TmpStack         ;and offset
          mov       dx,offset CrLf      ;get address of CR/LF
          mov       bx,STDOUT           ;get handle
          mov       cx,2                ;2 characters
          calldos   WRITE               ;and display it
          mov       ax,offset _TEXT:BreakHandler  ;get break handler addr
          jmp       short Ret4D         ;go back to 4DOS proper
          ;
          ;
          ; Loader code jumps here if it detects a break within 4DOS.
          ;
I23Break: push      ds                  ;save ds
          loadseg   ds,cs               ;set ds
          cmp       SigState,SIGENAB    ;check pointer value
          je        ProcBrk             ;if low enough, must be handler addr
                                        ;otherwise no handler so ignore ^C
          pop       ds                  ;restore ds
          clc                           ;show no error
          sti                           ;leave interrrupts enabled
          retf                          ;return to DOS
          ;
          ; ProcBrk -- handle a ^C occurring within 4DOS, with break signal
          ; enabled
          ;
ProcBrk:  cli                           ;hold interrupts
          mov       ss,TDataSeg         ;set temp stack segment
          mov       sp,TmpStack         ;and offset
          cld                           ;force direction clear
          calldos   GETTIME             ;get system time to stabilize stack
          mov       ax,SigAdr           ;get break handler address
          ;
          ; Return to 4DOS from one of the errors above
          ;
Ret4D:    call      OS2TRest            ;restore OS/2 session title
                                        ;  (preserves AX)
          mov       ds,TDataSeg         ;get main data segment
          sti                           ;be sure interrupts are enabled
          mov       bx,cs:TCodeSeg      ;get segment
          push      bx                  ;put on stack
          push      ax                  ;put offset on stack
          retf                          ;and jump to handler
          ;
          ;
          ; Int 2E handler (only called if full INT 2E support enabled)
          ;
          ; On entry:
          ;         DS = server segment
          ;         DI:SI = Address of line to execute
          ;         ES = resident segment
          ;         Stack points to caller's stack, with the following
          ;         items already pushed on (pushed in this order):
          ;           INT 2E caller's PSP
          ;           Loader's LMFlags and LMFlag2
          ;           Loader's ExecSig and next byte
          ;         After these are popped stack must be in state for
          ;           IRET to INT 2E caller
          ;
          ; Does not exit, returns via IRET
          ;
          ; First save crucial server and loader data on the caller's stack
          ;
I2EHdlr:  push      SavStack.foff       ;save saved stack offset
          push      SigState            ;save break state
          push      SigAdr              ;save break address
          push      wptr SrvFlags       ;save server flags (2 bytes)
          push      wptr Flags          ;save general flags (2 bytes)
          ;
          ; Copy the top of the loader's stack to the caller's stack so if
          ; an external program is run we don't clobber our own low-memory
          ; stack
          ;
          mov       ax,si               ;copy command offset
          mov       dx,di               ;copy command segment
          loadseg   ds,es               ;copy loader segment
          loadseg   es,ss               ;get destination segment
          mov       si,offset StackTop - 1  ;get top of space to save
          mov       di,sp               ;store at TOS
          dec       di                  ;adjust for how SP works
          std                           ;go backward
          mov       cx,LOWSTSAV         ;get bytes to save
          sub       sp,cx               ;allocate space on stack
          rep       movsb               ;copy stack to stack
          loadseg   ds,cs               ;put DS back
          mov       es,LMSeg            ;and reset ES
          ;
          pushm     dx,ax               ;save address of command to execute
          cld                           ;be sure DF is clear
          ;
          ; Swap 4DOS back in 
          ;
          bclr      SrvFlag2,INDV       ;kill DV flag so we don't try to
                                        ;  handle a close window command
          call      SwapIn              ;swap 4DOS back in if necessary
          popm      si,di               ;restore command address
          ;
          ; Switch to the transient stack and execute the command
          ;
          mov       I2EStack.fseg,ss    ;save SS
          mov       I2EStack.foff,sp    ;save SP
          cli                           ;hold interrupts
          mov       ss,SavStack.fseg    ;get back transient stack seg
          mov       sp,SavStack.foff    ;get back transient stack offset
          sti                           ;interrupts back on
          pushm     di,si               ;save address of command to execute
                                        ;  for DoINT2E
          mov       ax,TCodeSeg         ;get code segment
          mov       I2ECall[2],ax       ;stash in pointer
          mov       ds,TDataSeg         ;set transient data segment
          call      dword ptr cs:I2ECall  ;execute the command
          ;
          ; Restore stack and saved data then swap 4DOS back out.  AX MUST
          ; BE SAVED for return to calling program!
          ;
          loadseg   ds,cs               ;set DS to local
          cli                           ;hold interrupts
          mov       ss,I2EStack.fseg    ;get back caller's stack seg
          mov       sp,I2EStack.foff    ;get back caller's stack offset
          sti                           ;re-enable
          ;
          ; Restore the top of the loader's stack from the caller's stack
          ;
          mov       es,LMSeg            ;get destination segment
          loadseg   ds,ss               ;copy caller's stack segment
          mov       si,sp               ;get source offset
          mov       di,(offset StackTop  - LOWSTSAV) ;get bottom for restore
          mov       cx,LOWSTSAV         ;get words to restore
          rep       movsb               ;copy stack to stack
          add       sp,LOWSTSAV         ;deallocate space on stack
          loadseg   ds,cs               ;restore DS
          ;
          pop       wptr Flags          ;restore general flags (2 bytes)
          pop       wptr SrvFlags       ;restore server flags (2 bytes)
          pop       SigAdr              ;restore break address
          pop       SigState            ;restore break state
          pop       SavStack.foff       ;restore saved stack offset
          pop       wptr es:ExecSig     ;rest EXEC signature (and next byte)
          pop       wptr es:LMFlags     ;restore flags (2 bytes)
          push      ax                  ;save result again
          call      SwapOut             ;swap back out
          pop       ax                  ;restore result again
          pop       bx                  ;restore caller's PSP
          ;
          ; Restore the PSP and return to the caller
          ;
          push      ax                  ;save AX
          calldos   SETPSP              ;set old PSP as current PSP
          pop       ax                  ;restore it
          bclr      es:LMFlags,IGNORE23 ;clear flag set by SwapOut
          iret                          ;and return to caller
          ;
          ;
          ; ERRHANDL - Server error handler
          ;
          ; On entry:
          ;         DX = primary error code, high bit set if fatal
          ;
EMLoc     dw        offset ErrMsg       ;error message routine offset
          dw        ?                   ;segment goes here
          ;
ErrHandl: mov       bp,sp               ;save frame pointer
          sub       sp,ERRMAX           ;make frame for output buffer
          loadseg   ds,cs               ;set DS
          assume    ds:@curseg          ;fix assumes
          pushm     dx,dx               ;save error code twice
          mov       dx,offset SrvError  ;get error message offset
          calldos   MESSAGE             ;display message
          pop       ax                  ;get back major error code
          and       ah,7Fh              ;isolate error number
          ;
          loadseg   es,ss               ;destination is on stack
          mov       di,bp               ;buffer at BP[0]
          mov       si,offset ServErrs  ;get error table address
          mov       EMLoc.fseg,cs       ;save our segment for CALL
          call      dword ptr EMLoc     ;get error message
          mov       dx,bp               ;copy message address
          stocb     '$'                 ;dump a terminator into the buffer
          push      ds                  ;save local segment
          loadseg   ds,es               ;point to error segment
          calldos   MESSAGE             ;write the error message
          pop       ds                  ;restore local segment
          mov       dx,offset CrLf      ;point to cr/lf
          ;
ErrWrite: calldos   MESSAGE             ;output swap error or cr/lf
          ;
          mov       es,LMSeg            ;get loader segment
          mov       bl,es:ShellNum      ;get shell number
          or        bl,bl               ;are we the root loader?
          je        ErrClean            ;if so go on
          mov       ax,SERVSIG          ;if not, get server signature
          mov       bh,SERVSEC          ;get secondary shell function
          int       SERVINT             ;call server to close down shell num
          ;
ErrClean: call      SrvClean            ;unhook interrupts
          pop       ax                  ;get back caller's error code
          test      ah,80h              ;high bit set?
           jnz      Fatal               ;if so give up
          test      SrvFlags,ROOTFLAG   ;root shell?
           jnz      Fatal               ;if so give up
	;
	; Wait for a keystroke so user can read the message!
	;
	mov	dx,offset ExitMsg	;get message address
	calldos	MESSAGE		;display it
	;
ErrKeyRd:	mov       ah,1                ;check keyboard buffer
          int       16h                 ;any character there?
          jz        ErrRdChr		;if not go read the real character
          xor       ah,ah               ;read keyboard service
          int       16h                 ;read key from buffer
          jmp       short ErrKeyRd      ;loop until buffer is empty
          ;
ErrRdChr: xor       ah,ah               ;read keyboard service
          int       16h                 ;read key from buffer
	;
	mov       al,0FFh             ;exit with code 255
          call      DoQuit              ;quit -- never returns
          ;
Fatal:    mov       dx,offset FatalMsg  ;point to message
          calldos   MESSAGE             ;tell user
          ;
Forever:  sti                           ;be sure interrupts are enabled
          jmp       short Forever       ;give up but allow keyboard 
                                        ;  interrupts so Ctrl-Alt-Del works
                                        ;  (also allows partitions to be
                                        ;  closed in multitaskers)
          ;
          ;
@curseg   ends                          ;close segment
          ;
          ;
          end

