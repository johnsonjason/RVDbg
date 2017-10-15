#ifndef DEBUGOUTPUT
#define DEBUGOUTPUT
#include <windows.h>
#include <winsock.h>
#include <stdio.h>
#include "Dispatcher/exceptiondispatcher.h"

void SendDbgRegisters(SOCKET Server, BOOLEAN Protocol, DWORD EIP, VirtualRegisters Registers);
void SendDbgGet(SOCKET Server, BOOLEAN ExceptionType, PoolSect segment);

#endif
