// by jasonfish4
#ifndef IATRESOLVE
#define IATRESOLVE
#include <windows.h>

namespace IATResolver
{
	struct IMP_AT
	{
		DWORD Size;
		PVOID Address;
	};

	IMP_AT GetIAT(LPCSTR ModuleName);

	DWORD CalculateVirtualPageCount(IMP_AT IAT);

	void ResolveIAT(LPCSTR FirstModule, LPCSTR CopyModule);
}
#endif
