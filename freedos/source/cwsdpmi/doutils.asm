; New version Oct 1994  -  Morten Welinder (terra@diku.dk)
; Note: the shift on CX really should be ECX, but we never move a large block

	title	doutils
	include	segdefs.inc
	include gdt.inc

	start_code16

	public	__do_memmov32
__do_memmov32:
	mov	al,cl
	shr	cx,2
	rep	movs dword ptr es:[edi],dword ptr [esi]
	and	al,3
	mov	cl,al
	rep	movs byte ptr es:[edi],byte ptr [esi]
	jmpt	g_ctss

	end_code16

	end
