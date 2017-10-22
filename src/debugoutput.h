#ifndef DEBUGOUTPUT
#define DEBUGOUTPUT
#include <windows.h>
#include <winsock.h>
#include <stdio.h>
#include "Dispatcher/exceptiondispatcher.h"
#include "Dispatcher/rvdbg.h"

namespace DbgIO
{
	void SendDbgRegisters(SOCKET Server, BOOLEAN Protocol, DWORD EIP, Dbg::VirtualRegisters Registers);
	void SendDbgGet(SOCKET Server, BOOLEAN ExceptionType, Dispatcher::PoolSect segment);
};

#endif
