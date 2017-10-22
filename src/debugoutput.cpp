#include "stdafx.h"
#include "debugoutput.h"

void DbgIO::SendDbgRegisters(SOCKET Server, BOOLEAN Protocol, DWORD EIP, Dbg::VirtualRegisters Registers)
{
	char snbuffer[356];
	if (Protocol == 0)
	{
		snprintf(snbuffer, sizeof(snbuffer),
			"+    EAX: %08X\r\n" "    EBX: %08X\r\n"
			"    ECX: %08X\r\n" "    EDX: %08X\r\n"
			"    ESI: %08X\r\n" "    EDI: %08X\r\n"
			"    EBP: %08X\r\n" "    ESP: %08X\r\n"
			"    EIP: %08X\r\n", Registers.eax, Registers.ebx, Registers.ecx, Registers.edx,
			Registers.esi, Registers.edi, Registers.ebp, Registers.esp, EIP);
	}
	else if (Protocol == 1)
	{
		snprintf(snbuffer, sizeof(snbuffer),
			"+    EAX: %08X\r\n" "EBX: %08X\r\n"
			"ECX: %08X\r\n" "EDX: %08X\r\n"
			"ESI: %08X\r\n" "EDI: %08X\r\n"
			"EBP: %08X\r\n" "ESP: %08X\r\n"
			"EIP: %08X\r\n", Registers.eax, Registers.ebx, Registers.ecx, Registers.edx,
			Registers.esi, Registers.edi, Registers.ebp, Registers.esp, EIP);
	}
	else if (Protocol == 2)
	{
		snprintf(snbuffer, sizeof(snbuffer),
			"+    xmm0: %f\r\n" "    xmm1: %f\r\n"
			"    xmm2: %f\r\n" "    xmm3: %f\r\n"
			"    xmm4: %f\r\n" "    xmm5: %f\r\n"
			"    xmm6: %f\r\n" "    xmm7: %f\r\n",
			Registers.xmm0, Registers.xmm1, Registers.xmm2, Registers.xmm3,
			Registers.xmm4, Registers.xmm5, Registers.xmm6, Registers.xmm7);
	}
	else if (Protocol == 3)
	{
		snprintf(snbuffer, sizeof(snbuffer),
			"+    xmm0: %f\r\n" "    xmm1: %f\r\n"
			"    xmm2: %f\r\n" "    xmm3: %f\r\n"
			"    xmm4: %f\r\n" "    xmm5: %f\r\n"
			"    xmm6: %f\r\n" "    xmm7: %f\r\n",
			Registers.dxmm0, Registers.dxmm1, Registers.dxmm2, Registers.dxmm3,
			Registers.dxmm4, Registers.dxmm5, Registers.dxmm6, Registers.dxmm7);
	}
	send(Server, snbuffer, sizeof(snbuffer), 0);
}

void DbgIO::SendDbgGet(SOCKET Server, BOOLEAN ExceptionType, Dispatcher::PoolSect segment)
{
	char snbuffer[512];
	if (ExceptionType != TRUE)
		snprintf(snbuffer, sizeof(snbuffer), "^    Exception Type: IMM\r\n    Symbol: 0x%08X\r\n    Retn: 0x%08X\r\n    Index:%d\r\n", segment.ExceptionAddress, segment.ReturnAddress, segment.Index);
	else
		snprintf(snbuffer, sizeof(snbuffer), "^    Exception Type: PAGE\r\n    Symbol: 0x%08X\r\n    Retn: 0x%08X\r\n    Index:%d\r\n", segment.ExceptionAddress, segment.ReturnAddress, segment.Index);
	send(Server, snbuffer, sizeof(snbuffer), 0);
}
