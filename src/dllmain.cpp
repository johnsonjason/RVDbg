// dllmain.cpp : Defines the entry point for the DLL application.
#include "stdafx.h"
#include "rvdbg.h"
#include "exception_store.h"
#include <tuple>
#include <iostream>

#define CTL_SET_PTR 0 // Set the current address of debug exception
#define CTL_GET_PTR 1 // Get the current address of debug exception
#define CTL_SET_REG 2 // Set an immediate register value
#define CTL_GET_REG 3 // Get an immediate register value
#define CTL_SET_XMM 4 // Set an XMM register
#define CTL_GET_XMM 5 // Get an XMM register
#define CTL_SET_BPT 6 // Register the address of debug exception as a breakpoint with an exception condition


DWORD_PTR ControlRegister(std::tuple<BYTE, DWORD_PTR, DWORD_PTR>& ControlCode)
{
	//
	// Get the active snapshot of the debugger - the current thread context and associated debug data for the exception
	// Validate if the active snapshot is null, if it is then there are no active debug frames or a critical error in logic has occurred.
	//

	Debugger::DebuggerSnapshot* ActiveSnapshot = Debugger::GetActiveSnapshot();

	if (ActiveSnapshot == NULL)
	{
		std::cerr << "Error: No active debug frames." << std::endl;
		return;
	}

	//
	// Reiterates the control code parsing process and narrows it down to two control codes
	// CTL_SET_REG (2) and CTL_GET_REG (3) respectively 
	// Where CTL_SET_REG sets the value for a specified register and CTL_GET_REG gets the value for a specified register
	// Example - (CTL_SET_REG, 0, 1) Argument 2 references the Debugger::GENERAL_REGISTER enum. Argument 3 is the value
	// Example - (CTL_GET_REG, 0) Argument 2 references the Debugger::GENERAL_REGISTER enum and returns its value.
	//

	switch (std::get<0>(ControlCode))
	{
	case CTL_SET_REG:
		ActiveSnapshot->SetGeneralPurposeReg(static_cast<Debugger::GENERAL_REGISTER>(std::get<1>(ControlCode)), std::get<2>(ControlCode));
		return NULL;
	case CTL_GET_REG:
		return ActiveSnapshot->GetGeneralPurposeReg(static_cast<Debugger::GENERAL_REGISTER>(std::get<1>(ControlCode)));
	}
}

void AcquireDebugMonitor(void* data)
{
	//
	// The CurrentSymbol variable is the address to be operated on
	// The ControlCode variable is an operation code which tells the debugger what to do
	// Example (0, 0xFFFFFFFF) 0 is the control code for CTL_SET_PTR which will set the symbol to 0xFFFFFFFF
	// Example (1) is the control code for CTL_GET_PTR and outputs the value of the CurrentSymbol variable
	// Example (2, 3, 45) 2 is the control code for CTL_SET_REG which sets the register, 3 is the Edx register, and 45 is the immediate value that will be set for Edx
	//

	/* TBA: Missing logic which acquires the control code through IPC */

	DWORD CurrentSymbol = 0x00000000;
	std::tuple<BYTE, DWORD_PTR, DWORD_PTR> ControlCode(0, 0, 0);

	while (true)
	{
		switch (std::get<0>(ControlCode))
		{
		case CTL_SET_PTR:
			CurrentSymbol = std::get<1>(ControlCode);
			break;
		case CTL_GET_PTR:
			std::cout << CurrentSymbol << std::endl;
			break;
		case CTL_SET_REG:
		case CTL_GET_REG:
			ControlRegister(ControlCode);
			break;
		case CTL_SET_BPT:

			//
			// Register the exception condition
			// Address of exception - CurrentSymbol from the CTL_SET_PTR control code
			// Condition - the second argument of the CTL_SET_BPT control code
			//

			Debugger::RegisterExceptionCondition(CurrentSymbol, static_cast<Debugger::EXCEPTION_STATE>(std::get<1>(ControlCode)));
			break;
		}
	}
}

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}
