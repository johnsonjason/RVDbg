#include "injector.h"
#include <winsock.h>
#include <iostream>
#pragma comment(lib, "wsock32.lib")

HANDLE hStdout;
SOCKET ManagerClient;

void Welcome()
{
	hStdout = GetStdHandle(STD_OUTPUT_HANDLE);

	SetConsoleTextAttribute(hStdout, FOREGROUND_RED | FOREGROUND_INTENSITY);

	std::cout << R"(                                                                                                                     
  ____          _  __     ___            ____       _                                 
 |  _ \ ___  __| | \ \   / (_) ___ ___  |  _ \  ___| |__  _   _  __ _  __ _  ___ _ __ 
 | |_) / _ \/ _` |  \ \ / /| |/ __/ _ \ | | | |/ _ \ '_ \| | | |/ _` |/ _` |/ _ \ '__|
 |  _ <  __/ (_| |   \ V / | | (_|  __/ | |_| |  __/ |_) | |_| | (_| | (_| |  __/ |   
 |_| \_\___|\__,_|    \_/  |_|\___\___| |____/ \___|_.__/ \__,_|\__, |\__, |\___|_|   
                                                                |___/ |___/          
			  )" << std::endl;

	SetConsoleTextAttribute(hStdout, FOREGROUND_BLUE | FOREGROUND_INTENSITY);
	std::cout << R"(
  ____               _                        __ _     _    _  _   
 |  _ \             | |                      / _(_)   | |  | || |  
 | |_) |_   _       | | __ _ ___  ___  _ __ | |_ _ ___| |__| || |_ 
 |  _ <| | | |  _   | |/ _` / __|/ _ \| '_ \|  _| / __| '_ \__   _|
 | |_) | |_| | | |__| | (_| \__ \ (_) | | | | | | \__ \ | | | | |  
 |____/ \__, |  \____/ \__,_|___/\___/|_| |_|_| |_|___/_| |_| |_|  
         __/ |                                                     
        |___/            	 )" << std::endl;

	SetConsoleTextAttribute(hStdout, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
}

DWORD WINAPI Listener(LPVOID lpParam)
{
#define NEWLINE puts("");
	char recvbuffer[356];
	while (true)
	{
		memset(recvbuffer, 0, sizeof(recvbuffer));
		recv(ManagerClient, recvbuffer, sizeof(recvbuffer), 0);
		if (std::string(recvbuffer) == std::string("!DbgModeOn"))
		{
			SetConsoleTextAttribute(hStdout, FOREGROUND_GREEN | FOREGROUND_INTENSITY);
			puts("    Debugger is on");
			SetConsoleTextAttribute(hStdout, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
		}
		else if (recvbuffer[0] == '$')
		{
			puts("GET>");
			SetConsoleTextAttribute(hStdout, FOREGROUND_GREEN | FOREGROUND_INTENSITY);
			puts(recvbuffer + 1);
			SetConsoleTextAttribute(hStdout, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
		}
		else if (recvbuffer[0] == '^')
		{
			puts("DBGGET>");
			SetConsoleTextAttribute(hStdout, FOREGROUND_GREEN | FOREGROUND_INTENSITY);
			puts(recvbuffer + 1);
			SetConsoleTextAttribute(hStdout, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
		}
		else if (recvbuffer[0] == '+')
		{
			puts("DBGREG>");
			SetConsoleTextAttribute(hStdout, FOREGROUND_GREEN | FOREGROUND_INTENSITY);
			puts(recvbuffer + 1);
			SetConsoleTextAttribute(hStdout, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
		}
	}

	return 0;
}


int main(void)
{
	Welcome();
	WSAData wsaData;
	WSAStartup(MAKEWORD(2, 2), &wsaData);
	
	ManagerClient = NULL;

	SOCKET ManagerSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	SOCKADDR_IN ManagerAddress = { 0 };
	ManagerAddress.sin_addr.s_addr = inet_addr("127.0.0.1");
	ManagerAddress.sin_port = htons(8888);
	ManagerAddress.sin_family = AF_INET;

	bind(ManagerSocket, (SOCKADDR*)&ManagerAddress, sizeof(ManagerAddress));
	puts("\n\n--\n\n");
	std::wstring Processname;
	std::cout << "Process name: ";
	std::getline(std::wcin, Processname);
	DWORD ProcessId = FindProcessIdFromProcessName(Processname);
	DLLInject(ProcessId, "Filepath");
	listen(ManagerSocket, 1);
	ManagerClient = accept(ManagerSocket, NULL, NULL);

	if (ManagerClient != NULL)
	{
		puts("\nConnection established with debugger\n");
		HANDLE ListenThread = CreateThread(0, 0, Listener, 0, 0, 0);

		BOOLEAN Commandline = TRUE;
		std::string receiver;
		DWORD Address = NULL;

		char recvbuffer[128];

		while (Commandline)
		{
			receiver.clear();
			memset(recvbuffer, 0, sizeof(recvbuffer));
			std::getline(std::cin, receiver);

			if (receiver.substr(0, std::string("!Symbol @").length()) == std::string("!Symbol @"))
			{
				if (!receiver.substr(std::string("!Symbol @").length(), receiver.size()).empty())
				{
					std::string rcvr = receiver.substr(std::string("!Symbol @ ").length(), receiver.length());
					if (rcvr.length() == 8 && receiver.at(std::string("!Symbol @ ").length()) != (char)"0")
						receiver.insert(std::string("!Symbol @ ").length(), "0");
					send(ManagerClient, receiver.c_str(), receiver.size(), 0);
				}
			}

			else if (receiver == std::string("!Breakpoint"))
				send(ManagerClient, receiver.c_str(), receiver.size(), 0);
			else if (receiver.substr(0, std::string("!Undo ").length()) == std::string("!Undo "))
				send(ManagerClient, receiver.c_str(), receiver.size(), 0);
			else if (receiver == std::string("!Get"))
				send(ManagerClient, receiver.c_str(), receiver.size(), 0);

			else if (receiver == std::string("!DbgGet"))
				send(ManagerClient, receiver.c_str(), receiver.size(), 0);

			else if (receiver == std::string("!DbgDisplayRegisters"))
				send(ManagerClient, receiver.c_str(), receiver.size(), 0);
			else if (receiver == std::string("!xmm-f"))
				send(ManagerClient, receiver.c_str(), receiver.size(), 0);
			else if (receiver == std::string("!xmm-d"))
				send(ManagerClient, receiver.c_str(), receiver.size(), 0);
			else if (receiver == std::string("!DbgRun"))
				send(ManagerClient, receiver.c_str(), receiver.size(), 0);

			else if (receiver.substr(0, std::string("!setreg").length()) == std::string("!setreg"))
			{
				if (!receiver.substr(std::string("!setreg ?").length(), receiver.size()).empty())
					send(ManagerClient, receiver.c_str(), receiver.size(), 0);
			}
			else if (receiver.substr(0, std::string("!fsetreg").length()) == std::string("!fsetreg"))
			{
				if (!receiver.substr(std::string("!fsetreg ?").length(), receiver.size()).empty())
					send(ManagerClient, receiver.c_str(), receiver.size(), 0);
			}
			else if (receiver.substr(0, std::string("!dsetreg").length()) == std::string("!dsetreg"))
			{
				if (!receiver.substr(std::string("!dsetreg ?").length(), receiver.size()).empty())
					send(ManagerClient, receiver.c_str(), receiver.size(), 0);
			}
			else if (receiver == std::string("!Exit"))
			{
				send(ManagerClient, receiver.c_str(), receiver.size(), 0);
				Commandline = FALSE;
				break;
			}

		}
		TerminateThread(ListenThread, 0);
		CloseHandle(ListenThread);
	}
    return 0;
}

