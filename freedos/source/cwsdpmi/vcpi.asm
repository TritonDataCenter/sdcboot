; Copyright (C) 1995,1996 CW Sandmann (sandmann@clio.rice.edu) 1206 Braelinn, Sugarland, TX 77479
; Copyright (C) Aug 5th 1991 Y.Shibata
	title	vcpi
	include segdefs.inc
	include vcpi.inc

	start_data16
emm_handle	dw	0
emm_name	db	"EMMXXXX0",0
	end_data16

	start_code16

; Check for Int 67 hooked; destroys ax,bx,es; returns zero flag if not OK

check67	proc	near
	mov	ax,3500h+EMS_REQ	;Check for valid INT handler
	int	21h
	mov	ax,es
	or	ax,bx
	ret
check67	endp


;  EMM handle allocation - recommended by vcpi document
;
; void ems_init(void)

	public	_ems_init
_ems_init	proc	near
	mov	dx,offset DGROUP:emm_name
	mov	ax,3d00h		;Open Handle
	int	21h
	jc	short no_ems
	mov	bx,ax
	mov	ah,3eh			;Close Handle
	int	21h
	call	check67			;Double check; might be file
	jz	short no_ems
	mov	bx,1
	mov	ah,43h			;Allocate Pages(1 Page Only)
	int	EMS_REQ
	cmp	ah,0
	jne	short no_ems
	mov	emm_handle,dx		;Save to deallocate later
no_ems:
	ret
_ems_init	endp

;  EMS Page Deallocated
;
;void ems_free(void)

	public	_ems_free
_ems_free	proc	near
	call	check67
	jz	short no_ems

	mov	dx,emm_handle		;EMS_Handle
	or	dx,dx
	jz	short no_ems		;never allocated
	mov	ah,45h			;Deallocate Pages
	int	EMS_REQ
	ret
_ems_free	endp

;  VCPI Installed Check
;
;word16	vcpi_present(void)
;
;result	-1:VCPI Installed 0:VCPI Not Installed

	public	_vcpi_present
_vcpi_present	proc	near
	call	check67
	jz	no_ems			; also leaves ax zero for return

	mov	ax,VCPI_PRESENT		;VCPI Present
	int	VCPI_REQ
	sub	ah,1
	sbb	ax,ax			;ah = 0 -> AX = -1
	ret
_vcpi_present	endp

;  VCPI Get Interface
;
;word32	get_interface(word32 far *page_table,GDT_S *gdt)
;
	public	_get_interface
_get_interface	proc	near
	push	bp
	mov	bp,sp
	push	si
	push	di

	push	es
	mov	si,[bp+8]		;DS:SI = &GDT[g_vcpicode]
	les	di,[bp+4]		;ES:DI = Page Table (DI = 0)
	mov	ax,VCPI_INTERFACE
	int	VCPI_REQ
	mov	eax,ebx
	shld	edx,eax,16		;DX:AX = EBX
	pop	es

	pop	di
	pop	si
	pop	bp
	ret	
_get_interface	endp

;  VCPI Maximum page number
;
;word32	vcpi_maxpage(void)
;
;result	max returnable page number

	public	_vcpi_maxpage
_vcpi_maxpage	proc	near
	mov	ax,VCPI_MAX_PHYMEMADR
	int	VCPI_REQ
	jmp	short vcpi_alloc_success
_vcpi_maxpage	endp

;  VCPI Unallocated Page count
;
;word32	vcpi_capacity(void)
;
;result	Free VCPI Memory(Pages)

	public	_vcpi_capacity
_vcpi_capacity	proc	near
	mov	ax,VCPI_MEM_CAPACITY
	int	VCPI_REQ
	jmp	short vcpi_word32
_vcpi_capacity	endp

;  VCPI Memory Allocate
;
;word32	vcpi_alloc(void)
;
;result	Allocate Page No.

	public	_vcpi_alloc
_vcpi_alloc	proc	near
	mov	ax,VCPI_ALLOC_PAGE
	int	VCPI_REQ
	test	ah,ah
	je	short vcpi_alloc_success
	xor	edx,edx			;Error result = 0
vcpi_alloc_success:
	shr	edx,12
vcpi_word32:
	mov	ax,dx
	shr	edx,16
	ret
_vcpi_alloc	endp

;  VCPI Memory Deallocate
;
;void	vcpi_free(word32 page_number)

	public	_vcpi_free
_vcpi_free	proc	near
	push	bp
	mov	bp,sp
;	movzx	edx,word ptr 4[bp]
	mov	edx,[bp+4]
	pop	bp

	sal	edx,12			;Address = Page_number * 4KB
	mov	ax,VCPI_FREE_PAGE
	int	VCPI_REQ

	ret
_vcpi_free	endp

;  VCPI Get PIC Vector
;
;word16	vcpi_get_pic(void)
;
;Result MASTER PIC Vector No.(IRQ0)

	public	_vcpi_get_pic
_vcpi_get_pic	proc	near
	mov	ax,VCPI_GET_PIC_VECTOR
	int	VCPI_REQ
	mov	ax,bx			;MASTER PIC Vector
	ret
_vcpi_get_pic	endp

;  VCPI Get PIC Vector
;
;word16	vcpi_get_secpic(void)
;
;Result SLAVE PIC Vector No.(IRQ0)

	public	_vcpi_get_secpic
_vcpi_get_secpic	proc	near
	mov	ax,VCPI_GET_PIC_VECTOR
	int	VCPI_REQ
	mov	ax,cx			;SLAVE PIC Vector
	ret
_vcpi_get_secpic	endp

;  VCPI Set PIC Vector
;
;void	vcpi_set_pics(word16 master_pic, word16 slave_pic)

	public	_vcpi_set_pics
_vcpi_set_pics	proc	near
	push	bp
	mov	bp,sp
	mov	bx,4[bp]		;MASTER PIC Vector
	mov	cx,6[bp]		;SLAVE PIC Vector
	pop	bp
	mov	ax,VCPI_SET_PIC_VECTOR
	int	VCPI_REQ
	ret
_vcpi_set_pics	endp

	end_code16

	end
