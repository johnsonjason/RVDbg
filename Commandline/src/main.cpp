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
		std::string entrystring(recvbuffer.data());

		if (entrystring == "!dbgmode1")
		{
			SetConsoleTextAttribute(hStdout, FOREGROUND_RED | FOREGROUND_INTENSITY);
			std::cout << "Debugger is on\n\n";
			SetConsoleTextAttribute(hStdout, FOREGROUND_GREEN | FOREGROUND_INTENSITY);
			continue;
		}

		switch (recvbuffer[0])
		{
		case '$':
			std::cout << "\nGET>\n";
			SetConsoleTextAttribute(hStdout, FOREGROUND_RED | FOREGROUND_INTENSITY);
			std::cout << recvstring << std::endl;
			SetConsoleTextAttribute(hStdout, FOREGROUND_GREEN | FOREGROUND_INTENSITY);
			break;
		case '^':
			std::cout << "\nDBGGET>\n";
			SetConsoleTextAttribute(hStdout, FOREGROUND_RED | FOREGROUND_INTENSITY);
			std::cout << recvstring << std::endl;
			SetConsoleTextAttribute(hStdout, FOREGROUND_GREEN | FOREGROUND_INTENSITY);
			break;
		case '+':
			std::cout << "\nDBGREG>\n";
			SetConsoleTextAttribute(hStdout, FOREGROUND_RED| FOREGROUND_INTENSITY);
			std::cout << recvstring << std::endl;
			SetConsoleTextAttribute(hStdout, FOREGROUND_GREEN | FOREGROUND_INTENSITY);
			break;
		}
	}

	return 0;
}

void set_console_attributes()
{
	RECT console_rect;
	HWND console_handle = GetConsoleWindow();
	hStdout = GetStdHandle(STD_OUTPUT_HANDLE);

	SetWindowLongW(console_handle, GWL_EXSTYLE, GetWindowLongW(console_handle, GWL_EXSTYLE) | WS_EX_LAYERED);
	SetLayeredWindowAttributes(console_handle, 0, 200, LWA_ALPHA);
	SetConsoleTextAttribute(hStdout, FOREGROUND_GREEN | FOREGROUND_INTENSITY);

	GetWindowRect(console_handle, &console_rect);
	MoveWindow(console_handle, console_rect.left, console_rect.top, 1000, 800, static_cast<BOOL>(true));
	std::ios::sync_with_stdio(false);
}

void handle_debug_attach(std::wstring& generic_name, std::uint32_t dbg_pid)
{
	if (generic_name.size() > 0)
	{
		std::wcout << "\nRVDbg - Loading DLL from: " << generic_name;
		dll_inject(dbg_pid, generic_name);
	}
	else
	{
		std::array<wchar_t, 256> directory;

		GetCurrentDirectoryW(directory.size(), directory.data());

		std::wstring dll_path = std::wstring(directory.data()) + L"\\RedViceInternal.dll";
		std::wcout << "RVDbg - Loading DLL from: " << directory.data() << L"\\RedViceInternal.dll\n";

		dll_inject(dbg_pid, dll_path);
	}
}

void inline set_server_socket(SOCKET& _socket, SOCKADDR_IN& address_struct, std::string& address, std::uint16_t port)
{
	_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	address_struct.sin_addr.s_addr = inet_addr(address.c_str());
	address_struct.sin_port = htons(port);
	address_struct.sin_family = AF_INET;
	bind(_socket, reinterpret_cast<SOCKADDR*>(&address_struct), sizeof(address_struct));
}

void inline init_winsock()
{
	WSAData wsaData;
	WSAStartup(MAKEWORD(2, 2), &wsaData);
}

int main(void)
{
	const std::wstring dll_path = L"RedViceInternal.dll";
	set_console_attributes();

acquire_connection_and_process:

	SOCKET manager_socket;
	SOCKADDR_IN manager_address = { 0 };
	init_winsock();
	set_server_socket(manager_socket, manager_address, std::string("127.0.0.1"), 8888);

	std::cout << "RVDbg - Process name: ";
	std::wstring generic_name;

	std::getline(std::wcin, generic_name);
	std::uint32_t dbg_pid = getpid_n(generic_name);

	generic_name.clear();

	std::cout << "\nRVDbg - DLL path (Press enter if there is no special path): ";
	std::getline(std::wcin, generic_name);
	std::cout << "\nRVDbg - Trying to connect with debugger...\n";
	handle_debug_attach(generic_name, dbg_pid);

	listen(manager_socket, 1);
	manager_client = accept(manager_socket, NULL, NULL);

	if (manager_client == INVALID_SOCKET)
	{
		std::cout << WSAGetLastError() << std::endl;
		closesocket(manager_socket);
		goto acquire_connection_and_process;
	}

	std::cout << "\nRVDbg - Debugger connected\n\n";

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



