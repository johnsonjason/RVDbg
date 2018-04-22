// Programmed by jasonfish4
#ifndef EXCEPTIONDISPATCHER
#define EXCEPTIONDISPATCHER
#include <windows.h>
#include <tlhelp32.h>
#include <stdlib.h>


namespace Dispatcher
{
	/* specifies information about the exception in this section */
	struct PoolSect
	{
		// copy-module name
		char ModuleName[MAX_PATH];
		// current thread of exception
		HANDLE Thread;
		// current thread id of exception
		DWORD ThreadId;
		// if the section is being used
		BOOLEAN Used;
		// if a copy-module is being used
		BOOLEAN UseModule;
		// is the arbitrary exception handler present for this section
		BOOLEAN IsAEHPresent;
		// the type of exception
		BOOLEAN ExceptionType;
		// the exception code information
		DWORD ExceptionCode;
		// the address the exception occurred at
		DWORD ExceptionAddress;
		// the offset of the exception address
		DWORD ExceptionOffset;
		// Additional information
		DWORD ReturnAddress;
		DWORD SaveCode;
		DWORD Index;
	};


	void RaiseILGLAccessViolation(BYTE* ptr, BYTE save, BOOLEAN on);
	void RaisePageAccessViolation(BYTE* ptr, DWORD save, BOOLEAN on);
	void RaiseBreakpointException(BYTE* ptr, BYTE save, BOOLEAN on);
	void RaisePrivilegedCodeException(BYTE* ptr, BYTE save, BOOLEAN on);

	PVOID HandleException(Dispatcher::PoolSect segment, const char* ModuleName, BOOLEAN Constant);
	size_t CheckSector(PoolSect sector[], size_t size);
	size_t SearchSector(PoolSect sector[], size_t size, DWORD address);
	void UnlockSector(PoolSect sector[], size_t index);
	void LockSector(PoolSect sector[], size_t index);
	void AddException(PoolSect sector[], size_t index, BOOLEAN Type, DWORD ExceptionAddress);
}
#endif
