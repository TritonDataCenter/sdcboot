	assume cs:code, ds:code
CODE	SEGMENT
	org 100h
start:
	push cs
	pop  ds
	mov ax, 0909h
	mov dx, offset string
	int 21h
	int 20h
string  db 10,13,'This is some test program',10,13,'$'
CODE	ENDS
	end start
