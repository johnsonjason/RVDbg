// Programmed by jasonfish4
#include "stdafx.h"
#include "exceptiondispatcher.h"
#include "rvdbg.h"

void Dispatcher::RaiseILGLAccessViolation(BYTE* ptr, BYTE save, BOOLEAN on)
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

void Dispatcher::RaisePageAccessViolation(BYTE* ptr, DWORD save, BOOLEAN on)
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

void Dispatcher::RaiseBreakpointException(BYTE* ptr, BYTE save, BOOLEAN on)
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

void Dispatcher::RaisePrivilegedCodeException(BYTE* ptr, BYTE save, BOOLEAN on)
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

PVOID Dispatcher::HandleException(Dispatcher::PoolSect segment, const char* module_name, BOOLEAN b_module)
{
	segment.UseModule = b_module;
	switch (segment.ExceptionCode)
	{
	case STATUS_ACCESS_VIOLATION:
		Dispatcher::RaiseILGLAccessViolation((BYTE*)segment.ExceptionAddress, segment.SaveCode, FALSE);
		if (segment.UseModule)
			return (PVOID)((DWORD)GetModuleHandleA(module_name) + segment.ExceptionOffset);
		return (PVOID)segment.ExceptionAddress;
	case STATUS_BREAKPOINT:
		Dispatcher::RaiseBreakpointException((BYTE*)segment.ExceptionAddress, segment.SaveCode, FALSE);
		if (segment.UseModule)
			return (PVOID)((DWORD)GetModuleHandleA(module_name) + segment.ExceptionOffset);
		return (PVOID)segment.ExceptionAddress;
	case STATUS_PRIVILEGED_INSTRUCTION:
		Dispatcher::RaisePrivilegedCodeException((BYTE*)segment.ExceptionAddress, segment.SaveCode, FALSE);
		if (segment.UseModule)
			return (PVOID)((DWORD)GetModuleHandleA(module_name) + segment.ExceptionOffset);
		return (PVOID)segment.ExceptionAddress;
	}

	return NULL;
}

size_t Dispatcher::CheckSector(Dispatcher::PoolSect sector[], size_t size)
{
	for (size_t iterator = 0; iterator < size; iterator++)
	{
		if (sector[iterator].Used == FALSE)
			return iterator;
	}
	return (size + 1);
}

size_t Dispatcher::SearchSector(Dispatcher::PoolSect sector[], size_t size, DWORD address)
{
	for (size_t iterator = 0; iterator < size; iterator++)
	{
		if (sector[iterator].Used == TRUE && sector[iterator].IsAEHPresent == FALSE 
			&& sector[iterator].ExceptionAddress == address)
			return iterator;
	}
	return (size + 1);
}


void Dispatcher::UnlockSector(Dispatcher::PoolSect sector[], size_t index)
{
	memset(&sector[index], NULL, sizeof(Dispatcher::PoolSect));
}

void Dispatcher::LockSector(Dispatcher::PoolSect sector[], size_t index)
{
	sector[index].Used = TRUE;
	sector[index].IsAEHPresent = FALSE;
}


void Dispatcher::AddException(Dispatcher::PoolSect sector[], size_t index, BOOLEAN Type, DWORD ExceptionAddress)
{
	sector[index].ExceptionAddress = ExceptionAddress;
	sector[index].SaveCode = *(DWORD*)ExceptionAddress;
	sector[index].ExceptionType = Type;
	sector[index].Index = index;
	Dispatcher::LockSector(sector, index);

	switch (Type)
	{
	case IMMEDIATE_EXCEPTION:
		Dispatcher::RaisePrivilegedCodeException((BYTE*)sector[index].ExceptionAddress, 0, TRUE);
		return;
	case PAGE_EXCEPTION:
		Dispatcher::RaisePageAccessViolation((BYTE*)sector[index].ExceptionAddress, 0, TRUE);
		return;
	}

}
