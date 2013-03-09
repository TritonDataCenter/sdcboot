;                                                                           ;
;                       DELTREE.S  v1.02f  2005-04-06                       ;
;                       Copyright 1998-2003    C. Dye                       ;
;                       email:   raster@highfiber.com                       ;
;                                                                           ;
;       This is source for NASM, the Netwide Assembler.  Type               ;
;       NASM DELTREE.S -O DELTREE.COM  to re-assemble.  NASM is             ;
;       freeware, available from  http://www.cryogen.com/Nasm               ;
;       Safer deltree: NASM DELTREE.S -DDEFANGED -O DELTREE.COM             ;
;                                                                           ;
;       This program is copyrighted, but may be freely distributed          ;
;       under the terms of the Free Software Foundation's GNU General       ;
;       Public License v2 (or later.)  See the file COPYING for the         ;
;       legalities.  If you did not receive a copy of COPYING, you          ;
;       may request one from the Free Software Foundation, Inc.,            ;
;       59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.           ;
;       ABSOLUTELY NO WARRANTY -- use it at your own risk!                  ;
;                                                                           ;

%macro doscall 1                       ; call to dos int 21 with a one-byte
%if %1 >= 0100h                        ; value in .ah, or a two-byte value
mov ax,%1                              ; in .ax :
%else
mov ah,%1
%endif
int 21h
%endmacro

%macro dosprint 1                      ; a simple wrapper for the dos 21/09
mov dx,%1                              ; print-a-string function
mov ah,09h
int 21h
%endmacro

%macro zero 1                          ; cheap way to zero a two-byte
xor %1,%1                              ; register
%endmacro

%macro testzero 1                      ; test 16-bit register against zero
or %1,%1
%endmacro

%macro zprint 1                        ; a wrapper for a call to my asciiz-
mov di,%1                              ; print routine
call zprint1
%endmacro

%macro bomb 2                          ; jump to handler for fatal errors
call bombs_away                        ; ... does not return!
dw %1                                  ; inline:  address of error message
db %2                                  ; inline:  exit code (errorlevel)
%endmacro

%define hilo(a,b) (a * 100h + b)       ; easy way to code two-byte quantities


org 0100h


maxlen   equ  0102h                    ; maximum length of a filename

attr_d   equ  10h                      ; value of directory attribute
attr_c   equ  40h                      ; value of character-device attribute

SPC      equ  20h                      ; ascii space
BS       equ  08h                      ; ascii backspace
CR       equ  0dh                      ; ascii carriage return
LF       equ  0ah                      ; ascii line feed


;  Comment out the following line to remove the /Y switch :
%ifndef DEFANGED
%define Enable_Sw_Y
%endif

;  Comment out the following line to remove the /V switch :
%define Enable_Sw_V

;  Comment out the following line to remove the /D switch :
%define Enable_Sw_D

;  Comment out the following line to remove the root-directory safety check :
%define MungaBunga

;  Comment out the next %define to permit switches to follow filespecs on
;  the command line.  (Warning:  Makes DELTREE.COM syntax-incompatible with
;  Microsoft's, with possible Bad Results!  Not recommended.)
%define SwitchesOnlyAtStart

;  Comment out the next line to remove support for ancient DOS-C versions.
; %define DOSC                         ; set to 1 to support 1998/older DOS-C


; ------------------------------------ UNINITIALIZED VARIABLES IN PSP :

country  equ  005ch                    ; put country data in the psp
country_thou equ country + 07h         ; local thousands char (',' in usa)
country_deci equ country + 09h         ; local decimal point  ('.' in usa)

tempvar  equ  0070h                    ; variable used for sundry stuff

dp_t1    equ  0072h                    ; low word,  value to print as decimal
dp_t2    equ  0074h                    ; high word, value to print as decimal


; -------------------------------------- START OF CODE :

begin:
cld
doscall 3000h                          ; check ms-dos version:
cmp al,02h
ja dos_okay                            ; dos version prior to v3 :
bomb msg_err_dos_bad,12h               ; complain and exit with errorlevel 18
dos_okay:
%ifdef DOSC	; "1998/older" DOS-C (FreeDOS) compatibility stuff
cmp bh,0fdh                            ; running under freedos?
jne not_old_fd                         ; if not, never mind
		; jcxz fd_fix          ; (i don't think this ever happens)
		; REMOVED 2005: actually CX being 0 is the NORMAL case in
		; newer versions. Only CX being -1 means "old DOS-C"
cmp cx,byte -01h                       ; older beta release?
jne not_old_fd                         ; if not, never mind
fd_fix:
mov word [subdir_clear],attr_d         ; old freedos kernel, work around bug
not_old_fd:
%endif		; /DOS-C compatibility stuff
mov ax,sp                              ; examine stack pointer:
cmp ax,stack_end                       ; plenty of room?
jae trs_80.l1
trs_80:                                ; not enough memory:
bomb msg_trs_80,11h                    ; complain and exit with errorlevel 17
.l1:
mov sp,stack_end                       ; reduce stack size
zero ax                                ; push zero word onto stack
push ax                                ; for possible exit via ret
push cs
pop es                                 ; get program segment into .es
mov bx,(stack_end / 10h) + 1h
doscall 4ah                            ; and shrink program's memory block
jc trs_80                              ; any error, assume not enough memory

call alloc_list_buffer                 ; allocate buffer for file list
call get_country_info                  ; get current country info
mov dx,dta
doscall 1ah                            ; use primary disk transfer area
mov si,0080h                           ; read from command line

parse_main:                            ; PRIMARY PARSER LOOP :
inc si
switch_done:
mov al,[si]                            ; examine next character
call test_eol                          ; end of line?
je parse_done                          ; if so, terminate primary parser
call test_space                        ; is it a space?
je parse_main                          ; if so, ignore it
%ifdef SwitchesOnlyAtStart
cmp word [parse_save],byte 0000h       ; have we found any filespecs yet?
jne .l1                                ; if so, don't check for switches
%endif ; SwitchesOnlyAtStart
call test_switch                       ; is it a switch character?
je found_switch                        ; if so, interpret it
.l1:                                   ; found the start of a filespec :
call parse_fn                          ; parse through the filespec
mov [parse_save],si                    ; and save parse index
call handle_filespec                   ; delete item(s) as needed
mov si,[parse_save]                    ; restore the parse index
mov al,[si]
call test_eol                          ; found the end of the line yet?
je parse_done                          ; if so, terminate primary parser
jmp short parse_main                   ; otherwise, keep on truckin'

found_switch:                          ; FOUND A SWITCH CHARACTER :
inc si
lodsb                                  ; examine next character
call force_lc                          ; in lowercase :
mov bx,0ffffh                          ; start at beginning of switch table
.l0:
inc bx
cmp byte [switches+bx],00h             ; run out of legal switches to try?
je syntax                              ; if so, syntax error
cmp byte [switches+bx],al              ; found this letter in table?
jne .l0                                ; if not, keep looking
shl bx,01h                             ; multiply .bx by two
mov ax,[switch_routines+bx]            ; and get address of switch routine
jmp ax                                 ; to jump to

syntax:                                ; SYNTAX ERROR IN COMMAND LINE :
call error_out                         ; use stderr for output, fall through

switch_query:                          ; /? -- REQUEST SYNTAX DISPLAY :
dosprint msg_syntax                    ; print error message
doscall 4c10h                          ; and exit with errorlevel 16

parse_done:                            ; DONE PARSING COMMAND LINE :
cmp word [parse_save],byte 0000h       ; found any filespecs at all?
jne exit_program                       ; if not,
bomb msg_err_no_specs,10h              ; complain and exit with errorlevel 16
exit_program:                          ; if so,
%ifdef Enable_Sw_V
call show_report                       ; display damage report if desired
%endif ; Enable_Sw_V
mov al,[return_code]                   ; return success or failure errorlevel
doscall 4ch                            ; and exit to dos


handle_filespec:                       ; DELETE ITEM(S) AS NEEDED :
call prompt_fix                        ; force output to console for prompts
call clean_up_filespec                 ; canonicalize filespec
filespec_loop:
call get_fn_from_list                  ; read filespec from list, if needed
jc safety_abort.quit                   ; if eof, exit
%ifdef MungaBunga
%ifdef Enable_Sw_Y
call root_safety                       ; check for extra-dangerous commands
jne safety_abort
%endif ; Enable_Sw_Y
%endif ; MungaBunga
subdir_loop:
call find_multiple_files               ; find files matching this filespec
jnc subdir_loop                        ; if so, alter any files in it
safety_abort:
test byte [flags],fb_list              ; using a file list?
jne filespec_loop                      ; if so, get next filespec from it
.quit:
ret                                    ; if not, exit

%ifdef Enable_Sw_V

show_report:                           ; REPORT ON TOTAL DESTRUCTION :
test byte [flags],fb_sw_v              ; was /v specified on command line?
je safety_abort.quit                   ; if not, just exit
dosprint msg_num_deleted               ; print 'files deleted' message
mov ax,[deleted+0]                     ; get the number of files squiffed
mov dx,[deleted+2]                     ; low word / high word
call dec_print_big                     ; and display it
call show_totals                       ; show total of file sizes, if needed
dosprint msg_num_removed               ; print 'subdirs removed' message
mov ax,[removed+0h]                    ; get number of directories removed
mov dx,[removed+2h]                    ; low word / high word
call dec_print_big                     ; and display it
jmp crlf                               ; finish off the line and exit

show_totals:                           ; DISPLAY TOTAL OF SIZES AS KB OR MB :
mov ax,[deleted+0h]                    ; were any files at all deleted?
or  ax,[deleted+2h]                    ; if not,
je show_size_kb.quit                   ; exit without further comment
dosprint msg_total_sizes               ; print 'total of sizes' string
mov ax,[total_sizes+0h]                ; get total of file sizes
mov dx,[total_sizes+2h]                ; low word / high word
call dec_print_big                     ; and display it
dosprint msg_bytes                     ; 'bytes'
mov ax,[total_sizes+0h]                ; get total of file sizes
mov dx,[total_sizes+2h]                ; low word / high word
cmp dx,byte 0010h                      ; is the file 1 mb or larger?
jae show_size_mb                       ; if so, show file size in megs
show_size_kb:                          ; otherwise, report file size in kb :
mov cl,0ah
shr ax,cl                              ; divide low word by 1024
mov cl,06h
shl dx,cl                              ; get most significant bits
or ax,dx
call dec_print_small                   ; and display kb value
mov ax,[total_sizes+0h]                ; get fractional kb
shr ax,1h
shr ax,1h                              ; in al (0 to 255)
call show_fraction                     ; and display fractional kb
dosprint msg_kilobytes                 ; show 'kilobytes' message
.quit:
ret
show_size_mb:                          ; report file size in mb :
mov ax,[total_sizes+2]
mov cl,04h
shr ax,cl                              ; file size in mb
call dec_print_small                   ; display mb value
mov ax,[total_sizes+0h]
mov dx,[total_sizes+2h]
mov cl,0ch
shr ax,cl
mov cl,04h
shl dx,cl
or ax,dx                               ; fractional mb value (0-255) in al
call show_fraction                     ; display fractional mb
dosprint msg_megabytes                 ; show 'megabytes' message
ret

show_fraction:                         ; al contains a value 0-255 :
push ax                                ; save it
mov dl,[country_deci]
doscall 02h                            ; display a decimal point
pop ax                                 ; al contains 0-255 :
mov ah,00h                             ; convert to word length
mov bl,64h
mul bl                                 ; multiply by 100
test al,80h                            ; need to round up?
je show_frac_fix                       ; no, skip ahead
cmp ah,063h                            ; would rounding up crack 100?
jae show_frac_fix                      ; if so, forget it (lazy bastard!)
inc ah                                 ; round fraction up
show_frac_fix:
mov al,ah                              ; divide by 256
cbw
aam                                    ; convert to decimal digits
add ax,hilo ('0','0')                  ; and convert those to ascii
push ax                                ; save ones digit
mov dl,ah
doscall 02h                            ; print tens digit
pop dx                                 ; retrieve ones digit
int 21h                                ; and print that
ret                                    ; exit

%endif   ; Enable_Sw_V

parse_fn:                              ; READ FILESPEC FROM COMMAND LINE :
and byte [flags],(fb_quote | fb_list) ^ 0ffh   ; turn off quote-mode, at-mode
mov di,fnbuf                           ; point to filespec buffer
zero cx                                ; no characters yet
mov byte [di],cl                       ; empty the buffer

parse_fn_1:                            ; parse filename loop :
lodsb                                  ; get a character from command line
call force_lc                          ; and force it to lowercase
call test_eol                          ; end of line?
je parse_fn_eol                        ; if so, handle it
call test_space                        ; space or tab?
je parse_fn_space                      ; if so, handle it
cmp al,'"'                             ; quote mark?
je parse_fn_quote                      ; if so, handle it
cmp al,'/'                             ; forward slash?
jne .l2
mov al,'\'                             ; if so, convert to backslash
.l2:
cmp al,'@'                             ; at sign?
jne parse_fn_char                      ; no, treat like a normal character
testzero cx                            ; yes:  got any characters yet?
jne parse_fn_char                      ; yes:  at sign is part of filename
test byte [flags],fb_quote | fb_list   ; quote-mode or at-mode?  if either,
jne parse_fn_char                      ; at sign is part of filename
or byte [flags],fb_list                ; note:  file list
jmp short parse_fn_1                   ; continue parsing

parse_fn_char:                         ; NORMAL CHARACTER IN FILENAME :
mov ah,00h
mov [di],ax                            ; stash it and null terminate buffer
inc di
inc cx                                 ; increment count of characters
cmp cx,maxlen                          ; buffer overflow?
jb parse_fn_1                          ; if not, continue parsing; else
bomb msg_err_fn_ovf,10h                ; complain and exit with errorlevel 16

parse_fn_eol:                          ; FOUND THE END OF COMMAND LINE :
testzero cx                            ; found a filename yet?
jne parse_fn_space.l10                 ; if so, exit
error_empty_filespec:                  ; if not,
bomb msg_err_in_filespec,10h           ; complain and exit with errorlevel 16

parse_fn_space:                        ; FOUND A SPACE IN THE FILESPEC :
test byte [flags],fb_quote             ; parsing between quotes?
jne parse_fn_char                      ; if so, treat like any other char
testzero cx                            ; any characters in filespec yet?
jne .l10
jmp short error_empty_filespec         ; if not, bomb with errorlevel 16
.l10:
dec si                                 ; if so, we're done parsing filespec
ret

parse_fn_quote:                        ; found a quote mark :
test byte [flags],fb_quote             ; has an opening quote been found?
jne parse_close_quote                  ; if so, this is a close quote
testzero cx                            ; is the filename empty?
jne parse_fn_char                      ; if not, quote is part of filename
or byte [flags],fb_quote               ; note that open quote was found
jmp short parse_fn_1                   ; and continue parsing
parse_close_quote:
testzero cx                            ; empty filename?
jne find_multi_err.quit                ; if not, done parsing filespec
jmp short error_empty_filespec         ; if so, bomb out with errorlevel 16


find_multiple_files:                   ; FIND FILES MATCHING CURRENT SPEC :
mov dx,dta
doscall 1ah                            ; use primary disk transfer area
mov dx,fnbuf                           ; point to working filespec
mov cx,0016h                           ; include hidden and system files
doscall 4eh                            ; find first matching file
find_multi_1:
jc find_multi_err.quit                 ; any error, exit loop
cmp byte [dta_name],'.'                ; stupid dos . or .. entry ?
je find_next_file                      ; if so, ignore it
test byte [dta_attr],attr_c            ; name of a character device?
jne find_multi_err                     ; if so, exit
call nuke_item                         ; do it to it!
mov dx,dta
doscall 1ah                            ; return to primary disk transfer area
find_next_file:
doscall 4fh                            ; find next matching file
jmp short find_multi_1                 ; till the cows come home
find_multi_err:
stc
.quit:
ret

nuke_item:                             ; DO WHATEVER TO THE FOUND FILE :
call crlf_maybe                        ; print a crlf if needed
call append_found_filespec             ; use the filename that was found
call prompt_user                       ; prompt user, if necessary
jne find_multi_err.quit                ; if user declined, exit now 
dosprint msg_deleting                  ; print 'deleting' message
call show_fnbuf                        ; and the current filespec
dosprint msg_ellipsis                  ; finish by displaying some dots
test byte [flags],fb_sw_x              ; was /x specified?
jne find_multi_err.quit                ; if so, exit without doing anything
mov dx,alt_dta
doscall 1ah                            ; switch to alternate dta
call fix_last_bs
mov ax,[last_bs]                       ; save value of last_bs so that we
mov [topmost],ax                       ; don't exit the target directory !!!

nuke_loop:                             ; SHERMAN THROUGH GEORGIA :
mov dx,fnbuf
mov cx,0016h
mov ah,4eh                             ; find first matching entry
nuke_reloop:
int 21h                                ; look for item
jc .l05
jmp nuke_something                     ; found something?  if so, go kill it
.l05:
mov di,[last_bs]                       ; if not, target directory is empty
cmp di,[topmost]                       ; in the topmost directory now?
jbe .l20                               ; yes, exit nuke_loop
mov byte [di],00h                      ; no, move up a level
%ifdef Enable_Sw_D
test byte [flags],fb_sw_d              ; was /d specified?
je .l08                                ; if so,
dosprint msg_debug_rmdir               ; print debug info
zprint fnbuf
.l08:
%endif ; Enable_Sw_D
mov dx,fnbuf                           ; point to name of target directory
%ifdef DOSC
mov cx,[subdir_clear]                  ; and clear all attributes
%else
xor cx,cx                              ; and clear all attributes
%endif
doscall 4301h                          ; (should not be necessary, but ....)
jnc .l09
jmp err_rmdir
.l09:
mov dx,fnbuf                           ; point to name of target directory
doscall 3ah                            ; and rmdir it
jnc .l095
jmp err_rmdir
.l095:
add word [removed+0h],byte 0001h
adc word [removed+2h],byte 0000h       ; add one to count of removed subdirs
mov di,[last_bs]
.l10:
dec di                                 ; hunt backwards through filespec
cmp byte [di],'\'                      ; for previous backslash
jne .l10
cmp di,[topmost]                       ; have we backed up out of the target
jbe .l20                               ; area?  if so, time to quit!
mov [last_bs],di                       ; otherwise save new value for last_bs
inc di
mov si,star_dot_star                   ; and append a star-dot-star
call copy_string                       ; to exit to parent directory
%ifdef Enable_Sw_D
test byte [flags],fb_sw_d              ; was /d specified?
je .l18                                ; if so,
dosprint msg_debug_uplevel             ; print debug info
zprint fnbuf
.l18:
%endif ; Enable_Sw_D
jmp nuke_loop                          ; loop back for more destruction
.l20:                                  ; time to quit finding things to kill:
mov di,[topmost]                       ; restore correct value
mov [last_bs],di                       ; to last_bs
mov word [di],hilo (00h,'\')           ; add a backslash and a null
%ifdef Enable_Sw_D
test byte [flags],fb_sw_d              ; was /d specified?
je .quit                               ; if so,
dosprint msg_debug_quit                ; print debug info
%endif ; Enable_Sw_D
.quit:
ret                                    ; and exit
nuke_something:                        ; found something to destroy :
call append_found_alt                  ; append filename to fnbuf
cmp byte [alt_name],'.'                ; stupid dos . or .. entry?
jne nuke_realthing                     ; if so, just skip over it
%ifdef Enable_Sw_D
test byte [flags],fb_sw_d              ; was /d specified?
je .l38                                ; if so,
dosprint msg_debug_ignore              ; print debug info
zprint fnbuf
.l38:
%endif ; Enable_Sw_D
mov ah,4fh                             ; dos find_next -- ignore . or ..
jmp nuke_reloop                        ; loop back for more
nuke_realthing:                        ; found a real file or subdirectory :
test byte [alt_attr],attr_d            ; is this a subdirectory?
je nuke_file                           ; if not, it's a file -- red meat
%ifdef Enable_Sw_D
test byte [flags],fb_sw_d              ; was /d specified?
je .l48                                ; if so,
dosprint msg_debug_subdir              ; print debug info
zprint fnbuf
.l48:
%endif ; Enable_Sw_D
call find_null_fnbuf                   ; find the terminal null
mov [last_bs],di                       ; update last_bs variable
mov si,bs_star_dot_star                ; and tack on a star-dot-star
call copy_string                       ; to enter the subdirectory
jmp nuke_loop                          ; loop back, keep hunting
nuke_file:                             ; found a file to kill :
%ifdef Enable_Sw_D
test byte [flags],fb_sw_d              ; was /d specified?
je .l58                                ; if so,
dosprint msg_debug_file                ; print debug info
zprint fnbuf
.l58:
%endif ; Enable_Sw_D
mov dx,fnbuf                           ; point to name of target directory
zero cx
doscall 4301h                          ; and clear all attributes
jc err_del
mov dx,fnbuf                           ; point to name of target file
doscall 41h                            ; and delete it
jc err_del
add word [deleted+0],byte 0001h
adc word [deleted+2],byte 0000h        ; add one to count of deleted files
mov ax,[alt_size+0]
mov dx,[alt_size+2]                    ; add file size, low-high
add [total_sizes+0],ax                 ; to total of deleted files' sizes
adc [total_sizes+2],dx
mov ah,4fh                             ; dos find-next function
jmp nuke_reloop                        ; loop back for more fun

err_del:                               ; can't delete target file :
mov dx,msg_err_del
jmp err_nuke
err_rmdir:                             ; can't remove target directory :
mov dx,msg_err_rmdir
err_nuke:                              ; can't remove target whatever :
push dx                                ; save offset of the error message
mov [tempvar],ax                       ; save the error code
dosprint msg_err_dos                   ; print 'dos error' message
mov ax,[tempvar]                       ; get the error code
call dec_print_small                   ; and print it as a decimal number
pop dx                                 ; get pointer to error type message
doscall 09h                            ; and display that (to stdout)
call show_fnbuf                        ; display the current filespec
dosprint msg_err_finis                 ; and a close quote
mov ax,[tempvar]                       ; get the error code again
call show_error_string                 ; display error string, if any
call crlf                              ; then terminate the print line
mov di,[topmost]                       ; restore correct value
mov [last_bs],di                       ; to last_bs
mov word [di],hilo (00h,'\')           ; add a backslash and a null
mov byte [return_code],01h             ; set failure flag
or byte [flags],fb_crlf                ; note that a crlf is pending
ret                                    ; exit from nuke_loop

show_error_string:                     ; DISPLAY ERROR STRING, IF ANY :
cmp ah,00h                             ; error number greather than 255?
jne .quit                              ; if so, exit
cmp al,00h                             ; error number exactly zero?
je .quit                               ; if so, exit
mov bx,0ffffh
.l1:
inc bx                                 ; examine next possible error code
mov ah,[error_numbers+bx]
cmp ah,00h                             ; hit the end of the list?
je .quit                               ; if so, exit with no string
cmp al,ah                              ; found the actual error number?
jne .l1                                ; if not, loop back for next
mov cx,bx                              ; move index into count register
mov bx,error_strings                   ; and start at beginning of strings
.l3:
jcxz .l5                               ; found the correct string yet?
.l4:                                   ; if so, display it
inc bx                                 ; if not, scan forward through string
cmp byte [bx],'$'                      ; looking for dollar sign
jne .l4                                ; found it?  no, keep looking
inc bx                                 ; yes, skip over dollar sign
loop .l3                               ; and loop back
.l5:                                   ; have pointer to correct string :
dosprint msg_open_pat                  ; display an open parenthesis
mov dx,bx
doscall 09h                            ; display the error string
dosprint msg_close_pat                 ; and a close parenthesis
.quit:
ret

%ifdef Enable_Sw_Y
switch_y:                              ; SWITCH /Y -- DO NOT PROMPT
or byte [flags],fb_sw_y
jmp switch_done
%else ; Enable_Sw_Y
switch_y  equ  switch_done
%endif ; Enable_Sw_Y

%ifdef Enable_Sw_V
switch_v:                              ; SWITCH /V -- VERBOSE
or byte [flags],fb_sw_v
jmp switch_done
%endif ; Enable_Sw_V

%ifdef Enable_Sw_D
switch_d:                              ; SWITCH /D -- DISPLAY DEBUG INFO
or byte [flags],fb_sw_d
jmp switch_done
%endif ; Enable_Sw_D

switch_x:                              ; SWITCH /X -- DON'T REALLY DELETE
or byte [flags],fb_sw_x
jmp switch_done


dec_print_small:                       ; DISPLAY WORD IN AX
zero dx                                ; zero dx and fall through to ....

dec_print_big:                         ; DISPLAY DOUBLEWORD IN DX:AX
mov [dp_t1],ax                         ; save the doubleword value
mov [dp_t2],dx                         ; for later use
mov bx,000ah                           ; ten decimal is our divisor
push bx                                ; also our end-of-stack marker
mov cl,00h                             ; init digits-between-commas count
dec_print_1:                           ; main decimal division loop
zero dx
mov ax,[dp_t2]                         ; dx:ax contains the high word as quad
div bx                                 ; divide by ten
mov [dp_t2],ax                         ; save new high-order word
mov ax,[dp_t1]                         ; get the old low word
div bx                                 ; and divide by ten
mov [dp_t1],ax                         ; save new low word
add dx,byte '0'                        ; convert remainder to ascii digit
push dx                                ; and save it on the stack
or ax,ax                               ; anything left?
jne dec_print_12                       ; if so, continue with divisions
cmp word [dp_t2],byte 0000h
je dec_print_15                        ; if not, print out digits
dec_print_12:
inc cl
cmp cl,03h                             ; three digits yet?
jne dec_print_1                        ; if not, loop back for next digit
mov cl,00h                             ; yes, zero counter
mov dl,[country_thou]
mov dh,dl                              ; and push a comma on the stack
push dx
jmp short dec_print_1                  ; and loop back for next digit
dec_print_15:                          ; done with divisions :
mov ah,02h                             ; dos print-character function
dec_print_2:                           ; decimal printout loop
pop dx                                 ; get a digit from the stack
cmp dx,byte 000ah                      ; hit the end of stack yet?
je force_lc.quit                       ; if so, exit decimal print routine
int 21h                                ; otherwise, print the digit
jmp short dec_print_2                  ; and loop back for next digit


force_lc:                              ; FORCE CHARACTER IN .AL TO LOWERCASE:
cmp al,'A'
jb .quit
cmp al,'Z'
ja .quit
or al,20h
.quit:
ret

test_switch:                           ; IS CHAR IN .AL A SWITCH CHARACTER?
cmp al,'/'                             ; exit with zero flag set if it is
je .l10
cmp al,'-'
.l10:
ret

test_space:                            ; IS CHARACTER IN .AL A SPACE OR TAB?
cmp al,SPC                             ; exit with zero flag set if it is
je .l10
cmp al,09h
.l10:
ret

test_eol:                              ; IS CHARACTER IN .AL AN EOL OR NULL?
cmp al,CR                              ; exit with zero flag set if it is
je .l10
cmp al,00h
.l10:
ret

clean_up_filespec:                     ; CANONICALIZE FILESPEC :
call fix_last_bs                       ; find final backslash
call must_have_drive                   ; supply drive letter if needed
call must_have_path                    ; supply pathname if needed
call never_ends_in_dots                ; watch out for . and .. specs
call never_ends_in_bs                  ; make sure filespec doesn't end in \
call dots_fix                          ; eliminate any . or .. entries
jmp  no_double_bs                      ; remove any duplicated backslashes
                                       ; and exit

must_have_drive:                       ; MAKE SURE FILESPEC HAS A DRIVE :
cmp word [fnbuf],hilo ('\','\')        ; is this a unc filespec?
je .quit                               ; if so, don't muck about with it
mov ax,word [fnbuf]                    ; examine first two characters:
cmp ah,':'                             ; is the second a colon?
jne .l0                                ; if not, add drive letter
call force_lc
cmp al,'a'
jb .l0                                 ; is the first a letter?
cmp al,'z'                             ; if not, add drive letter
ja .l0                                 ; a drive letter was specified :
cmp byte [fnbuf+2],00h                 ; was there anything after it?
je .l5                                 ; if not, add star-dot-star
ret                                    ; otherwise, exit with no change
.l0:
call find_null_fnbuf                   ; find the end of the filespec
mov si,di                              ; source for copy
add di,byte 0002h                      ; and add 2 (destination)
cmp di,fnbuf + maxlen                  ; will this overflow fnbuf?
jae .l7                                ; if so, problem
.l1:
mov al,[si]                            ; copy a byte from source
dec si
mov [di],al                            ; to destination
dec di
cmp si,fnbuf                           ; done yet?
jae .l1                                ; no, continue
doscall 19h                            ; get current drive number
add al,'a'                             ; and convert to a letter
mov ah,':'
mov [fnbuf+0],ax                       ; stash it in the filename buffer
cmp word [last_bs],byte 0000h          ; if a backslash was found,
je .quit
add word [last_bs],byte 0002h          ; add 2 to its location
.quit:
ret                                    ; and we're done
.l5:                                   ; only a drive letter found :
mov si,star_dot_star
mov di,fnbuf+2h                        ; supply a star-dot-star
jmp copy_string                        ; and exit
.l7:                                   ; adding drive would overflow fnbuf :
jmp error_fnbuf_over                   ; handle error

must_have_path:                        ; SUPPLY PATHNAME IF NEEDED :
cmp word [fnbuf],hilo ('\','\')        ; is this a unc filespec?
je never_ends_in_dots.quit             ; if so, don't muck about with it
cmp byte [fnbuf+2],'\'                 ; did the user supply an abs. path?
je never_ends_in_dots.quit             ; if so, exit without further ado
mov si,fnbuf+2h                        ; starting immediately after colon,
mov di,alt_dta                         ; copy everything to temp buffer
call copy_string
mov byte [fnbuf+2],'\'                 ; poke in initial backslash
mov al,[fnbuf]                         ; get drive letter
call force_lc                          ; as lowercase
mov dl,al
sub dl,60h                             ; convert to a number
mov si,fnbuf + 3h
doscall 47h                            ; get current directory name
call find_null_fnbuf                   ; find the end of the pathname
dec di
cmp byte [di],'\'                      ; does path already end in backslash?
je .l3
inc di
.l3:
mov byte [di],'\'                      ; terminate pathname
inc di
mov si,alt_dta
call copy_string                       ; and copy remainder of filespec back
call fix_last_bs                       ; repair last_bs variable
cmp word [last_bs],fnbuf + maxlen      ; check for possible buffer overflow
jb never_ends_in_dots.quit             ; if no problem, exit
jmp error_fnbuf_over                   ; if problem, go bomb out

never_ends_in_dots:                    ; ELIMINATE . AND .. PROBLEMS :
call find_null_fnbuf                   ; find terminal null
dec di                                 ; examine the last character
cmp byte [di],'.'                      ; is it a dot?
jne .quit                              ; if not, exit
.l1:
dec di                                 ; examine the last character
mov al,[di]
cmp al,'.'                             ; another dot?
je .l1                                 ; if so, ignore it, loop back for more
cmp al,'\'                             ; backslash?
je .l3                                 ; if so, filespec needs attention
cmp al,':'                             ; colon?
jne .quit                              ; if so, filespec needs attention
.l3:                                   ; (anything else is okay)
call find_null_fnbuf                   ; filespec ends in dots, so
mov word [di],hilo (00h,'\')           ; append a backslash
cmp di,fnbuf + maxlen - 1h             ; spam the working buffer?
jb .l8
jmp error_fnbuf_over                   ; yes, go handle it
.l8:
mov [last_bs],di                       ; no, update last_bs variable
.quit:
ret                                    ; and exit

never_ends_in_bs:                      ; FIX PATHNAMES ENDING IN BACKSLASH :
mov di,[last_bs]                       ; get address of final backslash
inc di                                 ; and examine the following char.
cmp byte [di],00h                      ; is it a null?
jne never_ends_in_dots.quit            ; if not, do nothing and exit
mov si,star_dot_star                   ; if so, tack on a star-dot-star
call copy_string                       ; after the backslash
cmp di,fnbuf + maxlen                  ; didn't overflow the buffer, did we?
jna never_ends_in_dots.quit
jmp error_fnbuf_over                   ; oops

dots_fix:                              ; LOOK FOR . OR .. ENTRIES :
mov si,fnbuf                           ; start at beginning of filespec
.l0:
mov ax,[si]                            ; look at two bytes at a time
inc si
cmp al,00h                             ; found the end of the filespec yet?
jne .l1
.l9:                                   ; if so,
jmp fix_last_bs                        ; fix last_bs and exit
.l1:
cmp ax,hilo ('.','\')                  ; found a . entry?
jne .l0                                ; if not, keep looking
mov cx,0001h                           ; one dot found so far
.l2:
inc si                                 ; skip past that first dot
mov al,byte [si]                       ; examine the next character
cmp al,00h                             ; end of filespec?
je .l9                                 ; if so, exit
cmp al,'.'                             ; another dot?
jne .l3
inc cx                                 ; if so, count it
jmp short .l2                          ; and keep parsing
.l3:
cmp al,'\'                             ; another backslash?
jne .l0                                ; if not, ignore this (weird) entry
mov di,si                              ; point .di to backslash after dots
.l4:
dec di                                 ; and scan backwards through fnbuf
cmp di,fnbuf                           ; fell off the beginning of fnbuf?
jbe .l8                                ; if so error
cmp byte [di],'\'                      ; found a backslash?
jne .l4                                ; if not, keep searching backwards
loop .l4                               ; decrement dots count, continue
call copy_string                       ; copy string after dots down
jmp short dots_fix                     ; and loop back for more .\ entries
.l8:                                   ; too many dot entries:
bomb msg_err_dots_fix,15h              ; complain and exit with errorlevel 21

no_double_bs:                          ; remove any duplicated backslashes :
mov di,fnbuf                           ; start at beginning of filename
.l10:
inc di
mov ax,[di]                            ; examine two characters at a time
cmp al,00h                             ; found the end of the filespec yet?
je alloc_list_buffer.quit              ; if so, exit
cmp ax,hilo ('\','\')                  ; duplicate backslashes?
jne .l10                               ; if not, loop back, keep searching
mov si,di                              ; copy down
inc si                                 ; from the second backslash
call copy_string                       ; onto the first
dec word [last_bs]                     ; adjust last-backslash pointer
jmp short no_double_bs                 ; and loop back for more abuse


alloc_list_buffer:                     ; CREATE BUFFER FOR FILE LIST :
mov bx,0100h                           ; allocate four kilobytes
doscall 48h                            ; for list buffer
jnc .l0
jmp trs_80                             ; any problem, complain and abort
.l0:
mov [list_seg],ax                      ; no problem, remember segment
mov word [list_pnt],2000h              ; and note that the buffer is empty
.quit:
ret

get_fn_from_list:                      ; READ FILESPEC FROM FILE LIST :
test byte [flags],fb_list              ; are we using a file list?
jne .l1
clc
ret                                    ; if not, simply exit with carry clear
.l1:
cmp word [auxhandle],byte 0000h        ; is it open yet?
jne .l3                                ; if so, continue
mov dx,fnbuf
doscall 3d20h                          ; open file list file
jnc .l2
dosprint msg_err_listfile              ; problem; print error message
zprint fnbuf                           ; and filename
.l18:
stc                                    ; and exit with carry
ret
.l2:
mov [auxhandle],ax                     ; save file handle
mov word [list_pnt],2000h              ; and note that the buffer is empty
.l3:
mov di,fnbuf
mov byte [di],00h                      ; empty the filespec buffer
and byte [flags],fb_quote ^ 0ffh       ; not between quotes
read_fn_loop:
call read_from_list                    ; get a character from the file list
cmp al,1ah                             ; end of file?
je read_fn_eof                         ; if so, deal with it
call test_space                        ; space?
je read_fn_sp                          ; if so, deal with it
cmp al,SPC                             ; end of line?
jb read_fn_eol                         ; if so, deal with it
cmp al,'"'                             ; quote mark?
je read_fn_qu                          ; if so, deal with it
cmp al,';'                             ; semicolon?
je read_fn_semi                        ; if so, deal with it
cmp al,':'                             ; colon?
je read_fn_semi                        ; if so, deal with it
call force_lc
cmp al,'/'                             ; slash?
jne read_fn_ch
mov al,'\'                             ; if so, convert to a backslash
read_fn_ch:                            ; got a legal character for filename :
mov [di],al                            ; put it in the buffer
inc di
mov byte [di],00h                      ; and null-terminate it
cmp di,fnbuf + maxlen                  ; blown past the end of the buffer?
jb read_fn_loop                        ; no, loop back for more
jmp read_fn_spam                       ; yes, deal with it
read_fn_eof:                           ; found end of list file :
mov bx,[auxhandle]
doscall 3eh                            ; close the list file
mov word [auxhandle],0000h             ; and mark it closed
and byte [flags],fb_list ^ 0ffh        ; no more file list!
cmp byte [fnbuf],00h                   ; was a filename retrieved before eof?
je get_fn_from_list.l18                ; no,  exit with carry set
clc                                    ; yes, exit with carry clear
ret
read_fn_sp:                            ; found a space :
test byte [flags],fb_quote             ; between quotes?
jne read_fn_ch                         ; if so, treat like any other char
read_fn_eol:                           ; found end of line:
cmp byte [fnbuf],00h                   ; anything in the buffer yet?
je read_fn_loop                        ; if not, keep looking
call clean_up_filespec                 ; clean it up
clc
ret                                    ; and exit
read_fn_qu:                            ; found a quote:
test byte [flags],fb_quote             ; was there an open quote?
jne read_fn_eol                        ; if so, it ends the filespec
cmp byte [fnbuf],00h                   ; anything in the buffer yet?
jne read_fn_ch                         ; if so, treat like any other char
or byte [flags],fb_quote               ; otherwise, this is an open quote
jmp short read_fn_loop                 ; keep reading
read_fn_semi:                          ; found a semicolon :
test byte [flags],fb_quote             ; between quotes?
jne read_fn_ch                         ; if so, treat like any other char
cmp byte [fnbuf],00h                   ; anything in the buffer yet?
jne read_fn_ch                         ; if so treat like any other char
cmp al,':'                             ; was comment character a colon?
je read_fn_notice                      ; if so, this is a printable 'notice'
read_fn_remark:                        ; NONPRINTABLE COMMENT IN FILE LIST :
call read_from_list                    ; get a character from the file list
cmp al,1ah                             ; end of file?
je read_fn_eof                         ; if so, deal with it
cmp al,SPC                             ; found the end of the remark?
jae read_fn_remark                     ; if not, keep looking
jmp read_fn_loop                       ; otherwise, resume hunt for filespec
read_fn_notice:                        ; PRINTABLE NOTICE IN FILE LIST :
dosprint msg_notice                    ; introduce it
.l0:
call read_from_list                    ; get a character from the file list
cmp al,1ah                             ; end of file?
je .l2                                 ; if so, deal with it
cmp al,SPC                             ; found the end of the notice?
jae .l1
call crlf                              ; if so, terminate the output line
jmp read_fn_loop                       ; and go back to the filespec search
.l1:
mov dl,al                              ; otherwise, this is a printable char
doscall 02h                            ; print it
jmp short .l0                          ; and continue scanning through notice
.l2:                                   ; end-of-file within notice:
call crlf                              ; terminate the output line
jmp read_fn_eof                        ; and the input file

read_fn_spam:                          ; overflowed the working buffer :
mov byte [fnbuf],00h                   ; empty the working buffer
dosprint msg_err_list_spam             ; print an error message
jmp read_fn_eof                        ; and kill the current file list

read_from_list:                        ; GET CHARACTER FROM FILE LIST FILE :
mov ax,[list_pnt]
cmp ax,1000h                           ; need to refill the buffer?
jb read_list                           ; nope, skip ahead
mov cx,1000h                           ; read four kilobytes
mov bx,[auxhandle]                     ; from the file list
zero dx                                ; to the start
mov ds,[list_seg]                      ; of the list buffer
doscall 3fh
push cs
pop ds                                 ; restore data segment
jnc .l3
call error_out
dosprint msg_err_read_list             ; problem!  print error message
doscall 4c13h                          ; and exit with errorlevel 19
.l3:
mov word [list_pnt],0000h              ; move pointer back to start of buffer
cmp ax,1000h                           ; got four kilobytes?
je read_list                           ; cool!
or ax,ax                               ; got anything at all?
jne .l4
mov al,1ah                             ; if not, return end-of-file
ret
.l4:
mov si,ax                              ; got less than four kilobytes :
mov es,[list_seg]                      ; poke in an eof
mov byte [es:si],1ah                   ; just to be on the safe side
read_list:                             ; get buffered char from file list :
mov es,[list_seg]                      ; segment of file list buffer
mov si,[list_pnt]                      ; pointer into list buffer
mov al,[es:si]                         ; get character from buffer
inc si
mov [list_pnt],si                      ; and increment the pointer
push ds
pop es                                 ; fix data segment
.quit:
ret                                    ; and exit

find_null_fnbuf:                       ; FIND END OF FILESPEC :
mov di,fnbuf                           ; fall through to:

find_null:                             ; FIND END OF ASCIIZ STRING :
cmp byte [di],00h                      ; found null yet?
je read_list.quit                      ; no, keep searching
inc di
jmp short find_null

fix_last_bs:                           ; REPAIR LAST_BS VARIABLE :
mov word [last_bs],0000h               ; note no backslashes found yet
mov si,fnbuf                           ; point to start of filespec buffer
.l0:
mov al,[si]                            ; look at character
cmp al,00h                             ; terminal null?
je .l2                                 ; if so, exit
cmp al,'\'                             ; backslash?
jne .l1
mov [last_bs],si                       ; if so, save its address
.l1:
inc si                                 ; and keep on going
jmp short .l0
.l2:                                   ; found the terminal null
cmp si,fnbuf + maxlen                  ; have we wandered off the edge?
jae .l4                                ; if so, problem
ret
.l4:                                   ; spammed fnbuf :
jmp error_fnbuf_over                   ; abort

show_fnbuf:
mov ah,02h
mov si,fnbuf
.l10:
mov al,[si]
lodsb
cmp al,00h
je read_list.quit
call force_lc
mov dl,al
int 21h
jmp short .l10


%ifdef MungaBunga
%ifdef Enable_Sw_Y

root_safety:                           ; CHECK FOR EXTRA-DANGEROUS COMMANDS :

;  If items in the root directory are to be deleted, and /Y is specified, the
;  user will be prompted anyhow.  This modification was inspired by a loser
;  calling himself "Munga Bunga" who writes batch files to erase peoples'
;  hard drives and posts them on the Internet.  This routine adds a new
;  incompatibility with Microsoft's DELTREE.EXE; I hope the additional safety
;  will outweigh any inconvenience.  Anyhow, the check should rarely bother
;  legitimate users.
;
;  To remove this safety check, comment out the line defining the variable
;  'MungaBunga' near the top of the file, then re-assemble by typing
;  NASM DELTREE.S -O DELTREE.COM

test byte [flags],fb_sw_y              ; was /y specified?
je root_safety_okay                    ; if not, okay so exit
mov bx,fnbuf + 0002h                   ; normal position of root backslash
cmp word [fnbuf],hilo ('\','\')        ; is this a unc filespec?
jne .l20                               ; if not, proceed
mov cl,00                              ; found no more backslashes yet
.l10:
mov al,[bx]                            ; examine character :
cmp al,00h                             ; hit the end of the filspec yet?
je root_danger                         ; if so, problem, so assume danger
cmp al,'\'
jne .l15
inc cl
cmp cl,02h
jae .l20
.l15:
inc bx
jmp short .l10
.l20:                                  ; got offset of root backslash in .bx
cmp bx,[last_bs]                       ; compare against last_bs variable
jb root_safety_okay                    ; if not in root, okay so exit
root_danger:                           ; dangerous command....
mov bx,[fnbuf]                         ; get drive letter
cmp bx,hilo ('\','\')                  ; if this was a unc spec,
je root_warning                        ; do the warning
sub bl,60h                             ; convert drive letter to a number
cmp bl,01h
jb root_warning                        ; if it wasn't a letter,
cmp bl,1ah                             ; do the warning
ja root_warning
doscall 4408h                          ; is volume removable?
jc root_warning                        ; any error, do the warning
or ax,ax                               ; if volume is removable,
je root_safety_okay                    ; don't prompt the user

root_warning:
call prompt_fix_2                      ; force stdout to the console
dosprint msg_munga                     ; display a warning message
call show_fnbuf                        ; and the current filename
dosprint msg_bunga                     ; display a prompt message
jmp ask_yorn

root_safety_okay:
cmp al,al
ret

%endif ; Enable_Sw_Y
%endif ; MungaBunga

prompt_user:                           ; PROMPT USER, IF NEEDED :
%ifdef Enable_Sw_Y
test byte [flags],fb_sw_y              ; was /y specified?
je .l1                                 ; if not, continue normally
cmp al,al                              ; if so, exit with .z set
ret
%endif ; Enable_Sw_Y
.l1:                                   ; no /y, do prompt user :
test byte [dta_attr],attr_d            ; found a file or a directory?
jne .l12                               ; if a file,
dosprint msg_prompt_file               ; display 'delete file' message
call show_fnbuf                        ; and the filename
mov dl,'"'
doscall 02h                            ; close quote
jmp short .l20                         ; and continue ....
.l12:                                  ; if a directory,
dosprint msg_prompt_subdir             ; display 'delete directory ' message
call show_fnbuf                        ; and the filename
dosprint msg_prompt_contents           ; 'and all its subdirectories'
.l20:                                  ; in either case,
dosprint msg_prompt_yorn               ; print y/n prompt

ask_yorn:                              ; get user's yes-or-no reply
mov cl,00h                             ; no valid characters yet
.l30:                                  ; wait loop :
mov ah,01h
int 16h                                ; check keyboard buffer
pushf                                  ; and save status
mov ah,00h
int 16h                                ; get next key
popf                                   ; was keyboard buffer empty?
jne .l30                               ; if not, loop back, empty buffer
call force_lc                          ; force keystroke to lowercase
cmp al,CR                              ; enter?
je .l40                                ; maybe exit wait loop
or ax,ax
je .l45
cmp al,03h                             ; control-c or control-break?
je .l45                                ; if so, abort immediately
cmp al,'q'                             ; 'q' for quit
je .l35                                ; is valid
cmp al,'n'                             ; 'n' for no
je .l35                                ; is valid
cmp al,'y'                             ; 'y' for yes
jne .l30                               ; is valid; everything else is junk
.l35:                                  ; got a valid response :
mov cl,al                              ; remember it
mov dl,al
doscall 02h                            ; print it to the screen
mov dl,BS                              ; with a backspace
int 21h
jmp short .l30                         ; and return to the wait loop
.l40:                                  ; user pressed enter :
cmp cl,00h                             ; got a valid response yet?
je .l30                                ; if not, loop back, wait some more
call crlf                              ; if so, terminate output line
cmp cl,'q'                             ; alternate abort?
je .l50                                ; if so, go do it
cmp cl,'y'                             ; compare against 'yes' response
ret                                    ; and exit
.l45:                                  ; control-c or control-break pressed :
dosprint msg_break                     ; print '^C' message
doscall 4c03h                          ; and bomb with errorlevel 3
.l50:                                  ; q pressed -- alternate user abort :
mov byte [return_code],03h             ; return an errorlevel of 3
jmp exit_program                       ; and exit from program


get_country_info:                      ; time and date formats, etcetera
mov dx,country
doscall 3800h                          ; get current country info
ret                                    ; and exit

append_found_filespec:                 ; ADD FOUND FILESPEC AFTER PATHNAME :
mov di,[last_bs]                       ; destination is
inc di                                 ; character following final backslash
mov si,dta_name                        ; source is filename in dta
call copy_string                       ; copy found filename into fnbuf
cmp di,fnbuf + maxlen                  ; fnbuf overflow?
ja error_fnbuf_over                    ; if so, handle it
ret                                    ; else exit

append_found_alt:
mov di,[last_bs]
inc di
mov si,alt_name
call copy_string
cmp di,fnbuf + maxlen
ja error_fnbuf_over
ret

error_fnbuf_over:                      ; fnbuf has overflowed:
call error_out                         ; fix stdout to screen
dosprint msg_err_fnbuf_over            ; complain
doscall 4c14h                          ; and exit with errorlevel 20

copy_string:                           ; COPY ASCIIZ STRING [SI] TO [DI] :
push ds                                ; make .es equal to .ds
pop es
.l0:
lodsb                                  ; get a character from [ds:si]
stosb                                  ; put it in [es:di]
cmp al,00h                             ; until the final null is found
jne .l0
ret                                    ; then quit

zprint1:                               ; PRINT ASCIIZ STRING VIA .DI :
mov ah,02h                             ; dos print-a-character function
.l0:
mov dl,[di]                            ; get a byte from string
inc di
cmp dl,00h                             ; found the final null?
je crlf                                ; if so, deal with it
int 21h                                ; otherwise, print the character
jmp short .l0                          ; and loop back for more abuse
                                       ; found the final null:

crlf:                                  ; END OF PRINT LINE :
dosprint msg_crlf                      ; print a carriage return, line feed
and byte [flags],fb_crlf ^ 0ffh        ; clear the crlf-pending flag
ret

crlf_maybe:                            ; PRINT A CRLF IF ONE IS PENDING :
test byte [flags],fb_crlf              ; do we need a crlf?
jne crlf                               ; if so, go do it
ret                                    ; if not, just exit

prompt_fix:                            ; USE CONSOLE FOR PROMPTS :
%ifdef Enable_Sw_Y
test byte [flags],fb_sw_y              ; was /y specified?
jne prompt_fix_2.quit                  ; if so, don't need to force console
%endif ; Enable_Sw_Y
prompt_fix_2:
test byte [flags],fb_fixed             ; is prompt output already fixed?
jne .quit                              ; if so, don't do it again
mov dx,name_con                        ; open the console
doscall 3d42h                          ; for read and write, deny-none
push ax                                ; save the resulting handle
mov bx,ax
mov cx,0001h                           ; force standard output
doscall 46h                            ; to the console
pop bx
doscall 3eh                            ; close temporary console handle
or byte [flags],fb_fixed               ; note that prompt output was fixed
.quit:
ret                                    ; and exit

bombs_away:                            ; HANDLE VARIOUS ERRORS:
call error_out                         ; prepare to display error message
dosprint msg_error                     ; print start of error message
pop di
mov dx,[di]                            ; get pointer to error message
doscall 09h                            ; print error message
mov al,[di+02]                         ; get errorlevel
doscall 4ch                            ; and exit program
int 20h

error_out:                             ; FORCE STDOUT TO STDERR :
mov cx,0001h                           ; force stdout
mov bx,0002h                           ; to stderr
doscall 46h
ret


; -------------------------------------- STRINGS AND OTHER STATIC DATA :

;; msg_markver:                           ; info displayed by mark aitchison's
;; db 'VeRsIoN=1.02f',00                  ; version.exe utility
;; db 'CoPyRiGhT=Copyright 1998-2003, Charles Dye',00

msg_syntax:
db CR,LF
db 'DELTREE.COM   v1.02f   2005-04-06   C. Dye   raster@highfiber.com',CR,LF
db 'Freeware.  Copyright 1998-2003, Charles Dye.  No warranty!',CR,LF
db LF
db 'DELTREE [/Y] '
%ifdef Enable_Sw_V
db '[/V] '
%endif ; Enable_Sw_V
%ifdef Enable_Sw_D
db '[/D] '
%endif ; Enable_Sw_D
db '[/X] filespec [filespec...]',CR,LF
db LF
%ifdef Enable_Sw_Y
db '  /Y    Delete specified items without prompting',CR,LF
%else
db "  /Y    For compatibility with Microsoft's; ignored",CR,LF
%endif ; Enable_Sw_Y
%ifdef Enable_Sw_V
db '  /V    Report counts and totals when finished',CR,LF
%endif ; Enable_Sw_V
%ifdef Enable_Sw_D
db '  /D    Display debug info',CR,LF
%endif ; Enable_Sw_D
db "  /X    For testing; don't actually delete anything",CR,LF
db LF

%if 0	; do not use 1.02e / older style messages
%ifdef SwitchesOnlyAtStart
db 'Note that any switches must be specified at the start of the command',CR,LF
db 'line, before all filespecs.  This behavior is different from most DOS',CR,LF
db "utilities, but compatible with Microsoft's DELTREE.EXE.",CR,LF
db LF
%else
db 'This copy of DELTREE.COM has been altered to permit switches to appear',CR,LF
db 'anywhere on the command line, not only before filespecs.  Be advised',CR,LF
db "that this behavior is different from Microsoft's DELTREE.EXE.',CR,LF
db LF
%endif ; SwitchesOnlyAtStart
db 'Filespecs may name files, subdirectories (all contents will be lost),',CR,LF
db 'or DR DOS-style file lists.  DELTREE is a dangerous command!  Use at',CR,LF
db 'your own risk.',CR,LF
db LF
db 'This program may be freely distributed under the terms of the Free',CR,LF
db "Software Foundation's GNU General Public License, version 2; or, at",CR,LF
db 'your option, any later version of that License.  See the file COPYING',CR,LF
db 'for details.'
%else	; ... but use 1.02f style messages :-)
%ifdef SwitchesOnlyAtStart
db 'Switches must be before filespecs on command line - MS compatible syntax!',CR,LF
db LF
%else
db 'Switches can be anywhere on the command line - this differs from MS syntax!',CR,LF
db LF
%endif ; SwitchesOnlyAtStart
db 'Filespecs may name files, subdirectories, or DR DOS-style file lists.',CR,LF
db 'DELTREE is a dangerous command!  Use at your own risk.',CR,LF
db LF
db "This program may be freely distributed under the terms of the GNU General",CR,LF
db "Public License, version 2, or, at your option, any later version of GNU GPL.",CR,LF
db "DELTREE is distributed without any warranty. See the file COPYING for details."
%endif

msg_crlf:
db CR,LF,'$'

switches:
db 'y'
%ifdef Enable_Sw_V
db 'v'
%endif ; Enable_Sw_V
%ifdef Enable_Sw_D
db 'd'
%endif ; Enable_Sw_D
db 'x?',00

switch_routines:
dw switch_y
%ifdef Enable_Sw_V
dw switch_v
%endif ; Enable_Sw_V
%ifdef Enable_Sw_D
dw switch_d
%endif ; Enable_Sw_D
dw switch_x, switch_query

error_numbers:
db 05h,10h,13h,20h,41h,53h,56h,00h

error_strings:
db 'access denied$current directory$write protect$sharing violation$'
db 'access denied$fail on INT 24$invalid password$'


msg_error:
db CR,LF,'Error:  $'

msg_err_dos_bad:
db 'Requires DOS 3 or better!',CR,LF,'$'

msg_trs_80:
db 'Not enough memory!',CR,LF,'$'

msg_err_fn_ovf:
db 'Filespec too long!',CR,LF,'$'

msg_err_in_filespec:
db 'Empty filespec!',CR,LF,'$'

msg_err_no_specs:
db 'No filespec specified!',CR,LF,'$'

msg_err_dots_fix:
db 'Resolving dots in directory name!',CR,LF,'$'

msg_err_listfile:
db 'Unable to open file list:  $'

msg_err_read_list:
db CR,LF,'Error:  Problem reading from file list!$'

msg_err_fnbuf_over:
; db CR,LF,'Error:  Spammed working buffer!',CR,LF,'$'
db CR,LF,'Error:  Normalized filespec too long!',CR,LF,'$'

msg_err_list_spam:
db 'Filespec too long in file list!  Closing file list.',CR,LF,'$'

msg_err_dos:
db '*  DOS error $'

msg_err_rmdir:
db ' removing "$'

msg_err_del:
db ' deleting "$'

msg_err_finis:
db '"$'

msg_open_pat:
db '  ($'

msg_close_pat:
db ')$'

%ifdef Enable_Sw_D

msg_debug_rmdir:
db '- remove:  $'

msg_debug_uplevel:
db '- backup:  $'

msg_debug_quit:
db '-   quit:',CR,LF,'$'

msg_debug_ignore:
db '- ignore:  $'

msg_debug_subdir:
db '-  enter:  $'

msg_debug_file:
db '- delete:  $'

%endif ; Enable_Sw_D

msg_notice:
db 'Notice:  $'

msg_prompt_file:
db 'Delete file "$'

msg_prompt_subdir:
db 'Delete directory "$'

msg_prompt_contents:
db '" and all its subdirectories$'

msg_prompt_yorn:
db '? [ynq] $'

msg_deleting:
db 'Deleting $'

msg_ellipsis:
db '...',CR,LF,'$'

%ifdef Enable_Sw_V

msg_num_deleted:
db CR,LF,'*  Files deleted:  $'

msg_num_removed:
db CR,LF,'*  Subdirectories removed:  $'

msg_total_sizes:
db '    Total of sizes:  $'

msg_bytes:
db ' bytes ($'

msg_kilobytes:
db ' K)$'

msg_megabytes:
db ' M)$'

%endif ; Enable_Sw_V

msg_break:
db '^C',CR,LF,'$'

%ifdef MungaBunga
%ifdef Enable_Sw_Y

msg_munga:
db CR,LF
db 'Warning!  DELTREE called in root directory with /Y specified!',CR,LF
db 'Filespec to delete:  $'

msg_bunga:
db CR,LF,LF
; db 'Do you really want to do this?  Important system files may be',CR,LF
; db 'destroyed!  $'
db 'Do you REALLY want to do this??',CR,LF

%endif ; Enable_Sw_Y
%endif ; MungaBunga

bs_star_dot_star:
db '\'

star_dot_star:
db '*.*',00h

name_con:
db 'CON',00h


; -------------------------------------- BYTE LENGTH VARIABLES :

flags:
db 00h

; Values for manipulating the flags byte :

fb_sw_y   equ  80h                     ; do not prompt user                /y
fb_sw_v   equ  40h                     ; verbose report when finished      /v
fb_sw_x   equ  20h                     ; don't actually delete stuff       /x
fb_crlf   equ  10h                     ; end-of-line pending
fb_sw_d   equ  08h                     ; show debug info                   /d
fb_list   equ  04h                     ; using file list            @filename
fb_fixed  equ  02h                     ; prompt_fix has been performed
fb_quote  equ  01h                     ; parsing filename between quotes


return_code:                           ; 00 :  everything is beautiful,
db 00h                                 ; 01 :  something couldn't be deleted,
                                       ; 03 :  user abort (q or control-c)


align 2 ; ------------------------------ WORD LENGTH VARIABLES :

auxhandle:                             ; handle for file list
dw 0000h

last_bs:                               ; offset of final backslash in fnbuf
dw 0000h

topmost:                               ; last_bs value saved by nuke_item
dw 0000h

list_seg:                              ; segment of file list buffer
dw 0000h

list_pnt:                              ; pointer into file list buffer
dw 0000h

parse_save:                            ; value of .si saved by parse routine
dw 0000h

%ifdef DOSC
subdir_clear:                          ; value to clear subdir attributes
dw 0000h                               ; (workaround for freedos 4301 prob)
%endif

deleted:                               ; number of files deleted :
dw 0000h,0000h                         ; low word, high word

removed:                               ; number of subdirectories removed :
dw 0000h,0000h                         ; low word, high word

total_sizes:                           ; total of deleted files' sizes :
dw 0000h,0000h                         ; low word, high word


; ------------------------------------------------ UNINITIALIZED DATA :

fnbuf:

dta       equ  fnbuf + maxlen                    ; disk transfer area :
dta_attr  equ  dta   + 0015h                     ; attribute of found file
dta_name  equ  dta   + 001eh                     ; name of found file

alt_dta   equ  dta + 0030h                       ; second disk transfer area:
alt_attr  equ  dta + 0045h                       ; attribute of found file
alt_size  equ  dta + 004ah                       ; size of found file
alt_name  equ  dta + 004eh                       ; name of found file

stack_1      equ  alt_dta - $$ + 013fh
stack_start  equ  stack_1 & 0fff0h               ; stack starts on paragraph
stack_end    equ  stack_start + 07feh            ; stack is about 2k long

;
;   Errorlevels:
;   00   All's well
;   01   Something could not be deleted
;   03   User abort (control-C, control-break)
;   16   General syntax, buffer overflow
;   17   Not enough memory
;   18   Antique version of DOS
;   19   Problem with list file
;   20   Internal buffer overflow
;   21   Error resolving directory name (dots_fix)
;
;
;   VERSION LOG:
;
;   v0.90    1998-03-13    4,086 bytes                         2C8EBD68  2C4E
;   Initial beta release.
;
;   v0.91    1998-03-15    3,900 bytes                         AFB852A6  B4A7
;   Changed two-pass parser to one-pass, removing 31-filespec limit.  Moved
;   fnbuf to uninitialized data area.
;
;   v0.92    1998-03-26    3,968 bytes
;   Allow Q for 'quit' in addition to Y and N responses.  Q will abort with
;   errorlevel 3 like Control-C or Control-Break, but will print any /V
;   summary first.  Adds version and copyright strings for Mark Aitchison's
;   VERSION.EXE program.  Includes newer address for the Free Software
;   Foundation.
;
;   v0.93    1998-03-30    3,970 bytes
;   Fixed sloppy calculations involving stack bottom and top addresses, and
;   program size.  Should not make a bit of difference in program operation.
;
;   v1.00    1998-08-05    4,042 bytes
;   Workaround for a serious issue with earlier releases of the DOS-C kernel.
;   21/4301 could convert subdirectories into files....  The filespec cleanup
;   routine now removes any duplicated backslashes from the filespec.  Syntax
;   message now mentions /D (guess I won't delete the debug code after all.)
;
;   v1.00a   1998-08-13    4,050 bytes                         0961803F  93C4
;   Fixes an incompatibility with 8086 and 8088 CPUs (logical shifts with
;   immediate count values.)
;
;   v1.01    1999-01-17    4,318 bytes                         B62CB7DF  3DBA
;   Added a safety check:  If items in the root directory are to be deleted
;   and /Y is specified, the user will be prompted anyhow.  Also, new debug
;   switch /X prevents the program from actually deleting anything.
;
;   v1.02    1999-05-19    4,220 bytes                         EB86016F  FE99
;   Fix for a problem with redirection of stdin under FreeDOS.  The Y-or-N
;   keystroke is now read using the keyboard BIOS instead of via DOS.  New
;   error handler with inline arguments makes the program slightly smaller.
;
;   v1.02a   1999-05-23    4,250 bytes                         3E1DFD07  3EFE
;   Disabled the root-safety check on removable drives.
;
;   v1.02b   1999-07-27    4,220 bytes                         6228F0D7  5149
;   Rewritten for NASM.  Fixed a trivial bug (incorrect return code when no
;   filespec specified.)  Minor bumming.  Added %defines to control assembly
;   of /Y, /V, and /D code.
;
;   v1.02c   2000-04-24    4,210 bytes                         8B60E5F5  CF56
;   Expanded the filespec buffers to 260 bytes to handle the super-long
;   pathnames permitted under Windows 95/98.
;
;   v1.02e   2003-06-22    4,478 bytes                         2186E74C  E851
;   Compile-time option to permit switches to appear anywhere on the command
;   line.  (The default behavior is still to permit switches only before the
;   first filespec, mimicking Microsoft's.)  Tweaks to the syntax text to
;   make it more informative.  /X is now documented.  All dates are now in
;   ISO format.
;
;   v1.02f   2005-04-06
;   Removed a now inappropriate bug-bug compatibility mode for old FreeDOS
;   kernels: OEM version 0fdh, CX 0 or -1, but CX 0 has to be used by NEW
;   kernels as well, to work-around a 32RTM bug. Most DOS kernels use CX 0.
;   For reference: This un-does the "DOS-C" workaround introduced in 1.00!
;   Shortened some "switches before filespecs" / GPL / error messages.
;   Found COMPILE TIME OPTIONS in 1.02e to be: Y, V, D, munga check all on,
;   arbitrary argument order off. In 1.02e deltree2: Same: V, D on, but
;   Y and munga check off. Arbitrary argument order off.
;
