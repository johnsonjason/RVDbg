.MODEL flat, C

context STRUCT
	dwEax DWORD ?
	dwEbx DWORD ?
	dwEcx DWORD ?
	dwEdx DWORD ?
	dwEsi DWORD ?
	dwEdi DWORD ? 
	dwEbp DWORD ?
	dwEsp DWORD ?
	dwEip DWORD ?
	dwReturnAddress DWORD ?
	dwExceptionComparator DWORD ?
	dwExceptionCode DWORD ?
context ENDS

floatcontext STRUCT
	iXmm0 XMMWORD ?
	iXmm1 XMMWORD ?
	iXmm2 XMMWORD ?
	iXmm3 XMMWORD ?
	iXmm4 XMMWORD ?
	iXmm5 XMMWORD ?
	iXmm6 XMMWORD ?
	iXmm7 XMMWORD ?
floatcontext ENDS

EXTERN GlobalContext: context
EXTERN SSEGlobalContext: floatcontext
EXTERN RealKiUserExceptionDispatcher: DWORD
EXTERN EnterDebugState: PROC

.CODE

OPTION PROLOGUE:NONE 
OPTION EPILOGUE:NONE 

SAVE_REGISTER_STATE MACRO

	mov [GlobalContext.dwEax], eax
	mov [GlobalContext.dwEbx], ebx
	mov [GlobalContext.dwEcx], ecx
	mov [GlobalContext.dwEdx], edx
	mov [GlobalContext.dwEsi], esi
	mov [GlobalContext.dwEdi], edi
	mov [GlobalContext.dwEbp], ebp
	mov [GlobalContext.dwEsp], esp

ENDM

SAVE_XMM_STATE MACRO

	movaps [SSEGlobalContext.iXmm0], xmm0
	movaps [SSEGlobalContext.iXmm1], xmm1
	movaps [SSEGlobalContext.iXmm2], xmm2
	movaps [SSEGlobalContext.iXmm3], xmm3
	movaps [SSEGlobalContext.iXmm4], xmm4
	movaps [SSEGlobalContext.iXmm5], xmm5
	movaps [SSEGlobalContext.iXmm6], xmm6
	movaps [SSEGlobalContext.iXmm7], xmm7

ENDM

RESTORE_REGISTER_STATE MACRO

	mov eax, [GlobalContext.dwEax]
	mov ebx, [GlobalContext.dwEbx]
	mov ecx, [GlobalContext.dwEcx]
	mov edx, [GlobalContext.dwEdx]
	mov esi, [GlobalContext.dwEsi]
	mov edi, [GlobalContext.dwEdi]
	mov ebp, [GlobalContext.dwEbp]
	mov esp, [GlobalContext.dwEsp]

ENDM

RESTORE_XMM_STATE MACRO

	movaps xmm0, [SSEGlobalContext.iXmm0]
	movaps xmm1, [SSEGlobalContext.iXmm1]
	movaps xmm2, [SSEGlobalContext.iXmm2]
	movaps xmm3, [SSEGlobalContext.iXmm3]
	movaps xmm4, [SSEGlobalContext.iXmm4]
	movaps xmm5, [SSEGlobalContext.iXmm5]
	movaps xmm6, [SSEGlobalContext.iXmm6]
	movaps xmm7, [SSEGlobalContext.iXmm7]

ENDM


SaveRegisterState PROC ExceptionCode:DWORD, ExceptionContext:DWORD

	cmp dword ptr [esp + 08h], 0C0000096h ; Avoid C++ exceptions, add a comparison for each type of debugger exception (relative to this one)
	je ArbitraryExceptionState ; Jump to advanced exception handling
	jmp BadExceptionState ; Jump to regular exception handling

ArbitraryExceptionState:

	SAVE_REGISTER_STATE ; Saves the state of the general purpose registers prior to exception handling
	SAVE_XMM_STATE ; Saves the state of SSE registers prior to exception handling

	mov eax, [esp + 11Ch] ; Stores the former ESP 
	mov [GlobalContext.dwEsp], eax
	mov eax, [eax]
	mov [GlobalContext.dwReturnAddress], eax
	mov eax, [esp + 14h]
	mov [GlobalContext.dwExceptionComparator], eax ; Should be the address of exception, which we will use to check if it exists in our registered IP exception list
	mov eax, [esp + 08h]
	mov [GlobalContext.dwExceptionCode], eax

	call EnterDebugState ; Enter advanced exception handling
	cmp eax, 00000000h
	jne ContinueArbitraryException

BadExceptionState: ; Exception instructions that were overwritten

	mov eax, [GlobalContext.dwEax]
	mov ecx, [esp + 04h]
	mov ebx, [esp]
	jmp dword ptr [RealKiUserExceptionDispatcher] ; Jump to original exception execution path

ContinueArbitraryException:

	RESTORE_REGISTER_STATE
	RESTORE_XMM_STATE

	jmp dword ptr [GlobalContext.dwExceptionComparator] ; Return to exception address with restored instruction (in the case of IMMEDIATE exception)
SaveRegisterState ENDP
END
