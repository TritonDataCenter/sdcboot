; ### This file is part of PCISLEEP, a tool for PCI bus analysis and
; ### energy saving. (c) by Eric Auer <eric #@# coli.uni-sb.de> 2005.
; PCISLEEP is free software; you can redistribute it and/or modify it
; under the terms of the GNU General Public License as published
; by the Free Software Foundation; either version 2 of the License,
; or (at your option) any later version.
; ### PCISLEEP is distributed in the hope that it will be useful, but
; ### WITHOUT ANY WARRANTY; without even the implied warranty of
; ### MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
; ### GNU General Public License for more details.
; You should have received a copy of the GNU General Public License
; along with FDAPM; if not, write to the Free Software Foundation,
; Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
; (or try http://www.gnu.org/licenses/ at http://www.gnu.org).

; --------------

	; command reg [4] dw: bits 0/1/2 enable ports/memory/busmaster

; TODO?: somehow add this to FDAPM maybe? How about PnPBIOS power off?
; TODO?: ACPI table read with int 15.87 (may need EMM386 MEMCHECK option)
	; ... to use CPU throttling and S0/S1/S3-S4/S5 on/idle/suspend/off
	; S0 is not too useful, S3/S4 requires us to save all state first

; -------------- #####################


	org 100h

start:	mov ah,9
	mov dx,hellomsg
	int 21h
	mov ax,0b101h
	xor di,di
	int 1ah		; PCI BIOS install check, modifies e{a,b,c,d}x, edi
	cmp ah,0	; ... we ignore prot. mode entry point EDI ...
	jnz nopci
	cmp dx,4350h	; edx: 20494350h
	jnz nopci
	mov [maxbus],cl	; highest bus number
	test al,1	; config mechanism 1 okay?
	jnz okaypci

	mov dx,oldpcimsg
giveup:	mov ah,9
	int 21h
	mov ax,4cffh
	int 21h

nopci:	mov dx,helpmsg
	mov ah,9
	int 21h		; show help, even though no PCI is present...
	mov dx,nopcimsg
	jmp short giveup

okaypci:
	mov al,bh
	add al,'0'	; BH.BL is BCD version, but only ?.?? expected
	mov [pcivermsg],al
	mov al,bl
	shr al,4
	and al,15
	add al,'0'
	mov [pcivermsg+2],al
	mov al,bl
	and al,15
	add al,'0'
	mov [pcivermsg+3],al	; e.g. "2.10"
	mov al,cl	; max bus number
	aam		; AH is 10 digit, AL is 1 digit
	add ax,'00'
	cmp ah,'0'
	jnz twobus
	mov ah,' '
twobus:	xchg al,ah
	mov [pcicntmsg],ax
	mov ah,9
	mov dx,yespcimsg
	int 21h

; --------------

	cld
	mov si,81h	; command line arguments
skipcl:	lodsb
	cmp al,'a'
	jb nolower
	cmp al,'z'
	ja nolower
	sub al,'a'-'A'	; toupper
nolower:
	or al,al
	jz showhelp
	cmp al,13	; cr
	jz showhelp
	cmp al,' '
	jz skipcl
	cmp al,'/'
	jz skipcl
	cmp al,'-'
	jz skipcl
	cmp al,9	; tab
	jz skipcl
	cmp al,'L'	; Listing requested?
	jz near showlist
	cmp al,'S'	; Sleep requested?
	jz near sleepall
	cmp al,'V'	; VGA-Sleep requested?
	jz near sleepall_vga
	cmp al,'Q'
	jz near query_list
	; else: syntax error, show help

showhelp:
	mov ah,9
	mov dx,helpmsg
	int 21h
exitokay:
	mov ax,4c00h
	int 21h

; --------------

helpmsg	db "PCISLEEP Options: L(ist), S(leep), V(ga-sleep), Qnnnn",13,10
	db 13,10
	db "L(ist) shows a verbose list of PCI/AGP devices.",13,10
; ---	db "  Ask the net to translate vendor/device numbers to names:",13,10
; ---	db "  For example http://pciids.sourceforge.net/ has a list.",13,10
	db "  Look up vendor/device numbers on WWW.",13,10
	db "  Example: http://pciids.sourceforge.net/",13,10
	db "Qnnnn shows a numerical list of all devices of class n.",13,10
	db "  N must have 2 or 4 digits: Qff and Qffff show a list",13,10
	db "  of classes. Example: Q02 lists your network card(s).",13,10
	db "  Errorlevel is the number of found devices.",13,10
	db "S(leep) halts the CPU and puts (only) the D1/D2 sleep capable",13,10
	db "  devices (see LIST output) into D1/D2 energy saving state.",13,10
	db "V(ga-sleep) does S(leep) and suspends D3 capable VGA cards.",13,10
	db "  The monitor is put to (DPMS) sleep, too.",13,10
	db "Press a key to end V or S sleep. If it fails, let me know:",13,10
	db "  eric at coli.uni-sb.de (VGA wake-up is tricky!)",13,10
	db "$"


; -------------- #####################


query_list:		; si is still set from command line parsing
	xor bx,bx	; desired class code
	mov cl,16	; place to store digit, plus 4
queryparse:
	lodsb
	xor ah,ah
	cmp al,'a'
	jb nolower2
	cmp al,'z'
	ja nolower2
	sub al,'a'-'A'	; toupper
nolower2:
	cmp al,' '	; ignore trailing whitespace
	jz queryparse	; (e.g. for FreeCOM redirect...)
	cmp al,13	; end of line?
	jbe query
	cmp al,'0'
	jb badquery
	cmp al,'A'
	jae parsehex
	cmp al,'9'
	ja badquery
	sub al,'0'
	jmp short parsehexdec
parsehex:
	sub al,'A'-10
parsehexdec:
	cmp al,15
	ja badquery
	sub cl,4		; actual place to store digit
	or cl,cl
	js badquery		; too many digits
	shl ax,cl
	or bx,ax		; merge to class value
	jmp short queryparse

badquery:
	mov dx,badquerymsg
	mov ah,9
	int 21h
	mov ax,4cffh
	int 21h

query:
	test cl,4		; 1 or 3 digits? bad!
	jnz badquery
	cmp cl,16		; no digits? bad, too.
	jz badquery
	mov di,classmsg
	mov ax,bx		; class value (AH or AX)
	call dumpax
	mov bp,subclassmatch	; do 4 digit compare
	or cl,cl
	jz scanmatch
	mov word [classmsg+2],'??'
	mov bp,classmatch	; do 8 digit compare
scanmatch:			; look for BX class devices
	cmp bh,255	; class FF(xx) is show class list, do not scan...
	jz near listclasses
	mov [theclass],bx
	call scanpci
	mov ax,[cs:matchcnt]	; count of hits (low byte used)
	push ax
	cmp ax,1
	jnz notoneclassmatch
	mov byte [classmatchs],' '
notoneclassmatch:
	aam			; AH is 10 digit, AL is 1 digit
	add ax,'00'
	xchg al,ah
	mov [classcntmsg],ax
	mov ah,9
	mov dx,classcntmsg
	int 21h
	pop ax
	mov ah,4ch
	int 21h			; exit, return count as errorlevel


badquerymsg	db "Must use 2 or 4 hex digits for 'Qnn'.",13,10,"$"

classcntmsg	db "?? device"
classmatchs	db "s of class "
classmsg	db "???? found.",13,10,"$"

classlistmsg	db "Known classes:$"
subcllistmsg	db "Common subclasses:$"
waitmsg	db 13,10,"Press a key...$"
hex4msg	db 13,10,"???? $"

matchidmsg	db "????:????",13,10,"$"

theclass	dw 0	; class:xx or class:subclass to look for
matchcnt	dw 0	; number of matching devices


	; output the built-in list of known device classes
listclasses:
	cmp bp,subclassmatch
	jz listsubclasses
	mov ah,9
	mov dx,classlistmsg
	int 21h
	mov si,classmsglist	; array of string pointers
	xor bp,bp	; class counter
lcloop:
	mov di,hex4msg+2
	mov ax,bp		; class number
	inc bp		; prepare for next number
	call dumpal
	lodsw		; class string pointer
	or ax,ax		; end of list reached?
	jz lcdone
	push ax	; later popped as DX
	mov ah,9
	mov dx,hex4msg
	int 21h		; line break, class number
	pop dx	; earlier pushed as AX
	mov ah,9
	int 21h		; class name
	jmp short lcloop
lcdone:
	mov ah,9
	mov dx,crlfmsg
	int 21h
	jmp exitokay


	; output the built-in list of known device subclasses
listsubclasses:
	mov ah,9
	mov dx,subcllistmsg
	int 21h
	mov si,fineclasslist	; number / stringpointer pairs
lscloop:
	mov di,hex4msg+2
	lodsw		; class number
	or ax,ax		; end of list reached?
	jz lcdone
	cmp al,255
	jz hiddenclass
	call dumpax
	mov ah,9
	mov dx,hex4msg
	int 21h		; line break, class number
	lodsw		; class string pointer
	or ax,ax		; end of list reached?
	jz lcdone
	mov dx,ax
	mov ah,9
	int 21h		; class name
	jmp short lscloop

hiddenclass:		; subclasses numbered xxff are not shown
	lodsw
	mov ah,9
	mov dx,waitmsg
	int 21h
	;
	push ds
	push byte 40h		; BIOS data segment
	pop ds
	mov al,[6ch]		; timer tick count
	dec ax
	and al,3fh		; 3.5 second intervals
	mov dh,al
wait35:
	mov ah,1		; key pressed?
	int 16h
	jz nofetch35		; else keep waiting
	mov ah,0		; fetch key
	int 16h
	jmp short waited35		; ... and stop waiting
nofetch35:
	mov al,[6ch]
	cmp al,dh
	jnz wait35		; keep waiting for max 3.5 seconds
waited35:
	pop ds
	;
	jmp short lscloop


	; the test functions are called with bus/dev/func in CX,
	; and vendor / device in EAX.
subclassmatch:		; show vendor / device id if full class matches BX
	push eax
	push bx
	mov bl,8	; register: class:sub:iface:rev
	call getpci
	shr eax,16	; class:sub
	cmp ax,[cs:theclass]
	pop bx
	pop eax
	jz didmatch
didnotmatch:
	ret

classmatch:		; print vendor / device id if class matches BH
	push eax
	push bx
	mov bl,8	; register: class:sub:iface:rev
	call getpci
	shr eax,16	; class:sub
	cmp ah,[cs:theclass+1]
	pop bx
	pop eax
	jnz didnotmatch
didmatch:
	inc word [cs:matchcnt]
	push eax
	push di
	mov di,matchidmsg
	call dumpax	; vendor id
	inc di		; ":"
	shr eax,16
	call dumpax	; device id
	pop di
	push dx
	mov ah,9
	mov dx,matchidmsg
	int 21h
	pop dx
	pop eax
	ret


; -------------- #####################


sleepall_vga:
	mov byte [vgasleepflag],1
sleepall:
	mov ah,9
	mov dx,sleepmsg
	int 21h
	mov bp,vgabus_finder	; find correct vgabus value
	call scanpci
	mov bp,vga_counter	; find VGAs and ACPI
	call scanpci
	mov bp,sleep_wrapper	; put everything to D1/D2 sleep if possible
	call scanpci		; ... also finds correct vgabus value
	test byte [vgasleepflag],1
	jz normalsleep
	call vga_off		; soft-off (power down) VGA card
normalsleep:
	push word [sleepables]	; number of actually sleeping items
	sti
sloop:	hlt			; sleep until any IRQ happens
	mov ah,01h		; check for keystroke
	int 16h
	jz sloop		; keep sleeping until key pressed
	mov ah,0
	int 16h			; read keystroke from buffer
	mov bp,wake_wrapper	; restore everything to D0 wake state
	call scanpci
	test byte [vgasleepflag],1
	jz normalwake
	call vga_on		; re-enable and re-boot VGA card
normalwake:
	; (could check if word [sleepables] is 0 again here)
	pop ax			; number of actually sleeping items
	mov dx,wakemsg
	or ax,ax		; did anything really sleep?
	jnz normalwake2
	mov dx,boringwakemsg	; nothing except CPU really slept
normalwake2:
	mov ah,9
	int 21h

	; The following is interesting to know for VGASLEEP users:
	mov ax,[cs:vgacnt]
	mov di,vgacntmsg
	call dumpal
	or ax,ax
	jnz somevgaexists
novgaexists:
	mov dx,vganoactmsg	; no active VGA found??
	jmp short vgaclashshow
	;
somevgaexists:
	cmp byte [cs:vgadev],255
	jz novgaexists
	test ax,8000h
	jz novgaclash
	mov dx,vgamanyactmsg	; several active VGAs??
	jmp short vgaclashshow
	;
novgaclash:
	mov dx,vgaactmsg	; everything okay, exactly one active VGA
vgaclashshow:
	mov ah,9
	int 21h
	mov cx,[cs:vgadev]
	mov al,cl
	cmp al,255		; undefined?
	jz nodefinedvga		; leave "??.?" in string
	mov di,vgadevmsg
	call dumploc		; print location CX as ??.??.?
nodefinedvga:
	push di			; needed for "no bridge" message trick
	mov di,vgabridgemsg
	mov cx,[cs:vgabridge]	; bridge location
	call dumploc		; print location CX as ??.??.?
	pop di			; soon :)
	or ch,ch		; VGA bus
	jnz pcivgashow		; primary PCI bus? Then no bridge...
	mov al,'$'		; terminate string after VGA location
	stosb			; (no bridge mentioned, VGA is on 1st PCI)
pcivgashow:
	mov ah,9
	mov dx,vgainfomsg
	int 21h
	mov ah,9
	mov dx,crlfmsg
	int 21h
	;
	call show_acpi_info	; show ACPI port / device guess
	mov ax,4c00h
	int 21h

; --------------

show_acpi_info:			; show some ACPI scan results, destroys regs
	mov dx,acpionemsg
	mov cx,[cs:acpicnt]	; number of candidate devices
	cmp cx,1
	jb noacpishow
	jz oneacpishow
	mov dx,acpimsg
	mov ax,cx
	aam			; AH is 10 digit, AL is 1 digit
	add ax,'00'
	xchg al,ah
	mov [acpicntmsg],ax
oneacpishow:
	push dx
	mov cx,[cs:acpidev]	; most likely candidate device
	push cx
	call acpi_analyze
	pop cx
	mov [acpiio],ax		; first guessed base port
	or ax,ax
	jz noacpiio
	mov di,acpidevmsg
	call dumploc		; put CX location into our string
	mov di,acpibasemsg
	call dumpax		; put AX into our string
	mov di,acpiregmsg
	mov al,[cs:acpireg]	; used PCI config register index
	call dumpal
noacpiio:
	pop dx
	mov ah,9
	int 21h
noacpishow:
	ret

acpidev	dw 0	; nonzero if potential ACPI device found
acpicnt	dw 0	; number of ACPI candidates (only first candidate used)
acpiio	dw 0	; assumed ACPI port base: I/O(?) of 0600 or 0680 class dev
acpireg	db 0	; register number where we found the port base

; --------------

vgadev	db 255	; combines with vgabus to full bus.dev.func pointer
		; low byte 255 means VGA not yet known
vgabus	db 0	; which bus has the active VGA: either 0 or the one with
		; VGA+ on the bridge to it. If there is more than one VGA
		; on that bus, we could check which one has I/O+ MEM+ ...?
vgacnt	dw 0	; number of VGA devices in the system, or 8xxx if error

vgabridge	dw 0	; bus.dev.func pointer to bridge to VGA
		; useful to reset the VGA (set bit 6 on reg. 3e to reset)
vgasleepflag	db 0	; set to 1 for VGASLEEP mode

sleepmsg	db 13,10
		db "Sleep mode: CPU halt, PCI/AGP D1/D2 sleep if available."
		db 13,10
		db "Press a key to wake up...$"	; no CRLF here
		; *** no disk spin down here, DPMS only in VGASLEEP mode.
wakemsg		db "   Welcome back!",13,10,"$"
boringwakemsg	db "   Welcome back. Only CPU slept.",13,10,"$"

vgamanymsg	db 13,10,"Found "
vgacntmsg	db "?? VGA graphics cards.",13,10,"$"

vgaactmsg	db 13,10,"Active $"
vganoactmsg	db 13,10,"No active VGA graphics cards?",13,10
		db "Maybe the $"
vgamanyactmsg	db 13,10,"Several active VGA graphics cards?",13,10
		db "First active $"

vgainfomsg	db "VGA graphics card is at "
vgadevmsg	db "??.??.?"
		db "behind bridge at "	; suppressable part
vgabridgemsg	db "??.??.?$"
		; values found by sleep_wrapper and vga_counter

acpimsg		db "First ("
acpicntmsg	db "?? devices) "
acpionemsg	db "ACPI I/O base "
acpibasemsg	db "???? (guess, config reg. "
acpiregmsg	db "?? of "
acpidevmsg	db "??.??.? device).",13,10,"$"

; --------------

acpi_analyze:			; called with CX set, returns AX
	xor ax,ax
	push bx
	push eax
	mov bl,0		; get vendor ID word in AX
	call getpci
	mov bx,knownacpilist	; list of good config reg guesses...
scan_a:	cmp ax,[bx]		; match?
	jz guessed_acpi
	cmp word [bx],0		; end of list?
	jz scan_acpi
	add bx,4
	jmp short scan_a	; continue to browse list
	;
guessed_acpi:
	mov bx,[bx+2]		; the PCI config reg to test
	or bh,bh		; got two of them?
	jz guess_2_acpi		; else check the only one
	call getpci		; try BL one first
	cmp ax,1000h		; too low to be plausible?
	jb guess2
	and al,0feh		; be generous about the low bit here
	test eax,0ffff003fh	; should be at least multiple of 40h
	jz found_acpi		; we found it!
guess2:	xchg bh,bl		; try BH one next, or the only one...
guess_2_acpi:
	call getpci		; try PCI config reg BL...
	cmp ax,1000h		; too low to be plausible?
	jb scan_acpi
	and al,0feh		; be generous about the low bit here
	test eax,0ffff003fh	; should be at least multiple of 40h
	jz found_acpi		; we found it! Otherwise: Scan for it.
	;
scan_acpi:			; Fallback method: full config scan!
	mov bl,10h		; start in known base addresses
next_acpi:
	call getpci
	cmp ax,1000h		; too low to be plausible?
	jb wrong_acpi
	dec eax			; low bit should be set, rest...
	test eax,0ffff003fh	; ...should be multiple of 40h but max 64k
	jnz wrong_acpi		; (will be multiple of 100h in most cases)
found_acpi:
	mov [acpireg],bl	; the promising PCI config register number
	mov bx,ax
	pop eax
	mov ax,bx		; prepare return value
	push eax
	jmp short all_acpi	; port found
wrong_acpi:
	add bl,4		; try next register
	jz all_acpi		; done! (nothing found)
	cmp bl,24h		; still a known base thing?
	jbe next_acpi
	cmp bl,40h		; already in guess mode?
	jae next_acpi
	mov bl,40h		; else enter guess mode, scan unknown regs
	jmp short next_acpi
all_acpi:
	pop eax
	pop bx
	or ax,ax		; return NZ if something found
	ret

knownacpilist	dw 1039h, 74h, 10deh, 48h, 1022h, 58h
		dw 1106h, 8848h, 8086h, 40h, 0, 48h
	; This list means: If SiS, try PCI config reg 74 first, for
	; nVidia try 48, for VIA 48 (and if that fails, 88), and for
	; Intel, try 40. Else scan all vaguely plausible registers.

	; >> Based on data from http://www.oldskool.org/pc/throttle/ free <<
	; >> open source CPU THROTTLING DOS tool. Most chipsets have the  <<
	; >> P_BLK throttle reg at base+10h, bits 1..3 and 4, by the way. <<
	; PS: THROTTLE thinks that old PIIX 8086:7000/122e has broken ACPI.

; --------------

vga_counter:			; called by scanpci (after vgabus_finder)
	push bx			; scans for VGA and ACPI devices
	push eax
	mov bh,[cs:vgadev]	; already met an active VGA?
	mov bl,8		; class.subclass.interface.revision
	call getpci
	shr eax,16
				; BONUS: ACPI finder
	or cx,cx		; ignore bus.dev 00.00
	jz not_acpi
	cmp ah,06		; 0600 and 0680 are ACPI candidates
	jnz not_acpi
	test al,7fh
	jnz not_acpi
	inc word [acpicnt]	; ... only first device is used ...
	test word [acpidev],-1
	jnz not_acpi		; do not store others
	mov [acpidev],cx	; store bus/dev/func of probable ACPI dev
not_acpi:
	cmp ax,300h		; class.subclass for VGA?
	jnz done_vga		; ignore all other device types
	inc word [vgacnt]
	mov bl,4		; check config control
	call getpci
	test al,3		; bit 1 memory, bit 0 ports connected
	jz done_vga		; if not active, counting is all we do...
; active_vga
	cmp ch,[cs:vgabus]	; expected bus?
	jnz problem_vga		; no? then it should not be active!
	cmp bh,255		; still "unknown"?
	jnz problem_vga		; else we have several active VGAs!
	mov [vgadev],cl		; store the device/sub-function number
	jmp short done_vga
problem_vga:
	or word [vgacnt],8000h	; set error flag
done_vga:
	pop eax
	pop bx
	ret			; when done for all, vgadev must be set


vgabus_finder:			; called by scanpci
	push bx
	push eax
	mov bl,0ch		; looking at register 0e now
	call getpci
	shr eax,16
	and al,7fh		; read header type
	cmp al,1
	jnz not_to_vga		; only check classic non-CardBus bridges
	mov bl,3ch		; check register 3e, bridge control
	call getpci
	test eax,80000h		; VGA forwarding on?
	jz not_to_vga		; no VGA behind this bridge
	mov [vgabridge],cx
	mov bl,18h		; bridge bus info: [19] is 2ndary bus number
	call getpci
	mov [vgabus],ah		; store bus number (default: 0, not bridged)
not_to_vga:
	pop eax
	pop bx
	ret


sleep_wrapper:			; called by scanpci
	push bx
	push eax
	mov bl,8		; class.subclass.interface.revision
	call getpci
	shr eax,16
	cmp ah,7		; only a ser/par port?
	jz try_sleep		; sleeping is okay for those
	cmp ah,4		; potentially critical class?
	ja sleep_done		; then do not put device to sleep!
	; classes: 0 none, 1 disk, 2 network, 3 graphic, 4 multimedia
try_sleep:
	; 5 memory, 6 bridge, 7 port, 8 PC-AT thing, 9 input, 0a docking
	; 0a docking, 0b processor, 0c firewire/usb..., 0d... unknown
	mov bl,0ch		; looking at register 0e now
	call getpci
	shr eax,16
	and al,7fh		; read header type
	jnz sleep_done		; should not put bridges to sleep
	push bp			; (at least not before their clients)
	mov bp,sleep_device	; try to put device to sleep
	call check_power_mgmt
	pop bp
sleep_done:
	pop eax
	pop bx
	ret


wake_wrapper:			; called by scanpci
	push bp
	mov bp,wake_device	; try to wake up device (from D1/D2)
	call check_power_mgmt
	pop bp
	ret

; --------------

sleep_device:		; put device into D1 / D2 sleep
	push eax	; ***  would be nice to backup [4] bits 0..2  ***
	push bx		; *** and clear them (no i/o, mem, busmaster) ***
	call getpci	; capabilities are at [+2]
	shr eax,16
	mov bh,ah	; AX test 400/200 is NZ if D2/D1 supported
	test ah,6	; (note that we preserved BL...)
	jz not_sleepable	; supports only ON and OFF
	add bl,4	; now we access the status
	call getpci	; read power state
	and al,0fch	; D0 + something
	or ax,8100h	; enable PME and clear pending PME
	inc al		; at least D1 is possible
	test bh,4	; even D2 maybe?
	jz idle_sleepable
	inc al		; even D2 is possible!
idle_sleepable:
	call setpci	; update power state (might take a while?)
	inc word [sleepables]
not_sleepable:
	pop bx
	pop eax
	ret

wake_device:		; wake from D1 idle / D2 stopped sleep state
	push eax	; *** would be nice to restore [4] register ***
	push bx		; "D3-ers" also need config restore after this!
	add bl,4	; control the status
	call getpci	; read power state
	test al,3	; any sleep state?
	jz already_on
	and al,0fch	; force D0 (on) status
	call setpci	; update power state (might take a while?)
	dec word [sleepables]
already_on:
	pop bx
	pop eax
	ret

sleepables	dw 0	; count of sleeping devices

; -------------- #####################


showlist:
	mov ah,9
	mov dx,scanheadermsg
	int 21h
	mov bp,vgabus_finder	; find correct vgabus value
	call scanpci
	mov bp,vga_counter	; find VGAs and ACPI
	call scanpci
	mov bp,devcheck		; this time, we display a device list
	call scanpci

	mov ah,9
	mov dx,scandonemsg
	int 21h
	call show_acpi_info	; show ACPI port / device guess
	mov ax,4c00h
	int 21h

; --------------

scanheadermsg	db "Interfaces: 10 is OHCI for USB/FireWire, "
		db   "20 is EHCI for USB 2.0, etc.",13,10
		db "Bridges: 'from bus X to Y (subordinate Z)' shown as "
		db   "'[XX->YY(ZZ)]'",13,10
		db "Power Management support shown as: "
		db   "D1 idle, D2 halt, D3 soft-off"
		db 13,10,13,10
		db "bus.device(.function) [vendor:model] "
		db   "classcode(/iface) vendor class [details]",13,10
headline	db "BusDevF  vend:type  class   vendor     description...",13,10,"$"
pcidescmsg	db "??.??.? [????:????] "	; bus.dev.func venID:devID
pciclassmsg	db "????/?? $"			; class subclass / iface

scandonemsg	db "PCI bus scan done.",13,10,"$"

; --------------

devcheck:	; call with EAX device:vendor id, CX bus:device/function
	push bx	; returns AX class:subclass type, and shows a description
	push cx
	push dx
	push di
	;
	push eax
	mov di,pcidescmsg
	call dumploc	; print location CX as ??.??.?
	inc di		; ' '
	inc di		; '['
	pop eax
	call dumpax	; vendor ID
	inc di		; ':'
	shr eax,16
	call dumpax	; device ID
	inc di		; ']'
	inc di		; ' '
	mov bl,8	; register: class:subclass:iface:revision
	call getpci	; bus CH, device/function CL
	shr eax,8	; ignore device revision number
	ror eax,8
	call dumpax	; class and subclass
	rol eax,8
	or al,al	; nonzero interface type?
	jnz has_iface
	push ax
	mov ax,'  '	; no interface type
	stosw
	stosb
	pop ax
	jmp short did_iface

has_iface:
	push ax
	mov al,'/'
	stosb
	pop ax
	call dumpal	; interface type
did_iface:
	shr eax,8	; AX is class/subclass now
	push ax
	;
	or cx,cx	; bus 0 dev 0?
	jz firstdev
	cmp ax,600h	; CPU bridge?
	jnz firstdev
	mov ax,6ffh	; TRICK code: class 0600 but not at 00.00.0
firstdev:
	call describe_device	; convert AX -> pointer DX
	push dx
	mov ah,9
	mov dx,pcidescmsg
	int 21h		; show the numeric description stuff
	;
	mov bl,0	; fetch vendor ID word
	call getpci
	call describe_vendor	; convert AX -> pointer DX
	push si
	mov si,dx
cntv:	lodsb
	cmp al,'$'
	jnz cntv
	sub si,dx		; figure out length of string
	mov ax,12		; *** we want to pad to length 10
	; "ForteMedia ", "ICEnsemble " are 12 long, the rest max 10.
	; *** headline must be changed to match this!
	sub ax,si		; figure out length of padding
	xchg ax,si		; shorter than mov si,ax ;-)
	mov ah,9
	int 21h		; vendor name
padv:	mov ah,9
	mov dx,space		; pad with spaces (at least one)
	int 21h
	dec si
	jnz padv
	pop si
	;
	pop dx
	mov ah,9
	int 21h		; show the device class description
	;
	push bp
	mov bp,show_power_mgmt	; show power management info
	call check_power_mgmt	; do capability list processing
	pop bp
	;
	mov bl,0ch	; looking at register 0e now
	call getpci
	shr eax,16
	and al,7fh	; read header type
	jz nobridge	; all checks done for normal device
	;
	mov di,bridgemsg+2
	mov bl,18h	; bridge bus numbers: primary, 2ndary, subordinate
	call getpci
	call dumpal
	inc di		; al: reg 18h, primary bus number
	inc di		; skip "->"
	shr eax,8	; al: reg 19h, secondary bus number
	call dumpal
	cmp al,ah	; subordinate bus same as secondary?
	jz twobusbridge
	mov al,'('	; add (ss) subordinate bus info
	stosb
	mov al,ah	; reg 1ah, subordinate bus number
	call dumpal
	mov al,')'
	stosb
	twobusbridge:
	mov ax,']$'
	stosw
	mov ah,9
	mov dx,bridgemsg
	int 21h
nobridge:
	;
	mov ah,9
	mov dx,crlfmsg
	int 21h		; next line
	pop ax
	pop di
	pop dx
	pop cx
	pop bx
	ret

bridgemsg	db " [??->??(??)]$"	; usually "[??->??]$"
space		db " $"

; --------------

show_power_mgmt:	; describe power management for device CX
	push eax
	push dx
	push di
	call getpci	; capabilities are at [bl+2]
	shr eax,16	; (status at [bl+4] is less interesting now)
	mov dx,ax
	mov di,powcapmsg+2
	test dx,200h	; D1 supported?
	jz nod2
	mov ax,'D1'
	stosw
	mov al,','
	stosb
nod2:	test ax,400h	; D2 supported?
	jz nod1
	mov ax,'D2'
	stosw
	mov al,','
	stosb
nod1:	mov ax,'D3'
	stosw
	mov ax,']$'
	stosw
	mov ah,9
	mov dx,powcapmsg
	int 21h
	pop di
	pop dx
	pop eax
	ret

powcapmsg	db " [D1,D2,D3]$"

; --------------

describe_vendor:	; select device description string DX based
			; on vendor ID in AX. Returns a default DX if unknown.
	push si
	mov si,vendors	; list offset
scan_v:	cmp [si],ax	; value matched?
	jz got_v
	cmp word [si],0	; end of list?
	jz got_v	; got the "unknown" result
	add si,4	; next entry
	jmp short scan_v
got_v:	mov dx,[si+2]	; string for value
	pop si
	ret		; returns DX

vendors	dw 1039h, sisname, 10deh, nvidianame, 1022h, amdname
	dw 1106h, vianame, 8086h, intelname, 10ech, rtlname
	dw 5333h, s3name, 0e11h, compaqname, 10b7h, threecomname
	dw 10b9h, aliname, 102bh, matroxname, 121ah, threedfxname
	dw 1179h, toshname, 1274h, ensoniqname, 125dh, essname
	dw 9004h, adaptecname, 1102h, creativename, 13f6h, cmedianame
	dw 1000h, symbname, 104ch, tiname, 1002h, atiname
	dw 1023h, tridentname, 14e4h, broadcomname, 1095h, cmdname
	dw 1045h, optiname, 104bh, buslogname, 15adh, vmwarename,
	dw 1412h, icename
	dw 0, emptyname

	; *** I KNOW that this list is very incomplete. The full list
	; *** is longer than 1000 entries. Just listing a few popular
	; *** vendors here. See e.g. http://pciids.sf.net/ for more.

	; Warning: This list is "sorted" to squeeze out a bit more
	; compressability, by e.g. having "q$3" in it twice, and
	; "TI$A" and "a$CM" and "S$T" and "a$C" and so on ;-).
	; *** Mobo means "motherboard chipsets"

amdname		db "AMD$"	; mobo / CPUs...
vianame		db "VIA$"	; mobo / VGAs...
intelname	db "Intel$"	; mobo / CPUs...
rtlname		db "Realtek$"	; network / VGAs...
s3name		db "S3$"	; VGAs...
nvidianame	db "nVidia$"	; mobo / VGAs...
compaqname	db "Compaq$"	; network...
threecomname	db "3Com$"	; network stuff
aliname		db "ALi$"	; mobo (Acer Labs Inc)
matroxname	db "Matrox$"	; VGAs

ensoniqname	db "Ensoniq$"	; sound
threedfxname	db "3Dfx$"	; VGAs
essname		db "ESS$"	; sound
toshname	db "Toshiba$"	; mobo / PCMCIA...
cmedianame	db "CMedia$"	; sound (bad DOS compat...), softmodems...
cmdname		db "CMD$"	; RAID... "CMD$" or "CMD Tech$"
sisname		db "SiS$"	; mobo / VGAs...
tiname		db "TI$"	; network... (Texas Instruments)
atiname		db "ATI$"	; VGAs...
adaptecname	db "Adaptec$"	; SCSI...

creativename	db "Creative$"	; sound (needs special DOS drivers...)
symbname	db "Symbios$"	; SCSI... ("SymBIOS" compresses better)
tridentname	db "Trident$"	; classic VGAs
broadcomname	db "Broadcom$"	; network...
optiname	db "OPTi$"	; mobo
buslogname	db "BusLogic$"	; SCSI / RAID
fortemname	db "ForteMedia$"	; sound (ForteMedia, FM801)
vmwarename	db "VMWare$"	; virtual devices (VGA, network...)
icename		db "ICEnsemble$"	; IC Ensemble, e.g. Envy24
emptyname	db "other$"	; anything else

; --------------

describe_device:	; select device description string DX based
			; on class/subclass in AX
	cmp ah,11h
	jbe classknown
	mov dx,unkname	; no idea
	ret

classknown:
	push bx
	mov bh,0
	mov bl,ah
	add bx,bx
	mov dx,[classmsglist+bx]	; pointer to generic class string
	mov bx,fineclasslist		; offset
fcloop:	cmp [bx],ax			; search fine classification
	jz thatfineclass
	cmp word [bx],0			; end of list reached
	jz nofineclass
	add bx,4
	jmp short fcloop
thatfineclass:
	mov dx,[bx+2]			; pointer to subclass string
nofineclass:
	pop bx
	ret

classmsglist	dw resname, storname, netname, dispname, medname
		dw memname, bridgename, comname, sysname, inpname
		dw dockname, cpuname, busname, wlessname, i2oname
		dw satname, cryptname, dspname, 0

fineclasslist	dw 100h, scsiname, 101h, idename, 102h, flopname,
		dw 104h, raidname, 180h, gendiskname	; 103 ipi
			; Fox has IDE/iface 8a, something like UDMA?
			; actually, IDE are 8x or 9x quite often.
			; Bitfield? 80 master 08 PriP 02 SecP
		; class 0000 is "other", 0001 "other VGA", deprecated
		dw 200h, ethname, 280h, gennetname	; 201 token ring
						; 202 fddi, 203 atm...
		dw 300h, vganame, 380h, gengfxname	; 301 xga, 302 3d
		dw 400h, vidname, 401h, audname	; 402 phone
		dw 500h, ramname, 501h, flashname
		dw 600h, cpubrid, 601h, isabrid, 602h, eisabrid	; 603 mca
		dw 604h, pcibrid, 605h, pcmbrid, 607h, cbusbrid	; 606 nubus
		dw 680h, acpiname	; lspci calls 0680 chip of Fox ACPI
		dw 6ffh, cpubrid2	; 2nd CPU bridge might be ACPI
		dw 700h, sername, 701h, parname, 702h, multsername
		dw 703h, modemname
		dw 800h, picname, 801h, dmaname, 802h, timname
		dw 803h, rtcname, 804h, hotpname
		dw 900h, kbdname, 901h, penname, 902h, mousename
						; 903 scanner, 904 gameport
		dw 0a00h, docksname
;		dw 0b02h, thepentium		; b00 386, b01 486
;		dw 0b03h, theppro
		dw 0b40h, thecopro	; b10 alpha
		dw 0c00h, fiwiname, 0c03h, usbname	; c04 fiber
			; firewire / usb interface 10 is OHCI,
			; usb interface 00 is UHCI, 20 is USB 2.0 EHCI
		dw 0c05h, smbusname		; c01 access bus, c02 ssa
		dw 0d00h, irdaname, 0d10h, rfwlanname	; RadioFrequence
			; d01 consumer ir, d80 wireless
		; f00 sat-tv, f01 sat-audio, f03 sat-voice,
		; f04 sat-data, 1000 net/computer-encr., 1001 enter-
		; tainment encr., 1100 DPIO signal processing...
		dw 0, resname	; for all classes, subclass 80 is other / generic
		; ### I tried to include only non-exotic classes, to  ###
		; ### keep PCISLEEP binary size small: Let me know if ###
		; ### YOU have devices of commented-out classes above ###

resname		db "generic$"			; 00 ("unused")
			; (0000 generic, 0001 generic VGA, deprecated)
storname	db "storage (disk)$"		; 01 (*COMMON*)
scsiname		db "SCSI controller$"
idename			db "IDE controller$"	; interface: bitfield...
flopname		db "floppy controller$"
raidname		db "RAID system$"
gendiskname		db "disk controller (S-ATA?)$"
netname		db "network$"			; 02 (*COMMON*)
ethname			db "LAN / Ethernet$"
gennetname		db "network (WLAN?)$"
dispname	db "display / graphics$"	; 03 (*COMMON*)
vganame			db "VGA graphics$"
gengfxname		db "graphics unit$"		; e.g. _2nd_ VGA core
medname		db "multimedia$"		; 04 (*COMMON*)
vidname			db "video$"
audname			db "audio$"

memname		db "memory$"			; 05 (NORMAL)
ramname			db "RAM (memory) controller$"	; nForce2, Transmeta...
flashname		db "flash memory drive$"	; e.g. MMC/SD/...
bridgename	db "bridge$"			; 06 (*COMMON*)
cpubrid			db "CPU host bridge$"		; every system has one
cpubrid2		db "CPU host bridge / ACPI?$"	; probably not a bridge
isabrid			db "ISA bridge$"		; quite common
				; (for on-board printer/rs232 ports etc.)
eisabrid		db "EISA bridge$"		; rare
pcibrid			db "PCI bridge$"		; common (e.g. AGP)
pcmbrid			db "PCMCIA bridge$"		; ???
cbusbrid		db "CardBus bridge$"		; used for PCMCIA
acpiname		db "southbridge / ACPI?$"	; quite common
comname		db "comport$"			; 07 (COMMON)
sername			db "RS232 port$"		; e.g. an extra port card
parname			db "Centronics port$"		; same
multsername		db "multi RS232 card$"		; same (4-999 ports)
modemname		db "modem$"
sysname		db "PC system device$"		; 08 (NORMAL)
picname			db "PC interrupt controller$"	; also APICs
dmaname			db "PC DMA controller$"		; rare
timname			db "PC timer$"			; rare
rtcname			db "PC clock$"			; rare
hotpname		db "PCI hot-plug controller$"	; in servers?
; confname	db ...	; *** sub 80 can be generic config registers,
			; memory controllers, etc. - Who knows a nice NAME?
inpname		db "input$"			; 09 ("unused")
kbdname			db "keyboard controller$"
penname			db "pen / digitizer input$"
mousename		db "mouse controller$"

dockname	db "docking$"			; 0a ("unused")
docksname		db "docking station$"
cpuname		db "processor$"			; 0b (except 0b40 "never" used)
; thepentium		db "Pentium$"
; theppro			db "Pentium Pro$"
thecopro		db "coprocessor$"
busname		db "bus system$"		; 0c (*COMMON*)
fiwiname		db "FireWire IEEE1394$"
usbname			db "USB controller$"
smbusname		db "SMBus controller$"		; "smart motherboards"
wlessname	db "wireless (RF, IR, WLAN)$"	; 0d (e.g. in notebooks)
irdaname		db "IrDA controller$"
rfwlanname		db "WLAN / RF controller$"
i2oname		db "i2o device$"		; 0e (rarely used)

satname		db "SAT receiver$"	; 0f ("unused")
cryptname	db "crypto engine$"		; 10 ("unused")
			; e.g. for communication or nonfree-TV
dspname		db "signal processor$"		; 11 (rarely used, e.g. DPIO)
unkname		db "unknown$"			; other


; -------------- #####################


vga_off:			; requires valid vgadev and vgabridge values
	push eax
	push cx
	push bx
	push di
	mov ax,1201h		; disable VGA screen refresh
	mov bl,36h		; (DPMS screens will enter 'off' mode)
	int 10h
%if 0
	push es
	xor di,di
	mov es,di
	mov ax,4f10h
	mov bl,0
	int 10h			; also returns version in BCD in BL
	pop es
	cmp ax,004fh		; VBE/PM okay?
	jnz no_vbe_off
	test bh,4		; "off" supported? 1 stdby, 2 suspend,
	jz no_vbe_off		; 4 off, 8 reduced (reduced is for TFTs)
%endif
	mov ax,4f10h		; VESA VBE/PM screen off
	mov bx,0401h		; 4 is off...
	int 10h
no_vbe_off:
	;
	cld
	mov di,vgaconfig	; buffer offset
	mov bl,0fch		; start at the end
	mov cx,[cs:vgadev]
vgasave:
	call getpci
	stosd			; save PCI config data
	sub bl,4		; next register
	cmp bl,0fch		; done?
	jnz vgasave
				; *** could disable a bridge here
	push bp
	mov bp,soft_off_device
	call check_power_mgmt	; modify power state for VGA device
	pop bp
	;
	pop di
	pop bx
	pop cx
	pop eax
	ret

vga_on:				; only useable after vga_off
	push eax		; to be called AFTER wake_device
	push cx
	push bx
	push si
	test byte [cs:d3mode],1
	jz vga_just_dpms
	mov si,vgaconfig	; buffer offset
	mov bl,0fch		; start at the end
	mov cx,[cs:vgadev]
vgarestore:
	lodsd			; restore PCI config data
	call setpci		; caveat warning: we even write to
	; reserved / read-only registers here, but those should have
	; been read as 0 anyway. In addition, writing back '1' bits
	; of event flags can clear pending events. All acceptable?
	sub bl,4		; next register
	cmp bl,0fch		; done?
	jnz vgarestore
				; *** could re-enable a bridge here
	;
	call run_vgabios	; <<< may take several seconds
	;
vga_just_dpms:
	mov ax,4f10h		; VESA VBE/PM screen off
	mov bx,0001h		; 4 is on...
	int 10h
	mov ax,1200h		; enable VGA screen refresh
	mov bl,36h		; (DPMS screens will enter 'off' mode)
	int 10h
	pop si
	pop bx
	pop cx
	pop eax
	ret

; --------------

soft_off_device:		; called by check_power_mgmt
	push eax
	push bx
	push bx			; save power mgmt cap pointer
	mov bl,4
	call getpci		; [4] dw PCI config command register
	and al,0f8h		; disable: 4 busmaster, 2 memory, 1 ports
	call setpci
	pop bx			; restore power mgmt cap pointer
	add bl,4		; control the status
	call getpci
	test al,3
	jnz already_idle	; avoid double counts
	inc word [sleepables]
already_idle:
	or al,3			; force D3 (soft-off) status
	call setpci
	mov byte [cs:d3mode],1	; set flag
	pop bx
	pop eax
	ret


run_vgabios:
	pushf
	cli
	xor ax,ax
	mov ds,ax		; segment 0
	mov eax,[4*0x10]	; int 0x10 vector (current video API)
	mov [cs:i10],eax
	mov eax,[4*0x1f]	; int 0x1f vector (8x8 charset)
	mov [cs:i1f],eax
	mov eax,[4*0x42]	; int 0x42 vector (old video API)
	mov [cs:i42],eax
	mov eax,[4*0x43]	; int 0x43 vector (EGA charset)
	mov [cs:i43],eax
	mov eax,[4*0x6d]	; int 0x6d vector (VGA video API)
	mov [cs:i6d],eax
	mov eax,[0x4a8]		; video save pointer table pointer
	mov [cs:vspt],eax
	; *** other 40:xx video state information not saved:
	; *** should not be necessary to do this in our case
	;
	db 9ah			; call far absolute...
	dw 0003h,0c000h		; ...BIOS init entry point c000:0003
	;
	xor ax,ax
	mov ds,ax		; segment 0
	mov eax,[cs:i10]
	mov [4*0x10],eax	; int 0x10 vector (current video API)
	mov eax,[cs:i1f]
	mov [4*0x1f],eax	; int 0x1f vector (8x8 charset)
	mov eax,[cs:i42]
	mov [4*0x42],eax	; int 0x42 vector (old video API)
	mov eax,[cs:i43]
	mov [4*0x43],eax	; int 0x43 vector (EGA charset)
	mov eax,[cs:i6d]
	mov [4*0x6d],eax	; int 0x6d vector (VGA video API)
	mov eax,[cs:vspt]
	mov [0x4a8],eax		; video save pointer table pointer
	mov ax,cs
	mov ds,ax
	mov es,ax
	popf
	ret

i10	dd 0	; video API call
i1f	dd 0	; 8x8 charset pointer
i42	dd 0	; saved video API call (BIOS int 10)
i43	dd 0	; EGA charset pointer
i6d	dd 0	; direct VGA API call
vspt	dd 0	; video save table pointer

d3mode	db 0	; will be 1 if device got really turned off


; -------------- #####################


dumploc:	; dump CX (!) as bus.device.function to DI
	push ax
	mov al,ch	; bus
	call dumpal
	mov al,'.'
	stosb
	mov al,cl
	shr al,3	; device
	call dumpal
	mov ax,'  '	; no function string
	stosw
	call has_functions	; does device CX have sub-functions?
	jz shownofunc
	dec di		; rewind...
	dec di
	mov al,'.'	; and add function string
	stosb
	mov al,cl
	and al,7	; function
	add al,'0'
	stosb
shownofunc:
	pop ax
	ret

dumpax:		; dump AX as hex to DI
	xchg al,ah
	call dumpal
	xchg al,ah
	call dumpal
	ret

dumpal:		; dump AL as hex to DI
	push si
	cld
	mov si,ax
	shr si,4
	and si,15
	add si,hextab
	movsb
	mov si,ax
	and si,15
	add si,hextab
	movsb
	pop si
	ret

hextab	db '0123456789abcdef'


; -------------- #####################


has_functions:		; returns NZ if device has sub-functions
	push bx		; call with CH (bus) and CL (device, function)
	push cx		; changes no registers
	push eax
	and cl,0f8h	; ask for main function
	mov bl,0ch	; now we want flags from register 0e
	call getpci
	shr eax,16
	test al,80h	; set if device has sub-functions
	pop eax
	pop cx
	pop bx
	ret

check_power_mgmt:	; check if device at CX has caps, and call
	push bx		;   a function for power mgmt caps, if any.
	push eax	;   check_caps itself preserves registers.
	mov bl,04h	; check register 6, config status
	call getpci
	test eax,100000h	; capability list present?
	jz nocaps
	mov bl,0ch	; check register 0eh for header type
	call getpci
	shr eax,16
	mov bl,34h	; normal offset: 34h
	and al,7fh
	jz first_cap	; do check for normal device
	mov bl,14h	; offset for CardBus bridge: 14h
	cmp al,2	; CardBus bridge? (others have no cap list!?)
	jnz nocaps	; do check for CardBus bridge
first_cap:
	call getpci	; read byte: capability pointer
	mov bl,al	; use pointer
next_cap:
	test bl,3
	jnz nocaps	; must be aligned
	cmp bl,40h	; must be at least 40h
	jb nocaps
	call getpci	; read register suggested by caller
	cmp al,1	; check capability type identifier
	jnz nopowermgmt
	; high half of EAX tells about power mgmt, low
	; word of next register configures power mgmt.
	push ax
	call bp		; <<< trigger selected action BP for CX
	pop ax		; <<< which has power mgmt at register BL
nopowermgmt:
	mov bl,ah	; use chain pointer
	jmp short next_cap
	;
nocaps:	pop eax
	pop bx
	ret

; --------------

getpci:	; bus CH, device.function CL (5+3 bits), read register BL to EAX
	push dx		; register must be multiple of 4
	mov dx,0cf8h
	mov eax,ecx
	and eax,0ffffh
	shl eax,8
	or eax,80000000h
	mov al,bl
	out dx,eax
	add dx,4
	in eax,dx
	pop dx
	ret

setpci:	; bus CH, device.function CL (5+3 bits), write EAX to register BL
	push dx		; register must be multiple of 4
	push eax
	mov dx,0cf8h
	mov eax,ecx
	and eax,0ffffh
	shl eax,8
	or eax,80000000h
	mov al,bl
	out dx,eax
	add dx,4
	pop eax
	out dx,eax
	pop dx
	ret

; --------------

scanpci:		; call function BP for all PCI/AGP devices
	push eax
	push bx
	push cx
	xor bx,bx	; register 0
	xor cx,cx	; bus 0, device 0, function 0
devloop:
	call getpci	; find devices
	cmp ax,0ffffh	; nothing?
	jz nodev
	call bp		; <<< we have something to analyze!
nodev:	call has_functions
	jz nextdev	; no sub functions to scan
	inc cl
	test cl,7	; wrapped around to next device?
	jnz devloop	; else scan on
	dec cl		; wrap back and continue with nextdev

nextdev:
	and cl,0f8h	; start with function 0
	add cl,8	; next device (max 32 per bus)
	jz nextbus
	jmp short devloop

nextbus:
	mov cl,0	; start with device 0, function 0
	cmp ch,[cs:maxbus]
	jz scandone	; that was the last bus already
	inc ch		; next bus
	jmp short devloop
	
scandone:
	pop cx
	pop bx
	pop eax
	ret


; -------------- #####################


maxbus	db 0	; highest bus number

hellomsg	db "This is PCISLEEP by Eric Auer 12mar2005 " ; *** VERSION ***
		db    "- Free open source software.",13,10
		db "Read GNU General Public License 2 at www.gnu.org",13,10
		db 13,10,"$"

oldpcimsg	db "PCI BIOS mech. 1 required.",13,10,"$"
nopcimsg	db "PCI BIOS required.",13,10,"$"
yespcimsg	db "PCI BIOS version "
pcivermsg	db "?.??, highest bus number is "
pcicntmsg	db " ?.",13,10,"$"
crlfmsg		db 13,10,"$"

	align 4
vgaconfig:	; LABEL: start of a buffer for the PCI config area
		; of the temporarily-powered-off VGA card (256 bytes)

