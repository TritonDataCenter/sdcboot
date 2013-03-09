; ---------------------------------------------------------------------------
; DRVTYPES.ASM  Drive type detection unit.                       NASM version.
;
; *** PUBLIC DOMAIN ***
; *** THERE ARE NO WARRANTIES PROVIDED AND AS-IS BASIS IS ASSUMED ***
;
; Initially written, placed in public domain, detection of Norton Diskreet
;  AND SuperStor drives in assembly language by Mr. Byte.
; Network drive detection fix, Stacker 4, DoubleDisk AND
;  Bernoully drive detection by Bobby Z.
; RAM drive detection by Janis Smits.
; Diskreet, SuperStor detection code donated by Vitaly Lysenko
; Port to NASM by Imre Leber
; ---------------------------------------------------------------------------
; History:
;
;        mid November, 1994  -  initially released
;         29 November, 1994  -  Fixed network drive detection AND added
;                               Stacker 4 drive detection
;         10 December, 1994  -  added Janis's RAM drive detection method
;         16 January,  1995  -  added Vertisoft DoubleDisk 2.6 drive detection
;         31 January,  1995  -  added Bernoully drive detection
;          4 February, 1995  -  added Norton Diskreet drive detection
;          5 February, 1995  -  added SuperStor drive detection algo
;          6 February, 1995  -  Fixed bug with detection of CD-ROM drives,
;                               thanx to Ralf Quint
;          7 March,    1995  -  released version 2. Mr. Byte made some
;                               modifications to the entire code
;          2 February  2000  -  NASM port for use with the FreeDOS project.
;
;         17 March,    2002  -  Moved to large model.

			   ;INTERFACE

%assign dtError     00h    ;; Invalid drive, letter not assigned
%assign dtFixed     01h    ;; Fixed drive
%assign dtRemovable 02h    ;; Removeable (floppy, etc.) drive
%assign dtRemote    03h    ;; Remote (network) drive
%assign dtCDROM     04h    ;; MSCDEX V2.00+ driven CD-ROM drive
%assign dtDblSpace  05h    ;; DoubleSpace compressed drive
%assign dtSUBST     06h    ;; SUBST'ed drive
      ;; dudes, where are Stacker 1,2,3 check-routines?
%assign dtStacker4  08h    ;; Stacker 4 compressed drive
%assign dtRAMDrive  09h    ;; RAM drive
%assign dtDublDisk  0Ah    ;; Vertisoft DoubleDisk 2.6 compressed drive
%assign dtBernoully 0Bh    ;; IOmega Bernoully drive
%assign dtDiskreet  0Ch    ;; Norton Diskreet drive
%assign dtSuperStor 0Dh    ;; SuperStor compressed drive

%assign FALSE 0
%assign TRUE  1

segment DRVTYPES_DATA

	CheckABforStacker DB FALSE
	Boot    times 512 DB 0

	DrvName DB '@DSKREET'       ; ;-Diskreet driver name


;;Controls whether we should check drives A: AND B: for Stacker volumes.
;;There is no convenient way to determine if drive is Stacker volume (like
;;for DoubleSpace/Drive*Space volumes), so we should use absolute disk read
;;Function  to do it. Drives A: AND B: are typically floppy drives, thus one
;;may not want to check them causing reads from floppies (which could be
;;not inserted at all). /Bobby Z., 29/11/94

;; char GetDriveType     (char Drive);
;; char FloppyDrives     (void);
;; char CountValidDrives (void);


;;                               IMPLEMENTATION
;;
;; struct ControlBlk25 {
;;        long     StartSector;
;;        unsigned Count;
;;        unsigned BufferOfs;
;;        unsigned BufferSeg;
;; }
;;
		      CB
	  StartSector DD 0
	  Count       DW 0
	  BufferOffs  DW 0
	  BufferSeg   DW 0

			 Packet
	  Header times 6 DB 0
	  Drive          DB 0
	  Size           DD 0


	                Packet1
	  Sign          dw 0
	  Sign1         dw 0
	  p             dw 0
	  Res   times 4 db 0


segment DRVTYPES_TEXT 

;=========================================================================
;===                             CheckStacker4                         ===
;===-------------------------------------------------------------------===
;=== int CheckStacker4 (char Drive);                                   ===
;===                                                                   ===
;=== returns TRUE if Drive is Stacker 4 compressed volume,             ===
;=== FALSE otherwise.                                                  ===
;=== This also may return TRUE with previous versions of Stacker -     ===
;=== I didn't check it. /Bobby Z. 29/11/94                             ===
;=========================================================================
	 
	 global _CheckStacker4
_CheckStacker4:

	 push  bp
	 mov   bp, sp
	 push  si
	 push  di

	 push  ds
	 
         mov   ax, DRVTYPES_DATA
         mov   ds, ax
         mov   al, [bp+06h]                 ;;drive
         cmp   [CheckABforStacker], byte 1  ;; check A: & B: for Stacker volume?
	 
         jz    .L1
	 cmp   al, 1
	 ja    .L1
	 sub   ax, ax
	 jmp   .Q
.L1:
	 mov     bx, CB
	 sub     ax, ax
	 mov     [StartSector], word 0
	 mov     [StartSector+02h], word 0
	 mov     [Count], word 1
	 mov     dx, Boot
	 mov     [BufferOffs], dx
	 mov     [BufferSeg], ds
	 mov     al, [bp+06h]
	 sub     cx, cx
	 dec     cx
	 mov     si, sp
	 int     25h
	 cli
	 mov     sp, si
	 sti
	 pushf
	 mov     si, Boot
	 add     si, 1F0h                           ;; Stacker signature CD13CD14CD01CD03 should
	 sub     ax, ax                             ;; appear at offset 1F0 of boot sector.
	 popf
	 jc      .Q                                 ;; was error reading boot sector - assume
						    ;; not Stacker drive
	 cmp     [si], word 13cdh
	 jnz     .Q
	 cmp     [si+02h], word 14cdh
	 jnz     .Q
	 cmp     [si+04h], word 01cdh
	 jnz     .Q
	 cmp     [si+06h], word 03cdh
	 jnz     .Q
	 mov     ax, 1
.Q:
	 pop     ds
	 pop     di
	 pop     si
	 pop     bp
	 retf


;=========================================================================
;===                             CheckDiskreet                         ===
;===-------------------------------------------------------------------===
;=== int CheckDiskreet (char Drive);                                   ===
;===                                                                   ===
;=== returns TRUE if Drive is Norton Diskreet drive, otherwise it      ===
;=== returns FALSE.                                                    ===
;=========================================================================

	global _CheckDiskreet
_CheckDiskreet:

	push    bp
	mov     bp, sp
	push    si
	push    di

	push    ds
	mov     ax, 0FE00h
	mov     di, 'NU'        ; 4E55h='NU'
	mov     si, 'DC'        ; 4443h='DC'
	int     2fh
	or      al, al          ; check for zero
	je      .L2
	cmp     al, 1           ; check for 1
	je      .L2
.L1:
	sub     al, al          ; return FALSE
	jmp     .L4
.L2:
	mov     ax, DRVTYPES_DATA
        mov     ds, ax
        mov     dx, DrvName
	mov     ax, 3D02h
	int     21h
	jc      .L1
	mov     bx, ax
	mov     dx, Packet
	push    ds
	pop     es
	mov     di, dx
	mov     cx, 11          ;; Type tDiskreetPacket
	sub     al, al
	cld
	rep     stosb           ; initialize Packet fields
	mov     di, Header
	mov     ax, 12ffh       ; store first two bytes in Packet header
	stosw
	mov     di, Drive
	mov     al, [bp+06h]
	add     al, 64          ; convert drive number to drive letter
	stosb                   ; store drive letter
	mov     ax, 4403h       ; ready to send Diskreet Packet
	mov     cx, 7
	mov     si, 'dc'        ; 6463h = 'dc'
	mov     di, 'NU'        ; 4E55h = 'NU'
	int     21h             ; assuming DS=SEG [Packet], dx=offset [Packet],
				; bx=Handle
	mov     ah, 3Eh
	int     21h             ; close device
	mov     si, Size
	lodsw
	or      ax, ax
	jnz     .L3
	lodsw
	or      ax, ax
	jz      .L1
.L3:
	mov     al, TRUE        ; return TRUE
.L4:
	pop     ds
	pop     di
	pop     si
	pop     bp
	retf

;=========================================================================
;===                             CheckSuperStor                        ===
;===-------------------------------------------------------------------===
;=== int CheckSuperStor(char Drive);                                   ===
;===                                                                   ===
;=== returns TRUE if Drive is super stor drive, otherwise it           ===
;=== returns FALSE.                                                    ===
;=========================================================================
	
	global _CheckSuperStor
_CheckSuperStor:

	push    bp
	mov     bp, sp
	push    si
	push    di
	push    ds

        mov     ax, DRVTYPES_DATA
        mov     ds, ax
        
	mov     [Sign],  word 0AA55h
	mov     [Sign1], word 0201h

	mov     ax, 4404h
	mov     dx, Packet1
	mov     cx, 12
	mov     bl, [bp+06h]
	int     21h

	jc      .L02

	cmp     [Sign], word 0
	jne     .L02

	cmp     [Sign1], word 0201h
	jne     .L02

	les     di, [p]
	mov     ax, [es:di+5dh]
	test    ax, 40h
	jz      .L02

	mov     cl, [es:di+24h]
	add     cl, 'A'
	mov     ah, 30h
	int     21h
	cmp     ah, 04h
	jb      .L01
	inc     di

.L01:
	les     di, [es:di+05fh]
	mov     bl, [es:di]
	add     bl, 'A'
	cmp     cl, [bp+06h]

	jne     .L02
	mov     al, TRUE

	jmp     .L03

.L02:
	sub     ax, ax

.L03:
	pop     ds
	pop     di
	pop     si
	pop     bp
	retf

;==========================================================================
;===                         GetDriveType                               ===
;===--------------------------------------------------------------------===
;=== int GetDriveType (char drive);                                     ===
;===                                                                    ===
;=== Detects the type for a specified drive. Drive is a drive number to ===
;=== detect the type for (0=detect current (default) drive,             ===
;=== 1=A, 2=B, 3=C...)                                                  ===
;===                                                                    ===
;=== Returns: One of the dtXXX-constants.                               ===
;===                                                                    ===
;===  Note: Function  will work under DOS version 3.L30 or later        ===
;===        Also should work under DPMI AND Windows.                    ===
;==========================================================================

	global _GetDriveType
_GetDriveType:

	push    bp
	mov     bp, sp
	push    si
	push    di
        push    ds

	cmp     [bp+06h], word 0
	jne     .L1
	mov     ah, 19h                     ; get active drive number in AL
	int     21h
	xor     ah, ah
	mov     [bp+06h], ax
	inc     word [bp+06h]
.L1:
	mov     ax, [bp+06h]
	push    ax
        push    cs
	call    _CheckDiskreet
	pop     bx
	or      al, al
	jz      .CDROMcheck
	mov     bl, dtDiskreet
	jmp     .L7
.CDROMcheck:
	mov     ax, 1500h       ; check for CD-ROM v2.00+
	sub     bx, bx
	int     2Fh
	or      bx, bx
	jz      .L2
	mov     ax, 150Bh
	sub     ch, ch
	mov     cl, [bp+06h]
	dec     cl              ; bug Fixed with CD-ROM drives, thanx to Ralf Quint
	int     2fh             ; drives for this Function  start with 0 for A:
	cmp     bx, 0adadh
	jne     .L2
	or      ax,ax
	jz      .L2
	mov     bl, dtCDROM
	jmp     .L7
.L2:
	mov     ax, 4409h       ; check for subST'ed drive
	mov     bl, [bp+06h]
	int     21h
	jc      .DblSpaceChk
	test    dh, 80h
	jz      .DblSpaceChk
	mov     bl, dtSUBST
	jmp     .L7

.DblSpaceChk:
	mov     ax, 4a11h       ; check for DoubleSpace drive
	mov     bx, 1
	mov     dl, [bp+06h]
	dec     dl
	int     2fh
	or      ax, ax          ; is DoubleSpace loaded?
	jnz     .L3
	cmp     dl, bl          ; if a host drive equal to compressed, then get out...
	je      .L3
	test    bl, 80h         ; bit 7=1: DL=compressed,BL=host
				;      =0: DL=host,BL=compressed
	jz      .SStorChk       ; so avoid host drives, assume host=Fixed :)
	inc     dl
	cmp     [bp+06h], dl
	jne     .SStorChk
	mov     bl, dtDblSpace
	jmp     .L7
.SStorChk:
	mov     ax, [bp+06h]
	push    ax
        push    cs
	call    _CheckSuperStor
	pop     bx
	or      al, al
	jz      .L3
	mov     bl, dtSuperStor
	jmp     .L7
.L3:
	mov     ax, 4409h     ; check for remote drive
	mov     bx, [bp+06h]
	int     21h
	jnc     .NL5
	jmp     .L5
.NL5:
	and     dh, 10h
	jz      .L4
	mov     bl, dtRemote
	jmp     .L7
.L4:
	mov     al, [bp+06h]  ; check for Stacker 4 volume
	or      al, al
	jnz     .NGetDrv
	jmp     .GetDrv
.NGetDrv:

	dec     al
.GoStac:
	push    ax
        push    cs
	call    _CheckStacker4
	pop     bx
	or      al,al
	jz      .8
	mov     bl, dtStacker4
	jmp     .L7
.8:
	mov     ax, 4408h       ; check for Fixed (hard) drive
	mov     bx, [bp+06h]
	int     21h
	jnc     .NL05
	jmp     .L5
.NL05:
	or      al, al
	jnz     .NL6
	jmp     .L6
.NL6:
	push    ds              ; check for RAM drive
	mov     ax, SS
	mov     ds, ax
	mov     si, sp
	sub     sp, 28h         ; allocate 28h bytes on stack
	mov     dx, sp
	mov     ax, 440Dh       ; generic IOCTL
	mov     cx, 860h        ; get device parameters
	int     21h             ; RAMDrive AND VdiSK don't support this command
	jc      .CleanUp
	pushf
	mov     di, dx
	cmp     [di+6], byte 0F8h ; DoubleDisk returns 0F8h in media type
	jz      .DublDsk          ; field of BPB if drive in question is
				  ; compressed
	popf
	jmp     .CleanUp
.DublDsk:
	popf
	mov     bl, dtDublDisk
	mov     sp, si
	pop     ds
	jmp     .L7
.CleanUp:
	mov     sp, si
	pop     ds
	mov     bl, dtRAMDrive
	jc      .L7
	push    ds
	mov     ah, 1Ch         ; this Function  works _really_ slowly
	mov     dl, [bp+06h]    ; get media descriptor Pointer
	int     21h
	cmp     [bx], byte 0fdh
	pop     ds
	jnz     .Fixed
	push    ds
	mov     ah, 32h         ; get BPB Pointer
	mov     dl, [bp+06h]
	int     21h
	cmp     [bx+0Bh], byte 2 ; Sectors per FAT is more than 2 for
	pop     ds               ; Bernoully drives
	jz      .Fixed
	mov     bl, dtBernoully
	jmp     .L7
.Fixed:
	mov     bl, dtFixed
	jmp     .L7
.L5:
	sub     bl, bl          ; mov BL, dtError cuz dtError=0
	jmp     .L7
.GetDrv:
	mov     ah, 19h
	int     21h
	jmp     .GoStac
.L6:
	mov     bl, dtRemovable ; else - removeable media
.L7:
	mov     al, bl
	xor     ah, ah
	
        pop     ds
	pop     di
	pop     si
	pop     bp
	retf
%if 0        
;========================================================================
;===                         CountValidDrives                         ===
;===------------------------------------------------------------------===
;=== int CountValidDrives(void);                                      ===
;===                                                                  ===
;=== returns number of assigned letters in system.                    ===
;========================================================================
	
	global _CountValidDrives        
_CountValidDrives:        

	push    bp
	mov     bp, sp

	mov     dx, 1
	mov     cx, 26
.L1:
	push    cx
	push    dx
	
	push    dx
        push    cs
	call    _GetDriveType
        pop     dx                              ;; Pop arg from stack
	pop     dx
	pop     cx

	or      al, dtError
	jz      .L2                            ;; Remember: %assign dtError 0

        inc     dx
	loop    .L1
.L2:
	dec     cx
	jnz     .L1

	mov     ax, dx
	
	pop     bp
	retf

;======================================================================= 
;===                         FloppyDrives                            ===
;===-----------------------------------------------------------------===
;=== int FloppyDrives (void);                                        ===
;===                                                                 ===
;=== returns number of floppy drives in system                       ===
;=======================================================================
	
	global _FloppyDrives        
_FloppyDrives:        
	
	int     11h
	test    al, 1
	jz      .L1
	
	mov     cl, 6                 ;; 8086 code.
	shr     al, cl

	and     al, 3
	inc     al
	xor     ah, ah
	ret
.L1:
	xor     ax, ax
	retf
%endif
