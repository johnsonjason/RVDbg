#include "stdafx.h"
#include "debugoutput.h"

void dbg_io::send_dbg_registers(SOCKET server, std::uint8_t protocol, std::uint32_t eip, rvdbg::virtual_registers& registers)
{
	char snbuffer[356];
	if (protocol == 0 || protocol == 1)
	{
		std::snprintf(snbuffer, sizeof(snbuffer),
			"+eax: %08X\r\n" "ebx: %08X\r\n"
			"ecx: %08X\r\n" "edx: %08X\r\n"
			"esi: %08X\r\n" "edi: %08X\r\n"
			"ebp: %08X\r\n" "esp: %08X\r\n"
			"eip: %08X\r\n", registers.eax, registers.ebx, registers.ecx, registers.edx,
			registers.esi, registers.edi, registers.ebp, registers.esp, eip);
	}
	else if (protocol == 2)
	{
		std::snprintf(snbuffer, sizeof(snbuffer),
			"+xmm0: %f\r\n" "xmm1: %f\r\n"
			"xmm2: %f\r\n" "xmm3: %f\r\n"
			"xmm4: %f\r\n" "xmm5: %f\r\n"
			"xmm6: %f\r\n" "xmm7: %f\r\n",
			registers.xmm0, registers.xmm1, registers.xmm2, registers.xmm3,
			registers.xmm4, registers.xmm5, registers.xmm6, registers.xmm7);
	}
	else if (protocol == 3)
	{
		std::snprintf(snbuffer, sizeof(snbuffer),
			"+xmm0: %f\r\n" "xmm1: %f\r\n"
			"xmm2: %f\r\n" "xmm3: %f\r\n"
			"xmm4: %f\r\n" "xmm5: %f\r\n"
			"xmm6: %f\r\n" "xmm7: %f\r\n",
			registers.dxmm0, registers.dxmm1, registers.dxmm2, registers.dxmm3,
			registers.dxmm4, registers.dxmm5, registers.dxmm6, registers.dxmm7);
	}
	send(server, snbuffer, sizeof(snbuffer), 0);
}

void dbg_io::send_dbg_get(SOCKET server, dispatcher::exception_type dbg_exception_type, dispatcher::pool_sect& segment)
{
	char snbuffer[512];
	if (dbg_exception_type != dispatcher::exception_type::page_exception)
		std::snprintf(snbuffer, sizeof(snbuffer), "^    Exception Type: IMM\r\n    Symbol: 0x%08X\r\n    Retn: 0x%08X\r\n    index:%d\r\n", segment.dbg_exception_address, segment.return_address, segment.index);
	else
		std::snprintf(snbuffer, sizeof(snbuffer), "^    Exception Type: PAGE\r\n    Symbol: 0x%08X\r\n    Retn: 0x%08X\r\n    index:%d\r\n", segment.dbg_exception_address, segment.return_address, segment.index);
	send(server, snbuffer, sizeof(snbuffer), 0);
}
