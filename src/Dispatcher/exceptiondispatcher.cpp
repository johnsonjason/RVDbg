#include "exceptiondispatcher.h"

void RaiseILGLAccessViolation(BYTE* ptr, BYTE save, BOOLEAN on)
{
	DWORD OldProtect;
	switch (on)
	{
	case FALSE:
		VirtualProtect(ptr, 1, PAGE_EXECUTE_READWRITE, &OldProtect);
		*(BYTE*)ptr = save;
		VirtualProtect(ptr, 1, OldProtect, &OldProtect);
		return;
	case TRUE:
		VirtualProtect(ptr, 1, PAGE_EXECUTE_READWRITE, &OldProtect);
		*(BYTE*)ptr = 0xFF;
		VirtualProtect(ptr, 1, OldProtect, &OldProtect);
		return;
	}
}

void RaisePageAccessViolation(BYTE* ptr, DWORD save, BOOLEAN on)
{
	DWORD OldProtect;
	switch (on)
	{
	case FALSE:
		VirtualProtect(ptr, 1, save, &OldProtect);
		return;
	case TRUE:
		VirtualProtect(ptr, 1, PAGE_READONLY, &OldProtect);
		return;
	}
}

void RaiseBreakpointException(BYTE* ptr, BYTE save, BOOLEAN on)
{
	DWORD OldProtect;
	switch (on)
	{
	case FALSE:
		VirtualProtect(ptr, 1, PAGE_EXECUTE_READWRITE, &OldProtect);
		*(BYTE*)ptr = save;
		VirtualProtect(ptr, 1, OldProtect, &OldProtect);
		return;
	case TRUE:
		VirtualProtect(ptr, 1, PAGE_EXECUTE_READWRITE, &OldProtect);
		*(BYTE*)ptr = 0xCC;
		VirtualProtect(ptr, 1, OldProtect, &OldProtect);
		return;
	}
}

void RaisePrivilegedCodeException(BYTE* ptr, BYTE save, BOOLEAN on)
{
	DWORD OldProtect;
	switch (on)
	{
	case FALSE:
		VirtualProtect(ptr, 1, PAGE_EXECUTE_READWRITE, &OldProtect);
		*(BYTE*)ptr = save;
		VirtualProtect(ptr, 1, OldProtect, &OldProtect);
		return;
	case TRUE:
		VirtualProtect(ptr, 1, PAGE_EXECUTE_READWRITE, &OldProtect);
		*(BYTE*)ptr = 0xF4;
		VirtualProtect(ptr, 1, OldProtect, &OldProtect);
		return;
	}
}

PVOID HandleException(PoolSect segment, const char* ModuleName)
{
	switch (segment.ExceptionCode)
	{
	case STATUS_ACCESS_VIOLATION:
		RaiseILGLAccessViolation((BYTE*)segment.ExceptionAddress, segment.SaveCode, FALSE);
		if (segment.UseModule)
			return (PVOID)((DWORD)GetModuleHandleA(ModuleName) + segment.ExceptionOffset);
		return (PVOID)segment.ExceptionAddress;
	case STATUS_BREAKPOINT:
		RaiseBreakpointException((BYTE*)segment.ExceptionAddress, segment.SaveCode, FALSE);
		if (segment.UseModule)
			return (PVOID)((DWORD)GetModuleHandleA(ModuleName) + segment.ExceptionOffset);
		return (PVOID)segment.ExceptionAddress;
	case STATUS_PRIVILEGED_INSTRUCTION:
		RaisePrivilegedCodeException((BYTE*)segment.ExceptionAddress, segment.SaveCode, FALSE);
		if (segment.UseModule)
			return (PVOID)((DWORD)GetModuleHandleA(ModuleName) + segment.ExceptionOffset);
		return (PVOID)segment.ExceptionAddress;
	}
	return NULL;
}

size_t CheckSector(PoolSect sector[], size_t size)
{
	for (size_t iterator = 0; iterator < size; iterator++)
	{
		if (sector[iterator].Used == FALSE)
			return iterator;
	}
	return (size + 1);
}

size_t SearchSector(PoolSect sector[], size_t size, DWORD address)
{
	for (size_t iterator = 0; iterator < size; iterator++)
	{
		if (sector[iterator].Used == TRUE && sector[iterator].IsAEHPresent == FALSE 
			&& sector[iterator].ExceptionAddress == address)
			return iterator;
	}
	return (size + 1);
}


void UnlockSector(PoolSect sector[], size_t index)
{
	sector[index].IsAEHPresent = FALSE;
	sector[index].Used = FALSE;
	sector[index].ExceptionAddress = NULL;
}

void LockSector(PoolSect sector[], size_t index)
{
	sector[index].Used = TRUE;
	sector[index].IsAEHPresent = FALSE;
}

DWORD GetExceptionThreadId(PoolSect segment)
{
	return GetThreadId(segment.Thread);
}

void SuspendException(PoolSect segment)
{
	SuspendThread(segment.Thread);
	segment.IsAEHPresent = FALSE;
	segment.Used = TRUE;
}

void ContinueException(PoolSect segment)
{
	segment.IsAEHPresent = TRUE;
	segment.Used = TRUE;
	ResumeThread(segment.Thread);
}

size_t AccessQuery(DWORD AccessException)
{
	MEMORY_BASIC_INFORMATION Query;
	VirtualQuery((LPCVOID)AccessException, &Query, sizeof(Query));

	if (AccessException >= (DWORD)Query.BaseAddress && AccessException <= ((DWORD)Query.BaseAddress + Query.RegionSize))
	{
		DWORD OldProtect;
		void* SwapMemory = calloc(1, 16);
		VirtualProtect((LPVOID)AccessException, 1, PAGE_READONLY, &OldProtect);
		memcpy(SwapMemory, (const void*)AccessException, 16);
		VirtualProtect((LPVOID)AccessException, 1, OldProtect, &OldProtect);
		return (size_t)SwapMemory;
	}
	else
		return NULL;
}

void AddException(PoolSect sector[], size_t index, BOOLEAN Type, DWORD ExceptionAddress)
{
	sector[index].ExceptionAddress = ExceptionAddress;
	sector[index].SaveCode = *(DWORD*)ExceptionAddress;
	sector[index].ExceptionType = Type;
	LockSector(sector, index);

	switch (Type)
	{
	case 0:
		RaisePrivilegedCodeException((BYTE*)sector[index].ExceptionAddress, 0, TRUE);
		return;
	case 1:
		RaisePageAccessViolation((BYTE*)sector[index].ExceptionAddress, 0, TRUE);
		return;
	}

}
