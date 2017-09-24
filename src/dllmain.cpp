// Programmed by jasonfish4
/* The excessive use of global variables is due to me not wanting to consume stack space, overuse registers, etc...
within an exception signaling function */

#include "stdafx.h"
#include "Dispatcher\rvdbg.h"
#include "debugoutput.h"
#include <winsock.h>
#include <iostream>
#include <string>

#pragma comment(lib, "wsock32.lib")

BOOLEAN ProtocolGUI;

const char* ModuleName; // Name of the image
const char* SecModuleName; // Name of the copy image
const char* Filename; // File location to copy image

SOCKET Server;
SOCKADDR_IN ServerAddress;
HANDLE lThreads[2];


DWORD WINAPI Repeater(LPVOID lpParam)
{
	while (TRUE)
	{
		if (tDSend == FALSE)
			Sleep(1);
		else
		{
			tDSend = FALSE;
			send(Server, "!DbgModeOn", sizeof("!DbgModeOn"), 0);
		}
	}
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

	AssignThread(lThreads[0]);
	AssignThread(lThreads[1]);

	SetModule(FALSE);

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
				Symbol = strtol(receiver.substr(sizeof("!Symbol @ "), receiver.size()).c_str(), NULL, 16);
		}

		else if (receiver == std::string("**ProtocolGUI"))
			ProtocolGUI = TRUE;

		else if (receiver == std::string("!Breakpoint"))
		{
			SetExceptionMode(0);
			size_t ExceptionElement = CheckSector(GetSector(), 128);
			AddException(GetSector(), ExceptionElement, 0, Symbol);
		}
		else if (receiver == std::string("?Breakpoint"))
		{
			SetExceptionMode(1);
			size_t ExceptionElement = CheckSector(GetSector(), 128);
			AddException(GetSector(), ExceptionElement, 1, Symbol);
		}
		else if (receiver == std::string("!Get"))
		{
			snprintf(snbuffer, sizeof(snbuffer), "$    Symbol: 0x%02X\n", Symbol);
			send(Server, snbuffer, sizeof(snbuffer), 0);
		}

		else if (receiver == std::string("!DbgGet"))
			SendDbgGet(Server, GetExceptionMode(), GetPool());
		

		else if (receiver == std::string("!DbgDisplayRegisters"))
			SendDbgRegisters(Server, ProtocolGUI, GetExceptionAddress(), GetRegisters());

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

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
					 )
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		lThreads[0] = CreateThread(0, 0, Repeater, 0, 0, 0);
		lThreads[1] = CreateThread(0, 0, Dispatch, 0, 0, 0);
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
	case DLL_PROCESS_DETACH:
		break;
	}
	return TRUE;
}

