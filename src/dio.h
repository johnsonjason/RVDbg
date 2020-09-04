#pragma once
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <tuple>
#include <string>
#include <WinSock2.h>
#pragma comment(lib, "ws2_32.lib")

#define CTL_SET_PTR 0 // Set the current address of debug exception
#define CTL_GET_PTR 1 // Get the current address of debug exception
#define CTL_SET_REG 2 // Set an immediate register value
#define CTL_GET_REG 3 // Get an immediate register value
#define CTL_SET_XMM 4 // Set an XMM register
#define CTL_GET_XMM 5 // Get an XMM register
#define CTL_SET_BPT 6 // Register the address of debug exception as a breakpoint with an exception condition
#define CTL_DO_RUN 7 // Continue from debug state
#define CTL_DO_STEP 8 // Step over
#define CTL_STR_OUT 16 // Signal for an incoming string
#define CTL_ERROR_CON 255 // Error with connection

namespace dio
{

	void InitializeNetwork();

	class Server
	{
	public:
		Server(std::string Ip, std::uint16_t Port);
		~Server();
		void SendCommand(BYTE ControlCode, DWORD_PTR CommandParamOne, DWORD_PTR CommandParamTwo);
		std::tuple<BYTE, DWORD_PTR, DWORD_PTR> ReceiveCommand();

		std::string ReceiveString();
	private:
		SOCKET ServerSocket;
		SOCKET BoundDebugger;
	};

	class Client
	{
	public:
		Client(std::string Ip, std::uint16_t Port);
		void SendCommand(BYTE ControlCode, DWORD_PTR CommandParamOne, DWORD_PTR CommandParamTwo);
		std::tuple<BYTE, DWORD_PTR, DWORD_PTR> ReceiveCommand();

		void SendString(std::string Data);
	private:
		SOCKET ClientSocket;
	};
}
