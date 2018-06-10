// Programmed by jasonfish4
/* The excessive use of global variables is due to me not wanting to consume stack space, overuse registers, etc...
within an exception signaling function */

#define _CRT_SECURE_NO_DEPRECATE

#include "stdafx.h"
#include "Dispatcher\rvdbg.h"
#include "Dispatcher\exceptiondispatcher.h"
#include "dbgredefs.h"
#include "debugoutput.h"
#include <winsock.h>
#include <cstdlib>
#include <string>
#include <unordered_map>
#include <array>
#include <iostream>
#include <sstream>

#pragma comment(lib, "wsock32.lib")
typedef void(*str_comparator_function)();
void protocol_gui_func();
void str_breakpoint();
void str_imode();
void str_pmode();
void str_dbg_get();
void str_dbg_display_regs();
void str_xmmf_display();
void str_xmmd_display();
void str_dbg_run();


const char* g_module_name; // Name of the image
const char* g_ext_module_name; // Name of the copy image
const char* g_file_path; // File location to copy image
std::uint32_t g_server;
std::uint8_t g_protocol_interface;
std::uint32_t symbol;
sockaddr_in g_server_address;
std::array<void*, 2> g_threads;

const std::unordered_map<char, rvdbg::sse_register> xmm_register_map = {
	{ '0', rvdbg::sse_register::xmm0 },
	{ '1', rvdbg::sse_register::xmm1 },
	{ '2', rvdbg::sse_register::xmm2 },
	{ '3', rvdbg::sse_register::xmm3 },
	{ '4', rvdbg::sse_register::xmm4 },
	{ '5', rvdbg::sse_register::xmm5 },
	{ '6', rvdbg::sse_register::xmm6 },
	{ '7', rvdbg::sse_register::xmm7 }
};

const std::unordered_map<char, rvdbg::gp_reg_32> general_register_map = {
	{ '0', rvdbg::gp_reg_32::eax },
	{ '1', rvdbg::gp_reg_32::ebx },
	{ '2', rvdbg::gp_reg_32::ecx },
	{ '3', rvdbg::gp_reg_32::edx },
	{ '4', rvdbg::gp_reg_32::edi },
	{ '5', rvdbg::gp_reg_32::esi },
	{ '6', rvdbg::gp_reg_32::ebp },
	{ '7', rvdbg::gp_reg_32::esp },
	{ '8', rvdbg::gp_reg_32::eip }
};

const std::unordered_map<std::string, str_comparator_function> opt_func_table = {
	{ std::string("**ProtocolGUI"), &protocol_gui_func },
	{ std::string("!Breakpoint"), &str_breakpoint },
	{ std::string("imode"), &str_imode },
	{ std::string("pmode"), &str_pmode },
	{ std::string("!DbgGet"), &str_dbg_get },
	{ std::string("!DbgDisplayRegisters"), &str_dbg_display_regs },
	{ std::string("!xmm-f"), &str_xmmf_display },
	{ std::string("!xmm-d"), &str_xmmd_display },
	{ std::string("!DbgRun"), &str_dbg_run },
};

void register_value(char key, std::uint32_t value)
{
	rvdbg::set_register(static_cast<std::uint32_t>(general_register_map.at(key)), value);
}

void register_value_fp(char key, bool precision, double value)
{
	rvdbg::set_register_fp(static_cast<std::uint32_t>(xmm_register_map.at(key)), precision, value);
}

void protocol_gui_func()
{
	g_protocol_interface = 1;
}

void str_imode()
{
	rvdbg::set_exception_mode(dispatcher::exception_type::immediate_exception);
}

void str_pmode()
{
	rvdbg::set_exception_mode(dispatcher::exception_type::page_exception);
}

void str_dbg_get()
{
	dbg_io::send_dbg_get(g_server, rvdbg::get_exception_mode(), rvdbg::get_current_section());
}


void str_dbg_display_regs()
{
	dbg_io::send_dbg_registers(g_server, g_protocol_interface, rvdbg::get_dbg_exception_address(), rvdbg::get_registers());
}

void str_xmmf_display()
{
	dbg_io::send_dbg_registers(g_server, 2, rvdbg::get_dbg_exception_address(), rvdbg::get_registers());
}

void str_xmmd_display()
{
	dbg_io::send_dbg_registers(g_server, 3, rvdbg::get_dbg_exception_address(), rvdbg::get_registers());
}

void str_dbg_run()
{
	if (rvdbg::is_aeh_present())
	{
		rvdbg::continue_debugger();
	}
}

void str_breakpoint()
{
	std::size_t exception_element = dispatcher::check_sector(rvdbg::get_sector());
	MEMORY_BASIC_INFORMATION mbi;
	if (VirtualQuery(reinterpret_cast<void*>(symbol), &mbi, sizeof(mbi)) != dbg_redef::nullval)
	{
		if (mbi.Protect != static_cast<unsigned long>(dbg_redef::page_protection::page_na))
			dispatcher::add_exception(rvdbg::get_sector(), exception_element, rvdbg::get_exception_mode(), symbol);
	}
}

void str_get(std::array<char, 128>& snbuffer, std::size_t index)
{
	if (index < rvdbg::get_sector_size())
	{
		std::snprintf(snbuffer.data(), snbuffer.size(), "$    symbol: 0x%02X\r\n    index:%d\r\n", symbol, index);
		send(g_server, snbuffer.data(), snbuffer.size(), 0);
	}
	else
	{
		send(g_server, "$    symbol not registered", sizeof("$    symbol not registered"), 0);
	}
}

void dbg_undo(std::size_t index)
{
	if (index < rvdbg::get_sector_size())
	{
		std::uint32_t old_protect;
		switch (rvdbg::get_exception_mode())
		{
		case dispatcher::exception_type::immediate_exception:
			VirtualProtect(reinterpret_cast<void*>(rvdbg::get_sector()[index].dbg_exception_address),
				1, static_cast<unsigned long>(dbg_redef::page_protection::page_xrw), reinterpret_cast<unsigned long*>(&old_protect));

			*reinterpret_cast<std::uint32_t*>(rvdbg::get_sector()[index].dbg_exception_address) = (rvdbg::get_sector()[index].save_code);

			VirtualProtect(reinterpret_cast<void*>(rvdbg::get_sector()[index].dbg_exception_address), 1, 
				old_protect, reinterpret_cast<unsigned long*>(&old_protect));

			dispatcher::unlock_sector(rvdbg::get_sector(), index);
			break;

		case dispatcher::exception_type::access_exception:
			/* UNIMPLEMENTED */
		case dispatcher::exception_type::page_exception:
			VirtualProtect(reinterpret_cast<void*>(rvdbg::get_sector()[index].dbg_exception_address), 1, 
				rvdbg::get_sector()[index].save_code, reinterpret_cast<unsigned long*>(&old_protect));

			dispatcher::unlock_sector(rvdbg::get_sector(), index);
			break;
		}
	}
}

unsigned long __stdcall dbg_synchronization(void* lpParam)
{
	while (true)
	{
		if (rvdbg::debugger == false)
		{
			EnterCriticalSection(&rvdbg::repr);
			SleepConditionVariableCS(&rvdbg::reprcondition, &rvdbg::repr, dbg_redef::infinite);
			LeaveCriticalSection(&rvdbg::repr);
		}
		if (rvdbg::debugger == true)
		{
			send(g_server, "!DbgModeOn", strlen("!DbgModeOn"), 0);
			rvdbg::debugger = false;
		}
	}
}

unsigned long __stdcall dispatch_initializer(void* lpParam)
{	
	WSAData wsa_data;
	WSAStartup(MAKEWORD(2, 2), &wsa_data);

	g_server = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	g_server_address = { 0 };
	g_server_address.sin_addr.s_addr = inet_addr("127.0.0.1");
	g_server_address.sin_port = htons(8888);
	g_server_address.sin_family = AF_INET;
	connect(g_server, reinterpret_cast<sockaddr*>(&g_server_address), sizeof(g_server_address));

	rvdbg::attach_debugger();
	rvdbg::set_module(false, g_module_name, g_ext_module_name);
	rvdbg::set_pause_mode(rvdbg::suspension_mode::suspend_all);
	rvdbg::assign_thread(g_threads[0]);
	rvdbg::assign_thread(g_threads[1]);

	std::array<char, 128> buffer;
	std::array<char, 128> snbuffer;
	std::istringstream dbg_conversion_stream;

	while (true)
	{
		buffer.fill('\0');
		snbuffer.fill('\0');
		recv(g_server, buffer.data(), buffer.size(), 0);

		std::string receiver;
		receiver = std::string(buffer.data());
		
		try
		{
			str_comparator_function local_func = opt_func_table.at(receiver);
			local_func();
		}
		catch (std::out_of_range& e)
		{
			std::cerr << "end of map reached, string being compared is used for parsing\n";

			if (receiver.substr(0, std::string("!Symbol @").size()) == std::string("!Symbol @"))
			{
				if (!receiver.substr(sizeof("!Symbol @ "), receiver.size()).empty())
				{
					std::sscanf(receiver.substr(sizeof("!Symbol @ "), receiver.size()).c_str(), "%X", &symbol);
				}
			}

			else if (receiver.substr(0, std::string("!setreg").size()) == std::string("!setreg"))
			{
				std::uint32_t regv;
				dbg_conversion_stream.str(receiver.substr(std::string("!setreg ? ").size(), receiver.size()));
				dbg_conversion_stream >> std::hex >> regv;
				register_value(receiver.at(std::string("!setreg ").size()), regv);
			}

			else if (receiver.substr(0, std::string("!fsetreg").size()) == std::string("!fsetreg"))
			{
				double regv;
				dbg_conversion_stream.str(receiver.substr(std::string("!fsetreg ? ").size(), receiver.size()));
				dbg_conversion_stream >> regv;
				register_value_fp(receiver.at(std::string("!fsetreg ").size()), 0, regv);
			}

			else if (receiver.substr(0, std::string("!dsetreg").size()) == std::string("!dsetreg"))
			{
				double regv;
				dbg_conversion_stream.str((receiver.substr(std::string("!dsetreg ? ").size(), receiver.size())));
				dbg_conversion_stream >> regv;
				register_value_fp(receiver.at(std::string("!dsetreg ").size()), 1, regv);
			}

			else if (receiver.substr(0, std::string("!Undo ").size()) == std::string("!Undo "))
			{
				std::size_t index;
				dbg_conversion_stream.str(receiver.substr(std::string("!Undo ").size(), receiver.size()));
				dbg_conversion_stream >> index;
				dbg_undo(index);
			}

			else if (receiver == std::string("!Get"))
			{
				std::size_t index = dispatcher::search_sector(rvdbg::get_sector(), symbol);
				str_get(snbuffer, index);
			}

			else if (receiver == std::string("!Exit"))
			{
				str_dbg_run();
				break;
			}
		}
	}

	rvdbg::detach_debugger();
	rvdbg::remove_thread(g_threads[0]);
	rvdbg::remove_thread(g_threads[1]);
	closesocket(g_server);
	WSACleanup();
	TerminateThread(g_threads[0], 0);
	return 0;
}

BOOL APIENTRY DllMain(HMODULE hModule, std::uint32_t  ul_reason_for_call, void* lpReserved)
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		use_module = false;
		if (use_module)
			dll_inject(GetCurrentProcessId(), g_file_path);

		AllocConsole();
		freopen("CONIN$", "r", stdin);
		freopen("CONOUT$", "w", stdout);
		freopen("CONOUT$", "w", stderr);
		InitializeCriticalSection(&rvdbg::repr);
		InitializeConditionVariable(&rvdbg::reprcondition);
		g_threads[0] = CreateThread(0, 0, dbg_synchronization, 0, 0, 0);
		g_threads[1] = CreateThread(0, 0, dispatch_initializer, 0, 0, 0);
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
	case DLL_PROCESS_DETACH:
		break;
	}
	return TRUE;
}


