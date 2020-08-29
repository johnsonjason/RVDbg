#include "stdafx.h"
#include "rvdbg.h"

// Signature used to find KiUserExceptionDispatcher and where execution begins
#define KI_DISPATCH_SIGNATURE 0x04244C8B

// This is the debugger's global context, contains general debug information and general purpose registers. Has the final say on the thread state's actual values. Modifications are made through snapshots which are pushed to the global context.
Debugger::DBG_CONTEXT_STATE GlobalContext;

// This is the debugger's global SSE context, containing the values of the SSE registers (e.g. xmm0), has the final say on SSE register values. Modifications are made through snapshots which are pushed to the global SSE context.
Debugger::DBG_SSE_REGISTERS SSEGlobalContext;

// This is the value of the debugger's process state, which is how the debugger will handle threads due to an exception when it comes in, either through continuous processing, suspension of all but one, etc...
Debugger::PROCESS_STATE ProcessState;
Debugger::DebuggerSnapshot* ActiveSnapshot = NULL;
PVOID RealKiUserExceptionDispatcher;
PVOID RetDebuggerAddress;

// DebugInputThreadId is initialized from Debugger::InitializeDebugInfo, it is utilized to exempt the debugger interface from having its thread suspended or resumed
DWORD DebugInputThreadId = NULL;

// DebuggerPauseCondition is used in this source file's EnterDebugState, it is utilized to synchronize suspended access between the debugger and the debugger interface
bool Debugger::DebuggerPauseCondition = false;

// DebugInputReader is initialized from Debugger::InitializeDebugInfo, it is used to communicate with the debug server
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

	Wakes the thread being managed by the debugger waiting on an address to change, to resume its normal program execution path

Parameters:

	None

Return Value:

	None

--*/
void Debugger::RunDebugger()
{
	Debugger::DebuggerPauseCondition = false;
	WakeByAddressSingle(&Debugger::DebuggerPauseCondition);
}



/*++
Routine Description:

	Classifies the type of x86 jump or call, and then calculates the step address after calculation
	Emulates conditional jumps to evaluate the execution branch sequence

Parameters:

	Instruction - The x86 instruction that was decoded by Zydis

Return Value:

	A status code that specifies a classification was necessary, not necessary, or the routine failed

--*/
int Debugger::StepClassify(ZydisDecodedInstruction& Instruction, DWORD_PTR CurrentInstructionPointer)
{
	//
	// TODO:
	// Add classifier for all types of unconditional jmps (e.g. jmp dword ptr [xxxxx], jmp eax, etc)
	// Separate classifiers in individual routines instead of this large routine
	// Add classifier for calls
	// Classifier for ret, rest of path of execution change instructions
	// 
	if (Instruction.mnemonic >= ZydisMnemonic::ZYDIS_MNEMONIC_JB && Instruction.mnemonic <= ZydisMnemonic::ZYDIS_MNEMONIC_JZ)
	{

		if (Instruction.mnemonic == ZydisMnemonic::ZYDIS_MNEMONIC_JMP)
		{
			// WIP
		}
		//
		// De-allocate VirtualAlloc'd memory in EnterDebugState
		//

		PVOID BranchEmulator = VirtualAlloc(NULL, 1024, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);

		if (!BranchEmulator)
		{
			return -1;
		}

		// 
		// Check if the conditional jump is using both an immediate value and is relative/short (conditional jumps cannot have r/m32/m16, only rel8/16/32.)
		// If first AND fails, then OR and then check if it's a near jump
		// With ZYDIS_OPCODE_MAP_DEFAULT, the conditional jump can only have rel8
		// With ZYDIS_OPCODE_MAP_0F, the conditional can be rel16/32
		//

		if (Instruction.operands[0].type == ZydisOperandType::ZYDIS_OPERAND_TYPE_IMMEDIATE && Instruction.opcode_map == ZydisOpcodeMap::ZYDIS_OPCODE_MAP_DEFAULT ||
			Instruction.operands[0].type == ZydisOperandType::ZYDIS_OPERAND_TYPE_IMMEDIATE && Instruction.opcode_map == ZydisOpcodeMap::ZYDIS_OPCODE_MAP_0F)
		{
			//
			// Use BranchEmulatorVal for pointer arithmetic at the "jump emulation" location
			// RelativeNearJumpByteSize is the amount of bytes that a conditional rel/8 jump can take
			// VirtualJumpRange is the amount of bytes the jump will move forward with the next IP
			// JmpOpDiff is the difference of the single byte instruction when jumping from near vs short. example: je is 0x74 when doing a short jump, it's 0x84 when doing a near jump
			//

			DWORD_PTR BranchEmulatorVal = reinterpret_cast<DWORD_PTR>(BranchEmulator);
			const std::size_t RelativeShortJumpByteSize = 2;
			const std::size_t VirtualJumpRange = 0x0B;
			const std::size_t JmpOpDiff = 0x10;

			*reinterpret_cast<PBYTE>(BranchEmulator) = Instruction.opcode;
			*reinterpret_cast<PBYTE>(BranchEmulatorVal + 1) = VirtualJumpRange;

			//
			// VirtualLinearTarget/VirtualNonlinearTarget is used for emulating the condition of the jump
			// Upon either being triggered by exception, it will redirect the execution path to their corresponding ACTUAL locations (LinearTarget/NonlinearTarget)
			//

			DWORD_PTR VirtualLinearTarget = (BranchEmulatorVal + RelativeShortJumpByteSize);
			DWORD_PTR LinearTarget = (CurrentInstructionPointer + Instruction.length);

			DWORD_PTR VirtualNonlinearTarget = (BranchEmulatorVal + RelativeShortJumpByteSize + VirtualJumpRange);
			DWORD_PTR NonlinearTarget = NULL;

			//
			// Set the actual nonlinear location based on short/near jump.
			//

			if (Instruction.opcode_map == ZydisOpcodeMap::ZYDIS_OPCODE_MAP_0F)
			{
				*reinterpret_cast<PBYTE>(BranchEmulator) = Instruction.opcode - JmpOpDiff;
				NonlinearTarget = CurrentInstructionPointer + 6 + Instruction.operands[0].imm.value.u;
			}
			else
			{
				NonlinearTarget = (CurrentInstructionPointer + RelativeShortJumpByteSize + Instruction.operands[0].imm.value.u);
			}

			//
			// Set the breakpoint at the virtual target locations
			//

			Debugger::RegisterStepCondition(VirtualLinearTarget, Debugger::EXCEPTION_STATE::VirtualLinearStep, LinearTarget);
			Debugger::RegisterStepCondition(VirtualNonlinearTarget, Debugger::EXCEPTION_STATE::VirtualNonlinearStep, NonlinearTarget);

#ifdef _M_IX86
			Debugger::GetActiveSnapshot()->SetGeneralPurposeReg(Debugger::GENERAL_REGISTER::Eip, BranchEmulatorVal);
#elif defined _M_AMD64
			Debugger::GetActiveSnapshot()->SetGeneralPurposeReg(Debugger::GENERAL_REGISTER::Rip, BranchEmulatorVal);
#endif
		}

		return 1;
	}

	return 0;
}

/*++

Routine Description:

	The ability for the debugger to step to the next instruction (executing the current instruction)
	and then capturing context information prior to the execution of the next instruction.
	We must obscure single stepping, thus, we must manually step through disassembly.

Parameters:

	None

Return Value:

	None

--*/
void Debugger::StepDebugger()
{
	if (Debugger::GetProcessState() == Debugger::PROCESS_STATE::StepState)
	{
		Debugger::RunDebugger();
		return;
	}

	//
	// Create a DecoderBuffer (the memory contents which will be passed to Zydis for disassembly) with the MaxInstructionSize in reference to the maximum amount of bytes an instruction
	// can occupy according to the Intel manual
	// Create a ZydisDecoder for initializing the disassembler, and ZydisDecodedInstruction which is a struct that will receive information about the decoded instruction
	//

	const std::size_t MaxInstructionSize = 15;
	BYTE DecoderBuffer[MaxInstructionSize] = { 0 };

	ZydisDecoder Decoder;
	ZydisDecodedInstruction Instruction;
	PVOID CurrentInstructionPointer = NULL;

	//
	// Initialize the decoder and set the current instruction pointer depending on the platform processor architecture 
	//

#ifdef _M_IX86
	ZydisDecoderInit(&Decoder, ZYDIS_MACHINE_MODE_LONG_COMPAT_32, ZYDIS_ADDRESS_WIDTH_32);
	CurrentInstructionPointer = reinterpret_cast<PVOID>(Debugger::GetActiveSnapshot()->GetGeneralPurposeReg(Debugger::GENERAL_REGISTER::Eip));
#elif defined _M_AMD64
	ZydisDecoderInit(&Decoder, ZYDIS_MACHINE_MODE_LONG_64, ZYDIS_ADDRESS_WIDTH_64);
	CurrentInstructionPointer = reinterpret_cast<PVOID>(Debugger::GetActiveSnapshot()->GetGeneralPurposeReg(Debugger::GENERAL_REGISTER::Rip));
#endif

	//
	// Copy the contents at the current instruction pointer into the DecoderBuffer
	// Decode the DecoderBuffer and calculate the next address, then place a breakpoint at the next address and resume the debugger
	//

	memcpy(DecoderBuffer, CurrentInstructionPointer, 15);
	ZydisDecoderDecodeBuffer(&Decoder, DecoderBuffer, sizeof(DecoderBuffer), &Instruction);

	if (!Debugger::StepClassify(Instruction, reinterpret_cast<DWORD_PTR>(CurrentInstructionPointer)))
	{
		Debugger::RegisterExceptionCondition(reinterpret_cast<DWORD_PTR>(CurrentInstructionPointer) + Instruction.length, Debugger::EXCEPTION_STATE::ImmediateMode);
	}

	Debugger::RunDebugger();
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

Return Value:

	None
--*/

void Debugger::SetProcessState(PROCESS_STATE State)
{
	ProcessState = State;
}


/*++

Routine Description:

	This routine is a stub.

--*/

static PVOID ModuleModeDispatcher(int ExceptionIndex)
{
	return NULL;
}

/*++

Routine Description:

	This routine is a stub.

--*/

static PVOID PageModeDispatcher(int ExceptionIndex)
{
	return NULL;
}


/*++
Routine Description:

	This serves as the dispatcher for virtual step routines, where branch prediction is emulated for conditional execution
	The reason for emulating branch prediction is because in normal stepping (e.g. use of the trap flag), the processor will handle where a branch will go
	However, since stepping is implemented manually, we have to substitute for this

Parameters:
	
	StepIndex - The index of the record in the StepPipeline

Return Value:

	The real step memory location
--*/
static PVOID VirtualStepDispatcher(int StepIndex)
{
	//
	// Save the process state and then set it to a reserved stepping state
	//

	Debugger::PROCESS_STATE SaveState = Debugger::GetProcessState();
	Debugger::SetProcessState(Debugger::PROCESS_STATE::StepState);

	//
	// Get the base address of the virtual page used for emulated stepping, to free it
	//

	MEMORY_BASIC_INFORMATION MemoryInformation = { 0 };
	PVOID VirtualBranch = reinterpret_cast<PVOID>(Debugger::StepPipeline.at(StepIndex).ConditionAddress);

	if (VirtualQuery(VirtualBranch, &MemoryInformation, sizeof(MemoryInformation)))
	{
		VirtualFree(MemoryInformation.BaseAddress, 0, MEM_RELEASE);
	}

	//
	// Set the GlobalContext IP to the logical stepping location (not the emulated location.)
	// Empty the StepPipeline
	// Register a new condition for the real step location and reset the process state
	//

#ifdef _M_IX86
	GlobalContext.dwEip = Debugger::StepPipeline.at(StepIndex).LogicalStepAddress;
	Debugger::RegisterExceptionCondition(GlobalContext.dwEip, Debugger::EXCEPTION_STATE::ImmediateMode);
#elif _M_AMD64
	GlobalContext.dwRip = Debugger::StepPipeline.at(StepIndex).LogicalStepAddress;
	Debugger::RegisterExceptionCondition(GlobalContext.dwRip, Debugger::EXCEPTION_STATE::ImmediateMode);
#endif

	Debugger::StepPipeline.clear();


	Debugger::SetProcessState(SaveState);
	return reinterpret_cast<PVOID>(GlobalContext.dwEip);
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

Parameters:

	None

Return Value:

	The address of the exception, if it returns zero then statesave.asm 
	will pass the exception to the regular Windows exception handler execution path

--*/

PVOID EnterDebugState()
{
	//
	// Check if a regular breakpoint was encountered, or one of the virtual stepping modes
	//

	int ExceptionIndex = Debugger::DiscoverExceptionCondition(GlobalContext.dwExceptionComparator, Debugger::EXCEPTION_STATE::ImmediateMode);
	if (ExceptionIndex < 0)
	{
		ExceptionIndex = Debugger::DiscoverStepCondition(GlobalContext.dwExceptionComparator, Debugger::EXCEPTION_STATE::VirtualLinearStep);
		if (ExceptionIndex >= 0)
		{
			return VirtualStepDispatcher(ExceptionIndex);
		}

		ExceptionIndex = Debugger::DiscoverStepCondition(GlobalContext.dwExceptionComparator, Debugger::EXCEPTION_STATE::VirtualNonlinearStep);
		if (ExceptionIndex >= 0)
		{
			return VirtualStepDispatcher(ExceptionIndex);
		}
		return NULL;
	}

	//
	// Restore the original instruction at the address of exception, restore data (e.g. page properties)
	// Handle global context values that are not assigned from statesave.asm
	//

	Debugger::HandleException(Debugger::ExceptionStore.at(ExceptionIndex));
	GlobalContext.dwEip = Debugger::ExceptionStore.at(ExceptionIndex).ConditionAddress;

	//
	// Suspend all threads, except the one thread passed as an Id if Inclusive
	// Continue normal execution, future use for subtle hook; if Continuous
	// Else, the state is Exclusive so the only thread that will be "suspended" is this one, but actually in a waiting state
	//

	if (Debugger::GetProcessState() == Debugger::PROCESS_STATE::Inclusive)
	{
		SuspendThreads(GetCurrentThreadId());
		std::cout << "IP: " << std::hex << reinterpret_cast<PVOID>(GlobalContext.dwEip) << std::endl;
	}
	else if (Debugger::GetProcessState() == Debugger::PROCESS_STATE::Continuous)
	{
		std::cout << "IP: " << std::hex << reinterpret_cast<PVOID>(GlobalContext.dwEip) << std::endl;
#ifdef _M_IX86
		return reinterpret_cast<PVOID>(GlobalContext.dwEip);
#elif _M_AMD64
		return reinterpret_cast<PVOID>(GlobalContext.dwRip);
#endif
	}

	//
	// Open a handle to the debugger thread that takes input
	//

	HANDLE DebugThread = OpenThread(THREAD_SUSPEND_RESUME, FALSE, DebugInputThreadId);

	if (!DebugThread)
	{
		ResumeThreads(GetCurrentThreadId());
		Debugger::CloseExceptionCondition(GlobalContext.dwExceptionComparator, Debugger::EXCEPTION_STATE::ImmediateMode);
#ifdef _M_IX86
		return reinterpret_cast<PVOID>(GlobalContext.dwEip);
#elif _M_AMD64
		return reinterpret_cast<PVOID>(GlobalContext.dwRip);
#endif
	}

	//
	// Resume the debugger front-end thread
	// Initialize snapshot of the debug context and set it as the active state
	// Notify the debugger client that it can take input
	//

	ResumeThread(DebugThread);
	CloseHandle(DebugThread);

	Debugger::DebuggerSnapshot* Snapshot = new Debugger::DebuggerSnapshot(GlobalContext, SSEGlobalContext);
	ActiveSnapshot = Snapshot;

	DebugInputRepeater->SendString("DebuggerOn");

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
	Debugger::CloseExceptionCondition(GlobalContext.dwExceptionComparator, Debugger::EXCEPTION_STATE::ImmediateMode);
	ResumeThreads(GetCurrentThreadId());

#ifdef _M_IX86
	return reinterpret_cast<PVOID>(GlobalContext.dwEip);
#elif _M_AMD64
	return reinterpret_cast<PVOID>(GlobalContext.dwRip);
#endif
}
