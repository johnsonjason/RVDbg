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
		if (*reinterpret_cast<PDWORD>(iBase) == KI_DISPATCH_SIGNATURE)
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

--*/

void Debugger::InitializeDebugInfo()
{
	RealKiUserExceptionDispatcher = reinterpret_cast<PVOID>(GetProcAddress(GetModuleHandleW(L"ntdll.dll"), "KiUserExceptionDispatcher"));
	HookAPIRoutine(ScanKiSignature(RealKiUserExceptionDispatcher), SaveRegisterState, "ntdll:KiUserExceptionDispatcher");
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

	Suspend all threads in the process during debug mode, except the debug control thread 
	Later resume the networking and general control threads

Parameters:

	ExceptionThreadId - The thread id of the debug control thread to be exempt

--*/

static void SuspendThreads(DWORD_PTR ExceptionThreadId)
{
	HANDLE Snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
	if (Snapshot != INVALID_HANDLE_VALUE)
	{
		THREADENTRY32 ThreadEntry;
		ThreadEntry.dwSize = sizeof(ThreadEntry);
		if (Thread32First(Snapshot, &ThreadEntry))
		{
			do
			{
				if (ThreadEntry.dwSize >= FIELD_OFFSET(THREADENTRY32, th32OwnerProcessID) + sizeof(ThreadEntry.th32OwnerProcessID))
				{
					if (ThreadEntry.th32ThreadID != ExceptionThreadId && ThreadEntry.th32OwnerProcessID == GetCurrentProcessId())
					{
						HANDLE Thread = OpenThread(THREAD_ALL_ACCESS, FALSE, ThreadEntry.th32ThreadID);
						if (Thread != NULL)
						{
							SuspendThread(Thread);
							CloseHandle(Thread);
						}
					}
				}
				ThreadEntry.dwSize = sizeof(ThreadEntry);
			} while (Thread32Next(Snapshot, &ThreadEntry));
		}
		CloseHandle(Snapshot);
	}
}

/*++

Routine Description:

	Resume all threads in the process during debug mode

Parameters:

	ExceptionThreadId - The thread id that was exempt during suspension

--*/

static void ResumeThreads(DWORD ExceptionThreadId)
{
	HANDLE Snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
	if (Snapshot != INVALID_HANDLE_VALUE)
	{
		THREADENTRY32 ThreadEntry;
		ThreadEntry.dwSize = sizeof(ThreadEntry);
		if (Thread32First(Snapshot, &ThreadEntry))
		{
			do
			{
				if (ThreadEntry.dwSize >= FIELD_OFFSET(THREADENTRY32, th32OwnerProcessID) + sizeof(ThreadEntry.th32OwnerProcessID))
				{
					if (ThreadEntry.th32ThreadID != ExceptionThreadId && ThreadEntry.th32OwnerProcessID == GetCurrentProcessId())
					{
						HANDLE Thread = OpenThread(THREAD_ALL_ACCESS, FALSE, ThreadEntry.th32ThreadID);
						if (Thread != NULL)
						{
							ResumeThread(Thread);
							CloseHandle(Thread);
						}
					}
				}
				ThreadEntry.dwSize = sizeof(ThreadEntry);
			} while (Thread32Next(Snapshot, &ThreadEntry));
		}
		CloseHandle(Snapshot);
	}
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

*/

PVOID EnterDebugState()
{
	if (!Debugger::DiscoverExceptionCondition(GlobalContext.dwExceptionComparator, Debugger::EXCEPTION_STATE::ImmediateMode))
	{
		if (!Debugger::DiscoverExceptionCondition(GlobalContext.dwExceptionComparator, Debugger::EXCEPTION_STATE::ModuleMode))
		{
			return NULL;
		}
		return ModuleModeDispatcher();
	}

	SuspendThreads(GetCurrentThreadId());
	Debugger::DebuggerSnapshot* Snapshot = new Debugger::DebuggerSnapshot(GlobalContext, SSEGlobalContext);
	ActiveSnapshot = Snapshot;

	if (Debugger::GetProcessState() == Debugger::PROCESS_STATE::Continuous)
	{
		
		return reinterpret_cast<PVOID>(GlobalContext.dwEip);
	}

	ActiveSnapshot = NULL;
	delete Snapshot;

	Debugger::CloseExceptionCondition(GlobalContext.dwExceptionComparator, Debugger::EXCEPTION_STATE::ImmediateMode);
	return reinterpret_cast<PVOID>(GlobalContext.dwEip);
}
