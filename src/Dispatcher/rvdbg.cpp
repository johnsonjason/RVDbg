#include "stdafx.h"
#include "rvdbg.h"

BOOLEAN Dbg::Debugger;
CRITICAL_SECTION Dbg::repr;
CONDITION_VARIABLE Dbg::reprcondition;
static Dbg::VirtualRegisters r_registers;

static void HandleSSE();

/* Resume exception-type threads */
static void ResumeSelfThreads()
{
	for (size_t iterator = 0; iterator < sizeof(Threads); iterator++)
	{
		if (Threads[iterator] != NULL)
			ResumeThread(Threads[iterator]);
	}
}

/* WIP, will be used for page exceptions */
static PVOID CallChainVPA()
{
	size_t ExceptionElement = SearchSector(Sector, 128, ExceptionComparator);
	if (ExceptionElement > 128)
		return NULL;
	return NULL;
}

static PVOID CallChain()
{
	// Search a sector for a listed exception
	size_t ExceptionElement = SearchSector(Sector, Dbg::GetSectorSize(), ExceptionComparator); 

	// If sector does not contain exception element
	if (ExceptionElement > Dbg::GetSectorSize()) 
	{
		// If true, used to bypass memory integrity checks
		if (ExceptionMode == MEMORY_EXCEPTION_CONTINUE) 
			// returns executable memory address in a copied module
			return (GetModuleHandleA(Dbg::GetCopyModuleName()) + (ExceptionComparator - (DWORD)GetModuleHandleA(MainModule))); 
		// if not MEMORY_EXCEPTION_CONTINUE, return unhandled exception
		return NULL; 
	}

	/*
	* PAUSE_ALL = Pauses all threads
	* PAUSE_ALL ^ MOD_OPT = Pauses all threads, but with the module option; the module option specifies that execution will continue in another module
	*/
	if (Dbg::GetPauseMode() == PAUSE_ALL || Dbg::GetPauseMode() == PAUSE_ALL ^ MOD_OPT)
	{
		// Suspend all threads except this debugger thread
		SuspendThreads(GetCurrentProcessId(), GetCurrentThreadId()); 
		// Set the debugger flag to true, a debugger is currently in session for this process
		Dbg::Debugger = TRUE;
		// wake the condition variable in the main module loop
		WakeConditionVariable(&Dbg::reprcondition); 
		// Resume all threads assigned to the thread-excepted pool
		ResumeSelfThreads(); 
	}

	// Used in the dispatcher-exception-handler's switch statement, decides what type of exception to be used
	Sector[ExceptionElement].ExceptionCode = ExceptionCode; 
	if (Dbg::GetPauseMode() == PAUSE_ALL ^ MOD_OPT || Dbg::GetPauseMode() == PAUSE_SINGLE ^ MOD_OPT || Dbg::GetPauseMode() == PAUSE_CONTINUE ^ MOD_OPT)
	{
		// Module option supplied, handle exception and set future EIP to module-copy
		r_registers.eip = Dispatcher::HandleException(Sector[ExceptionElement], Sector[ExceptionElement].ModuleName, TRUE); 
	}
	else
	{
		// No module option, regular handling, future EIP is the exception address
		r_registers.eip = Dispatcher::HandleException(Sector[ExceptionElement], Sector[ExceptionElement].ModuleName, FALSE); 
	}
	
	Sector[ExceptionElement].ThreadId = GetCurrentThreadId();
	Sector[ExceptionElement].Thread = GetCurrentThread();
	// Set the flag to true, notifying that the arbitrary exception handler is present for this section
	Sector[ExceptionElement].IsAEHPresent = TRUE; 
	// the return address for this exception context
	Sector[ExceptionElement].ReturnAddress = r_registers.ReturnAddress; 

	CurrentSection = Sector[ExceptionElement];

	if (Dbg::GetPauseMode() == PAUSE_ALL || Dbg::GetPauseMode() == PAUSE_SINGLE || Dbg::GetPauseMode() == PAUSE_CONTINUE || 
		Dbg::GetPauseMode() == Dbg::GetPauseMode() == PAUSE_ALL ^ MOD_OPT || Dbg::GetPauseMode() == PAUSE_SINGLE ^ MOD_OPT || PAUSE_CONTINUE ^ MOD_OPT  )
	{
		EnterCriticalSection(&RunLock);
		// Put the debugger in a waiting state to allow user response until the debugger is continued
		SleepConditionVariableCS(&Runnable, &RunLock, INFINITE); 
		LeaveCriticalSection(&RunLock);
		// Resume all threads
		if (Dbg::GetPauseMode() != PAUSE_SINGLE && Dbg::GetPauseMode() != PAUSE_CONTINUE)
			ResumeThreads(GetCurrentProcessId(), GetCurrentThreadId()); 
	}

	// Free space in the sector list
	Dispatcher::UnlockSector(Sector, ExceptionElement);
	// If PAUSE_CONTINUE, then add another exception sector at the same location
	if (Dbg::GetPauseMode() == PAUSE_CONTINUE)
	{
		// Check the sector list for space
		size_t step_element = Dispatcher::CheckSector(Dbg::GetSector(), Dbg::GetSectorSize());
		// Add the exception to the sector
		Dispatcher::AddException(Dbg::GetSector(), step_element, ExceptionMode, Sector[ExceptionElement].ExceptionAddress);
	}
	// Debugger session not active
	Dbg::Debugger = FALSE;

	// Return with the exception address to jump back to
	return r_registers.eip;
}

static __declspec(naked) VOID NTAPI KiUserExceptionDispatcher(PEXCEPTION_RECORD ExceptionRecord, PCONTEXT Context)
{
	__asm
	{
		movss r_registers.xmm0, xmm0;
		movss r_registers.xmm1, xmm1;
		movss r_registers.xmm2, xmm2;
		movss r_registers.xmm3, xmm3;
		movss r_registers.xmm4, xmm4;
		movss r_registers.xmm5, xmm5;
		movss r_registers.xmm6, xmm6;
		movss r_registers.xmm7, xmm7;
		movsd r_registers.dxmm0, xmm0;
		movsd r_registers.dxmm1, xmm1;
		movsd r_registers.dxmm2, xmm2;
		movsd r_registers.dxmm3, xmm3;
		movsd r_registers.dxmm4, xmm4;
		movsd r_registers.dxmm5, xmm5;
		movsd r_registers.dxmm6, xmm6;
		movsd r_registers.dxmm7, xmm7;
		mov r_registers.eax, eax;
		mov r_registers.ebx, ebx;
		mov r_registers.ecx, ecx;
		mov r_registers.edx, edx;
		mov r_registers.esi, esi;
		mov r_registers.edi, edi;
		mov r_registers.ebp, ebp;

		// Reserved former esp
		mov eax, [esp + 0x11c];
		mov r_registers.esp, eax;

		mov eax, [eax];
		mov r_registers.ReturnAddress, eax;

		// [esp + 0x14] contains the exception address
		mov eax, [esp + 0x14]; 
		// move into the ExceptionComparator, aka the address to be compared with the exception address
		mov[ExceptionComparator], eax; 
		// [esp + 0x0C] contains the exception code
		mov eax, [esp + 0x08]; 
		// move into the ExceptionCode global.
		mov[ExceptionCode], eax; 
		mov eax, [esp + 20];
		// when access exceptions are enabled, move the accessed memory address here
		mov[AccessException], eax;
	}
	
	// Jump to an assigned exception handler for the exception mode
	switch (Dbg::GetExceptionMode())
	{
	case 0:
		// Initiate the CallChain function used for immediate/execution exceptions
		Decision = (PVOID)CallChain(); 
		break;
	case 1:
		// Initiate the CallChainVPA function used for page exceptions
		Decision = (PVOID)CallChainVPA();
		break;
	}

	// if the decision is zero, then jump back to the real dispatcher
	if (!Decision)
	{
		__asm
		{
			mov eax, r_registers.eax;
			mov ecx, [esp + 04];
			mov ebx, [esp];
			jmp KiUserRealDispatcher;
		}
	}

	if (r_registers.SSESet == TRUE)
		HandleSSE();
	r_registers.SSESet = FALSE;

	__asm
	{
		mov eax, r_registers.eax;
		mov ebx, r_registers.ebx;
		mov ecx, r_registers.ecx;
		mov edx, r_registers.edx;
		mov esi, r_registers.esi;
		mov edi, r_registers.edi;
		// [esp + 0x11c] contains stack initation information such as the return address, arguments, etc...
		mov esp, r_registers.esp;
		// jump to the catch block
		jmp Decision;
	}

}

/* Get and set essential hook information */
static void SetKiUser()
{
	// Hook, tampered exception dispatcher later
	KiUser = (PVOID)GetProcAddress(GetModuleHandleA("ntdll.dll"), "KiUserExceptionDispatcher"); 
	// If something fails, will jump back to the real dispatcher
	KiUserRealDispatcher = (PVOID)GetProcAddress(GetModuleHandleA("ntdll.dll"), "KiUserExceptionDispatcher");

	DWORD KiUserRealDispatcher2 = (DWORD)KiUserRealDispatcher + 8;
	DWORD KiUser2 = (DWORD)KiUser + 1;

	KiUser = (PVOID)KiUser2;
	KiUserRealDispatcher = (PVOID)KiUserRealDispatcher2;
}

/* Set the pause-specification for the debug session */
void Dbg::SetPauseMode(BOOLEAN PauseMode)
{
	Pause = PauseMode;
}

/* Get the pause-specification for the debug session */
BOOLEAN Dbg::GetPauseMode()
{
	return Pause;
}

/* Wait for the module to load and then resolve the import address table */
static int WaitOptModule(const char* OriginalModuleName, const char* OptModuleName)
{
	if (!UseModule)
		return -1;
	volatile PVOID ModPtr = NULL;
	// Wait for a copy-module to load if there is none
	while (!ModPtr) 
		ModPtr = (PVOID)GetModuleHandleA(OptModuleName);
	// Resolve the import address table for this copy-module
	IATResolver::ResolveIAT(OriginalModuleName, OptModuleName);
	return 0;
}

/* Set the copy-module to be used in control flow redirection */
void Dbg::SetModule(BOOLEAN use, const char* OriginalModuleName, const char* ModuleCopyName)
{
	UseModule = use;

	if (!use)
		return;

	strcpy_s(CopyModule, MAX_PATH, ModuleCopyName);
	strcpy_s(MainModule, MAX_PATH, OriginalModuleName);
	WaitOptModule(OriginalModuleName, ModuleCopyName);
}

/* Acquire the name of the CopyModule */
char* Dbg::GetCopyModuleName()
{
	return CopyModule;
}

/* Assign a thread to the thread-excepted pool */
int Dbg::AssignThread(HANDLE Thread)
{
	for (size_t iterator = 0; iterator < sizeof(Threads); iterator++)
	{
		if (Threads[iterator] == NULL)
		{
			Threads[iterator] = Thread;
			return iterator;
		}
	}
	return -1;
}

/* Remove a thread from the thread-excepted pool*/
void Dbg::RemoveThread(HANDLE Thread)
{
	for (size_t iterator = 0; iterator < sizeof(Threads); iterator++)
	{
		if (Threads[iterator] == Thread)
		{
			CloseHandle(Threads[iterator]);
			Threads[iterator] = NULL;
			return;
		}
	}
}

/* Attach the debugger by hooking the user-system supplied exception dispatcher */
void Dbg::AttachRVDbg()
{
	InitializeConditionVariable(&Runnable);
	InitializeCriticalSection(&RunLock);
	SetKiUser();
	HookFunction(KiUser, (PVOID)KiUserExceptionDispatcher, "ntdll.dll:KiUserExceptionDispatcher");
}

/* Detach the debugger by unhooking the user-system supplied exception dispatcher */
void Dbg::DetachRVDbg()
{
	UnhookFunction(KiUser, (PVOID)KiUserExceptionDispatcher, "ntdll.dll:KiUserExceptionDispatcher");
}

/* Continue the suspendeded debugger state */
void Dbg::ContinueDebugger()
{
	WakeConditionVariable(&Runnable);
}

/* Check if the arbitrary exception handler is present within the sector */
BOOLEAN Dbg::IsAEHPresent()
{
	return CurrentSection.IsAEHPresent;
}

/* Set the value of a general purpose register */
void Dbg::SetRegister(DWORD Register, DWORD Value)
{
	// Map the value-representation of a register to the actual register model
	switch (Register) 
	{
	case Dbg::GPRegisters::EAX:
		r_registers.eax = Value;
		return;
	case Dbg::GPRegisters::EBX:
		r_registers.ebx = Value;
		return;
	case Dbg::GPRegisters::ECX:
		r_registers.ecx = Value;
		return;
	case Dbg::GPRegisters::EDX:
		r_registers.edx = Value;
		return;
	case Dbg::GPRegisters::ESI:
		r_registers.esi = Value;
		return;
	case Dbg::GPRegisters::EDI:
		r_registers.edi = Value;
		return;
	case Dbg::GPRegisters::EBP:
		r_registers.ebp = Value;
		return;
	case Dbg::GPRegisters::ESP:
		r_registers.esp = Value;
		return;
	case Dbg::GPRegisters::EIP:
		r_registers.eip = (PVOID)Value;
	}
}

/* Set the value of an SSE register as well as the value's precision type
* bxmm* = (1 : double-precision) | (2 : single-precision)
* dxmm* = double-precision storage
* xmm* = single-precision storage
*/
void Dbg::SetRegisterFP(DWORD Register, BOOLEAN Precision, double Value)
{
	r_registers.SSESet = TRUE;
	// Map a value - representation of an SSE register to the actual register model
	switch (Register)
	{
	case Dbg::SSERegisters::xmm0:
		if (Precision)
		{
			r_registers.bxmm0 = 1;
			r_registers.dxmm0 = Value;
			return;
		}
		r_registers.bxmm0 = 2;
		r_registers.xmm0 = (float)Value;
		return;
	case Dbg::SSERegisters::xmm1:
		if (Precision)
		{
			r_registers.bxmm1 = 1;
			r_registers.dxmm1 = Value;
			return;
		}
		r_registers.bxmm1 = 2;
		r_registers.xmm1 = (float)Value;
		return;
	case Dbg::SSERegisters::xmm2:
		if (Precision)
		{
			r_registers.bxmm2 = 1;
			r_registers.dxmm2 = Value;
			return;
		}
		r_registers.bxmm2 = 2;
		r_registers.xmm2 = (float)Value;
		return;
	case Dbg::SSERegisters::xmm3:
		if (Precision)
		{
			r_registers.bxmm3 = 1;
			r_registers.dxmm3 = Value;
			return;
		}
		r_registers.bxmm3 = 2;
		r_registers.xmm3 = (float)Value;
		return;
	case Dbg::SSERegisters::xmm4:
		if (Precision)
		{
			r_registers.bxmm4 = 1;
			r_registers.dxmm4 = Value;
			return;
		}
		r_registers.bxmm4 = 2;
		r_registers.xmm4 = (float)Value;
		return;
	case Dbg::SSERegisters::xmm5:
		if (Precision)
		{
			r_registers.bxmm5 = 1;
			r_registers.dxmm5 = Value;
			return;
		}
		r_registers.bxmm5 = 2;
		r_registers.xmm5 = (float)Value;
		return;
	case Dbg::SSERegisters::xmm6:
		if (Precision)
		{
			r_registers.bxmm6 = 1;
			r_registers.dxmm6 = Value;
			return;
		}
		r_registers.bxmm6 = 2;
		r_registers.xmm6 = (float)Value;
		return;
	case Dbg::SSERegisters::xmm7:
		if (Precision)
		{
			r_registers.bxmm7 = 1;
			r_registers.dxmm7 = Value;
			return;
		}
		r_registers.bxmm7 = 2;
		r_registers.xmm7 = (float)Value;
		return;
	}
}

/* Set the exception mode for the debug session */
void Dbg::SetExceptionMode(BOOLEAN _ExceptionMode)
{
	ExceptionMode = _ExceptionMode;
}

/* Get the exception mode for the debug session */
BOOLEAN Dbg::GetExceptionMode()
{
	return ExceptionMode;
}

/* Get the exception virtual address of the current section */
DWORD Dbg::GetExceptionAddress()
{
	return CurrentSection.ExceptionAddress;
}

/* Return a model-representation of the registers */
Dbg::VirtualRegisters Dbg::GetRegisters()
{
	return r_registers;
}

/* Return the current section being used in the debug session */
Dispatcher::PoolSect Dbg::GetCurrentSection()
{
	return CurrentSection;
}

/* Return the current sector being used to store sections for a debug session */
Dispatcher::PoolSect* Dbg::GetSector()
{
	return Sector;
}

/* Get the size of the current sector being used to store sections for a debug session */
int Dbg::GetSectorSize()
{
	return sizeof(Sector) / sizeof(Dispatcher::PoolSect);
}

/*
* Decides whether to use double-precision or single-precision on floating point values
* Uses bxmm* as a flag for the decision
*/
static void HandleSSE()
{
	if (r_registers.bxmm0 == 1)
		__asm movsd xmm0, r_registers.dxmm0;
	else if (r_registers.bxmm0 == 2)
		__asm movss xmm0, r_registers.xmm0;

	if (r_registers.bxmm1 == 1)
		__asm movsd xmm1, r_registers.dxmm1;
	else if (r_registers.bxmm1 == 2)
		__asm movss xmm1, r_registers.xmm1;

	if (r_registers.bxmm2 == 1)
		__asm movsd xmm2, r_registers.dxmm2;
	else if (r_registers.bxmm2 == 2)
		__asm movss xmm2, r_registers.xmm2;

	if (r_registers.bxmm3 == 1)
		__asm movsd xmm3, r_registers.dxmm3;
	else if (r_registers.bxmm3 == 2)
		__asm movss xmm3, r_registers.xmm3;

	if (r_registers.bxmm4 == 1)
		__asm movsd xmm4, r_registers.dxmm4;
	else if (r_registers.bxmm4 == 2)
		__asm movss xmm4, r_registers.xmm4;

	if (r_registers.bxmm5 == 1)
		__asm movsd xmm5, r_registers.dxmm5;
	else if (r_registers.bxmm5 == 2)
		__asm movss xmm5, r_registers.xmm5;

	if (r_registers.bxmm6 == 1)
		__asm movsd xmm6, r_registers.dxmm6;
	else if (r_registers.bxmm6 == 2)
		__asm movss xmm6, r_registers.xmm6;

	if (r_registers.bxmm7 == 1)
		__asm movsd xmm7, r_registers.dxmm7;
	else if (r_registers.bxmm7 == 2)
		__asm movss xmm7, r_registers.xmm7;

	r_registers.bxmm0 = 0;
	r_registers.bxmm1 = 0;
	r_registers.bxmm2 = 0;
	r_registers.bxmm3 = 0;
	r_registers.bxmm4 = 0;
	r_registers.bxmm5 = 0;
	r_registers.bxmm6 = 0;
	r_registers.bxmm7 = 0;
}
