

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


          title     4DOS - 4DOS low-memory loader & server
          ;
          page      ,132                ;make wide listing
          ;
          comment   }

          Copyright 1989-1999, J.P. Software, All Rights Reserved

          Author:  Tom Rawson  10/28/88
                   substantial swapping mods 3/13/89
                   totally rewritten 12/89 - 2/90

          This routine loads the 4DOS program file and starts it.  It also
          contains the 4DOS low-memory server which executes external
          programs for 4DOS.

          This is a tiny-model file designed to be run as 4DOS.COM.


          } end description
          ;
          ;
          ; Includes
          ;
          include   product.asm         ;product / platform definitions
          include   trmac.asm           ;general macros
          .cseg     LOAD                ;set loader segment if not defined
                                        ;  externally
          include   model.inc           ;Spontaneous Assembly memory models
          include   4dlparms.asm        ;loader / server parameters
          include   4dlstruc.asm        ;loader / server data structures
          ;
          ;
          ; External references
          ;
          ;         Set external declarations for routines which are
          ;         assembled for both loader and transient portion.
          ;
          .extrn    ErrMsg:far
          ;
          ;         Standard external declarations
          ;
          extrn     Init:near
          ;
          ; Define public labels for variables accessed by 4DLINIT
          ;
          ;         Variables defined in LOADDATA.ASM
          ;
          public    LocStack, StackTop, ShellNum
          public    LMFlags, LMFlag2, LMHandle
          public    DiskSig, DSigTime, OldInt, PSPSeg
          public    ExecSig, ServLenW, ServChk, ReopenP
          public    ShellInf, CodeReg, CodeDev, Err24Hdl
          ;
          ;         Variables defined locally
          ;
          public    Prompt
          public    ServFake, ServSeg, ServBrk, ServI2E
          public    SigBuf, TPLenP, SwapInP
          public    ShellDP, ShellPtr
          public    RODP, ROAccess
          public    CMIPtr, CMHPtr, PEPtr
          public    EMOffset, EMSeg
          public    FrameSeg, XMSDrvr, XMSMove
          public    IntServ, LMExec, ExecExit
          public    Int23, Int24
          public    ResErrP
          ;
          ;
          .defcode  ,1                  ;set up code segment, no DGROUP
          ;
          assume    cs:@curseg, ds:nothing, es:nothing, ss:nothing
          ;
          page
          ;
          ;
          errset    ResErrH             ;set error handler name
          ;
          ; Set origin so resident and initialization code is relocated
          ; beginning at address 100, as in a .COM file.  This allows us
          ; to move the code down and reset CS to point to the PSP,
          ; so that the resident part of 4DOS is behaving just like a
          ; .COM file, to ensure compatibility with other products that
          ; require the resident command processor's CS to be the same
          ; as its PSP address.
          ;
          org       100h
BaseLoc   equ       $                   ;start of block to move down
          ;
          ;
          ; Data accessible by high-memory code.  This data is assumed by
          ; the code in SERVER.ASM to reside at location 100h of the low-
          ; memory segment.  If you move it, modify SERVER.ASM accordingly!
          ;
          ; (NOTE:  This file includes the local stack).
          ;
          include   loaddata.asm
          ;
          ;
          ; Resident private data (not accessible from high memory)
          ;
          ;
SavStack  farptr    <>                  ;saved 4dos SS:SP
          ; -------------------------------------------------------------
                                        ;NOTE:  following two bytes are a
                                        ;dword ptr and must be together
ServFake  dw        ?                   ;pointer to server fake break code
ServSeg   dw        ?                   ;high memory server segment
          ; -------------------------------------------------------------
ServBrk   dw        0                   ;pointer to server signal code
ServI2E   dw        0                   ;pointer to server INT 2E handler
SigBuf    db        SIGBYTES dup (?)    ;disk swap signature buffer
TPLenP    dw        0                   ;transient portion length in par.
SwapInP   dw        0                   ;swap in code address
RODP      dw        0                   ;swap file reopen data pointer
ROAccess  db        0                   ;swap file access flags
ShellDP   dw        0                   ;shell table data address
ShellPtr  dw        0                   ;shell number manager code address
CMIPtr    dw        0                   ;COMMAND.COM msg handler init ptr
CMHPtr    dw        0                   ;COMMAND.COM msg handler ptr
PEPtr     dw        0                   ;CC msg handler parse error table ptr
ResErrP   dw        offset ResErrH      ;error handler address
          ;
          ;         CAUTION:  Next two words must be together!
          ;
EMOffset  dw        offset ErrMsg       ;offset of ErrMsg
EMSeg     dw        ?                   ;segment of LErrMsg
          ;
FrameSeg  dw        ?                   ;EMS page frame segment
XMSDrvr   farptr    <>                  ;XMS driver address
XMSMove   XMovData  <0,0,0,0,0,0,0>     ;XMS move data (initially zeroed)
GlobData  dw        GDCNT dup (?)       ;global data area
          ;
          ; Critical error handler data and message storage
          ;
SaveCode  dw        ?                   ;temp save of return code
          ;
BSMsg     db        BS, ' ', BS, 0      ;backspace and overwrite
LocCrLf   db        CR, LF, 0           ;cr / lf
          ;
          ifdef     ENGLISH
Prompt    db        43 dup (?)          ;prompt, filled in by 4DLInit
          endif
          ;
          ifdef     GERMAN
Prompt    db        61 dup (?)          ;prompt, filled in by 4DLInit
          endif
          ;
          ;
          ; Loader messages
          ;
_MsgType  equ       0                   ;assemble resident messages
          ifdef     ENGLISH
          include   4DLMSG.ASM
          endif
          ;
          ifdef     GERMAN
          include   4DLMSG.ASD
          endif
          ;
          ;
          ; INTSERV - Low memory interrupt 2F handler
          ;
          ; This is the 4DOS low-memory interrupt server.  It is entered
          ; via a call to the server interrupt (SERVINT, defined in 
          ; 4DLPARMS.ASM, intended to be interrupt 2F, the multiplex
          ; interrupt).
          ;
          ; The possible arguments are:
          ;
          ;         AX = 4DOS interrupt signature, set to SERVSIG value
          ;         BH = function code:
          ;                   0 = inquiry
          ;                   1 = locate swapping info
          ;                   2 = start or terminate a shell
          ;                   3 = get global data
          ;                   4 = set global data
          ;         BL = If BH = 2:  0 for new shell, or terminating shell
          ;                   number for secondary shell exit
          ;              if BH = 3 or 4:  data word index (0 - (GDCNT-1))
          ;         DX = global data value if BH = 4
          ;
          ; Note ServJump table must be here, not immediately after the
          ; JMP that uses it -- otherwise MASM gets a phase error, even
          ; though there is no phase problem.
          ;
ServJump  label     word                ;function code branch table
          dw        Inquiry             ; 0 = get 4DOS info
          dw        GetSwap             ; 1 = get swap info
          dw        SecShell            ; 2 = new or ending secondary shell
          dw        GetGlob             ; 3 = get global data
          dw        SetGlob             ; 4 = set global data
          ;
          ;
IntServ   proc      far                 ;start interrupt server
          ;
          assume    ds:nothing          ;assume no DS
          ;
          ; See if it's our interrupt
          ;
          pushf                         ;save flags
          cmp       ax,SERVSIG          ;is it ours?
          je        ServRun             ;if so go handle it
          test      cs:LMFlag2,NOCCMSG  ;COMMAND.COM messages disabled?
          jnz       DoChain             ;if so go on
          cmp       ax,CCMSIG           ;check if signature is there
          jne       DoChain             ;if not, chain
          jmp       wptr CMIPtr         ;go to message handler init code
          ;
DoChain:  test      cs:LMFlags,NOCHAIN  ;interrupt chain inhibited?
          jnz       ServNCh             ;if so just return to caller
          popf                          ;restore flags
          cli                           ;chain with interrupts off
          jmp       dword ptr OldInt    ;chain to next handler
          ;
ServNCh:  popf                          ;clean up stack
          iret                          ;and return
          ;
          ; Server call not handled, chain if possible
          ;
ServChn:  mov       ax,SERVSIG          ;reset ax for chain
          popm      di,ds               ;restore registers
          pushf                         ;for POPFs above
          jmp       short DoChain       ;go chain to next handler
          ;
          ; Server call handled, return
          ;
ServRet:  popm      di,ds               ;restore DI and DS
          mov       ax,SERVRSIG         ;get return signature
          iret                          ;return to caller
          ;
          ; Interrupt is from 4DOS.  Figure out if it is for us or not.
          ;
ServRun:  popf                          ;clean up stack
          sti                           ;reallow interrupts
          pushm     ds,di               ;save registers
          cmp       bh,4                ;check if valid function code
           ja       ServChn             ;if not go chain
          loadseg   ds,cs               ;set ds to local segment
          mov       al,bh               ;copy function code
          xor       ah,ah               ;clear high byte
          shl       ax,1                ;make word offset
          mov       di,ax               ;copy to di
          jmp       wptr ServJump[di]   ;branch to appropriate routine
          ;
          ; Inquiry -- return current state.
          ;
Inquiry:  mov       bx,VERSION          ;get 4DOS version in bx
          mov       cx,PSPSeg           ;get our PSP segment
          mov       dl,ShellNum         ;get shell number
          jmp       short ServRet       ;and return
          ;
          ; Get swap information
          ;
GetSwap:  loadseg   es,cs               ;get local segment
          mov       bx,offset ShellInf  ;point to local shell information
          jmp       short ServRet       ;and return
          ;
          ; New or terminating secondary shell
          ;
SecShell: cmp       ShellNum,0          ;are we root loader copy?
          jne       ServChn             ;if not ignore call
          call      wptr ShellPtr       ;handle request
           jnc       ServRet            ;quit if OK
          error     SHNEW               ;error if no shell number available
          ;
          ; Get global data
          ;
GetGlob:  call      GDSetup             ;set up to handle global data
           jc       ServChn             ;if any problem, chain
          mov       dx,GlobData[bx]     ;get the word
          jmp       short ServRet       ;and return
          ;
          ; Set global data
          ;
SetGlob:  call      GDSetup             ;set up to handle global data
           jc       ServChn             ;if any problem, chain
          mov       GlobData[bx],dx     ;get the word
          jmp       short ServRet       ;and return
          ;
IntServ   endp                          ;end of interrupt server
          ;
          ;
          ; GDSETUP - Set up for global data access
          ;
          ; On entry:
          ;         BL = data word index (0 - (GDCNT-1))
          ;
          ; On exit:
          ;         Carry set if any problem, clear if all OK
          ;         If carry clear, BX = offset of data word in array
          ;         
          ;
GDSetup   proc      near                ;start global data setup
          test      LMFlags,ROOTFLAG    ;are we the root?
           jz       GDErr               ;if not return error
          cmp       bl,(GDCNT-1)        ;index OK?
           ja       GDErr               ;if not get out
          xor       bh,bh               ;clear high byte
          shl       bx,1                ;make word offset (also clears carry)
          ret                           ;and return
          ;
GDErr:    stc                           ;something is wrong
          ret
          ;
GDSetup   endp
          ;
          ;
          ; LMExec
          ;
          ; This is the 4DOS low-memory EXEC code.
          ;
LMExec    proc      far                 ;low-memory exec entry point
          ;
          assume    ds:nothing          ;assume no DS
          ;
          ; Registers are set up by high-memory code, as follows:
          ;
          ;         AX = EXEC code (4B00h)
          ;         ES:BX = parameter block address
          ;         DX = filename address
          ;
Exec:     mov       cs:SavStack.fseg,ss   ;save ss
          mov       cs:SavStack.foff,sp   ;save sp
          sti                           ;allow interrupts
          int       I_DOS               ;call DOS
          ;
          ; DOS doesn't always set the current PSP correctly when a process
          ; terminates, so we'd better set it ourselves if this is DOS 3
          ; or above
          ;
ExecExit: pushf                         ;save flags
          cli                           ;hold interrupts
          push      ax                  ;save result
          mov       bx,cs:PSPSeg        ;get our PSP segment
          calldos   SETPSP              ;set the current PSP
          pop       ax                  ;restore ax
          ;
          ; If EXEC wasn't really active when we get here, probably an
          ; ill-behaved TSR has issued a terminate (INT 21/4C) call from
          ; the prompt.  When this occurs, fake a ^Break.
          ;
ChkExec:  cmp       cs:ExecSig,EXECACT  ;was EXEC really active?
          je        ExecTerm            ;if so go on
          test      cs:LMFlags,INSHELL  ;are we in the shell?
          erre      PRTERM              ;if not, illegal process termination
          loadseg   ss,cs               ;set stack segment
          mov       sp,offset StackTop  ;set top of stack
          jmp       FakeBrk             ;wasn't active, fake a ^Break
          ;
          ; Save result code and restore stack
          ;
ExecTerm: popf                          ;restore flags
          mov       ss,cs:SavStack.fseg   ;restore ss
          mov       sp,cs:SavStack.foff   ;restore sp
          sti                           ;allow interrupts
          pushf                         ;hold flags while we swap
          push      ax                  ;ditto ax
          loadseg   ds,cs               ;set DS
          assume    ds:@curseg          ;fix assumes
          mov       ExecSig,0           ;clear signature
          bset      LMFlags,INSHELL+IGNORE23 ;tell INT 23/24 code we are in
                                        ;  shell, but to ignore ^Cs during
                                        ;  swap in -- this allows INT 24s
                                        ;  to be handled properly as well
          ;
          ; Kill any open upper memory chain; reset allocation strategy
          ;
          mov       bx,ULINKOFF         ;get value to clear UMB links
          mov       ax,(D_SETMETH shl 8) + SETULINK  ;set UMB link function
          calldos                       ;link or unlink the UMBs
          mov       bx,LFIRST + FIRSTFIT  ;set for first fit in low mem
          mov       ax,(D_SETMETH shl 8) + SETSTRAT  ;set strategy function
          calldos                       ;set new allocation strategy
          ;
          mov       bx,0FFFFh           ;try to reserve everything -- this 
                                        ;  forces DOS to consolidate all free
                                        ;  memory blocks, reducing the chance
                                        ;  that we will overwrite an MCB 
                                        ;  header with the swap in
          calldos   ALLOC               ;force MCB consolidation, get avail
                                        ;  space
          ;
          test      LMFlags,SWAPENAB    ;is swapping enabled?
          jz        ExecDone            ;if not skip it
          ;
          ; Find a place to put the transient portion (this moves 4DOS in
          ; case high memory has been reserved by someone, or memory has been
          ; expanded by something like VIDRAM)
          ;
          mov       dx,bx               ;copy size of largest block
          calldos   ALLOC               ;allocate that block
           jc       LMSwap              ;if can't get it go on
          sub       dx,TPLenP           ;get length without transient
          add       dx,ax               ;add base to get new server address
          mov       ServSeg,dx          ;save it for swappers
          mov       es,ax               ;copy segment
          calldos   FREE                ;and free it
          ;
LMSwap:   call      wptr SwapInP        ;call swap in code
          ;
          ;
          ; Get back interrupt return codes, return to server
          ;
          assume    ds:nothing          ;don't know what DS is now
ExecDone: pop       ax                  ;get back DOS error code
          popf                          ;and flags
          pop       bx                  ;get back return offset
          pop       cx                  ;and return segment
          push      ServSeg             ;put new segment on stack
          push      bx                  ;and offset
          retf                          ;return to server
          ;
LMExec    endp
          ;
          ;
          ; RESERRH - Server error handler
          ;
          ; On entry:
          ;         DX = primary error code
          ;
ResErrH:  loadseg   ds,cs               ;data seg = code seg
          assume    ds:@curseg          ;fix assumes
          mov       wptr ErrCode,dx     ;save error code in message
          mov       dx,offset LdError   ;get error message offset
          calldos   MESSAGE             ;display message
          test      LMFlags,(ROOTFLAG or FATALERR)  ;root or fatal error?
           jnz      Fatal               ;if so give up
          mov       al,0FFh             ;get error code
          calldos   TERM                ;and quit
          ;
Fatal:    sti                           ;be sure interrupts are enabled
          jmp       short Fatal         ;give up but allow keyboard 
                                        ;  interrupts so Ctrl-Alt-Del works
          ;
          ;
          ; INT 24 critical error handler
          ;
Int24     proc      far                 ;start int 24 handler
          ;
          sti                           ;restore interrupts
          pushm     ds,ax,bx            ;save some registers
          mov       bx,sp               ;copy stack pointer
          add       bx,6                ;make bx = original stack pointer
          pushm     cx,dx,si,di,bp,es   ;save other registers
          push      ax                  ;save ax for a bit
          loadseg   ds,cs               ;set ds to data area
          ;
          ; Figure out the error code
          ;
          and       di,0FFH             ;strip high byte of error number
          ;
          ; DOS 3 or above -- get extended error
          ;
GetExt:   pushm     si,bp,ds            ;save registers destroyed by dos call
          xor       bx,bx               ;need bx=0 for dos get ext error
          calldos   GETERR              ;get extended error code
          mov       di,ax               ;copy to di
          popm      ds,bp,si            ;restore registers
          ;
          ; Check for automatic Fail response
          ;
ChkAuto:  pop       ax                  ;get back original ax
          test      LMFlag2,AUTOFAIL    ;automatic Fail response?
           jz       FixFH               ;if not go on
          mov       SaveCode,3          ;set code to Fail
          jmp       Ret24               ;and issue the fail response
          ;
          ; Fix up file handles so we can use them
          ;
FixFH:    push      ax                  ;save error code
          mov       es,PSPSeg           ;get our PSP
          mov       dx,Err24Hdl         ;get SFT handles for error I/O
          calldos   GETPSP              ;get the current program's PSP
          mov       es,bx               ;copy it
          les       bx,es:[PSP_FPTR]    ;get JFT pointer
          xchg      es:[bx],dx          ;set STDIN and STDOUT to true STDERR,
                                        ;  get old value
          pop       ax                  ;restore error code
          push      dx                  ;save old file handles for restore
          ;
          ; Figure out what kind of error it is
          ;
          cld                           ;all moves go forward
          test      ah,080H             ;test for block dev error (bit 7 = 0)
           jz       DiskErr             ;if so go handle that
          ;
          ; Handle character device or FAT error
          ;
          mov       es,bp               ;set es = driver header segment
          test      wptr es:[si+4],08000H  ;check driver char dev bit
           jz       FATErr              ;not a character device, must be FAT
          cmp       di,21               ;device not ready?
           jne      GetDname            ;if some other error go on
          mov       di,40               ;use "not ready" instead of "drive
                                        ;  not ready" error message
          ;
          ; Get device name from driver header
          ;
GetDName: pushm     di,ds               ;save registers
          loadseg   es,ds               ;destination is in data segment
          mov       ds,bp               ;source is device driver header
          add       si,10               ;point to name field in driver header
          mov       di,offset DevName   ;point to local storage for name
          mov       cx,8                ;device name length
          rep       movsb               ;copy device name
          popm      ds,di               ;restore registers
          ;
          ; Display error message
          ;
          call      PrtErr              ;print error message
          mov       si,offset DevErr    ;point to "on device" message
          call      PrtMsg              ;print it
          jmp       short DoPrompt      ;go print prompt
          ;
          ; Handle FAT error
          ;
FATErr:   mov       di,41               ;FAT error message number
          ;
          ; Handle block device errors
          ;
;FIXME - Change this to display drive letter first
;Maybe also get file name used in transient portion (if 4DOS is active) and
;display full name??
DiskErr:  call      PrtErr              ;print error message
          ;
          ; Get user response to error
          ;
DoPrompt: mov       si,offset Prompt    ;point to prompt
          call      PrtMsg              ;display it
          ;
GetKey:   
;          mov       ax,0E00h + BELL     ;get write char func + beep character
;          xor       bx,bx               ;page 0
;          int       10h                 ;output beep
          mov       dl,BELL             ;get beep character
          calldos   DISPCHR             ;output beep
          ;
KeyRead:  mov       al,D_CONSIN         ;get console input function
          calldos   KFLUSH              ;flush keyboard buffer and do input
          or        al,al               ;extended key code?
           jnz      CheckChr            ;if not go on
          calldos   CONSIN              ;read second part
          jmp       GetKey              ;and try again (ext keys invalid)
;          mov       ah,1                ;check keyboard buffer
;          int       16h                 ;any character there?
;          jz        ReadChr             ;if not go read the real character
;          xor       ah,ah               ;read keyboard service
;          int       16h                 ;read key from buffer
;          jmp       short KeyRead       ;loop until buffer is empty
;          ;
;ReadChr:  xor       ah,ah               ;read keyboard service
;          int       16h                 ;read key from buffer
;          cmp       al,32               ;too small?
          ;
CheckChr: cmp       al,32               ;too small?
          jb        GetKey              ;if so retry
          cmp       al,126              ;too large?
          ja        GetKey              ;if so retry
;          push      ax                  ;save it (int 10h destroys AL)
;          mov       ah,0Eh              ;get write character function code
;          xor       bx,bx               ;page 0
;          int       10h                 ;output character to display
;          pop       ax                  ;get back character
          push      ax                  ;save it (echo destroys AL)
          mov       dl,al               ;copy character
          calldos   DISPCHR             ;echo it
          pop       ax                  ;get back character
          cmp       al,97               ;is it lower case alpha?
           jb       ChkResp             ;if too small for lower case go on
          sub       al,32               ;convert lower case to upper case
          ;
          ; Calculate response value
          ;
ChkResp:  mov       di,offset RespTab   ;point to table
          loadseg   es,ds               ;set segment
          mov       cx,RespCnt          ;get number of possible responses
          repne     scasb               ;find response
           je       RespVal             ;if found go on
          mov       si,offset BSMsg     ;illegal key pressed -- backspace 
                                        ;  over it
          call      PrtMsg              ;do backspace
          jmp       short GetKey        ;go try again
          ;
RespVal:  sub       di,offset RespTab + 1  ;get response value (0=I, 1=R,
                                           ;  2=A, 3=F)
          mov       SaveCode,di          ;save to send back to DOS
          ;
          ; All done, display final message
          ;
          mov       si,offset LocCrLf   ;point to final message
          call      PrtMsg              ;display it
          ;
          ; Restore file handles
          ;
          calldos   GETPSP              ;get the current program's PSP
          mov       es,bx               ;copy it
          les       bx,es:[PSP_FPTR]    ;get JFT pointer
          pop       wptr es:[bx]        ;restore original STDIN and STDOUT
          ;
          ; Return to DOS
          ;
Ret24:    popm      es,bp,di,si,dx,cx,bx,ax   ;restore registers
          mov       al,bptr SaveCode    ;get return code (low byte)
          pop       ds                  ;restore ds
          iret                          ;return to dos
          ;
Int24     endp
          ;
          ;
          ; Code to fake a ^Break when an unexpected process termination
          ; occurs.
          ;
          ; NOTE:  This code assumes 4DOS is in memory, and jumps directly
          ; to the high-memory break handler code.  If 4DOS is swapped out
          ; then we shouldn't get here because no self-respecting TSR should
          ; try to terminate via a false exit unless it is at the DOS prompt!
          ;
FakeBrk   equ       $                   ;here to fake a ^Break
          jmp       dword ptr cs:ServFake ;jump to high memory fake break code
          ;
          ;
          ; ^C handler
          ;
Int23     proc      far                 ;interrupt 23 handler
          ;
          test      cs:LMFlags,IGNORE23 ;ignore ^Cs?
           jnz      IgnorBrk            ;yes, go ignore it
          test      cs:LMFlags,INSHELL  ;are we in the shell?
           jnz      LocalBrk            ;if so handle separately
          bset      cs:LMFlags,BRKOCCUR ;set break flag
          stc                           ;abort program on return to dos
          jmp       short BreakRet      ;and return
          ;
IgnorBrk: clc                           ;show no error
          ;
BreakRet: sti                           ;allow interrupts
          ret                           ;and return
          ;
Int23     endp                          ;that's all there is to it
          ;
          ;
LocalBrk: push      cs:ServSeg          ;segment to go to on stack
          push      cs:ServBrk          ;offset on stack
          retf                          ;go to high-memory signal handler
          ;
          ;
          ; INT24 support routines
          ;
          ;
          ; PRTERR - Print error message
          ;
          ; On entry:
          ;         DI = error number
          ;
          ; On exit:
          ;         AX destroyed
          ;         All other registers and interrupt state unchanged
          ;
          entry     PrtErr,varframe,,local  ;print error message and prompt
          ;
          var       OutBuf,ERRMAX       ;output buffer on stack
          varend
          ;
          pushm     bx,si,ds            ;save prompt message address, ds
          mov       ax,di               ;copy error number
          loadseg   es,ss               ;get buffer segment
          lea       di,OutBuf           ;get buffer offset
          pushm     di,es               ;save output address
          lds       si,ShellInf.ShCETAdr  ;get message table address
          call      dword ptr EMOffset  ;get critical error message
          xor       al,al               ;get null
          stosb                         ;store null at end of message
          popm      ds,si               ;point to output buffer
          call      PrtMsg              ;print error message
          popm      ds,si,bx            ;get back prompt address, ds
          exit                          ;all done
          ;
          ;
          ; PRTMSG - Print a message using BIOS output
          ;
          ; On entry:
          ;         DS:SI = address of null-terminated message
          ;
          ; On exit:
          ;         AX destroyed
          ;         DS:SI = offset of last byte of message + 1
          ;         All other registers and interrupt state unchanged
          ;
          entry     PrtMsg,noframe,,local  ;print a message to screen
          ;
          pushm     bx,cx               ;save registers
          ;
prt_loop: lodsb                         ;load a character
          or        al,al               ;end of message?
           je       PMDone              ;if so exit
;          mov       ah,0Eh              ;get write character function
;          xor       bx,bx               ;page 0
;          int       10h                 ;output the character
          mov       dl,al               ;copy byte to DL for INT 21
          calldos   DISPCHR             ;output the character
          jmp       short prt_loop      ;and loop
          ;
PMDone:   popm      cx,bx               ;restore registers
          exit                          ;all done
          ;
          ;
@curseg   ends                          ;close code seg
          ;
          end                           ;that's all folks

