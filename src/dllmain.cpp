/* MIT License

Copyright(c) 2019-2020 Jason Johnson

Permission is hereby granted, free of charge, to any person obtaining a copy
of this softwareand associated documentation files(the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and /or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions :

The above copyright noticeand this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE. */

#include "stdafx.h"
#include "dio.h"
#include "rvdbg.h"
#include "exception_store.h"
#include <tuple>
#include <iostream>

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
		return NULL;
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
	return NULL;
}

void RunDebugger()
{
	Debugger::DebuggerPauseCondition = false;
	WakeByAddressSingle(&Debugger::DebuggerPauseCondition);
}

/*++

Routine Description:

	The debug input routine, responsible for receiving input from a client (e.g. breakpoint, set symbol, etc...)
	and acting on it or it transmits debug data to a client

--*/
DWORD WINAPI StartDebugMonitor(LPVOID data)
{
	//
	// The CurrentSymbol variable is the address to be operated on
	// The ControlCode variable is an operation code which tells the debugger what to do
	// Example (0, 0xFFFFFFFF) 0 is the control code for CTL_SET_PTR which will set the symbol to 0xFFFFFFFF
	// Example (1) is the control code for CTL_GET_PTR and outputs the value of the CurrentSymbol variable
	// Example (2, 3, 45) 2 is the control code for CTL_SET_REG which sets the register, 3 is the Edx register, and 45 is the immediate value that will be set for Edx
	//

	//
	// Establish a connection with the debug server
	//

	dio::InitializeNetwork();
	dio::Client DebugClient("127.0.0.1", 8888);

	Debugger::SetProcessState(Debugger::PROCESS_STATE::Inclusive);
	Debugger::InitializeDebugInfo(GetCurrentThreadId(), &DebugClient);

	DWORD CurrentSymbol = 0x00000000;

	std::tuple<BYTE, DWORD_PTR, DWORD_PTR> ControlCode(0, 0, 0);

	while (true)
	{
		//
		// Receive the command tuple from the Debug Server
		//

		ControlCode = DebugClient.ReceiveCommand();

		switch (std::get<0>(ControlCode))
		{
		case CTL_SET_PTR:
			CurrentSymbol = std::get<1>(ControlCode);
			break;

		case CTL_GET_PTR:
			DebugClient.SendString(std::to_string(CurrentSymbol));
			break;

		case CTL_SET_REG:
			ControlRegister(ControlCode);
			break;
		case CTL_GET_REG:
			DebugClient.SendString(std::to_string(ControlRegister(ControlCode)));
			break;

		case CTL_SET_BPT:

			//
			// Register the exception condition
			// Address of exception - CurrentSymbol from the CTL_SET_PTR control code
			// Condition - the second argument of the CTL_SET_BPT control code
			//

			Debugger::RegisterExceptionCondition(CurrentSymbol, static_cast<Debugger::EXCEPTION_STATE>(std::get<1>(ControlCode)));
			break;
		case CTL_DO_RUN:
			RunDebugger();
			break;
		default:
			break;
		}
	}
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
		AllocConsole();
		FILE* Stream;
		freopen_s(&Stream, "CONIN$", "r", stdin);
		freopen_s(&Stream, "CONOUT$", "w", stderr);
		freopen_s(&Stream, "CONOUT$", "w", stdout);
		CloseHandle(CreateThread(NULL, NULL, (LPTHREAD_START_ROUTINE)StartDebugMonitor, NULL, NULL, NULL));
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}
