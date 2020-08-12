#include "stdafx.h"
#include "rvdbg.h"

// Signature used to find KiUserExceptionDispatcher and where execution begins
#define KI_DISPATCH_SIGNATURE 0x04244C8B

Debugger::DBG_CONTEXT_STATE GlobalContext;
Debugger::DBG_SSE_REGISTERS SSEGlobalContext;
Debugger::PROCESS_STATE ProcessState;
Debugger::DebuggerSnapshot* ActiveSnapshot = NULL;
PVOID RealKiUserExceptionDispatcher;
PVOID RetDebuggerAddress;
DWORD DebugInputThreadId = NULL;
bool Debugger::DebuggerPauseCondition = false;
static dio::Client* DebugInputRepeater = NULL;

/*++

Routine Description:

	Scans KI_DISPATCH_SIGNATURE to find KiUserExceptionDispatcher

Parameters:

	Base - The base address of where the scan should start

Return Value:

	Returns the address of where KiUserExceptionDispatcher is executed for handling an exception

--*/

static PVOID ScanKiSignature(PVOID Base)
{
	for (size_t iBase = reinterpret_cast<size_t>(Base); iBase < 128; iBase++)
	{
		if (*reinterpret_cast<DWORD*>(iBase) == KI_DISPATCH_SIGNATURE)
		{
			return reinterpret_cast<PVOID>(iBase);
		}
	}
	return NULL;
}

/*++

Routine Description:

	Initializes information for the debugger
	Imports routines for use at run-time

Parameters:
	
	InputThreadId - The thread id of the thread that takes input for the debugger and translates it to certain functions (I.e triggering a breakpoint.)

	Repeater - The debugger client ptr, to be used by the debugger to notify the client that the debugger is active or inactive

Return Value:

	None

--*/

void Debugger::InitializeDebugInfo(DWORD InputThreadId, dio::Client* Repeater)
{
	DebugInputThreadId = InputThreadId;
	RealKiUserExceptionDispatcher = reinterpret_cast<PVOID>((DWORD_PTR)GetProcAddress(GetModuleHandleW(L"ntdll.dll"), "KiUserExceptionDispatcher") + 0x18);
	HookAPIRoutine(RealKiUserExceptionDispatcher, SaveRegisterState, "ntdll:KiUserExceptionDispatcher");
	DebugInputRepeater = Repeater;
}

/*++

Routine Description:

	Get the state of threads to be used for the process when it enters debug mode

Return Value:

	The process state (PROCESS_STATE) which can be Inclusive, Exclusive, or Continuous

--*/

Debugger::PROCESS_STATE Debugger::GetProcessState()
{
	return ProcessState;
}

/*++

Routine Description:

	Sets the state of threads to be used for the process when it enters debug mode

Parameters:

	State - The state for the process, states include -

	* Inclusive - Suspend all threads 
	* Exclusive - Keep all threads resumed, need-to-suspend basis
	* Continuous - Used to implement experimental debug features

--*/

void Debugger::SetProcessState(PROCESS_STATE State)
{
	ProcessState = State;
}


/*++

Routine Description:

	This routine is a stub.

--*/

static PVOID ModuleModeDispatcher()
{
	return NULL;
}

/*++

Routine Description:

	This routine is a stub.

--*/

static PVOID PageModeDispatcher()
{
	return NULL;
}

/*++

Routine Description:

	Gets the active snapshot for the debugger, or the active debug context

Return Value:

	Returns a pointer to the active debug context/snapshot

--*/

Debugger::DebuggerSnapshot* Debugger::GetActiveSnapshot()
{
	return ActiveSnapshot;
}

/*++

Routine Description:

	A debug exception occurred, enter the debug state mode.
	The main debugger routine calls DiscoverExceptionCondition to find a registered debug record.
	The registered debug record should match the global exception frame that was built from the thread context.
	Depending on the debug record, two branches of execution can occur

	Module Mode Dispatcher - Creates a cloned module which will be used in page exceptions. Page exceptions
	are used with NO_ACCESS and redirect static code to a cloned module

Return Value:

	The address of the exception, if it returns zero then statesave.asm 
	will pass the exception to the regular Windows exception handler execution path

--*/

PVOID EnterDebugState()
{
	int ExceptionIndex = Debugger::DiscoverExceptionCondition(GlobalContext.dwExceptionComparator, Debugger::EXCEPTION_STATE::ImmediateMode);
	if (ExceptionIndex < 0)
	{
		if (!Debugger::DiscoverExceptionCondition(GlobalContext.dwExceptionComparator, Debugger::EXCEPTION_STATE::ModuleMode))
		{
			//
			// NOT IMPLEMENTED
			//

			return NULL;
		}
		else
		{
			return ModuleModeDispatcher();
		}
	}

	//
	// Restore the original instruction at the address of exception, restore data (e.g. page properties)
	//

	Debugger::HandleException(Debugger::ExceptionStore.at(ExceptionIndex));

	//
	// Suspend all threads, except the one thread passed as an Id 
	// Create a snapshot of the debug context and set it as the active state
	//

	SuspendThreads(GetCurrentThreadId());
	Debugger::DebuggerSnapshot* Snapshot = new Debugger::DebuggerSnapshot(GlobalContext, SSEGlobalContext);
	ActiveSnapshot = Snapshot;

	//
	// Check if continuous mode is enabled, this is useful for subtle hooking
	//

	if (Debugger::GetProcessState() == Debugger::PROCESS_STATE::Continuous)
	{
		ResumeThreads(GetCurrentThreadId());
		return reinterpret_cast<PVOID>(GlobalContext.dwEip);
	}

	//
	// Handle global context values that are not obtained from statesave.asm
	//

	GlobalContext.dwEip = Debugger::ExceptionStore.at(ExceptionIndex).ConditionAddress;

	//
	// Notify the debugger client that it can take input
	// Resume the debugger thread that takes input
	//

	DebugInputRepeater->SendString("DebuggerOn");
	HANDLE DebugThread = OpenThread(THREAD_SUSPEND_RESUME, FALSE, DebugInputThreadId);
	ResumeThread(DebugThread);
	CloseHandle(DebugThread);

	//
	// Set debugger event condition values and wait for a change notification
	//

	Debugger::DebuggerPauseCondition = true;
	bool SaveWait = Debugger::DebuggerPauseCondition;
	WaitOnAddress(&Debugger::DebuggerPauseCondition, &SaveWait, sizeof(Debugger::DebuggerPauseCondition), INFINITE);

	//
	// Struct assignment from the snapshot to the global processor context (global- relative to the debugged process)
	// We won't be using the snapshot in immediate mode further then when it is in demand, so we will delete it
	// Notify the debugger client that debugging has stopped and resume all threads.
	//

	ActiveSnapshot->CopyToContext(GlobalContext, SSEGlobalContext);
	ActiveSnapshot = NULL;
	delete Snapshot;

	DebugInputRepeater->SendString("DebuggerOff");
	ResumeThreads(GetCurrentThreadId());
	Debugger::CloseExceptionCondition(GlobalContext.dwExceptionComparator, Debugger::EXCEPTION_STATE::ImmediateMode);
	return reinterpret_cast<PVOID>(GlobalContext.dwEip);
}
