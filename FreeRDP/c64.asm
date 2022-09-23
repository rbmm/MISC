extern __imp_CommandLineToArgvW:QWORD
.code

findCTA proc

	shr ecx,3
	jrcxz @retz
	xchg rdi,rdx
	mov rax,__imp_CommandLineToArgvW
	repne scasq
	lea rax,[rdi-8]
	cmovne rax, rcx
	mov rdi,rdx
	ret
@retz:
	xor eax,eax
	ret

findCTA endp

end