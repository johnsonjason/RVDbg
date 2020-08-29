#include "stdafx.h"
#include "exception_store.h"
#include <Zydis/Zydis.h>
//
// Record of all currently registered exceptions
//

std::vector<Debugger::EXCEPTION_BASE> Debugger::ExceptionStore;
std::vector<Debugger::EXCEPTION_BASE> Debugger::StepPipeline;

/*++ 

 Routine Description:

	Uses the BaseException parameter and changes the protection on .ConditionAddress so it can be written
	Modifies the address in the BaseException to cause an exception based on a privileged instruction (HLT) : UseSave = FALSE
	OR
	Modifies the address in the BaseException to the operation code (SaveCode) prior to an exception : UseSave = TRUE

	This is a routine that occurs a two points of time, one which is prior to an exception being handled, and one point in time that is post-handling.
	This is why the SaveCode and UseSave optins exist within EXCEPTION_BASE

Parameters:

	BaseException - The exception record to apply the condition to

Return Value:

	None

--*/
static void SetHltCondition(Debugger::EXCEPTION_BASE& BaseException)
{
	DWORD OldProtection;
	VirtualProtect(reinterpret_cast<PVOID>(BaseException.ConditionAddress), 1, PAGE_EXECUTE_READWRITE, &OldProtection);

	//
	// If UseSave is false, then write the instruction to trigger an exception (hlt)
	// If UseSave is true, then write the instruction that should actually be there (correct instruction / integrity)
	//

	*reinterpret_cast<PBYTE>(BaseException.ConditionAddress) = BaseException.UseSave ? BaseException.SaveCode : 0xF4;
	VirtualProtect(reinterpret_cast<PVOID>(BaseException.ConditionAddress), 1, OldProtection, &OldProtection);
}

/*

Routine Description:

	Handles the exception from the debugger and restores information about the exception point prior

Parameters:

	BaseException - The structure that contains information about the exception utilized by the debugger

*/
void Debugger::HandleException(Debugger::EXCEPTION_BASE& BaseException)
{
	switch (BaseException.Mode)
	{
	case Debugger::EXCEPTION_STATE::ImmediateMode:
		BaseException.UseSave = true;
		SetHltCondition(BaseException);
		break;
	case Debugger::EXCEPTION_STATE::ModuleMode:
		break;
	case Debugger::EXCEPTION_STATE::PageMode:
		break;
	}
}



/*++

 Routine Description:

	Searches for the registered step exception

Parameters:

	VirtualStepAddress - The registered address for virtual stepping
	Mode - Used for querying based on linear/nonlinear records

Return Value:

	An integer specifying an index, -1 = an error occured or not found

--*/
int Debugger::DiscoverStepCondition(DWORD_PTR VirtualStepAddress, Debugger::EXCEPTION_STATE Mode)
{
	if (Mode == Debugger::EXCEPTION_STATE::VirtualLinearStep || Mode == Debugger::EXCEPTION_STATE::VirtualNonlinearStep)
	{
		for (std::vector<Debugger::EXCEPTION_BASE>::iterator iRecord = StepPipeline.begin(); iRecord != StepPipeline.end(); ++iRecord)
		{
			if (iRecord->ConditionAddress == VirtualStepAddress && iRecord->Mode == Mode)
			{
				return std::distance(StepPipeline.begin(), iRecord);
			}
		}
		return -1;
	}
}

/*++

 Routine Description:

	Registers a virtual step exception with an immediate breakpoint

Parameters:

	VirtualStepAddress - The "virtual step" address which will later map to an actual/logical step address
	Mode - Specifying whether it is a linear or nonlinear step, meaning will the branch change or not

--*/
void Debugger::RegisterStepCondition(DWORD_PTR VirtualStepAddress, Debugger::EXCEPTION_STATE Mode, DWORD_PTR LogicalStepAddress)
{
	if (Mode != Debugger::EXCEPTION_STATE::VirtualLinearStep && Mode != Debugger::EXCEPTION_STATE::VirtualNonlinearStep)
	{
		return;
	}

	Debugger::EXCEPTION_BASE Record;
	Record.UseSave = FALSE;
	Record.SaveCode = *reinterpret_cast<PBYTE>(VirtualStepAddress);
	Record.ConditionAddress = VirtualStepAddress;
	Record.LogicalStepAddress = LogicalStepAddress;
	Record.Mode = Mode;

	StepPipeline.push_back(Record);
	SetHltCondition(Record);
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
	//
	// Store exception conditions within the record and push it into the exception store
	// The exception store will be linearly searched by the debugger for the exception
	//

	Debugger::EXCEPTION_BASE Record;
	MEMORY_BASIC_INFORMATION AddressQuery = { 0 };

	//
	// Query the exception address, to validate the memory exists
	//

	DWORD QueryResult = VirtualQuery(reinterpret_cast<PDWORD_PTR>(ExceptionAddress), &AddressQuery, sizeof(AddressQuery));
	if (!QueryResult || AddressQuery.Protect == PAGE_NOACCESS)
	{
		return;
	}

	Record.UseSave = FALSE;
	Record.ConditionAddress = ExceptionAddress;
	Record.SaveCode = *reinterpret_cast<PBYTE>(ExceptionAddress);
	Record.Mode = Mode;

	if (ExceptionStore.size() > INT_MAX)
	{
		return;
	}

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

	An integer specifying an index, -1 = an error occured or not found

--*/
int Debugger::DiscoverExceptionCondition(DWORD_PTR ExceptionAddress, Debugger::EXCEPTION_STATE Mode)
{
	for (std::vector<Debugger::EXCEPTION_BASE>::iterator iRecord = ExceptionStore.begin(); iRecord != ExceptionStore.end(); ++iRecord)
	{
		if (iRecord->ConditionAddress == ExceptionAddress && iRecord->Mode == Mode)
		{
			return std::distance(ExceptionStore.begin(), iRecord);
		}
	}
	return -1;
}
