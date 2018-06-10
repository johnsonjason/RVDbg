#ifndef DEBUGOUTPUT
#define DEBUGOUTPUT
#include <windows.h>
#include <winsock.h>
#include <stdio.h>
#include "Dispatcher/exceptiondispatcher.h"
#include "Dispatcher/rvdbg.h"

namespace dbg_io
{
	void send_dbg_registers(SOCKET server, std::uint8_t protocol, std::uint32_t eip, rvdbg::virtual_registers registers);
	void send_dbg_get(SOCKET server, dispatcher::exception_type dbg_exception_type, dispatcher::pool_sect segment);
};

#endif
