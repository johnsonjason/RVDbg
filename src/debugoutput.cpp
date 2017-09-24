#include "debugoutput.h"

void SendDbgRegisters(SOCKET Server, BOOLEAN ProtocolGUI, DWORD EIP, VirtualRegisters Registers)
{
	char snbuffer[356];
	if (ProtocolGUI == FALSE)
	{
		snprintf(snbuffer, sizeof(snbuffer),
			"+    EAX: %08X\r\n" "    EBX: %08X\r\n"
			"    ECX: %08X\r\n" "    EDX: %08X\r\n"
			"    ESI: %08X\r\n" "    EDI: %08X\r\n"
			"    EBP: %08X\r\n" "    ESP: %08X\r\n"
			"    EIP: %08X\r\n", Registers.eax, Registers.ebx, Registers.ecx, Registers.edx,
			Registers.esi, Registers.edi, Registers.ebp, Registers.esp, EIP);
	}
	else
	{
		snprintf(snbuffer, sizeof(snbuffer),
			"+    EAX: %08X\r\n" "EBX: %08X\r\n"
			"ECX: %08X\r\n" "EDX: %08X\r\n"
			"ESI: %08X\r\n" "EDI: %08X\r\n"
			"EBP: %08X\r\n" "ESP: %08X\r\n"
			"EIP: %08X\r\n", Registers.eax, Registers.ebx, Registers.ecx, Registers.edx,
			Registers.esi, Registers.edi, Registers.ebp, Registers.esp, EIP);
	}
	send(Server, snbuffer, sizeof(snbuffer), 0);
}

void SendDbgGet(SOCKET Server, BOOLEAN ExceptionType, PoolSect segment)
{
	char snbuffer[512];
	if (ExceptionType != TRUE)
		snprintf(snbuffer, sizeof(snbuffer), "^    Exception Type: IMM\r\nSymbol: 0x%08X\r\nRetn: 0x%08X\r\n", segment.ExceptionAddress, segment.ReturnAddress);
	else
		snprintf(snbuffer, sizeof(snbuffer), "^    Exception Type: PAGE\r\nSymbol: 0x%08X\r\nRetn: 0x%08X\r\n", segment.ExceptionAddress, segment.ReturnAddress);
	send(Server, snbuffer, sizeof(snbuffer), 0);
}
