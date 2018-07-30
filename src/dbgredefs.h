#ifndef DBG_REDEFS
#define DBG_REDEFS
#include <windows.h>
#include <cstdint>

namespace dbg_redef
{
	typedef HANDLE handle;
	const std::uint8_t nullval = 0;
	const std::uint32_t infinite = INFINITE;


	enum class exception_status_code : std::uint32_t
	{
		access_violation = STATUS_ACCESS_VIOLATION,
		breakpoint_exception = STATUS_BREAKPOINT,
		privileged_instruction = STATUS_PRIVILEGED_INSTRUCTION
	};

	enum class page_protection : std::uint32_t
	{
		page_rwx = PAGE_EXECUTE_READWRITE,
		page_ro = PAGE_READONLY,
		page_rw = PAGE_READWRITE,
		page_xr = PAGE_EXECUTE_READ,
		page_xo = PAGE_EXECUTE,
		page_na = PAGE_NOACCESS
	};
}

#endif
