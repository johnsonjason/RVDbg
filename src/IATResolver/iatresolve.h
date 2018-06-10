// by jasonfish4
#ifndef IATRESOLVE
#define IATRESOLVE
#include <windows.h>
#include <cstdint>
#include "..\dbgredefs.h"

namespace iat_resolution
{
	struct imp_at
	{
		std::uint32_t size;
		void* address;
	};

	imp_at get_iat(LPCSTR module_name);

	std::uint32_t calc_virtual_page_count(imp_at iat);

	void resolve_iat(LPCSTR first_module, LPCSTR copy_module);
}
#endif
