; CPULEVEL - This tool is hereby donated to the public domain.

; Written by Eric Auer in 2/2004. Displays the basic CPUID 1
; information including family, model, revision, brand name,
; CPU name and the names of the active feature bit flags.

; Updated in 12/2007: More bit names, more ext, padded texts
; Binary is now 3.0k instead of 2.2k (uncompressed size) but
; Compressed binary size is still below 2.0k with UPX :-)
; Distinguishes 486-w/o-CPUID and 186 from 386 and 8086 now.

; It returns the family code as errorlevel (0 808x, 1 80186,
; 2 80286, 3 80386, 4 80486, 5 Pentium class, 6 PentiumPro
; class, higher values mark REALLY fancy CPUs like 64bit
; ones: Even P. IV or AthlonXP are only PentiumPro family).
; NOTE: 64 bit CPUs are often class 15 which means "see the
; new 8 bit class number instead". So expect to see code 15.

; Please spread the binary together with this source code
; file to keep the contained extra information around
; (because there is no readme.txt!). The binary is only
; 2.8kby small! Compile with NASM (http://nasm.sf.net/),
; simply run:    nasm -o cpulevel.com cpulevel.asm

	org 100h	; it is a com file

start:	mov dx,hellomsg	; identify yourself
	mov ah,9
	int 21h

; test if we have 386 or better: 
	pushf   ; save flags
	mov byte [family],0	; 808x or 186
		xor ax,ax
		push ax
		popf	; try to clear all bits
		pushf
	        pop ax
	and ax,0f000h
	cmp ax,0f000h
	jnz is286		; 4 msb stuck to 1: 808x or 80186
	mov ax,1
	mov cl,64	; try to shift further than size of ax
	shr ax,cl
	or ax,ax
	jz is086	; 186 ignores the upper bits of cl
	mov byte [family],1	; 186: above 808x, below 286
is086:	jmp short noid
is286:	mov byte [family],2	; at least 286
		mov ax,0f000h
		push ax
		popf	; try to set 4 msb
		pushf
		pop ax
	test ax,0f000h
	jz noid		; 4 msb stuck to 0: 80286
	mov byte [family],3	; at least 386

	pushfd		; rescue EFLAGS
		pushfd
		pop eax
	mov ebx,eax
		xor eax, 200000h	; CPUID test bit changeable?
		push eax
		popfd
		pushfd
		pop eax
	popfd		; restore EFLAGS
	xor eax,ebx			; which bits changed?
	test eax,200000h		; CPUID test bit changed?
	jnz hasid	; yes, it has CPUID!

	mov ebp,esp
	and sp,0fff0h	; force aligned stack (fffc would be enough)
	mov esp,ebp
	pushfd		; rescue EFLAGS
		pushfd
		pop eax
	mov ebx,eax
		xor eax, 40000h		; alignment check bit changeable?
		push eax
		popfd
		pushfd
		pop eax
	popfd		; restore EFLAGS
	xor eax,ebx			; which bits changed?
	test eax,40000h			; alignment check bit changed?
	jz no486		; no, it is really only a 386
	mov byte [family],4	; at least 486
no486:				; ... but at least 386

	cld
	mov si,81h	; command line
cline:	lodsb
	cmp al,13	; EOL?
	jz noid
	or al,20h	; tolower()
	cmp al,'i'	; try CPUID anyway (user wish)?
	jz hasid
	jmp short cline

noid:	popf		; restore flags
	; *** Add FPU (x87, numerical coprocessor) detection maybe? ***
	call describelevel
	cmp byte [family],3	; 386 / 486 might have unannounced cpuid
	jb idfree
	mov dx,nocpuid	; hint user about /i option to force cpuid
	mov ah,9
	int 21h
idfree:	jmp done	; nothing more to tell


hasid:	popf		; restore flags
	xor eax,eax	; do basic type 0 CPUID query
	cpuid		; 0f a2
	mov [cpuidlevel],eax	; remember CPUID level
	cld
	mov di,vendor	; store vendor string
	mov eax,ebx
	stosd
	mov eax,edx
	stosd
	mov eax,ecx
	stosd

	cmp dword [cpuidlevel], byte 1
	jnb id1
	mov byte [vendcut],'$'	; terminate string
	mov dx,vendmsg	; display at least the brand name
	mov ah,9
	int 21h
	mov dx,vendmsg2
	mov ah,9
	int 21h
	jmp notype1	; cannot show type/family/brand

id1:	mov eax,1
	cpuid		; CPUID type 1 query
	mov [one_a],eax
	mov [one_b],ebx
	mov [one_c],ecx
	mov [one_d],edx

	mov byte [rev],al
	mov byte [model],al
	and word [rev],15
	shr word [model],4
	and word [model],15
	and ah,15
	mov byte [family],ah
	and word [family],15

	mov bx,[family]
	mov al,[hexdigits+bx]
	mov [famchar],al
	mov bx,[model]
	mov al,[hexdigits+bx]
	mov [modchar],al
	mov bx,[rev]
	mov al,[hexdigits+bx]
	mov [revchar],al

	mov dx,vendmsg	; display vendor string (e.g. "AuthenticAMD")
	mov ah,9
	int 21h

	; EAX now is 0FFMtfmr (xfamily xmodel type family model revision)
	;   xfamily: 1 for IA64, 0 for Pentium4 / AMD K8 (Athlon64)
	;   type: 0 normal 1 overdrive 2 secondary MP processor 3 reserved
	;   family: 4 most 486 / AMD 5x86 / Cyrix 5x86
	;           5 most Pentium class CPUs
	;           6 Pentium II/III/M, AMD K7, Cyrix 6x86 M2, VIA C3
	;           7 Intel Itanium IA64
	;           F see xfamily (used for e.g. Intel Xeon)
	;   model:  i486 -> 0 DX, 1 DX50, 2 SX, 3 DX2, 4 SL, 5 SX2,
	;             6 ???, 7 DX2WB, 8 DX4, 9 DX4WB
	;     UMC 486 -> 1 U5D, 2 U5S
	;     AMD 486 -> 3 DX2, 7 DX2WB, 8 DX4, 9 DX4WB, e 5x86, f 5x86WB
	;     Cyrix 5x86 ->  9
	;     Cyrix MediaGX -> 4
	;     Pentium -> 0 Pentium A, 1 Pentium, 2 P54C, 3 P24T Overdrive,
	;       4 P55C, 7 P54C, 8 P55C 0.25ym
	;     NexGen Nx586 -> 0
	;     Cyrix M1 -> 2 6x86
	;     NS Geode -> 4 GX1/GXLV/GXm, 5 GX2
	;     AMD K5 -> 0 PR75..100, 1 PR120..133, 2 PR166, 3 PR200
	;     AMD K6 -> 6 K6 0.30ym, 7 K6 0.25ym, 8 K6-2, 9 K6-III
	;       d K6-2+/K6-III+ 0.18ym
	;     Centaur -> 4 C6, 8 C2, 9 C3
	;     VIA C3 -> 5 Cyrix M2, 6 WinChip C5A, 7 WinChip C5B/C
	;       (stepping 0..7/8..f), 8 WinChip C5N, 9 WinChip C5XL/P
	;       (stepping 0..7/8..f)
	;     Rise -> 0 mP6 0.25ym, 2 mP6 0.18ym
	;     SiS -> 0 55x
	;     Transmeta -> 4 Crusoe TM3x00 / TM5x00
	;     Pentium family 6 -> 0 PentiumPro A, 1 Pentium Pro,
	;       3 Pentium II 0.28ym, 5 Pentium II 0.25ym, 6 Pentium II
	;       with on-die L2 cache, 7 Pentium III 0.25ym, 8 Pentium III
	;       0.18ym with 256k on-die L2, 9 Pentium M 0.13ym with 1 MB
	;       on-die L2, a Pentium III 0.18ym with 1-2 MB on-die L2,
	;       b Pentium III 0.13ym with 256-512 kB on-die L2
	;     AMD K7 -> 1 Athlon 0.25ym, 2 Athlon 0.18ym, 3 Duron SF,
	;       4 Athlon TB, 6 Athlon PM, 7 Duron MG, 8 Athlon TH/AP,
	;       A Athlon BT
	;     AMD K8 -> 4 Athlon64 0.13ym, 5 Athlon64FX/Opteron 0.13ym
	;     Intel Pentium 4 -> 0/1 0.18ym, 2 0.13ym, 3 0.09ym
	;
	; EBX now is AALLCCBB (apic id, logical CPUs, cache flush chunk
	;   size, brand ID (if Pentium III or newer))
	;   brand -> 0 -, 1 Celeron 0.18ym, 2 Pentium III 0.18ym,
	;     3 Pentium III Xeon 0.18ym, 3 Celeron 0.13ym, 4 P III 0.13ym,
	;     6 Pentium III mobile, 7 Celeron mobile, 8/a Pentium 4 0.18ym
	;     or Celeron 4 mobile 0.13ym (f.2.4)
	;     9 Pentium 5 0.13ym, b Pentium 4 Xeon 0.18ym MP or 0.13ym,
	;     c Pentium 4 Xeon MP 0.13ym, e Pentium 4 Xeon 0.18ym or
	;     Pentium 4 mobile, f Pentium 4 mobile samples, 16 Pentium M
	;     0.13ym
	;     If AMD: 0..1f samples, 20+00..1f Athlon64 100*00..1f+
	;     (quantispeed rating), 40+00..1f Athlon64 mobile 100*00..1f+,
	;     60+00..1f Opteron UP 1xx (xx=38+2*0..1f), 80+00..1f same
	;     for Opteron DP 2xx, a0+00..1f same for Opteron MP 8xx ...
	;   logical CPUs tells how many virtual CPUs (HyperThreading)...
	;
	; ECX is now speed step / monitor / SSE3 / thermal / context id
	;    data cache mode etc. information flags (bits 10,8,7,4,3,0)
	;    on SOME models
	;
	; EDX now is a feature bit list. Bits are:
	;   0 FPU 1 Virt86ModeExt 2 DebugExt (I/O breakpoints) 3 PSE
	;   (page size extension, 4 MB page size), 4 TSC (time stamp counter)
	;   5 ModelSpecificRegisters 6 PhysAddrExt (36 bit address bus)
	;   7 MachineCheckException
	;
	;   8 CMPXCHG8B 9 APIC present (or if AMD K5: global pages supported)
	;   10 (AMD SYSCALL) 11 SYSENTER/SYSEXIT 12 MemTypeRangeRegs
	;   13 global pages 14 machine check architecture 15 ConditionalMOVe
	;
	;   16 PageAttribTable 17 PageSizeExt2 (36bit page size) 18 SerialNo
	;   19 CacheLineFlush 20 ??? 21 Debug Trace Store 22 ACPI Support
	;   (and therm control MSR) 23 MMX
	;
	;   24 FXSAVE/FXRSTOR 25 SSE 26 SSE2 27 self-snoop 28 HyperThreading
	;   29 auto clock ctrl (thermal control)
	;   30 IA64 31 PendingBreakEvent (STPCLK/FERR#)

	mov di,id1str
	mov eax,[one_a]
	call hexdump
	inc di
	mov eax,[one_b]
	call hexdump
	inc di
	mov eax,[one_c]
	call hexdump
	inc di
	mov eax,[one_d]
	call hexdump

	mov dx,id1msg	; just dump EAX EBX ECX EDX of CPUID level 1
	mov ah,9
	int 21h

	mov dx,featlistEDX	; explain the feature bit fields in EDX
	mov ah,9
	int 21h
	mov eax,[one_d]
	mov si,bitnamesEDX
	call bits_describe
	mov eax,[one_c]
	or eax,eax
	jz no_ecx
	mov dx,featlistECX	; explain the feature bit fields in ECX
	mov ah,9
	int 21h
	mov eax,[one_c]
	mov si,bitnamesECX
	call bits_describe
no_ecx:	jmp short bits_done

bits_describe:
	xor ecx,ecx
	xor bx,bx
b_loop:	bt eax,ecx	; check ecx-th bit of EAX
	jnc b_off
	inc bx		; count set flags
	push eax
	mov dx,bitheadmsg
	mov ah,9
	int 21h
	mov dx,[cs:si]	; pick explanation string
	mov ah,9
	int 21h
	pop eax
	test bx,3
	jnz b_line
	push eax
	mov dx,crlfmsg	; new line after 4 set flags
	mov ah,9
	int 21h
	pop eax
b_line:
b_off:	inc cx
	add si,2
	cmp cx,32	; done with all bits?
	jb b_loop
	mov dx,crlfmsg	; new line after 4 set flags
	mov ah,9
	int 21h
	ret

bits_done:
	; other CPUID query types:
	; 2 - PentiumPro/newer cache+TLB info
	; 3 - serial number (if enabled), in EBX (only Crusoe) / ECX / EDX

	mov eax,80000000h	; query extended CPUID level
	cpuid
	push eax
	cmp eax,80000001h
	jb short noxmin

idx1:	mov eax,80000001h
	cpuid		; extended CPUID type 80000001h query
	mov [one_a],eax
	mov [one_b],ebx
	mov [one_c],ecx
	mov [one_d],edx

	mov di,idx1str
	mov eax,[one_a]
	call hexdump
	inc di
	mov eax,[one_b]
	call hexdump
	inc di
	mov eax,[one_c]
	call hexdump
	inc di
	mov eax,[one_d]
	call hexdump

	mov dx,idx1msg	; just dump EAX EBX ECX EDX of CPUID level 1
	mov ah,9
	int 21h

noxmin:
	pop eax
	cmp eax,80000004h	; lazy: only check 3DNow! if it has
	jb near noxid		; support for product name, too.
	cmp eax,80000100h
	ja near noxid

	mov dx,xcheckmsg
	mov ah,9
	int 21h

	mov eax,80000001h
	cpuid
	test edx,0e0000000h	; any 3dNow! etc at all?
	jz none3d
	test edx,80000000h
	jz no3d
	push edx
	mov dx,msg3dnow
	mov ah,9
	int 21h
	pop edx
no3d:	test edx,40000000h
	jz no3d2
	mov dx,msg3dnow2
	mov ah,9
	int 21h
no3d2:  test edx,20000000h
	jz no3d3
	mov dx,msgath64
	mov ah,9
	int 21h
no3d3:	mov dx,crlfmsg
	mov ah,9
	int 21h
	jmp short cpuname

none3d:	mov dx,xcheckmsg2	; "none"
	mov ah,9
	int 21h	

cpuname:			; fetch processor name
	mov eax,80000002h	; part 1 of the name
	cpuid
	mov di,product		; store processor name
	stosd
	mov eax,ebx
	stosd
	mov eax,ecx
	stosd
	mov eax,edx
	stosd
	mov eax,80000003h	; part 2 of the name
	cpuid
	stosd
	mov eax,ebx
	stosd
	mov eax,ecx
	stosd
	mov eax,edx
	stosd
	mov eax,80000004h	; part 3 of the name
	cpuid
	stosd
	mov eax,ebx
	stosd
	mov eax,ecx
	stosd
	mov eax,edx
	stosd

	mov si,product		; remove unprintable chars
	mov cx,48
p_loop:	lodsb
	cmp al,' '
	jae isprn
	mov al,'_'
isprn:	mov [si-1],al
	loop p_loop

	mov dx,prodmsg		; display the string
	mov ah,9
	int 21h

	; 80000000h - query extended CPUID level (returns x as 8000000xh)
	;   also returns the vendor id string EBX EDX ECX as above
	; 80000001h - like 1 (EDX MMX bit buggy on VIA C3 nehemiah 693!?)
	;   DIFFERENCE: bit 29 Athlon64 long mode, 30 x3DNow! 31 3DNow!
	;     24 xMMX if Cyrix, 22 MMX-SSE ifAmd, ...
	; 80000002h .... 80000004h - EAX:EBX:ECX:EDX returns CPU name,
	;   where ...2 ...3 ...4 are the 3 parts of the string, 0 padded
	;   or with leading whitespace.
	; 80000005h - query L1 cache info
	; 80000006h - query L2 cache info
	; 80000007h - query power management info, bits in EDX:
	;   5 software therm control, 4 therm monitoring, 3 therm trip,
	;   2 voltage ID control, 1 frequency ID control, 0 temp sensor
	; 80000008h - xxxxvvpp virt/phys address bits on AMD hammer
	;
	; See http://www.sandpile.org/ia32/cpuid.htm for lots of details!
	;
	; 80860000h - Transmeta extended CPUID, query level...
	; 80860001h - get family, model, stepping, revision, clock,
	;   bit flags: LongRun Table Interface, LongRun, recovery CMS on
	; 80860002h - get software revision
	; 80860003h ... 80860006h - get software information string
	; 80860007h - get core clock / voltage / LongRun level / delay
	;
	; 0c0000000h - Centaur extended CPUID, query level...
	; 0c0000001h - flags: crypto / random num. / alt. instr. set
	;
	; 8fffffffeh - EAX 'DEI\0' (low..high), Divide Et Empera?
	;   AMD K6 family mystery level / easter egg.
	; 8ffffffffh - EAX EBX ECX EDX "NexGenerationAMD" - K6 mystery...
noxid:
notype1:

done:	mov al,[cs:family]	; return family in errorlevel
	mov ah,4ch	; leave program
	int 21h


describelevel:
	xor bx,bx
	mov bl,[cs:family]
	cmp bl,7
	jbe normf
	mov bl,8	; families 8..f: Xeon-like (Xeon uses f + xfamily)
normf:	add bx,bx
	add bx,families	; array of family descriptions
	mov dx,[cs:bx]	; get string
	mov ah,9	; show string
	int 21h
	mov dx,famtail	; common tail string ("CPU found. CR LF")
	mov ah,9
	int 21h
	ret

hexdump:	; convert EAX to hex string at EDI
	push eax
	push bx
	push cx
	mov cx,8
hloop:	rol eax,4
	mov bx,ax
	and bx,15
	mov bh,[cs:hexdigits+bx]
	mov [cs:di],bh
	inc di
	loop hloop
	pop cx
	pop bx	
	pop eax
	ret

hellomsg	db "CPULEVEL - public domain by Eric Auer 2004-2008"
		db 13,10,"$"

hexdigits	db "0123456789abcdef"

vendmsg	db "CPU brand is "
vendor	db "abcdabcdabcd"
vendcut	db " / CPU is family "
famchar	db "0 model "
modchar	db "0 revision "
revchar	db "0",13,10,"$"
vendmsg2	db 13,10
	db "Family / model / revision unknown!",13,10,"$"

prodmsg	db "This CPU calls itself: ["
product	db "abcdabcdabcdabcdABCDABCDABCDABCDabcdabcdabcdabcd]",13,10,"$"

id1msg	db 13,10
	db 9,"CPUID level 1 EAX_.... EBX_.... ECX_.... EDX_....",13,10
	db 9,"  values are: "
id1str	db "00000000 00000000 00000000 00000000",13,10,13,10,"$"

idx1msg	db 13,10
	db 9,"ext. level +1 EAX_.... EBX_.... ECX_.... EDX_....",13,10
	db 9,"  values are: "
idx1str	db "00000000 00000000 00000000 00000000",13,10,13,10,"$"

xcheckmsg	db "AMD specific features:$"
xcheckmsg2	db " [none]"
crlfmsg		db 13,10,"$"

		; things from ext CPUID level 80000001h:
msg3dnow	db " [3DNow!]$"		; bit 31
msg3dnow2	db " [ext 3DNow!]$"	; bit 30
msgath64	db " [64bit AMD]$"	; bit 29 "Long Mode, AA-64"
		; EDX more bits: 27 TSC? 26 PML3E (new page table level)
		; 25 "efer ffxsr", 24 cyrix mmx+ / amd fxsave/fxrstor
		; 23 mmx 22 amd mmx-sse/sse-mem 21 - 20 "efer.nxe"
		; 19 "mp capable" 18 - 18 "4mb pde" 16 FCMOVcc... etc
		; ECX also has nice bits, EAX / EBX are extra ID
		; ECX: 0 LAHF/SAHF on (64bit related) 1 HTT or CMP
		; 2 VM stuff 3 ext APIC space 4 MOV CR8D (LOCK: MOV CR0)
		; 5 LZCNT(?) 6 SSE4 7 unalignedSSE 8 PREFETCH(W) 3dNow!
		; 9 "OS visible workaround" 10 "instr based sampling"
		; 11 SSE5 12 SKINIT/STGI (?) 13 WatchDogTimer 14..31 n/a 

		; ext CPUID level 80000007 is also nice, low 9 bits:
		; 8 InvariantTSC 7 PStates 6 100MHzSteps 5 SoftThermal
		; 4 Th.Moni 3 Th.Trip 2 VoltCtrl 1 FreqCtrl 0 TempSensor

nocpuid	db "CPUID support check negative -"
	db " use /i option to try CPUID anyway.",13,10,13,10,"$"
famtail	db " CPU found.",13,10,"$"

families	dw x86, x186, i286, i386, i486, pent, ppro, cpu64, xeon

x86	db "8086 or compatible 16bit$"
x186	db "80186 or compatible 16bit$"
i286	db "80286 (16bit with protected mode)$"
i386	db "i386 or compatible$"
	; all better 486/Pentium class CPU have CPUID, so the
	; messages below are only shown if CPUID is disabled:
i486	db "i486 or compatible$"	; no FPU check
pent	db "Pentium class$"		; Pentium+ all have FPU
ppro	db "PentiumPro class or newer$"
cpu64	db "Intel Itanium IA64 or similar 64bit$"
xeon	db "Intel Xeon or similar server$"

featlistEDX	db "CPUID level 1 feature bits in EDX:",13,10,"$"
featlistECX	db 13,10,"CPUID level 1 feature bits in ECX:",13,10,"$"

bitnamesEDX	dw fpu, vme, dbe, pse, tsc, msr, pae, mce
		dw c8b, apic, asys, isys, mtrr, gp, mca, cmov
		dw pat, pse2, serno, clf, x20, dts, acpi, mmx
		dw fxsr, sse, sse2, ssnoop, hthread, acc, ia64, pbe

bitnamesECX	dw sse3, y01, y02, mwt, dscpl, vmxe, smxe, sstep
		dw acc2, sse3b, ccache, y11, y12, c16b, etrpd, perfm
		dw y16, y17, dcache, sse41, sse42, xapic, y22, popc
		dw y24, y25, y26, y27, y28, y29, y30, y31

bitheadmsg	db "  [$"	; printed before each bit name string
	;  "max average len]$" - there are max 4 strings / line
fpu	db "FLOATING POINT] $"
vme	db "VM EXT FLAGS]   $"	; VME, VIP, VIF
dbe	db "debug ext]      $"	; DR7.RW, CR4.DE
pse	db "Page Size Ext.] $"
tsc	db "TIMESTAMP COUNT]$"
msr	db "Pentium MSR]    $"
pae	db "PAE 36bit addr] $"
mce	db "mce exception]  $"	; MCAR/MCTR "mach check except"

c8b	db "cmpxchg8b op]   $"
apic	db "APIC present]   $"
asys	db "AMD SYSCALL]    $"	; or "reserved"?
isys	db "SYSENTER/-EXIT] $"
mtrr	db "MTRR present]   $"	; memory type range reg
gp	db "global pages]   $"
mca	db "mca]            $"	; "machine check arch"
cmov	db "CMOV opcode]    $"

pat	db "page attrib tab]$"
pse2	db "4 MB PSE]       $"	; 4 MB page size ext
serno	db "serial number]  $"	; CPUID lvl 3 (EAX:EBX:)ECX:EDX
clf	db "cacheline flush]$"
x20	db "bit 20?]        $"
dts	db "debug trace]    $"	; debug trace / EMON MSR
acpi	db "ACPI thermal]   $"	; "therm control msr" throttle?
mmx	db "MMX UNIT]       $"

fxsr	db "FXSAVE/restore] $"
sse	db "SSE UNIT]       $"
sse2	db "SSE2 UNIT]      $"
ssnoop	db "self snoop]     $"
hthread	db "HyperThreading] $"	; and pause? sti nop? nop sti?
acc	db "therm. throttle]$"	; +1 "TM1E", therm int/status
ia64	db "IA64 64bit CPU] $"	; and JMPE Jv/Ev?
pbe	db "PBE / STPCLK]   $"	; STPCLK, FERR

	; --------

sse3	db "SSE3 UNIT]      $"
y01	db "bit 1?]         $"
y02	db "bit 2?]         $"
mwt	db "MWAIT]          $"	; monitor ...
dscpl	db "CPL debug store]$"	; whatever this means...
vmxe	db "VMXE]           $"	; VMread/write, CR4 VMXE
smxe	db "SMXE]           $"	; GETSEC, CR4 SMXE
sstep	db "ext SpeedStep]  $"

acc2	db "thermal TM2E]   $"	; TM2E int/status xAPIC...
sse3b	db "SSE3 ext]       $"	; SSSE3 MXCSR ...?
ccache	db "cachecontext ID]$"	; L1 context adaptive/shared
y11	db "bit 11?]        $"
y12	db "bit 12?]        $"
c16b	db "cmpxchg16b op]  $"
etrpd	db "etprd]          $"	; "enable ETPRD" (?)
perfm	db "perf debug MSR] $"	; performance debug cap MSR

y16	db "bit 16?]        $"
y17	db "bit 17?]        $"	; dcache, direct cache access:
dcache	db "MMIO cache]     $"	; able to prefetch from MMIO?
sse41	db "SSE4.1]         $"
sse42	db "SSE4.2]         $"
xapic	db "x2APIC]         $"	; MSR 800..bff, 64bit ICR 30
y22	db "bit 22?]        $"	; (LZCNT: leading 0 count)
popc	db "populationcount]$"	; SSE POPCNT

y24	db "bit 24?]$"		; ...
y25	db "bit 25?]$"		; ...
y26	db "bit 26?]$"		; ...
y27	db "bit 27?]$"		; ...
y28	db "bit 28?]$"		; ...
y29	db "bit 29?]$"		; ...
y30	db "bit 30?]$"		; ...
y31	db "bit 31?]$"		; ...

	; CPUID level 80000001 ECX bit 5 is for LZCNT ...

one_a	dd 0	; raw type 1 CPUID EAX 0FFMtfmr
one_b	dd 0	; raw type 1 CPUID EBX AALLCCBB
one_c	dd 0	; raw type 1 CPUID ECX speed step / SSE3 / thermal...
one_d	dd 0	; raw type 1 CPUID EDX feature bit mask

family	dw 0
model	dw 0
rev	dw 0

cpuidlevel	dd 0
xcpuidlevel	dd 0

