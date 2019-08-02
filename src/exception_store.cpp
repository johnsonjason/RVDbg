#include "stdafx.h"
#include "exception_store.h"

//
// Record of all currently registered exceptions
//

std::vector<Debugger::EXCEPTION_BASE> Debugger::ExceptionStore;

/*++ 

 Routine Description:

	Uses the BaseException parameter and changes the protection on .ConditionAddress so it can be written
	Modifies the address in the BaseException to cause an exception based on a privileged instruction (HLT) 
	OR
	Modifies the address in the BaseException to the operation code prior to an exception

--*/

static void SetHltCondition(Debugger::EXCEPTION_BASE& BaseException)
{
	DWORD OldProtection;
	VirtualProtect(reinterpret_cast<PVOID>(BaseException.ConditionAddress), 1, PAGE_EXECUTE_READWRITE, &OldProtection);
	*reinterpret_cast<PBYTE>(BaseException.ConditionAddress) = BaseException.UseSave ? BaseException.SaveCode : 0xFF;
	VirtualProtect(reinterpret_cast<PVOID>(BaseException.ConditionAddress), 1, OldProtection, &OldProtection);
}

/*++

 Routine Description:

	Registers an exception with a specific condition (mode)

Parameters:
	
	ExceptionAddress - The address to register the exception
	Mode - The condition of the exception being registered

--*/

void Debugger::RegisterExceptionCondition(DWORD_PTR ExceptionAddress, Debugger::EXCEPTION_STATE Mode)
{
	Debugger::EXCEPTION_BASE Record;
	Record.UseSave = FALSE;
	Record.ConditionAddress = ExceptionAddress;
	Record.SaveCode = *reinterpret_cast<PBYTE>(ExceptionAddress);
	Record.Mode = Mode;
	ExceptionStore.push_back(Record);

	switch (Mode)
	{
	case Debugger::EXCEPTION_STATE::ImmediateMode:
		SetHltCondition(Record);
		break;
	case Debugger::EXCEPTION_STATE::ModuleMode:
		break;
	case Debugger::EXCEPTION_STATE::PageMode:
		break;
	}
}

/*++

 Routine Description:

	Removes an exception condition from the registration record

Parameters:

	ExceptionAddress - The registered address
	Mode - Used for querying the records by searching one branch of records and not the other

Return Value:
	
	A true/false boolean which indicates a successful or unsuccessful closing operation

--*/

bool Debugger::CloseExceptionCondition(DWORD_PTR ExceptionAddress, Debugger::EXCEPTION_STATE Mode)
{
	for (std::vector<Debugger::EXCEPTION_BASE>::iterator iRecord = ExceptionStore.begin(); iRecord != ExceptionStore.end(); ++iRecord)
	{
		if (iRecord->ConditionAddress == ExceptionAddress && iRecord->Mode == Mode)
		{
			ExceptionStore.erase(iRecord);
			return true;
		}
	}
	return false;
}

/*++

 Routine Description:

	Searches for the registered exception condition

Parameters:

	ExceptionAddress - The registered address
	Mode - Used for querying the records by searching one branch of records and not the other

Return Value:

	A true/false boolean which whether a record was discovered

--*/

bool Debugger::DiscoverExceptionCondition(DWORD_PTR ExceptionAddress, Debugger::EXCEPTION_STATE Mode)
{
	for (std::vector<Debugger::EXCEPTION_BASE>::iterator iRecord = ExceptionStore.begin(); iRecord != ExceptionStore.end(); ++iRecord)
	{
		if (iRecord->ConditionAddress == ExceptionAddress && iRecord->Mode == Mode)
		{
			return true;
		}
	}
	return false;
}

