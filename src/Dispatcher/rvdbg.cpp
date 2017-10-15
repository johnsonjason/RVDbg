#include "stdafx.h"
#include "rvdbg.h"

BOOLEAN tDSend;
CRITICAL_SECTION repr;
CONDITION_VARIABLE reprcondition;

static void HandleSSE()
{
	if (r_registers.bxmm0 == 1)
		__asm movsd xmm0, r_registers.dxmm0;
	else if (r_registers.bxmm0 == 2)
		__asm movss xmm0, r_registers.xmm0;

	if (r_registers.bxmm1)
		__asm movsd xmm1, r_registers.dxmm1;
	else if (r_registers.bxmm1 == 2)
		__asm movss xmm1, r_registers.xmm1;

	if (r_registers.bxmm2)
		__asm movsd xmm2, r_registers.dxmm2;
	else if (r_registers.bxmm2 == 2)
		__asm movss xmm2, r_registers.xmm2;

	if (r_registers.bxmm3)
		__asm movsd xmm3, r_registers.dxmm3;
	else if (r_registers.bxmm3 == 2)
		__asm movss xmm3, r_registers.xmm3;

	if (r_registers.bxmm4)
		__asm movsd xmm4, r_registers.dxmm4;
	else if (r_registers.bxmm4 == 2)
		__asm movss xmm4, r_registers.xmm4;

	if (r_registers.bxmm5)
		__asm movsd xmm5, r_registers.dxmm5;
	else if (r_registers.bxmm5 == 2)
		__asm movss xmm5, r_registers.xmm5;

	if (r_registers.bxmm6)
		__asm movsd xmm6, r_registers.dxmm6;
	else if (r_registers.bxmm6 == 2)
		__asm movss xmm6, r_registers.xmm6;

	if (r_registers.bxmm7)
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

static PVOID CallChain()
{
	size_t ExceptionElement = SearchSector(Sector, 128, ExceptionComparator);

	if (ExceptionElement > 128)
	{
		if (ExceptionMode == 2)
		{
			SuspendThreads(GetCurrentProcessId(), GetCurrentThreadId());
			DWORD swap_ad = SwapAccess(AccessException, AccessException);

			if (!swap_ad)
			{
				ChunkExecutable = FALSE;
				return NULL;
			}

			Swaps.push_back((PVOID)swap_ad);
			return (PVOID)swap_ad;
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
	r_registers.eip = ReturnException;

	Sector[ExceptionElement].Thread = GetCurrentThread();
	Sector[ExceptionElement].IsAEHPresent = TRUE;
	Sector[ExceptionElement].ReturnAddress = r_registers.ReturnAddress;

	CurrentPool = Sector[ExceptionElement];

	EnterCriticalSection(&RunLock);
	SleepConditionVariableCS(&Runnable, &RunLock, INFINITE);
	LeaveCriticalSection(&RunLock);

	ResumeThreads(GetCurrentProcessId(), GetCurrentThreadId());
	UnlockSector(Sector, ExceptionElement);
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


static IMP_AT GetIAT(LPCSTR ModuleName)
{
	HMODULE mod = GetModuleHandleA(ModuleName);

	PIMAGE_DOS_HEADER img_dos_headers = (PIMAGE_DOS_HEADER)mod;
	PIMAGE_NT_HEADERS img_nt_headers = (PIMAGE_NT_HEADERS)((BYTE*)img_dos_headers + img_dos_headers->e_lfanew);

	PIMAGE_IMPORT_DESCRIPTOR img_import_desc = (PIMAGE_IMPORT_DESCRIPTOR)((BYTE*)img_dos_headers +
		img_nt_headers->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress);

	DWORD IATSize = (img_nt_headers->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].Size * 4);

	IMP_AT Retn;
	Retn.Size = IATSize;
	Retn.Address = (PVOID)((DWORD)mod + img_import_desc->FirstThunk - 0x1DC);
	return Retn;
}

static void SetImportAddressTable(const char* ModuleName)
{
	DWORD OldProtect;
	MEMORY_BASIC_INFORMATION inf;
	IMP_AT CopyModule = GetIAT(0);
	IMP_AT SelfModule = GetIAT(ModuleName);
	printf("_iat: %p\n", CopyModule.Address);
	printf("iat: %p\n", SelfModule.Address);
	VirtualProtect(SelfModule.Address, 1, PAGE_EXECUTE_READWRITE, &OldProtect);
	printf("Error1: %d\n", GetLastError());
	VirtualQuery(SelfModule.Address, &inf, sizeof(inf));
	memcpy(SelfModule.Address, CopyModule.Address, inf.RegionSize);
	VirtualProtect(SelfModule.Address, 1, OldProtect, &OldProtect);
}

int WaitOptModule(const char* OptModuleName)
{
	if (!UseModule)
		return -1;
	volatile PVOID ModPtr = NULL;
	while (!ModPtr)
		ModPtr = (PVOID)GetModuleHandleA(OptModuleName);
	SetImportAddressTable(OptModuleName);
	return 0;
}

void SetModule(BOOLEAN use)
{
	UseModule = use;
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
	case GPRegisters::EAX:
		r_registers.eax = Value;
		return;
	case GPRegisters::EBX:
		r_registers.ebx = Value;
		return;
	case GPRegisters::ECX:
		r_registers.ecx = Value;
		return;
	case GPRegisters::EDX:
		r_registers.edx = Value;
		return;
	case GPRegisters::ESI:
		r_registers.esi = Value;
		return;
	case GPRegisters::EDI:
		r_registers.edi = Value;
		return;
	case GPRegisters::EBP:
		r_registers.ebp = Value;
		return;
	case GPRegisters::ESP:
		r_registers.esp = Value;
		return;
	case GPRegisters::EIP:
		r_registers.eip = (PVOID)Value;
	}
}

void SetRegisterFP(DWORD Register, BOOLEAN Precision, double Value)
{
	r_registers.SSESet = TRUE;
	switch (Register)
	{
	case SSERegisters::xmm0:
		if (Precision)
		{
			r_registers.bxmm0 = 1;
			r_registers.dxmm0 = Value;
			return;
		}
		r_registers.bxmm0 = 2;
		r_registers.xmm0 = (float)Value;
		return;
	case SSERegisters::xmm1:
		if (Precision)
		{
			r_registers.bxmm1 = 1;
			r_registers.dxmm1 = Value;
			return;
		}
		r_registers.bxmm1 = 2;
		r_registers.xmm1 = (float)Value;
		return;
	case SSERegisters::xmm2:
		if (Precision)
		{
			r_registers.bxmm2 = 1;
			r_registers.dxmm2 = Value;
			return;
		}
		r_registers.bxmm2 = 2;
		r_registers.xmm2 = (float)Value;
		return;
	case SSERegisters::xmm3:
		if (Precision)
		{
			r_registers.bxmm3 = 1;
			r_registers.dxmm3 = Value;
			return;
		}
		r_registers.bxmm3 = 2;
		r_registers.xmm3 = (float)Value;
		return;
	case SSERegisters::xmm4:
		if (Precision)
		{
			r_registers.bxmm4 = 1;
			r_registers.dxmm4 = Value;
			return;
		}
		r_registers.bxmm4 = 2;
		r_registers.xmm4 = (float)Value;
		return;
	case SSERegisters::xmm5:
		if (Precision)
		{
			r_registers.bxmm5 = 1;
			r_registers.dxmm5 = Value;
			return;
		}
		r_registers.bxmm5 = 2;
		r_registers.xmm5 = (float)Value;
		return;
	case SSERegisters::xmm6:
		if (Precision)
		{
			r_registers.bxmm6 = 1;
			r_registers.dxmm6 = Value;
			return;
		}
		r_registers.bxmm6 = 2;
		r_registers.xmm6 = (float)Value;
		return;
	case SSERegisters::xmm7:
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

int GetSectorSize()
{
	return sizeof(Sector) / sizeof(PoolSect);
}

