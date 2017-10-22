#include "stdafx.h"
#include "rvdbg.h"

BOOLEAN tDSend;
CRITICAL_SECTION repr;
CONDITION_VARIABLE reprcondition;

static void Dbg::HandleSSE()
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

static PVOID Dbg::CallChain()
{
	size_t ExceptionElement = SearchSector(Sector, 128, ExceptionComparator);

	if (ExceptionElement > 128)
	{
		if (ExceptionMode == 2)
		{
			SuspendThreads(GetCurrentProcessId(), GetCurrentThreadId());
			DWORD swap_ad = Dispatcher::SwapAccess(AccessException, AccessException);

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
	WakeConditionVariable(&reprcondition);


	for (size_t iterator = 0; iterator < sizeof(Threads); iterator++)
	{
		if (Threads[iterator] != NULL)
			ResumeThread(Threads[iterator]);
	}

	Sector[ExceptionElement].ExceptionCode = ExceptionCode;
	PVOID ReturnException = Dispatcher::HandleException(Sector[ExceptionElement], Sector[ExceptionElement].ModuleName);
	r_registers.eip = ReturnException;

	Sector[ExceptionElement].Thread = GetCurrentThread();
	Sector[ExceptionElement].IsAEHPresent = TRUE;
	Sector[ExceptionElement].ReturnAddress = r_registers.ReturnAddress;

	CurrentPool = Sector[ExceptionElement];

	EnterCriticalSection(&RunLock);
	SleepConditionVariableCS(&Runnable, &RunLock, INFINITE);
	LeaveCriticalSection(&RunLock);

	ResumeThreads(GetCurrentProcessId(), GetCurrentThreadId());
	Dispatcher::UnlockSector(Sector, ExceptionElement);
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

	Decision = (PVOID)Dbg::CallChain(); // Initiate the CallChain function

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
		Dbg::HandleSSE();
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

static void Dbg::SetKiUser()
{
	KiUser = (PVOID)GetProcAddress(GetModuleHandleA("ntdll.dll"), "KiUserExceptionDispatcher"); // Hook, tampered exception dispatcher later
	KiUserRealDispatcher = (PVOID)GetProcAddress(GetModuleHandleA("ntdll.dll"), "KiUserExceptionDispatcher"); // If something fails, will jump back to the real dispatcher

	DWORD KiUserRealDispatcher2 = (DWORD)KiUserRealDispatcher + 8;
	DWORD KiUser2 = (DWORD)KiUser + 1;

	KiUser = (PVOID)KiUser2;
	KiUserRealDispatcher = (PVOID)KiUserRealDispatcher2;
}


static Dbg::IMP_AT Dbg::GetIAT(LPCSTR ModuleName)
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

int Dbg::WaitOptModule(const char* OriginalModuleName, const char* OptModuleName)
{
	if (!UseModule)
		return -1;
	volatile PVOID ModPtr = NULL;
	while (!ModPtr)
		ModPtr = (PVOID)GetModuleHandleA(OptModuleName);
	IATResolver::ResolveIAT(OriginalModuleName, OptModuleName);
	return 0;
}

void Dbg::SetModule(BOOLEAN use)
{
	UseModule = use;
}

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

void Dbg::AttachRVDbg()
{
	InitializeConditionVariable(&Runnable);
	InitializeCriticalSection(&RunLock);
	Dbg::SetKiUser();
	HookFunction(KiUser, (PVOID)KiUserExceptionDispatcher, "ntdll.dll:KiUserExceptionDispatcher");
}

void Dbg::DetachRVDbg()
{
	UnhookFunction(KiUser, (PVOID)KiUserExceptionDispatcher, "ntdll.dll:KiUserExceptionDispatcher");
}

void Dbg::ContinueDebugger()
{
	WakeConditionVariable(&Runnable);
}

BOOLEAN Dbg::IsAEHPresent()
{
	return CurrentPool.IsAEHPresent;
}

void Dbg::SetRegister(DWORD Register, DWORD Value)
{
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

void Dbg::SetRegisterFP(DWORD Register, BOOLEAN Precision, double Value)
{
	r_registers.SSESet = TRUE;
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

void Dbg::SetExceptionMode(BOOLEAN lExceptionMode)
{
	ExceptionMode = lExceptionMode;
}

BOOLEAN Dbg::GetExceptionMode()
{
	return ExceptionMode;
}

DWORD Dbg::GetExceptionAddress()
{
	return CurrentPool.ExceptionAddress;
}

Dbg::VirtualRegisters Dbg::GetRegisters()
{
	return r_registers;
}

Dispatcher::PoolSect Dbg::GetPool()
{
	return CurrentPool;
}

Dispatcher::PoolSect* Dbg::GetSector()
{
	return Sector;
}

int Dbg::GetSectorSize()
{
	return sizeof(Sector) / sizeof(Dispatcher::PoolSect);
}

