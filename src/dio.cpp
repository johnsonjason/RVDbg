#include "stdafx.h"
#include "dio.h"


/*++

Routine Description:

	Initializes WSA data / Winsock

Parameters:

	None

Return Value:

	None

--*/
void dio::InitializeNetwork()
{
	WSAData WsaData;
	WSAStartup(MAKEWORD(2, 2), &WsaData);
}

/*++

Routine Description:

	Starts the Debug Server, which interfaces with the Debug Client
	The Debug Server is what receives user input while the Debug Client is the actual debugger that manipulates the process

Parameters:

	Ip - The IP address for the debug server, typically loopback
	Port - The port number to use for the debug server

Return Value:

	None

--*/
dio::Server::Server(std::string Ip, std::uint16_t Port)
{
	SOCKADDR_IN SocketAddress = { 0 };
	SocketAddress.sin_addr.s_addr = inet_addr(Ip.c_str());
	SocketAddress.sin_port = htons(Port);
	SocketAddress.sin_family = AF_INET;

	ServerSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	bind(ServerSocket, reinterpret_cast<SOCKADDR*>(&SocketAddress), sizeof(SocketAddress));

	listen(ServerSocket, 1);
	BoundDebugger = accept(ServerSocket, NULL, NULL);
}

dio::Server::~Server()
{
		closesocket(this->ServerSocket);
		closesocket(this->BoundDebugger);
}

/*++
Routine Description:

	Sends a command to the debug client

Parameters:

	ControlCode - A single byte-sized value that represents the type of command operation
	CommandParamOne - Optional command parameter
	CommandParamTwo - Optional command parameter

Return Value:

	None
--*/
void dio::Server::SendCommand(BYTE ControlCode, DWORD_PTR CommandParamOne, DWORD_PTR CommandParamTwo)
{
	//
	// Create a buffer that can store ControlCode, CommandParamOne, and CommandParamTwo
	//

	const size_t ControlSize = sizeof(ControlCode) + sizeof(CommandParamOne) + sizeof(CommandParamTwo);
	BYTE Buffer[ControlSize] = { 0 };

	//
	// Set each value in the buffer, separated and parsed by size
	//

	memset(Buffer, ControlCode, sizeof(ControlCode));
	memcpy(Buffer + sizeof(ControlCode), &CommandParamOne, sizeof(CommandParamOne));
	memcpy(Buffer + sizeof(ControlCode) + sizeof(CommandParamOne), &CommandParamTwo, sizeof(CommandParamTwo));

	//
	// Send the buffer's data to the debugger
	//

	send(BoundDebugger, reinterpret_cast<char*>(Buffer), sizeof(Buffer), 0);
}

/*++
Routine Description:

	Receives a command from a debug client

Parameters:

	None

Return Value:

	None
--*/
std::tuple<BYTE, DWORD_PTR, DWORD_PTR> dio::Server::ReceiveCommand()
{
	BYTE ControlCode = NULL;
	DWORD_PTR CommandParamOne = NULL;
	DWORD_PTR CommandParamTwo = NULL;

	const size_t ControlSize = sizeof(BYTE) + sizeof(DWORD_PTR) + sizeof(DWORD_PTR);
	BYTE Buffer[ControlSize] = { 0 };

	recv(BoundDebugger, reinterpret_cast<char*>(Buffer), sizeof(Buffer), 0);

	//
	// Parse the buffer into each individual command attribute
	// Convert each individual command attribute into a tuple
	//

	memcpy(&ControlCode, Buffer, sizeof(ControlCode));
	memcpy(&CommandParamOne, Buffer + sizeof(ControlCode), sizeof(CommandParamOne));
	memcpy(&CommandParamTwo, Buffer + sizeof(ControlCode) + sizeof(CommandParamOne), sizeof(CommandParamTwo));

	std::tuple<BYTE, DWORD_PTR, DWORD_PTR> Command(ControlCode, CommandParamOne, CommandParamTwo);

	//
	// If the control code is greater than 16...TBA
	//

	if (std::get<0>(Command) > 16)
	{

	}

	return Command;
}

std::string dio::Server::ReceiveString()
{
	char Buffer[2048] = { 0 };
	recv(BoundDebugger, Buffer, sizeof(Buffer), 0);
	return std::string(Buffer);
}

dio::Client::Client(std::string Ip, std::uint16_t Port)
{
	SOCKADDR_IN SocketAddress = { 0 };
	SocketAddress.sin_addr.s_addr = inet_addr(Ip.c_str());
	SocketAddress.sin_port = htons(Port);
	SocketAddress.sin_family = AF_INET;

	ClientSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	connect(ClientSocket, reinterpret_cast<SOCKADDR*>(&SocketAddress), sizeof(SocketAddress));
}



/*++
Routine Description:

	Sends a command to the debug server

Parameters:

	ControlCode - A single byte-sized value that represents the type of command operation
	CommandParamOne - Optional command parameter
	CommandParamTwo - Optional command parameter

Return Value:

	None
--*/
void dio::Client::SendCommand(BYTE ControlCode, DWORD_PTR CommandParamOne, DWORD_PTR CommandParamTwo)
{
	//
	// Create a buffer that can store ControlCode, CommandParamOne, and CommandParamTwo
	//

	const size_t ControlSize = sizeof(ControlCode) + sizeof(CommandParamOne) + sizeof(CommandParamTwo);
	BYTE Buffer[ControlSize] = { 0 };

	//
	// Set each value in the buffer, separated and parsed by size
	//

	memset(Buffer, ControlCode, sizeof(ControlCode));
	memset(Buffer + sizeof(ControlCode), CommandParamOne, sizeof(CommandParamOne));
	memset(Buffer + sizeof(ControlCode) + sizeof(CommandParamOne), CommandParamTwo, sizeof(CommandParamTwo));

	//
	// Send the buffer's data to the debugger
	//

	send(ClientSocket, reinterpret_cast<char*>(Buffer), sizeof(Buffer), 0);
}



/*++
Routine Description:

	Receives a command from a debug server

Parameters:

	None

Return Value:

	None
--*/
std::tuple<BYTE, DWORD_PTR, DWORD_PTR> dio::Client::ReceiveCommand()
{
	BYTE ControlCode = NULL;
	DWORD_PTR CommandParamOne = NULL;
	DWORD_PTR CommandParamTwo = NULL;

	const size_t ControlSize = sizeof(BYTE) + sizeof(DWORD_PTR) + sizeof(DWORD_PTR);
	BYTE Buffer[ControlSize] = { 0 };

	//
	// Parse the buffer into each individual command attribute
	// Convert each individual command attribute into a tuple
	//

	recv(ClientSocket, reinterpret_cast<char*>(Buffer), sizeof(Buffer), 0);
	memcpy(&ControlCode, Buffer, sizeof(ControlCode));
	memcpy(&CommandParamOne, Buffer + sizeof(ControlCode), sizeof(CommandParamOne));
	memcpy(&CommandParamTwo, Buffer + sizeof(ControlCode) + sizeof(CommandParamOne), sizeof(CommandParamTwo));

	std::tuple<BYTE, DWORD_PTR, DWORD_PTR> Command(ControlCode, CommandParamOne, CommandParamTwo);

	return Command;
}

void dio::Client::SendString(std::string Data)
{
	send(ClientSocket, Data.c_str(), Data.size(), 0);
}
