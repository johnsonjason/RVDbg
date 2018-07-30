// RedViceDebugger.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "injector.h"
#include "ptinfo.h"
#include <winsock.h>
#include <iostream>
#include <array>
#pragma comment(lib, "wsock32.lib")

HANDLE hStdout;
SOCKET manager_client;

void clear_console(HANDLE stdhandle)
{
	DWORD cwritten;
	CONSOLE_SCREEN_BUFFER_INFO csbi;
	COORD coordinates = { 0, };
	GetConsoleScreenBufferInfo(stdhandle, &csbi);
	FillConsoleOutputCharacterW(stdhandle, static_cast<TCHAR>(' '), csbi.dwSize.X * csbi.dwSize.Y, coordinates, &cwritten);
	SetConsoleCursorPosition(stdhandle, coordinates);
}

DWORD WINAPI dbg_handler(LPVOID lpParam)
{
	std::array<char, 512> recvbuffer;
	while (true)
	{
		std::memset(recvbuffer.data(), 0, recvbuffer.size());
		recv(manager_client, recvbuffer.data(), recvbuffer.size(), 0);
		std::string recvstring(recvbuffer.data()+1);

		if (recvstring == "!dbgmode1")
		{
			SetConsoleTextAttribute(hStdout, FOREGROUND_GREEN | FOREGROUND_INTENSITY);
			std::cout << "\nDebugger is on\n";
			SetConsoleTextAttribute(hStdout, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
			continue;
		}
		switch (recvbuffer[0])
		{
		case '$':
			std::cout << "GET>\n";
			SetConsoleTextAttribute(hStdout, FOREGROUND_GREEN | FOREGROUND_INTENSITY);
			std::cout << recvstring << std::endl;
			SetConsoleTextAttribute(hStdout, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
			break;
		case '^':
			std::cout << "DBGGET>\n";
			SetConsoleTextAttribute(hStdout, FOREGROUND_GREEN | FOREGROUND_INTENSITY);
			std::cout << recvstring << std::endl;
			SetConsoleTextAttribute(hStdout, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
			break;
		case '+':
			std::cout << "DBGREG>\n";
			SetConsoleTextAttribute(hStdout, FOREGROUND_GREEN | FOREGROUND_INTENSITY);
			std::cout << recvstring << std::endl;
			SetConsoleTextAttribute(hStdout, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
			break;
		}
	}

	return 0;
}


int main(void)
{
	const std::wstring dll_path = L"RedViceInternal.dll";
	hStdout = GetStdHandle(STD_OUTPUT_HANDLE);
	WSAData wsaData;
	WSAStartup(MAKEWORD(2, 2), &wsaData);
	std::ios::sync_with_stdio(false);

acquire_connection_and_process:

	SOCKET manager_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	SOCKADDR_IN manager_address = { 0 };
	manager_address.sin_addr.s_addr = inet_addr("127.0.0.1");
	manager_address.sin_port = htons(8888);
	manager_address.sin_family = AF_INET;

	bind(manager_socket, (SOCKADDR*)&manager_address, sizeof(manager_address));

	std::cout << "RVDbg: Process name: ";
	std::wstring generic_name;

	std::getline(std::wcin, generic_name);
	std::uint32_t dbg_pid = getpid_n(generic_name);
	generic_name.clear();

	std::cout << "RVDbg: DLL path (Press enter if there is no special path): ";
	std::getline(std::wcin, generic_name);

	if (generic_name.size() > 0)
	{
		std::wcout << "RVDbg: Loading DLL from: " << generic_name;
		std::cout << "RVDbg: DLL load - status: " << dll_inject(dbg_pid, generic_name);
	}
	else
	{
		std::array<wchar_t, 256> directory;
		GetCurrentDirectoryW(directory.size(), directory.data());
		std::wstring dll_path = std::wstring(directory.data()) + L"\\RedViceInternal.dll";
		std::wcout << "RVDbg: Loading DLL from: " << directory.data() << L"\\RedViceInternal.dll\n";
		dll_inject(dbg_pid, dll_path);
	}

	std::cout << "\nRVDbg: Trying to connect with debugger...\n";
	listen(manager_socket, 1);
	manager_client = accept(manager_socket, NULL, NULL);

	if (manager_client == INVALID_SOCKET)
	{
		goto acquire_connection_and_process;
	}

	std::cout << "RVDbg: Debugger connected\n";

	HANDLE hdbg_handler = CreateThread(0, 0, dbg_handler, 0, 0, 0);

	bool command_processor = true;
	while (command_processor)
	{
		std::string command;
		std::getline(std::cin, command);

		if (command == "!exit")
		{
			send(manager_client, command.c_str(), command.size(), 0);
			command_processor = false;
			break;
		}
		else
		{
			send(manager_client, command.c_str(), command.size(), 0);
		}
	}

	TerminateThread(hdbg_handler, 0);
	CloseHandle(hdbg_handler);
	return 0;
}


