

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


	title	COMPDRV - Compressed drive support
	;
	page	,132		;make wide listing
	;
	comment	}

	Copyright 1993, JP Software Inc., All Rights Reserved

	Author:  Tom Rawson  01/10/93

	This module contains support code to access compressed
	drives (DBLSPACE, Stacker, etc.) in order to get compression
	ratios.

	} end description
	;
	;
	; Includes
	;
	include	product.asm
	include	trmac.asm 	;general macros
;	include	model.inc
	;
	;
	; Compressed drive parameters and data structures
	;
	;	DBLSPACE INT 2F API call values
	;
DSAPISig	equ	4A11h		;INT 2F signature
DS_GetVer equ	0		;get DBLSPACE version
DS_GetMap equ	1		;get drive mapping
DS_Swap	equ	2		;swap drive letters
DS_ActDrv equ	5		;activate drive
DS_DADrv	equ	6		;deactivate drive
DS_DrvSpc equ	7		;get drive space
DS_GetFrg equ	8		;get file fragment space
DS_GetXtr equ	9		;get extra information
	;
DSRetSig	equ	444Dh		;GetVer returned signature
DSMapped	equ	80h		;mapped drive bit from GetMap call
DSHost	equ	7Fh		;host drive mask from GetMap call
	;
	;	DBLSPACE INT 2F API error codes
	;
DSE_BADF	equ	100h		;bad function
DSE_BADD	equ	101h		;bad drive letter
DSE_NCMP	equ	102h		;not compressed
DSE_ASWP	equ	103h		;already swapped
DSE_NSWP	equ	104h		;not swapped
	;
	;	DBLSPACE constants
	;
MDFELen	equ	4		;bytes in an MDFAT entry
LMDFELen	equ	2		;log2(MDFELen)
LDirELen	equ	5		;log2(bytes per directory entry)
	;
	;	  Masks for high word of MDFAT entry:
	;
MEHSecNo	equ	0001Fh		;5 bits of sector number
MEHResvd	equ	00020h		;1 bit reserved
MEHCSec	equ	003C0h		;4 bits of compressed sector count
MEHCSShf	equ	6		;shift right for above
MEHUSec	equ	03C00h		;4 bits of uncompressed sector count
MEHUSShf	equ	10		;shift right for above
MEHCmpF	equ	04000h		;compressed flag bit
MEHUseF	equ	08000h		;in-use flag bit
	;
	;	DBLSPACE compressed drive info data structure
	;
	;	Start with MDBPB ...
	;
CDInfo	struc			;begin with DBLSPACE extended BPB
	;
	;	  DOS BPB section
	;
JBoot	db	3 dup (?) 	;bootstrap jump
OEMName	db	8 dup (?) 	;OEM name
ByteSec	dw	?		;bytes per sector [512]
SecClust	db	?		;sectors per cluster [16]
SecRes	dw	?		;reserved sectors
FATCnt	db	?		;FAT count [1]
RootCnt	dw	?		;root directory entries [512]
TotSecW	dw	?		;total sectors (word value, if 0 use
				;  TotSecD)
Media	db	?		;media byte [F8]
SecFAT	dw	?		;sectors in the FAT
SecTrack	dw	?		;sectors per track [random]
HeadCnt	dw	?		;heads on drive [random]
HidSec	dd	?		;hidden sectors
TotSecD	dd	?		;total sectors (dword value, used if
				;  TotSecW is 0)
	;
	;	  DBLSPACE extensions
	;
StMDFAT	dw	?		;log sector for MDFAT start
LByteSec	db	?		;log2(ByteSec)
SecMDRes	dw	?		;sectors before DOS boot sector
StRoot	dw	?		;log sector for start of root
StHeap	dw	?		;log sector for start of sec heap
BegMDFAT	dw	?		;MDFAT entries at beginning for boot
				;  sector, reserved area, root dir
BitFATPg	db	?		;2K pages in BitFAT
Res1	dw	?		;reserved area 1
LSecClu	db	?		;log2(SecClust)
Res2	dw	?		;reserved area 1
Res3	dd	?		;reserved area 2
Res4	dd	?		;reserved area 3
FATType	db	?		;0 for 16-bit, 1 for 12-bit
CVFMax	dw	?		;max CVF size in MB
	;
	;	... add our compressed drive info on the end
	;	(CAUTION:  if you add anything before CDLetter, be sure
	;	to adjust definition of LenMDBPB below!)
	;
CDLetter	db	?		;compressed drive letter, 0 if none
DrvType	db	?		;drive type:  1=DBLSPACE, 2=SuperStor
HostDrv	db	?		;host drive number (0=A etc.)
CDFile	dw	?		;CVF file handle
UCCSec	dw	?		;uncompressed sectors / cluster (from
				;  DBLSPACE drive or host drive)
UCCBytes	dw	?		;uncompressed bytes / cluster (from
				;  DBLSPACE drive or host drive)
LMDFEnt	db	?		;log2(MDFAT entries per sector)
LDirSec	db	?		;log2(directory entries per sector)
BSecMask	dw	?		;bytes per sector - 1 (mask for
				;  modulo calculations)
MDFEMask	dw	?		;MDFAT entries per sector - 1 (mask
				;  for modulo calculations)
DirEMask	dw	?		;directory entries per sector - 1
				;  (mask for modulo calculations)
RootSecs	dw	?		;sectors in root directory
BufSeg	dw	?		;buffer segment
DirSecNo	dw	?		;relative sector of directory
				;  currently in directory buffer
	;
	;	  see BufInfo structure below for structure field names
	;	  corresponding to the following items
	;
SecDir	dw	?		;low directory start sector 
	dw	?		;high directory start sector 
	dw	?		;sector in directory buffer
	dw	?		;offset of directory buffer
SecDFAT	dw	?		;low DOS FAT start sector
	dw	?		;high DOS FAT start sector
	dw	?		;sector in FAT buffer
	dw	?		;offset of DOS FAT buffer
SecMDFFl	dw	?		;low file MDFAT start sector
	dw	?		;high file MDFAT start sector
	dw	?		;sector in file MDFAT buffer
	dw	?		;offset of file MDFAT buffer
SecMDFDr	dw	?		;low directory MDFAT start sector
	dw	?		;high directory MDFAT start sector
	dw	?		;sector in directory MDFAT buffer
	dw	?		;offset of directory MDFAT buffer
	;
CDInfo	ends
	;
LenMDBPB	equ	CDLetter		;length of extended BPB is position
				;  of our first data
	;
	;	Structure of buffer information substructures in CDInfo
	;	structure above
	;
BufInfo	struc
RegStart	dw	?		;low sector no. at start of region
	dw	?		;high sector no. at start of region
RegCur	dw	?		;current relative sector in buffer
RegBuf	dw	?		;offset of region's buffer in buffer
				;  segment
BufInfo	ends
	;
	;
	;	SuperStor compressed drive info data structure
	;
SSCDInfo	struc			;begin with DBLSPACE extended BPB
	;
	;	  DOS BPB section
	;
SSBSec	dw	?		;bytes per sector [512]
SSSecClu	db	?		;sectors per cluster [16]
SSSecRes	dw	?		;reserved sectors
SSFATCnt	db	?		;FAT count [1]
SSRCnt	dw	?		;root directory entries [512]
SSTSecW	dw	?		;total sectors
SSMedia	db	?		;media byte [F8]
SSSecFAT	dw	?		;sectors in the FAT
SSSecTrk	dw	?		;sectors per track [random]
SSHeads	dw	?		;heads on drive [random]
SSHidSec	dd	?		;hidden sectors
SSTSecD	dd	?		;total sectors (dword value, used if
				;  TotSecW is 0)
	;
	;	  SuperStor extensions
	;
SSMisc1	db	11 dup (?)	;11 bytes of BPB extensions
	;
SSLetter	db	?		;drive letter
SSUnit	db	?		;unit
SSMisc2	dw	?		;???
SSMskClu	db	?		;(sec/clust) - 1
SSLSClu	db	?		;log2(sec/clust)
SSRes2	dw	?		;reserved sector count 2
SSNFat2	db	?		;fat count 2
SSRCnt2	dw	?		;root dir count 2
SSSecDat	dw	?		;start of sector data area
SSLstClu	dw	?		;last cluster - 1
SSMisc3	db	20 dup (?)	;20 bytes DPB ext (d3 or d4 struct)
SSRBTOff	dw	?		;RBT offset
SSFAT	dw	?		;offset to FAT?
SSDatOff	dw	?		;offset to data
SSLBSec	db	?		;log2(bytes/sec)
SSCluSz	dw	?		;bytes per cluster
SSSPC	dw	?		;sectors per cluster
SSLHSec	db	?		;log2(host sectors / sector)
SSMaxCmp	dw	?		;max compression ratio
SSTotSec	dd	?		;total physical sectors
	;
SSCDInfo	ends
	;
LenSSBPB	equ	SSMisc1		;bytes to copy for BPB
	;
	;	SuperStor IOCTL Packet data
	;
SSIODATA	struc
SSSig	dw	?		;AA55h signature
SSProd	db	?		;product ID byte
SSMode	db	?		;IOCTL mode
SSInfo	db	8 dup (?) 	;additional info
SSIODATA	ends
	;
SSGETPRM	equ	06h		;get drive parameters
SSCHAIN	equ	07h		;get chain size
SSGETHST	equ	08h		;get host drive
SSGETVER	equ	13h		;get SuperStor version
	;
	;	DISKIO structure for INT 25 calls
	;
DiskIO	struc
DIStart	dd	?		;starting sector
DICount	dw	?		;number of sectors
DIBuf	dd	?		;buffer address
DiskIO	ends
	;
	;
	;	Macros
	;
CallDS	macro	func,drv		;;call DBLSPACE INT 2F API
	mov	ax,DSAPISig	;;get signature
	mov	bx,DS_&func	;;get function
	ifnb	<drv>		;;drive specified?
	mov	dl,drv		;;if so use it
	endif
	int	I_MPX		;;call multiplex
	or	ax,ax		;;set flags for result
	endm
	;
_DATA	segment	word public 'DATA'	;start data segment
DSFName	db	"C:\D"		;start of compressed name
DSFType	db	"BL"		;"BL" for DBLSPACE, "RV" for DRVSPACE
	db	"SPACE."		;end of compressed volume name
DSFSeq	db	"000",0
	;
SSData	SSIODATA	<>		;SuperStor data area
	;
	extrn	__osmajor:byte, __osminor:byte
	;
_DATA	ends
	;
DGROUP	group	_DATA
	;
	;
	ife	_WIN
SERV_TEXT segment	word public 'CODE'	;start server segment
	extrn	ServCtrl:far
SERV_TEXT	ends
	endif
	;
	;
DIRCMDS_TEXT	segment	word public 'CODE'	;start code segment
	;
	assume	cs:@curseg, ds:DGROUP, es:nothing, ss:nothing
	;
	;
	; IsCompDrive - Check if a drive is compressed and get the host
	;	      file name
	;
	; On entry:
	;	Arguments on stack:
	;	  char DriveLet	drive letter to check (upper case)
	;	  char *HDrive	pointer to returned host drive
	;	  int *CVFSeq	pointer to returned host file
	;			sequence number
	;
	; On exit:
	;	AX = 0 if drive is not compressed, 1 if drive is
	;	  compressed by DBLSPACE, 2 if compressed by SuperStor
	;	HostLet and CVFSeq filled in for compressed drives
	;	BX, CX, DX, ES destroyed
	;	All other registers and interrupt state unchanged
	;
	entry	IsCompDrive,varframe
	;
	argW	CVFSeq		;pointer to sequence number
	argW	HDrive		;pointer to host drive letter
	argW	DriveLet		;drive letter
	;
	var	TempPath,32	;temporary for Win95 check
	varend
	;
	pushm	si,di		;save registers
	;
	mov	al,DriveLet	;get drive letter
	callDS	GetVer		;is DBLSPACE there?
	 jnz	ICNotDS		;if not go on
	cmp	bx,DSRetSig	;signature OK?
	 je	ICDSGo		;OK, it's DBLSPACE
	;
ICNotDS:	mov	al,DriveLet	;get drive letter
	mov	ah,SSGETVER	;SuperStor get version function
	call	SSIOCtl		;do the SuperStor call
	 jc	ICNComp		;get out if not SuperStor drive
	mov	al,DriveLet	;get drive letter
	mov	ah,SSGETHST	;SuperStor get host function
	call	SSIOCtl		;do the SuperStor call
	 jc	ICNComp		;get out if not SuperStor drive
	mov	al,3		;show it's a SuperStor drive (type 3)
	mov	bl,SSData.SSInfo	;get host drive number
	jmp	ICPutHst		;go store host drive
	;
ICDSGo:	mov	dl,DriveLet	;get drive
	sub	dl,'A'		;convert to 0-based
	callDS	GetMap		;get map for this drive
	 jnz	ICNComp		;if error get out
	test	bl,DSMapped	;is drive mapped?
	 jz	ICNComp		;if not get out
	and	bl,DSHost 	;get host drive number
	push	bx		;save it
	mov	al,bh		;copy sequence number
	xor	ah,ah		;clear high
	mov	si,CVFSeq 	;point to sequence number
	mov	[si],ax		;store it
	mov	cl,__osmajor	;get major OS version
	cmp	cl,7		;maybe Win95?
	 jb	ICDOSDS		;if not go on
	loadseg	es,ss		;set ES to dummy buffer
	lea	di,TempPath	;point to dummy buffer
	mov	cx,32		;size of buffer
	mov	al,DriveLet	;get drive letter
	mov	ah,':'		;get colon
	mov	[di],ax		;save "x:"
	mov	ax,'\'		;get "\" and null
	mov	[di+2],ax 	;save "\<nul>"
	lea	dx,TempPath	;point to "x:\" (seg in DS)
	stc			;assume error
	calldos	95_VOL		;get volume information
	 jc	ICDOSDS		;if can't get it, go on
	mov	al,2		;it's Win95 DRVSPACE (type 2)
	jmp	ICPopHst		;and go save it
	;
ICDOSDS:	mov	al,1		;it's DOS DRVSPACE (type 1)
	;
ICPopHst:	pop	bx		;restore host drive letter
	;
ICPutHst: mov	si,HDrive 	;point to host drive byte
	mov	[si],bl		;store host drive
	jmp	short ICDone	;and exit
	;
ICNComp:	xor	al,al		;show not compressed
	;
ICDone:	popm	di,si		;restore registers
	exit			;all done
	;
	;
	; CompDriveSetup - Set up for compression data retrieval
	;
	; On entry:
	;	Arguments on stack:
	;	  char DriveLet	    drive letter
	;	  int HostFlag	    host drive compression ratio flag
	;	  CDInfo *CDriveData    compressed drive data
	;
	; On exit:
	;	AX = -1 if not compressed, 0 if OK, >0 if DOS error
	;	CDriveData filled in if AX = 0
	;	BX, CX, DX, ES destroyed
	;	All other registers and interrupt state unchanged
	;
	entry	CompDriveSetup,varframe,far
	;
	argW	CDDAddr		;pointer to compressed drive data
	argW	HostFlag		;host drive compression ratio flag
	argW	DriveLet		;drive letter
	;
	varW	CVFSeq		;CVF sequence number
	varend
	;
	pushm	si,di		;save registers
	;
	; See if we already have a CVF open for this drive
	;
	mov	si,CDDAddr	;point to drive data area
	mov	ax,DriveLet	;get new drive letter
	cmp	al,'a'		;lower case?
	 jb	CSChkDrv		;if not go on
	sub	al,20h		;make it upper
	mov	DriveLet,al	;save it back
	;
CSChkDrv: cmp	al,[si].CDLetter	;same drive as before?
	 je	CSUClust		;if so just set uncompressed cluster
				;  size
	;
	; New drive -- check if it's compressed
	;
	push	ax		;push drive letter for IsCompDrive
	lea	ax,[si].HostDrv	;point to host drive number byte
	push	ax		;on stack for IsCompDrive
	lea	ax,CVFSeq 	;point to sequence number
	push	ax		;on stack
	call	IsCompDrive	;see if drive is compressed
	or	al,al		;check result
	 jnz	CSIsComp		;if compressed go on
	mov	ax,-1		;get error code
	jmp	CSExit		;get out
	;
	; Drive is compressed, see if it's DBLSPACE or SuperStor
	;
CSIsComp: mov	[si].DrvType,al	;save drive type
	cmp	al,3		;DBLSPACE or SuperStor?
	 jb	CSDS		;go on if DBLSPACE
	;
	; SuperStor drive -- get the BPB info
	;
	mov	al,DriveLet	;get drive letter
	mov	ah,SSGETPRM	;SuperStor get version function
	call	SSIOCtl		;do the SuperStor call
	 jc	CSExit		;get out if error
	mov	di,si		;copy as destination
	add	di,ByteSec	;point to first byte that matches SS
	pushm	si,ds		;save registers
	lds	si,dptr SSData.SSInfo  ;pointer to BPB
	mov	al,[si].SSLSClu	;get log2(sec/cluster)
	mov	ah,[si].SSLBSec	;get log2(bytes/sec)
	dload	bx,dx,[si].SSTotSec   ;get total sectors
	mov	cx,LenSSBPB	;get length
	cld			;go forward
	rep	movsb		;copy BPB info
	popm	ds,si		;restore registers
	mov	[si].LSecClu,al	;save log2(sec/cluster)
	mov	[si].LByteSec,ah	;save log2(bytes/sec)
	mov	cl,ah		;copy for shift
	xor	ch,ch		;clear high byte
	xor	ah,ah		;assume 16-bit FAT
	;
CSSecShf: dshl	bx,dx		;slide sector count left
	loop	CSSecShf		;loop to make it byte count
	;
	cmp	dx,512		;check against 32 megs
	 ja	CSSSDone		;if over go set 16-bit FAT
	 jb	CSSS12		;if under go set 12-bit
	or	bx,bx		;check low word
	 jnz	CSSSDone		;if not exactly 32 MB set 16-bit
	;
CSSS12:	inc	ah		;flag 12-bit FAT
	;
CSSSDone: mov	[si].FatType,ah	;save FAT type
	jmp	CSCalc		;go on and calculate everything
	;
	; DBLSPACE drive -- check if another CVF was already open and
	; close it if so
	;
CSDS:	xor	al,al		;get zero
	xchg	[si].CDLetter,al	;clear open flag, get old value
	or	al,al		;was a drive open?
	 jz	CSNewDrv		;if not go on
	mov	bx,[si].CDFile	;get old CVF handle
	calldos	CLOSE		;close it
	;
	; New compressed drive -- set up CVF name
	;
CSNewDrv: mov	al,[si].HostDrv	;get host drive
	add	al,'A'		;convert to alpha
	mov	DSFName,al	;store in name
	mov	ax,CVFSeq 	;get sequence number
	loadseg	es,ds		;set ES for STOSB
	lea	di,DSFSeq+2	;point to end of sequence number area
	std			;go backward
	mov	cx,3		;3 digits
	mov	bl,10		;get divisor
	;
CSDvLoop: div	bl		;divide to get next digit
	xchg	ah,al		;swap to get remainder in AL
	add	al,'0'		;make it a digit
	stosb			;store it
	mov	al,ah		;copy back remainder
	xor	ah,ah		;clear high
	loop	CSDvLoop		;do next byte
	cld			;go forward again
	;
	; Open the new CVF and read the MDBPB
	;
	mov	bl,__osminor	;get true DOS minor version
	mov	bh,__osmajor	;get true DOS major version
	cmp	bx,0616h		;compare against 6.22
	 jb	CSTryDbl		;if below 6.22 try DBLSPACE only
	mov	ax,'VR'		;set for DRVSPACE ('VR' gives us "RV"
				;  in AX)
	call	DSPOpen		;try to open DRVSPACE.nnn
	 jnc	CSRdBPB		;go on if OK
	cmp	ax,2		;was it a file not found error?
	 jne	CSExit		;if not, exit immediately
	;
CSTryDbl: mov	ax,'LB'		;try DBLSPACE ('LB' gives us "BL" in
				;  AX)
	call	DSPOpen		;try to open DBLSPACE.nnn
	 jnc	CSRdBPB		;go on if OK
	jmp	CSExit		;exit with error in AX if we can't
	;
CSRdBPB:	mov	[si].CDFile,ax	;save CVF file handle
	mov	bx,ax		;copy file handle
	mov	dx,si		;point to MDBPB buffer
	mov	cx,LenMDBPB	;get bytes in extended BPB 
	calldos	READ		;read the MDBPB
	 jc	CSError		;if error, close file and get out
	;
	; Set up FAT and MDFAT positions and bit masks; leave bytes/sec
	; in BX
	;
CSCalc:	xor	ax,ax		;get zero for high words
	mov	dx,[si].SecRes	;skip reserved stuff after boot sec
	dstore	[si].SecDFAT.RegStart,dx,ax  ;store DOS FAT rel sector
	cmp	[si].DrvType,3	;is it DBLSPACE?
	 jae	CSBytSec		;if not go on
	mov	dx,[si].StMDFAT	;get MDFAT logical sector (garbage for
				;  SuperStor, but not used there)
	inc	dx		;count in the BPB
	dstore	[si].SecMDFFl.RegStart,dx,ax	;store file MDFAT rel sec
	dstore	[si].SecMDFDr.RegStart,dx,ax	;store dir MDFAT rel sec
	;
CSBytSec: mov	cl,[si].LByteSec	;get log2(sector size)
	mov	ch,cl		;copy it
	sub	ch,LMDFElen	;get log2(MDFAT entries per sector)
	mov	[si].LMDFEnt,ch	;save it
	sub	cl,LDirELen	;subtract log2(bytes per dir entry)
				;  to get log2(entries per sector)
	mov	[si].LDirSec,cl	;save that
	mov	ax,[si].RootCnt	;get root dir entry count (assumes
				;  integral # of sectors)
	shr	ax,cl		;make it a sector count
	mov	[si].RootSecs,ax	;save it
	mov	bx,[si].ByteSec	;get bytes per sector
	mov	ax,bx		;copy it
	dec	ax		;get bytes per sector - 1
	mov	[si].BSecMask,ax	;save as bit mask
	push	ax		;save it
	mov	cl,LMDFELen	;get log2(MDFAT entry length)
	shr	ax,cl		;get MDFAT entries per sector - 1
	mov	[si].MDFEMask,ax	;save it
	pop	ax		;get back bytes per sector mask
	mov	cl,LDirELen	;get log2(bytes per dir entry)
	shr	ax,cl		;get dir entries per sector - 1
	mov	[si].DirEMask,ax	;save that
	;
	; Reserve buffer areas for the directory, DOS FAT, and MDFAT
	; (one sector each)
	;
	cmp	[si].BufSeg,0	;is it already reserved?
	 jnz	CSClrBuf		;if so just clear buffers
	mov	[si].SecDir.RegBuf,0    ;directory buffer is at 0
	mov	[si].SecDFAT.RegBuf,bx  ;DOS FAT buffer is after
				    ;  directory buffer
	mov	ax,bx		;copy bytes per sector
	cmp	[si].DrvType,3	;DBLSPACE?
	 jae	CSResBuf		;if not go on
	shl	bx,1		;get 2 * bytes per sector
	mov	[si].SecMDFFl.RegBuf,bx  ;file MDFAT buffer is after
				     ;  DOS FAT buffer
	add	bx,ax		;get 3 * bytes per sector
	mov	[si].SecMDFDr.RegBuf,bx  ;directory MDFAT buffer is after
				     ;  file MDFAT buffer
	;
CSResBuf: add	bx,ax		;add space for last buffer
	;
	ife	_WIN		;no TPA in Windows
	xor	ax,ax		;get free value
	call	TPA		;go free the TPA
	endif
	;
	shrn	bx,4,cl		;convert to paragraphs
	calldos	ALLOC		;allocate the buffer space
	;
	ife	_WIN		;no TPA in Windows
	pushf			;save allocation result
	push	ax		;save segment
	mov	ax,1		;get reserve value
	call	TPA		;re-reserve the TPA
	pop	ax		;get back segment
	popf			;get back allocation result
	endif
	;
	 jc	CSError		;give up if no memory
	mov	[si].BufSeg,ax	;save buffer segment
	;
CSClrBuf: mov	ax,-1		;get empty buffer flag
	mov	[si].SecDFAT.RegCur,ax   ;no sector in DOS FAT buffer
	mov	[si].SecMDFFl.RegCur,ax  ;no sector in file MDFAT buffer
	mov	[si].SecMDFDr.RegCur,ax  ;no sector in dir MDFAT buffer
	;
	; Set up uncompressed cluster size
	;
CSUClust: mov	al,[si].SecClust	;get sectors per cluster
	xor	ah,ah		;clear high
	mov	bx,ax		;save it
	cmp	wptr HostFlag,0	;host-based ratios requested?
	 je	CSMulClu		;if not go on
	mov	dl,[si].HostDrv	;get host drive letter
	inc	dl		;bump for 1=A etc.
	calldos	DSKFREE		;get sectors per cluster in AX
	cmp	ax,0FFFFh 	;any problem?
	 jne	CSMulClu		;if not go on
	mov	ax,bx		;error -- get back compressed
				;  cluster size
	;
CSMulClu: mov	[si].UCCSec,ax	;set uncompressed cluster sectors
	mov	cl,[si].LByteSec	;get log2(bytes per sector)
	shl	ax,cl		;get bytes per cluster
	mov	[si].UCCBytes,ax	;set uncompressed cluster bytes
	;
	; Clear directory buffer, show drive is open, then exit
	;
	mov	[si].DirSecNo,-1	;no sector in directory buffer
	mov	al,DriveLet	;get drive letter
	mov	[si].CDLetter,al	;save as current compressed drive
	;
CSOK:	xor	ax,ax		;show no error
	jmp	short CSExit	;and get out
	;
	; Error handling
	;
CSError:	push	ax		;save error code
	mov	bx,[si].CDFile	;get CVF handle
	calldos	CLOSE		;close it
	mov	[si].CDLetter,0	;show nothing there
	pop	ax		;get back error code
	;
CSExit:	popm	di,si		;restore registers
	exit			;all done
	;
	;
	; CompDriveCleanup - Clean up after compression data retrieval
	;
	; On entry:
	;	Argument on stack:
	;	  CDInfo *CDriveData    compressed drive data
	;
	; On exit:
	;	No return value
	;	AX, BX, CX, DX, ES destroyed
	;	All other registers and interrupt state unchanged
	;
	entry	CompDriveCleanup,argframe,far
	;
	argW	CDDAddr		;pointer to compressed drive data
	;
	pushm	si,di		;save registers
	mov	si,CDDAddr	;get compressed drive data address
	cmp	[si].CDLetter,0	;was a drive open?
	 jz	CCChkBuf		;if not go on
	cmp	[si].DrvType,3	;is it DBLSPACE?
	 jae	CCChkBuf		;if not go on
	mov	bx,[si].CDFile	;get file handle
	calldos	CLOSE		;close the CVF
	;
CCChkBuf: mov	cx,[si].BufSeg	;get buffer segment
	 jcxz	CCDone		;if not reserved we're done
	;
	ife	_WIN		;no TPA in Windows
	xor	ax,ax		;get free value
	call	TPA		;go free the TPA
	endif
	;
	mov	es,cx		;copy buffer segment
	calldos	FREE		;free it
	;
	ife	_WIN		;no TPA in Windows
	mov	ax,1		;get reserve value
	call	TPA		;re-reserve the TPA
	endif
	;
CCDone:	xor	ax,ax		;get zero
	mov	[si].CDLetter,al	;show no drive open
	mov	[si].BufSeg,ax	;show no buffer reserved
	;
	popm	di,si		;restore registers
	exit			;all done
	;
	;
	; CompData - Calculate compression data values for a file
	;
	; On entry:
	;	Arguments on stack:
	;	  CDInfo *CDriveData    compressed drive data
	;	  find_t *FindData	    directory search data area
	;	  char *FileName	    full file name for Win95 call
	;	  ulong *UncompSectors  returned total uncompressed
	;			    sectors
	;	  ulong *CompSectors    returned total compressed sectors
	;
	; On exit:
	;	AX = DOS error number if I/O error, 0 if OK, -1 if
	;	     disk structure error
	;	TotalClusters and TotalSectors filled in if AX = 0
	;	BX, CX, DX, ES destroyed
	;	All other registers and interrupt state unchanged
	;
	entry	CompData,varframe,far
	;
	argW	CSecAddr		;pointer to total comp sectors
	argW	USecAddr		;pointer to total uncomp sectors
	argW	FNAddr		;pointer to filename
	argW	FDAddr		;pointer to directory search data
	argW	CDDAddr		;pointer to compressed drive data
	;
	varW	TempDSec		;relative directory sector for this
				;  entry
	varD	CompSec		;total compressed sectors
	varend
	;
	pushm	si,di		;save registers
	;
	; Initialize pointers
	;
	mov	si,CDDAddr	;point to compressed drive data
	mov	di,FDAddr 	;point to find data
	mov	es,[si].BufSeg	;get buffer segment
	;
	; Figure out how many uncompressed sectors the file takes,
	; rounding up to the proper cluster size (host or compressed).
	;
	dload	ax,dx,[di].DX_SIZE	;get size from find buffer
	mov	bx,[si].UCCBytes	;get cluster size in bytes
	mov	cx,bx		;copy it
	dec	cx		;get cluster size - 1
	add	ax,cx		;add for roundoff
	adc	dx,0		;add any carry
	div	bx		;get file's cluster count in AX
	mov	bx,[si].UCCSec	;get sectors / cluster
	mul	bx		;now DX:AX = uncompressed sectors
				;  (rounded up to next cluster)
	mov	bx,USecAddr	;get uncompressed count addr
	dstore	[bx],ax,dx	;store it for caller
	;
	; For Win95 DRVSPACE, get the compressed size directly and
	; convert it to a sector count
	;
; FIXME!
;	cmp	[si].DrvType,2	;Win95 DRVSPACE?
;	 jne	CDDirEnt		;if not go on
	mov	dx,FNAddr		;point to name
	mov	bl,2		;function 2, get compressed size
	calldos	95_ATT		;make LFN call
	 jc	CDErrJmp		;get out if error
	mov	cl,[si].LByteSec	;get shift count
	xor	ch,ch		;clear high byte

CD95Shft:	dshr	ax,dx		;slide right one
	loop	CD95Shft		;loop to get sector count
	jmp	CDResult		;go store result
	;
	; Figure out the DOS cluster number for the directory entry for
	; this file from the entry number and the starting cluster number
	; for the directory
	;
CDDirEnt:

;push ax
;mov ax,0DEF0h
;int 2Fh
;pop ax
	mov	bx,[di].DX_DENUM	;get entry number
	mov	cl,[si].LDirSec	;get log2(entries per sector)
	shr	bx,cl		;convert entry number to relative
				;  sector no. w/in directory
	mov	TempDSec,bx	;save it
	cmp	bx,[si].DirSecNo	;is it what's already there?
	 je	CDDirClu		;if so skip FAT scan
	mov	ax,[di].DX_DCLUS	;get directory starting cluster
	or	ax,ax		;is it the root directory?
	 jz	CDRoot		;if so go on
	;
	; If the directory is fragmented, link through the DOS FAT to
	; find the desired cluster within the directory
	;
CDDirLp:	mov	cl,[si].SecClust	;get sectors per cluster
	xor	ch,ch		;clear high part
	sub	bx,cx		;is dir entry in current cluster?
	 jb	CDLdDir		;if so go load it
	pushm	bx,di		;save registers destroyed by FATLink
	call	FATLink		;link to next directory cluster
	popm	di,bx		;restore registers
	 jc	CDErrJmp		;get out if error
	cmp	ax,0FFF8h 	;end of file?
	 jb	CDDirLp		;if not done keep looking
	mov	ax,-1		;if EOF something is wrong!
	;
CDErrJmp: jmp	CDError		;if error get out
	;
	; Now we have the directory cluster number.  Convert it to a sector
	; number on the compressed drive.
	;
CDLdDir:	add	bx,cx		;get back sector within cluster
	mov	dx,ax		;copy starting cluster
	sub	dx,2		;adjust for FAT offset
	xor	ax,ax		;clear high part
	mov	cl,[si].LSecClu	;get shift count
	;
CLDShift: dshl	dx,ax		;slide left one
	loop	CLDShift		;and loop until we have sector #
	;
	add	dx,[si].RootSecs	;skip over root directory
	adc	ax,0		;add any carry
	jmp	short CDDirSec	;and go on
	;
	; This is the root directory -- effectively relative sector "0"
	;
CDRoot:	xor	dx,dx		;set sector 0
	;
	; Find and load the directory sector -- DBLSPACE and SuperStor
	; both show 1 FAT, but DBLSPACE pretends it has 2!
	;
CDDirSec: mov	cx,[si].SecFAT	;get FAT space
	cmp	[si].DrvType,3	;DBLSPACE?
	 jae	CDAddRes		;if not go on
	shl	cx,1		;double for second "pseudo" FAT
	;
CDAddRes: add	cx,[si].SecRes	;add reserved space
	add	dx,cx		;calculate correct INT 25 sector
	adc	ax,0		;add any carry
	dstore	[si].SecDir.RegStart,dx,ax  ;store cluster's start sector
	mov	dx,bx		;sector within cluster to DX
	mov	bx,SecDir 	;point to directory buffer info
	mov	[si].DirSecNo,-1	;clear current sector in case of
				;  error during read
;push ax
;mov ax,0DEF1h
;int 2Fh
;pop ax
	mov	ah,1		;use INT 25 read
	call	LoadSec		;load the directory sector
	 jc	CDError		;holler if error
	mov	bx,TempDSec	;get number of relative directory
				;  sector we just read
	mov	[si].DirSecNo,bx	;save it for later
	;
	; The directory sector is in our buffer.  Get the file's starting
	; cluster number from its directory entry.
	;
CDDirClu: mov	bx,[si].SecDir.RegBuf  ;point to directory buffer
	mov	di,[di].DX_DENUM	;get back raw entry number
	and	di,[si].DirEMask	;get entry within sector
	mov	cl,LDirELen	;get log2(bytes per entry)
	shl	di,cl		;get byte offset in sector
	mov	ax,es:[bx][di].DE_CLUST  ;got the starting cluster
				     ;  (whew)!
	;
	; For SuperStor drives, call the driver to get the chain size
	;
	cmp	[si].DrvType,3	;DBLSPACE?
	 jb	CDDSComp		;if so go handle that
	mov	wptr SSData.SSInfo,ax  ;save starting cluster
	mov	al,[si].CDLetter	;get drive letter
	mov	ah,SSCHAIN	;function code
	call	SSIOCtl		;get chain size
	 jc	CDError		;get out if error
	dload	ax,dx,<dptr SSData.SSInfo>  ;load the chain size in bytes
	mov	cl,[si].LByteSec	;get shift count
	xor	ch,ch		;clear high byte
	;
CDSSShft: dshr	ax,dx		;shift right one
	loop	CDSSShft		;loop to convert to sector count
	;
	jmp	CDResult		;go save result and exit
	;
	; It's DBLSPACE -- clear the cumulative compression count for
	; the file
	;
CDDSComp: xor	dx,dx		;get zero
	dstore	CompSec,dx,dx	;clear compressed counter
	;
	; For each cluster in the file, get its sector counts and
	; add them to the totals, then link to the next cluster in
	; the DOS FAT chain.
	;
;push ax
;mov ax,0DEF2h
;int 2Fh
;pop ax
CDLoop:	push	ax		;save current DOS FAT cluster
	mov	bx,SecMDFFl	;point to file MDFAT info
	call	GetMDFAT		;get MDFAT entry in AX:DX
	pop	bx		;restore DOS FAT cluster to BX
	 jc	CDError		;if error in GetMDFAT get out
	and	ax,MEHCSec	;get compressed sector count - 1
	mov	cl,MEHCSShf	;get shift count for it
	shr	ax,cl		;slide into place
	inc	ax		;bump to get actual count
	add	wptr CompSec,ax	;bump compressed count
	adc	wptr CompSec+2,0	;add any carry
	mov	ax,bx		;copy current DOS FAT cluster
	call	FATLink		;link down the DOS FAT
	 jc	CDError		;get out if error
	cmp	ax,0FFF8h 	;end of file?
	 jb	CDLoop		;if not done go on (AX = new cluster)
	;
	; Reached end of DOS FAT chain for the file.  Save the collected
	; compression data in the caller's variable and exit.
	;
	dload	ax,dx,CompSec	;get compressed count
	;
CDResult: mov	bx,CSecAddr	;get compressed count addr
	dstore	[bx],ax,dx	;store it
	xor	ax,ax		;get success code
	jmp	short CDExit	;and get out
	;
CDError:	mov	ax,1		;show no compression data available
	;
CDExit:	popm	di,si		;restore registers
	exit			;all done
	;
	;
	; DSPOpen - Try to open the DBLSPACE.nnn / DRVSPACE.nnn file
	;
	; On entry:
	;	AX = "BL" for DBLSPACE or "RV" for DRVSPACE
	;
	; On exit:
	;	AX, flags set by DOS open call
	;	DX destroyed
	;	All other registers and interrupt state unchanged
	;
	;
	entry	DSPOpen,noframe	;define entry point
	;
	mov	wptr DSFType,ax	;store middle characters into name
	mov	al,ACC_READ	;read-only mode
	lea	dx,DSFName	;point to name
	calldos	OPEN		;open the CVF
	exit			;outta here
	;
	;
	; FATLink - Link to next cluster in the DOS FAT
	;
	; On entry:
	;	AX = Current DOS FAT cluster number
	;	SI = pointer to compressed drive data
	;	ES = buffer segment
	;
	; On exit:
	;	Carry flag clear if OK, set if error
	;	If error, AX = DOS error number
	;	If no error, AX = next DOS FAT cluster number, or
	;	  FFF8 - FFFF if EOF
	;	BX, CX, DX, DI destroyed
	;	All other registers and interrupt state unchanged
	;
	;
	entry	FATLink,noframe	;define entry point
	;
	push	ax		;save original cluster
	mov	cx,ax		;copy it
	xor	dx,dx		;clear high word
	dshl	ax,dx		;get 2 * cluster
	cmp	bptr [si].FATType,0 ;check if 12-bit or 16-bit
	 jz	FLGetSec		;if 16-bit, byte offset = 2 * cluster
	add	ax,cx		;get 3 * cluster
	adc	dx,0		;add in carry
	dshr	ax,dx		;12-bit, byte offset = 1.5 * cluster
	;
	; Now DX:AX is byte offset into FAT
	;
FLGetSec: push	ax		;save low part of byte offset
	mov	cl,[si].LByteSec	;get shift count to convert to sector
	xor	ch,ch		;clear high part
	;
FLShfLp:	dshr	ax,dx		;shift byte offset over
	loop	FLShfLp		;loop through shift
	;
	; Now AX is sector number within FAT
	;
	pop	cx		;restore low part of offset to CX
	mov	bx,SecDFAT	;point to DOS FAT info
	cmp	ax,[si][bx].RegCur	;is the sector already there?
	 je	FLGotSec		;if so go on
	mov	dx,ax		;copy sector number for LoadSec
;push ax
;mov ax,0DEF3h
;int 2Fh
;pop ax
	mov	ah,1		;use INT 25 read
	call	LoadSec		;load the FAT sector
	 jc	FLError		;if error get out
	;
FLGotSec: mov	di,[si][bx].RegBuf	;get buffer address
	mov	bx,cx		;copy offset
	and	bx,[si].BSecMask	;get sector-relative part
	mov	ax,es:[di][bx]	;get byte of FAT entry
	cmp	[si].FATType,0	;is it a 12-bit?
	 jz	FL16OK		;if not go on
	inc	bx		;check second byte
	cmp	bx,[si].ByteSec	;second byte overflow?
	 jb	FLGot12		;if not go on
	inc	dx		;bump offset
	mov	bx,SecDFAT	;point to DOS FAT info
	mov	ah,1		;use INT 25 read
	call	LoadSec		;load next FAT sector
	 jc	FLError		;if error get out
	mov	ah,es:[di]	;get high byte from start of buffer
				;  (low is in AL already)
	;
FLGot12:	pop	bx		;get back original cluster
	test	bl,1		;was original cluster odd?
	 jnz	FL12Shft		;if so go shift entry over
	and	ax,0FFFh		;even cluster, isolate 12 bits
	jmp	short FL12Eof	;go check for EOF
	;
FL12Shft: shrn	ax,4,cl		;odd cluster, shift new entry right
	;
FL12Eof:	cmp	ax,0FF8h		;is it in EOF range?
	 jb	FLOK		;if not get out
	or	ah,0F0h		;set high nibble for EOF detect
	jmp	short FLOK	;and return
	;
FLError:	add	sp,2		;throw away original cluster number
	stc			;set error
	jmp	short FLExit	;and get out
	;
FL16OK:	add	sp,2		;throw away original cluster number
	;
FLOK:	clc			;show no error
	;
FLExit:	exit			;outta here
	;
	;
	; GetMDFAT - Get an MDFAT entry for a cluster
	;
	; On entry:
	;	AX = DOS FAT cluster number
	;	BX = pointer to MDFAT buffer info to use in compressed
	;	     drive data
	;	SI = pointer to compressed drive data
	;	ES = buffer segment
	;
	; On exit:
	;	Carry flag clear if OK, set if error
	;	If error, AX = DOS error number
	;	If no error, AX:DX = MDFAT entry 
	;	CX destroyed
	;	All other registers and interrupt state unchanged
	;
	;
	entry	GetMDFAT,noframe	;define entry point
	;
	push	di		;save registers
	add	ax,[si].BegMDFAT	;adjust for reserved clusters in
				;  MDFAT ==> gives MDFAT entry no.
	mov	dx,ax		;copy it
	mov	cl,[si].LMDFEnt	;get log2(MDFAT entries / sector)
	shr	dx,cl		;calculate MDFAT sector number
	cmp	dx,[si][bx].RegCur	;is the sector already there?
	 je	GCGotSec		;if so go on
;push ax
;mov ax,0DEF4h
;int 2Fh
;pop ax
	push	ax		;save AX
	xor	ah,ah		;use CVF read
	call	LoadSec		;load the MDFAT sector
	pop	ax		;restore AX
	 jc	GCExit		;exit w/ carry set if error
	;
GCGotSec: and	ax,[si].MDFEMask	;get entry number within sector
	mov	cl,LMDFELen	;get shift count
	shl	ax,cl		;make offset within sector
	mov	di,[si][bx].RegBuf	;get buffer offset
	add	di,ax		;point to entry
	dload	dx,ax,es:[di]	;get MDFAT entry
	clc			;show no error
	;
GCExit:	pop	di		;restore registers
	exit			;outta here
	;
	;
	; LoadSec - Load a sector of data from the CVF or drive
	;
	; On entry:
	;	AH = 0 to load from CVF, 1 to load direct from drive
	;	BX = offset of sector info for the desired region in
	;	     compressed drive data
	;	DX = relative sector number within this region
	;	SI = pointer to compressed drive data
	;	ES = buffer segment
	;
	; On exit:
	;	Carry flag clear if OK, set if error
	;	If no error, appropriate buffer filled in
	;	If error, AX = DOS error number
	;	Otherwise all registers and interrupt state unchanged
	;
	;
	entry	LoadSec,varframe	;define entry point
	;
	varW	LSFunc		;saved function code
	var	ReadData,<sizeof DiskIO>  ;data for INT 25
	varend
	;
	pushm	ax,bx,cx,dx,di	;save registers
	mov	LSFunc,ah 	;save function code
	mov	ax,[si][bx].RegStart+2  ;get high starting sector
	add	dx,[si][bx].RegStart    ;add low starting sector
	adc	ax,0		;add any overflow
	mov	[si][bx].RegCur,-1	;show nothing in buffer in case
				;  of error during read
	mov	di,[si][bx].RegBuf	;get buffer address
	cmp	bptr LSFunc,0	;check function
	 je	LSCVF		;if CVF read go on
	;
	; Read data via INT 25
	;
	lea	bx,ReadData	;point to data structure
	dstore	[bx].DIStart,dx,ax	;store sector number
	mov	[bx].DICount,1	;read 1 sector
	dstore	[bx].DIBuf,di,es	;store buffer address
	mov	al,[si].CDLetter	;get drive
	sub	al,'A'		;make it 0-based
	mov	cx,0FFFFh 	;set flag for extended call
	int	25h		;load the sector
	pop	ax		;INT 25 leaves flags on stack
	jmp	LSDone		;return result
	;
	; Convert sector number to CVF byte offset and seek to it
	;
LSCVF:	mov	cl,[si].LByteSec	;get shift count for bytes / sector
	xor	ch,ch		;clear high byte
	;
LSShift:	dshl	dx,ax		;shift it over 1
	loop	LSShift		;loop for all bits
	mov	cx,ax		;copy high-order offset
	mov	bx,[si].CDFile	;get file handle
	mov	al,SEEK_BEG	;seek from beginning
	calldos	SEEK		;seek to proper sector
	 jc	LSDone		;get out if error
	;
	; read the sector from the CVF
	;
	mov	dx,di		;copy buffer offset
	mov	cx,[si].ByteSec	;get read count
	push	ds		;save DS
	loadseg	ds,es		;copy buffer segment to DS
	calldos	READ		;read the buffer
	pop	ds		;restore DS
	;
LSDone:	popm	di,dx,cx,bx,ax	;restore caller's registers
	 jc	LSExit		;if error get out
	mov	[si][bx].RegCur,dx	;store new current sector number
	;
LSExit:	exit			;outta here
	;
	;
	; TPA - Free the transient program area prior to doing memory
	;       allocation / deallocation, or reserve it after
	;
	; On entry:
	;	AX = 0 to free, 1 to reserve
	;
	; On exit:
	;	AX destroyed
	;	All registers and interrupt state unchanged
	;
	;
	ife	_WIN
	entry	TPA,noframe	;define entry point
	;
	push	bx		;save register destroyed by ServCtrl
	mov	bx,3		;TPA function
	push	bx		;put it on stack
	push	ax		;arg on stack
	call	ServCtrl		;free or reserve the TPA
	pop	bx		;restore BX
	exit			;outta here
	endif
	;
	;
	; SSIOCtl - Make a SUperStor IOCTL call
	;
	; On entry:
	;	AL = drive letter
	;	AH = SuperStor function code
	;
	; On exit:
	;	AX destroyed
	;	All registers and interrupt state unchanged
	;
	;
	entry	SSIOCtl,noframe	;define entry point
	;
	push	si		;be sure SI is preserved
	mov	SSData.SSSig,0AA55h ;signature bytes
	mov	SSData.SSProd,1	;product ID
	mov	SSData.SSMode,ah	;function code for call
	mov	bl,al		;drive letter
	sub	bl,('A'-1)	;convert to 1=A, etc.
	mov	cx,(sizeof SSIODATA)    ;bytes to send
	mov	ax,(D_IOCTL shl 8) + 5  ;IOCTL write function
	mov	dx,offset SSData	;point to IOCTL packet
	calldos			;make the call
	 jc	SSDone		;get out if error
	cmp	SSData.SSSig,0AA55h  ;signature changed?
	 je	SSError		;if not it's wrong
	test	bptr SSData.SSSig,80h  ;high bit set?
	 jnz	SSError		;if so it's an error
	clc			;no error
	jmp	SSDone		;get out
	;
SSError:	stc			;error flag
	;
SSDone:	pop	si		;restore SI
	exit			;outta here
	;
	;
@curseg	ends			;close segment
	;
	;
	end			;that's all

