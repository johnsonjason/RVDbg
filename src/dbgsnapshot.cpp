#include "stdafx.h"
#include "rvdbg.h"

/*++

 Routine Description:

	Creates a snapshot of the debug context
	Pushes the snapshot into the state (snapshot) history

Parameters:

	PrcState - State of general purpose registers and additional values
	PrcSSEState - State of SSE registers and values i.e XMM

--*/

Debugger::DebuggerSnapshot::DebuggerSnapshot(DBG_CONTEXT_STATE PrcState, DBG_SSE_REGISTERS PrcSSEState)
{
	this->ProcessorState = PrcState;
	this->ProcessorSSEState = PrcSSEState;
	this->StateHistory.push_back({this->ProcessorState, this->ProcessorSSEState});
}

/*++

 Routine Description:

	Sets the immediate value of a general purpose register
	64-bit addition TBA

Parameters:

	Register - The enum which indicates the register in the switch-case statement
	Value - The value to set

--*/

void Debugger::DebuggerSnapshot::SetGeneralPurposeReg(GENERAL_REGISTER Register, DWORD_PTR Value)
{
	switch (Register)
	{
	case GENERAL_REGISTER::Eax:
		this->ProcessorState.dwEax = Value;
		break;
	case GENERAL_REGISTER::Ebx:
		this->ProcessorState.dwEbx = Value;
		break;
	case GENERAL_REGISTER::Ecx:
		this->ProcessorState.dwEcx = Value;
		break;
	case GENERAL_REGISTER::Edx:
		this->ProcessorState.dwEdx = Value;
		break;
	case GENERAL_REGISTER::Esi:
		this->ProcessorState.dwEsi = Value;
		break;
	case GENERAL_REGISTER::Edi:
		this->ProcessorState.dwEdi = Value;
		break;
	case GENERAL_REGISTER::Ebp:
		this->ProcessorState.dwEbp = Value;
		break;
	case GENERAL_REGISTER::Esp:
		this->ProcessorState.dwEsp = Value;
		break;
	case GENERAL_REGISTER::Eip:
		this->ProcessorState.dwEip = Value;
		break;
	}
}

/*++

 Routine Description:

	Gets the immediate value of a general purpose register
	64-bit addition TBA

Parameters:

	Register - The enum which indicates the register in the switch-case statement

Return Value:
	
	A DWORD_PTR value (32-bit or 64-bit depending on compilation) representing the contents of the register

--*/

DWORD_PTR Debugger::DebuggerSnapshot::GetGeneralPurposeReg(GENERAL_REGISTER Register)
{
	switch (Register)
	{
	case GENERAL_REGISTER::Eax:
		return this->ProcessorState.dwEax;
	case GENERAL_REGISTER::Ebx:
		return this->ProcessorState.dwEbx;
	case GENERAL_REGISTER::Ecx:
		return this->ProcessorState.dwEcx;
	case GENERAL_REGISTER::Edx:
		return this->ProcessorState.dwEdx;
	case GENERAL_REGISTER::Esi:
		return this->ProcessorState.dwEsi;
	case GENERAL_REGISTER::Edi:
		return this->ProcessorState.dwEdi;
	case GENERAL_REGISTER::Ebp:
		return this->ProcessorState.dwEbp;
	case GENERAL_REGISTER::Esp:
		return this->ProcessorState.dwEsp;
	case GENERAL_REGISTER::Eip:
		return this->ProcessorState.dwEip;
	}
	return NULL;
}
