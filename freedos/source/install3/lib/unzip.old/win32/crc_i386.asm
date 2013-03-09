; crc_i386.asm, optimized CRC calculation function for Zip and UnZip, not
; copyrighted by Paul Kienitz and Christian Spieler.  Last revised 19 Jan 96.
;
; FLAT memory model assumed.
;
; The loop unroolling can be disabled by defining the macro NO_UNROLLED_LOOPS.
; This results in shorter code at the expense of reduced performance.
;
;==============================================================================
;
; Do NOT assemble this source if external crc32 routine from zlib gets used.
;
    IFNDEF USE_ZLIB
;
        .386p
        name    crc_i386
        .MODEL  FLAT

extrn   _get_crc_table:near    ; ulg near *get_crc_table(void);

;
    IFNDEF NO_STD_STACKFRAME
        ; Use a `standard' stack frame setup on routine entry and exit.
        ; Actually, this option is set as default, because it results
        ; in smaller code !!
STD_ENTRY       MACRO
                push    ebp
                mov     ebp,esp
        ENDM

        Arg1    EQU     08H[ebp]
        Arg2    EQU     0CH[ebp]
        Arg3    EQU     10H[ebp]

STD_LEAVE       MACRO
                pop     ebp
        ENDM

    ELSE  ; NO_STD_STACKFRAME

STD_ENTRY       MACRO
        ENDM

        Arg1    EQU     18H[esp]
        Arg2    EQU     1CH[esp]
        Arg3    EQU     20H[esp]

STD_LEAVE       MACRO
        ENDM

    ENDIF ; ?NO_STD_STACKFRAME

; This is the loop body of the CRC32 cruncher.
; registers modified:
;   ebx  : crc value "c"
;   esi  : pointer to next data byte "text++"
; registers read:
;   edi  : pointer to base of crc_table array
; scratch registers:
;   eax  : requires upper three bytes of eax = 0, uses al
Do_CRC  MACRO
                lodsb                        ; al <-- *text++
                xor     al,bl                ; (c ^ *text++) & 0xFF
                shr     ebx,8                ; c = (c >> 8)
                xor     ebx,[edi+eax*4]      ;  ^ table[(c ^ *text++) & 0xFF]
        ENDM

_TEXT   segment para

        public  _crc32
_crc32          proc    near       ; ulg crc32(ulg crc, uch *text, extent len)
                STD_ENTRY
                push    edi
                push    esi
                push    ebx
                push    edx
                push    ecx

                mov     esi,Arg2             ; 2nd arg: uch *text
                test    esi,esi
                jne     short Crunch_it      ;> if (!text)
                sub     eax,eax              ;>   return 0;
    IFNDEF NO_STD_STACKFRAME
                jmp     short fine           ;>
    ELSE
                jmp     fine                 ;>
    ENDIF
; align destination of commonly taken jump at longword boundary
                align   4
Crunch_it:                                   ;> else {
                call    _get_crc_table
                mov     edi,eax
                mov     ebx,Arg1             ; 1st arg: ulg crc
                sub     eax,eax              ; eax=0; make al usable as a dword
                mov     ecx,Arg3             ; 3rd arg: extent textlen
                not     ebx                  ;>   c = ~crc;
                cld                          ; incr. idx regs on string ops

    IFNDEF  NO_UNROLLED_LOOPS
                mov     edx,ecx              ; save textlen in edx
                shr     ecx,3                ; ecx = textlen / 8
                and     edx,000000007H       ; edx = textlen % 8
                jecxz   No_Eights
; align loop head at start of 486 internal cache line !!
                align   16
Next_Eight:
                Do_CRC
                Do_CRC
                Do_CRC
                Do_CRC
                Do_CRC
                Do_CRC
                Do_CRC
                Do_CRC
                loop    Next_Eight
No_Eights:
                mov     ecx,edx
    ENDIF ; NO_UNROLLED_LOOPS
                jecxz   bail                 ;>   if (textlen)
; align loop head at start of 486 internal cache line !!
                align   16
loupe:                                       ;>     do {
                Do_CRC                       ;        c = CRC32(c, *text++);
                loop    loupe                ;>     } while (--textlen);

bail:                                        ;> }
                mov     eax,ebx
                not     eax                  ;> return ~c;
fine:
                pop     ecx
                pop     edx
                pop     ebx
                pop     esi
                pop     edi
                STD_LEAVE
                ret
_crc32          endp

_TEXT   ends
;
    ENDIF ;!USE_ZLIB
;
end
