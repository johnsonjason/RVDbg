#ifndef EXCEPTION_STORE_H
#define EXCEPTION_STORE_H

#include <Windows.h>
#include <vector>
#include <iterator>

namespace Debugger
{
	//
	// Enum specifying the state of the exception condition
	// ImmediateMode - Execution interruption based on injection
	// ModuleMode - Execution interruption based on page exceptions
	//

	typedef enum _EXCEPTION_STATE : BYTE
	{
		ImmediateMode,
		PageMode,
		ModuleMode
	} EXCEPTION_STATE;

	//
	// Stores the registered exception
	// SaveCode is the operation code prior to an immediate exception injection
	// ConditionAddress is the address for the exception
	// Mode is the type of exception condition
	// ModulePtr is the cloned module to use if specified
	//

	typedef struct _EXCEPTION_BASE
	{
		EXCEPTION_STATE Mode;
		BYTE SaveCode;
		bool UseSave;
		DWORD_PTR ConditionAddress;
		DWORD_PTR ModulePtr;
	} EXCEPTION_BASE;

	extern std::vector<EXCEPTION_BASE> ExceptionStore;

	void RegisterExceptionCondition(DWORD_PTR ExceptionAddress, EXCEPTION_STATE Mode);
	bool CloseExceptionCondition(DWORD_PTR ExceptionAddress, EXCEPTION_STATE Mode);
	int DiscoverExceptionCondition(DWORD_PTR ExceptionAddress, EXCEPTION_STATE Mode);
	void HandleException(Debugger::EXCEPTION_BASE& BaseException);
}

#endif
