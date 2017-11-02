// Programmed by jasonfish4
#ifndef EXCEPTIONDISPATCHER
#define EXCEPTIONDISPATCHER
#include <windows.h>
#include <tlhelp32.h>
#include <stdlib.h>

namespace Dispatcher
{
	struct PoolSect
	{
		char ModuleName[MAX_PATH];
		HANDLE Thread;
		BOOLEAN Used;
		BOOLEAN UseModule;
		BOOLEAN IsAEHPresent;
		BOOLEAN ExceptionType;
		DWORD ExceptionCode;
		DWORD ExceptionAddress;
		DWORD ExceptionOffset;
		DWORD ReturnAddress;
		DWORD SaveCode;
		DWORD Index;
	};

	void RaiseILGLAccessViolation(BYTE* ptr, BYTE save, BOOLEAN on);
	void RaisePageAccessViolation(BYTE* ptr, DWORD save, BOOLEAN on);
	void RaiseBreakpointException(BYTE* ptr, BYTE save, BOOLEAN on);
	void RaisePrivilegedCodeException(BYTE* ptr, BYTE save, BOOLEAN on);

	PVOID HandleException(PoolSect segment, const char* ModuleName);
	size_t CheckSector(PoolSect sector[], size_t size);
	size_t SearchSector(PoolSect sector[], size_t size, DWORD address);
	void UnlockSector(PoolSect sector[], size_t index);
	void LockSector(PoolSect sector[], size_t index);
	void AddException(PoolSect sector[], size_t index, BOOLEAN Type, DWORD ExceptionAddress);
}
#endif
