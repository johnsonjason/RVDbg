#include "rvdbg.h"
BOOLEAN tDSend;

static __declspec(naked) void Execute_Chunk1()
{
	switch (SelectedARegister)
	{
	case mEAX:
		__asm mov eax, SelectedAddress;
		__asm jmp(ExceptionComparator + 5);
	case mEBX:
		__asm mov ebx, SelectedAddress;
		__asm jmp(ExceptionComparator + 6);
	case mECX:
		__asm mov ecx, SelectedAddress;
		__asm jmp(ExceptionComparator + 6);
	case mEDX:
		__asm mov edx, SelectedAddress;
		__asm jmp(ExceptionComparator + 6);
	case mESI:
		__asm mov esi, SelectedAddress;
		__asm jmp(ExceptionComparator + 6);
	case mEDI:
		__asm mov edi, SelectedAddress;
		__asm jmp(ExceptionComparator + 6);
	case mEBP:
		__asm mov ebp, SelectedAddress;
		__asm jmp(ExceptionComparator + 6);
	case mESP:
		__asm mov esp, SelectedAddress;
		__asm jmp(ExceptionComparator + 6);
	}
}

static PVOID CallChain()
{
	size_t ExceptionElement = SearchSector(Sector, 128, ExceptionComparator);

	if (ExceptionElement > 128)
	{
		if (ExceptionMode == 2)
		{
			size_t result = AccessQuery(AccessException);
			if (!result)
			{
				ChunkExecutable = FALSE;
				return NULL;
			}
			ChunkExecutable = D_ADDRESSING;
			Swaps.push_back((void*)result);
			SelectedAddress = result;
			return (PVOID)result;
		}
		return NULL;
	}

	SuspendThreads(GetCurrentProcessId(), GetCurrentThreadId());
	Debugger = TRUE;
	tDSend = TRUE;

	for (size_t iterator = 0; iterator < sizeof(Threads); iterator++)
	{
		if (Threads[iterator] != NULL)
			ResumeThread(Threads[iterator]);
	}

	Sector[ExceptionElement].ExceptionCode = ExceptionCode;
	PVOID ReturnException = HandleException(Sector[ExceptionElement], Sector[ExceptionElement].ModuleName);

	Sector[ExceptionElement].Thread = GetCurrentThread();
	Sector[ExceptionElement].IsAEHPresent = TRUE;
	Sector[ExceptionElement].ReturnAddress = r_registers.ReturnAddress;

	CurrentPool = Sector[ExceptionElement];

	EnterCriticalSection(&RunLock);
	SleepConditionVariableCS(&Runnable, &RunLock, INFINITE);
	LeaveCriticalSection(&RunLock);

	ResumeThreads(GetCurrentProcessId(), GetCurrentThreadId());
	UnlockSector(Sector, ExceptionElement);
	return ReturnException;
}

static __declspec(naked) VOID NTAPI KiUserExceptionDispatcher(PEXCEPTION_RECORD ExceptionRecord, PCONTEXT Context)
{
	__asm
	{
		mov r_registers.eax, eax;
		mov r_registers.ebx, ebx;
		mov r_registers.ecx, ecx;
		mov r_registers.edx, edx;
		mov r_registers.esi, esi;
		mov r_registers.edi, edi;
		mov r_registers.ebp, ebp;
		mov eax, [esp + 0x11c]; // Reserved former esp
		mov r_registers.esp, eax;
		mov eax, [eax];
		mov r_registers.ReturnAddress, eax;
		mov eax, [esp + 0x14]; // [esp + 0x14] contains the exception address
		mov[ExceptionComparator], eax; // move into the exception comparator, aka the address to be compared with the exception address
		mov eax, [esp + 0x08]; // [esp + 0x0C] contains the exception code
		mov[ExceptionCode], eax; // move into the ExceptionCode global.
		mov eax, [esp + 20]; // when access exceptions are enabled, move the accessed memoryh ere
		mov[AccessException], eax;
	}

	Decision = (PVOID)CallChain(); // Initiate the CallChain function

	if (!Decision) // if the decision is null, then jump back to the real dispatcher
	{
		__asm
		{
			mov eax, r_registers.eax;
			mov ecx, [esp + 04];
			mov ebx, [esp];
			jmp KiUserRealDispatcher;
		}
	}
	else if (Decision && ChunkExecutable)
	{
		switch (ChunkExecutable)
		{
		case D_ADDRESSING:
			ChunkExecutable = NULL;
			__asm jmp Execute_Chunk1;
		}
	}
	__asm
	{
		mov Debugger, 0;
		mov eax, r_registers.eax;
		mov ebx, r_registers.ebx;
		mov ecx, r_registers.ecx;
		mov edx, r_registers.edx;
		mov esi, r_registers.esi;
		mov edi, r_registers.edi;
		mov esp, r_registers.esp; // [esp + 0x11c] contains stack initation information such as the return address, arguments, etc...
		jmp Decision; // jump to the catch block
	}

}

static void SetKiUser()
{
	KiUser = (PVOID)GetProcAddress(GetModuleHandleA("ntdll.dll"), "KiUserExceptionDispatcher"); // Hook, tampered exception dispatcher later
	KiUserRealDispatcher = (PVOID)GetProcAddress(GetModuleHandleA("ntdll.dll"), "KiUserExceptionDispatcher"); // If something fails, will jump back to the real dispatcher

	DWORD KiUserRealDispatcher2 = (DWORD)KiUserRealDispatcher + 8;
	DWORD KiUser2 = (DWORD)KiUser + 1;

	KiUser = (PVOID)KiUser2;
	KiUserRealDispatcher = (PVOID)KiUserRealDispatcher2;
}


void AttachRVDbg()
{
	InitializeConditionVariable(&Runnable);
	InitializeCriticalSection(&RunLock);
	SetKiUser();
	HookFunction(KiUser, (PVOID)KiUserExceptionDispatcher, "ntdll.dll:KiUserExceptionDispatcher");
}

void DetachRVDbg()
{
	UnhookFunction(KiUser, (PVOID)KiUserExceptionDispatcher, "ntdll.dll:KiUserExceptionDispatcher");
}

void ContinueDebugger()
{
	WakeConditionVariable(&Runnable);
}

BOOLEAN IsAEHPresent()
{
	return CurrentPool.IsAEHPresent;
}

void SetRegister(DWORD Register, DWORD Value)
{
	switch (Register)
	{
	case mEAX:
		r_registers.eax = Value;
		return;
	case mEBX:
		r_registers.ebx = Value;
		return;
	case mECX:
		r_registers.ecx = Value;
		return;
	case mEDX:
		r_registers.edx = Value;
		return;
	case mESI:
		r_registers.esi = Value;
		return;
	case mEDI:
		r_registers.edi = Value;
		return;
	case mEBP:
		r_registers.ebp = Value;
		return;
	case mESP:
		r_registers.esp = Value;
		return;
	}
}

void SetExceptionMode(BOOLEAN lExceptionMode)
{
	ExceptionMode = lExceptionMode;
}

BOOLEAN GetExceptionMode()
{
	return ExceptionMode;
}

DWORD GetExceptionAddress()
{
	return CurrentPool.ExceptionAddress;
}

VirtualRegisters GetRegisters()
{
	return r_registers;
}

PoolSect GetPool()
{
	return CurrentPool;
}

PoolSect* GetSector()
{
	return Sector;
}

int AssignThread(HANDLE Thread)
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

void RemoveThread(HANDLE Thread)
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

int WaitOptModule(const char* OptModuleName)
{
	if (!UseModule)
		return -1;
	volatile PVOID ModPtr = NULL;
	while (!ModPtr)
		ModPtr = (PVOID)GetModuleHandleA(OptModuleName);
	return 0;
}

void SetModule(BOOLEAN use)
{
	UseModule = use;
}
