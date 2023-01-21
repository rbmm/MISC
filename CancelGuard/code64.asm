extern GenericProtectionHandler:proc
extern KiGenericProtectionFault:qword
extern __imp_ExIsSafeWorkItem:qword

.code

MyGenericProtectionFault proc
	
	push r15
	
	mov r15w,cs
	test r15b,1
	jnz @@1
	
	push r14
	push r13
	push r12
	push r11
	push r10
	push r9
	push r8
	push rdi
	push rsi
	push rbp
	push rsp
	push rbx
	push rdx
	push rcx
	push rax
	
	mov rcx,rsp
	
	sub rsp,20h
	call GenericProtectionHandler
	add rsp,20h
	
	test al,al
		
	pop rax
	pop rcx
	pop rdx
	pop rbx
	pop rsp
	pop rbp
	pop rsi
	pop rdi
	pop r8
	pop r9
	pop r10
	pop r11
	pop r12
	pop r13
	pop r14
	
@@1:
	pop r15
	jnz @@3
	
	add rsp,8
	iretq
	
@@3: jmp KiGenericProtectionFault
	
MyGenericProtectionFault endp

testSafeWorkItem proc
	xor rdx,rdx
	mov rcx,rdx
	call __imp_ExIsSafeWorkItem
	mov rax,rdx
	ret
testSafeWorkItem endp

GetRegs proc
	mov [rcx],rsp
	mov [rcx+8],rbx
	mov [rcx+16],rdi
	mov [rcx+24],rsi
	mov [rcx+32],rbp
	mov [rcx+40],r12
	mov [rcx+48],r13
	mov [rcx+56],r14
	mov [rcx+64],r15
	ret
GetRegs endp

GetIdtEntry proc
	sidt [rsp + 16h]
	mov rax,[rsp + 18h]
	shl rcx,4
	add rax,rcx
	ret
GetIdtEntry endp

MOV_XMM MACRO N
	movq @CatStr(xmm,N),qword ptr[rdx]
	ret
	IF N LT 8
	nop
	ENDIF
ENDM

MOV_XMM128 MACRO N
	movdqa @CatStr(xmm,N),[rdx]
	ret
	IF N LT 8
	nop
	ENDIF
ENDM

XMM proc

	mov eax,6
	mul cl
	mov rcx,offset @@0
	add rax,rcx
	jmp rax
	
@@0:

	n = 0
	REPT 16
	MOV_XMM %n
	n = n + 1
	endm

XMM endp

XMM128 proc

	mov eax,6
	mul cl
	mov rcx,offset @@0
	add rax,rcx
	jmp rax
	
@@0:
	
	n = 0
	REPT 16
	MOV_XMM128 %n
	n = n + 1
	endm
	
XMM128 endp

end