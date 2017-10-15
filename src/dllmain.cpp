// dllmain.cpp : Defines the entry point for the DLL application.
// Programmed by jasonfish4
/* The excessive use of global variables is due to me not wanting to consume stack space, overuse registers, etc...
within an exception signaling function */

#define _CRT_SECURE_NO_DEPRECATE

#include "stdafx.h"
#include "Dispatcher\rvdbg.h"
#include "debugoutput.h"
#include <winsock.h>
#include <string>
#include <map>

#pragma comment(lib, "wsock32.lib")

BOOLEAN ProtocolGUI;

const char* ModuleName;
const char* SecModuleName;
const char* Filename;

SOCKET Server;
SOCKADDR_IN ServerAddress;
HANDLE lThreads[2];
std::map<char, DWORD> XMMRegister;
std::map<char, DWORD> GeneralRegister;



DWORD WINAPI Repeater(LPVOID lpParam)
{
	while (TRUE)
	{
		if (tDSend == FALSE)
		{
			EnterCriticalSection(&repr);
			SleepConditionVariableCS(&reprcondition, &repr, INFINITE);
			LeaveCriticalSection(&repr);
		}
		if (tDSend == TRUE)
			send(Server, std::string("!DbgModeOn").c_str(), std::string("!DbgModeOn").size(), 0);
		tDSend = FALSE;
	}
}

void RegisterValue(char Key, DWORD Value)
{
	SetRegister(GeneralRegister.at(Key), Value);
}

void RegisterValueFP(char Key, BOOLEAN Precision, double Value)
{
	SetRegisterFP(XMMRegister.at(Key), Precision, Value);
}

void CreateMap()
{
	GeneralRegister['0'] = GPRegisters::EAX;
	GeneralRegister['1'] = GPRegisters::EBX;
	GeneralRegister['2'] = GPRegisters::ECX;
	GeneralRegister['3'] = GPRegisters::EDX;
	GeneralRegister['4'] = GPRegisters::EDI;
	GeneralRegister['5'] = GPRegisters::ESI;
	GeneralRegister['6'] = GPRegisters::EBP;
	GeneralRegister['7'] = GPRegisters::ESP;
	GeneralRegister['8'] = GPRegisters::EIP;

	XMMRegister['0'] = SSERegisters::xmm0;
	XMMRegister['1'] = SSERegisters::xmm1;
	XMMRegister['2'] = SSERegisters::xmm2;
	XMMRegister['3'] = SSERegisters::xmm3;
	XMMRegister['4'] = SSERegisters::xmm4;
	XMMRegister['5'] = SSERegisters::xmm5;
	XMMRegister['6'] = SSERegisters::xmm6;
	XMMRegister['7'] = SSERegisters::xmm7;
}

DWORD WINAPI Dispatch(PVOID lpParam)
{	
	WSAData wsaData;
	WSAStartup(MAKEWORD(2, 2), &wsaData);
	Server = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	ServerAddress = { 0 };
	ServerAddress.sin_addr.s_addr = inet_addr("127.0.0.1");
	ServerAddress.sin_port = htons(8888);
	ServerAddress.sin_family = AF_INET;
	connect(Server, (SOCKADDR*)&ServerAddress, sizeof(ServerAddress));

	AttachRVDbg();
	CreateMap();
	AssignThread(lThreads[0]);
	AssignThread(lThreads[1]);

	char buffer[128];
	char snbuffer[128];

	DWORD Symbol = NULL;

	while (TRUE)
	{
		memset(buffer, 0, sizeof(buffer));
		memset(snbuffer, 0, sizeof(snbuffer));

		recv(Server, buffer, sizeof(buffer), 0);
		std::string receiver;
		receiver = std::string(buffer);
		
		if (receiver.substr(0, std::string("!Symbol @").length()) == std::string("!Symbol @"))
		{
			if (!receiver.substr(sizeof("!Symbol @ "), receiver.size()).empty())
				sscanf(receiver.substr(sizeof("!Symbol @ "), receiver.size()).c_str(), "%X", &Symbol);
		}

		else if (receiver.substr(0, std::string("!setreg").length()) == std::string("!setreg"))
		{
			DWORD regv = strtol(receiver.substr(std::string("!setreg ? ").length(), receiver.length()).c_str(), NULL, 16);
			RegisterValue(receiver.at(std::string("!setreg ").length()), regv);
		}

		else if (receiver.substr(0, std::string("!fsetreg").length()) == std::string("!fsetreg"))
		{
			double regv = strtod(receiver.substr(std::string("!fsetreg ? ").length(), receiver.length()).c_str(), NULL);
			RegisterValueFP(receiver.at(std::string("!fsetreg ").length()), 0, regv);
		}

		else if (receiver.substr(0, std::string("!dsetreg").length()) == std::string("!dsetreg"))
		{
			double regv = strtod(receiver.substr(std::string("!dsetreg ? ").length(), receiver.length()).c_str(), NULL);
			RegisterValueFP(receiver.at(std::string("!dsetreg ").length()), 1, regv);
		}

		else if (receiver == std::string("**ProtocolGUI"))
			ProtocolGUI = TRUE;

		else if (receiver == std::string("!Breakpoint"))
		{
			SetExceptionMode(0);
			size_t ExceptionElement = CheckSector(GetSector(), 128);
			MEMORY_BASIC_INFORMATION mbi;
			VirtualQuery((LPVOID)Symbol, &mbi, sizeof(mbi));
			if (mbi.Protect != PAGE_NOACCESS)
				AddException(GetSector(), ExceptionElement, 0, Symbol);
		}
		else if (receiver.substr(0, std::string("!Undo ").length()) == std::string("!Undo "))
		{
			int index = strtol(receiver.substr(std::string("!Undo ").length(), receiver.length()).c_str(), NULL, 12);
			if (index < GetSectorSize())
			{
				DWORD OldProtect;
				VirtualProtect((LPVOID)GetSector()[index].ExceptionAddress, 1, PAGE_EXECUTE_READWRITE, &OldProtect);
				*(DWORD*)(GetSector()[index].ExceptionAddress) = (GetSector()[index].SaveCode);
				VirtualProtect((LPVOID)GetSector()[index].ExceptionAddress, 1, OldProtect, &OldProtect);
				UnlockSector(GetSector(), index);
			}
			index = 0;
		}
		else if (receiver == std::string("?Breakpoint"))
		{
			SetExceptionMode(1);
			size_t ExceptionElement = CheckSector(GetSector(), 128);
			AddException(GetSector(), ExceptionElement, 1, Symbol);
		}

		else if (receiver == std::string("!Get"))
		{
			int index = SearchSector(GetSector(), GetSectorSize(), Symbol);
			if (index < GetSectorSize())
			{
				snprintf(snbuffer, sizeof(snbuffer), "$    Symbol: 0x%02X\r\n    Index:%d\r\n", Symbol, index);
				send(Server, snbuffer, sizeof(snbuffer), 0);
			}
			else
				send(Server, "$    Symbol not registered", sizeof("$    Symbol not registered"), 0);
		}

		else if (receiver == std::string("!DbgGet"))
			SendDbgGet(Server, GetExceptionMode(), GetPool());
		

		else if (receiver == std::string("!DbgDisplayRegisters"))
			SendDbgRegisters(Server, ProtocolGUI, GetExceptionAddress(), GetRegisters());
		else if (receiver == std::string("!xmm-f"))
			SendDbgRegisters(Server, 2, GetExceptionAddress(), GetRegisters());
		else if (receiver == std::string("!xmm-d"))
			SendDbgRegisters(Server, 3, GetExceptionAddress(), GetRegisters());

		else if (receiver == std::string("!DbgRun"))
		{
			if (IsAEHPresent())
				ContinueDebugger();
		}

		else if (receiver == std::string("!Exit"))
		{
			if (IsAEHPresent())
				ContinueDebugger();
			break;
		}
	}

	DetachRVDbg();
	closesocket(Server);
	WSACleanup();
	TerminateThread(lThreads[0], 0);
	RemoveThread(lThreads[0]);
	RemoveThread(lThreads[1]);
	return 0;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved)
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		UseModule = FALSE;
		if (UseModule)
			DLLInject(GetCurrentProcessId(), Filename);
		InitializeCriticalSection(&repr);
		InitializeConditionVariable(&reprcondition);
		lThreads[0] = CreateThread(0, 0, Repeater, 0, 0, 0);
		lThreads[1] = CreateThread(0, 0, Dispatch, 0, 0, 0);
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
	case DLL_PROCESS_DETACH:
		break;
	}
	return TRUE;
}


