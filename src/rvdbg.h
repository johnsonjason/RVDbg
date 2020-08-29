#ifndef RVDBG_H
#define RVDBG_H

#include <iostream>
#include <Windows.h>
#include <xmmintrin.h>
#include "dio.h"
#include "dbghooks.h"
#include "exception_store.h"
#include "thread_manager.h"
#include <synchapi.h>
#include <Zydis/Zydis.h>
#pragma comment(lib, "Zydis.lib")
#pragma comment(lib, "synchronization.lib")

namespace Debugger
{
	//
	// Strictly typed enum for general purpose and SSE registers
	// Used for mapping positions (values passed to the debugger from another task) to actual register states
	//

	typedef enum _GENERAL_REGISTER : DWORD
	{
#if defined _M_IX86
		Eax, Ebx, Ecx, Edx,
		Esi, Edi, Ebp, Esp, Eip, Last
#elif defined _M_AMD64
		Rax, Rbx, Rcx, Rdx,
		Rsi, Rdi, Rbp, Rsp,
		Rip, R8, R9, R10,
		R11, R12, R13, R14, R15, Last
#endif
	} GENERAL_REGISTER;

	typedef enum _SSE_REGISTER : DWORD
	{
		Xmm0, Xmm1, Xmm2, Xmm3,
		Xmm4, Xmm5, Xmm6, Xmm7,
#if defined _M_AMD64
		Xmm8, Xmm9, Xmm10, Xmm11,
		Xmm12, Xmm13, Xmm14, Xmm15
#endif
	} SSE_REGISTER;

	typedef enum _PROCESS_STATE : DWORD
	{
		StepState,
		Inclusive,
		Exclusive,
		Continuous
	} PROCESS_STATE;

	//
	// Define all general purpose registers for x86 and x64
	// Add additional fields for debugger purposes
	//

	typedef struct _DBG_CONTEXT_STATE
	{
#if defined _M_IX86

		DWORD dwEax;
		DWORD dwEbx;
		DWORD dwEcx;
		DWORD dwEdx;
		DWORD dwEsi;
		DWORD dwEdi;
		DWORD dwEbp;
		DWORD dwEsp;
		DWORD dwEip;
		DWORD dwReturnAddress;
		DWORD dwExceptionComparator;
		DWORD dwExceptionCode;
		DWORD EFlags;
#elif defined _M_AMD64
		DWORD64 dwRax;
		DWORD64 dwRbx;
		DWORD64 dwRcx;
		DWORD64 dwRdx;
		DWORD64 dwRsi;
		DWORD64 dwRdi;
		DWORD64 dwRbp;
		DWORD64 dwRsp;
		DWORD64 dwRip;
		DWORD64 dwR8;
		DWORD64 dwR9;
		DWORD64 dwR10;
		DWORD64 dwR11;
		DWORD64 dwR12;
		DWORD64 dwR13;
		DWORD64 dwR14;
		DWORD64 dwR15;
		DWORD64 dwReturnAddress;
		DWORD64 ExceptionCode;
		DWORD64 ExceptionComparator;
		DWORD64 RFlags;
#endif
	} DBG_CONTEXT_STATE;

	//
	// Define all Streaming SIMD Extensions registers for x86 and x64
	//

	typedef struct _DBG_SSE_REGISTERS
	{
		__m128 Xmm0;
		__m128 Xmm1;
		__m128 Xmm2;
		__m128 Xmm3;
		__m128 Xmm4;
		__m128 Xmm5;
		__m128 Xmm6;
		__m128 Xmm7;
#if defined _M_AMD64
		__m128 Xmm8;
		__m128 Xmm9;
		__m128 Xmm10;
		__m128 Xmm11;
		__m128 Xmm12;
		__m128 Xmm13;
		__m128 Xmm14;
		__m128 Xmm15;
#endif
	} DBG_SSE_REGISTERS;

	//
	// DebuggerSession is a session instantiated when a thread enters an arbitrary exception state
	// Supports x86 and AMD64 thread contexts
	// Supports general purpose and SSE registers
	// Used for controlling the debugger operating on the thread
	//

	class DebuggerSnapshot
	{
	public:

		DebuggerSnapshot(DBG_CONTEXT_STATE PrcState, DBG_SSE_REGISTERS PrcSSEState);


		void SetSSEReg(SSE_REGISTER Xmm, __m128 Value);
		__m128 GetSSEReg(SSE_REGISTER Xmm);

		void SetGeneralPurposeReg(GENERAL_REGISTER Register, DWORD_PTR Value);
		DWORD_PTR GetGeneralPurposeReg(GENERAL_REGISTER Register);
		void CopyToContext(DBG_CONTEXT_STATE& NewProcessorState, DBG_SSE_REGISTERS& NewProcessorSSEState);

	private:
		DBG_CONTEXT_STATE ProcessorState;
		DBG_SSE_REGISTERS ProcessorSSEState;
	};

	extern bool DebuggerPauseCondition;
	void SetProcessState(PROCESS_STATE State);
	PROCESS_STATE GetProcessState();
	DebuggerSnapshot* GetActiveSnapshot();

	//
	// Initializes important information for the debugger such as critical sections
	// Initializes conditon variables
	// Initializes subroutine hooks
	//

	void InitializeDebugInfo(DWORD InputThreadId, dio::Client* Repeater);
	void RunDebugger();
	int StepClassify(ZydisDecodedInstruction& Instruction, DWORD_PTR CurrentInstructionPointer);
	void StepDebugger();

}

extern "C" void SaveRegisterState(DWORD, DWORD);
extern "C" PVOID EnterDebugState();
extern "C" Debugger::DBG_CONTEXT_STATE GlobalContext;
extern "C" Debugger::DBG_SSE_REGISTERS SSEGlobalContext;
extern "C" PVOID RealKiUserExceptionDispatcher;
extern "C" PVOID RetDebuggerAddress;

#endif
