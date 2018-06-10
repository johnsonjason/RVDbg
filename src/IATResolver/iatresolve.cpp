#include "stdafx.h"
#include "iatresolve.h"

iat_resolution::imp_at iat_resolution::get_iat(LPCSTR module_name)
{
	HMODULE mod = GetModuleHandleA(module_name);

	PIMAGE_DOS_HEADER img_dos_headers = reinterpret_cast<PIMAGE_DOS_HEADER>(mod);
	PIMAGE_NT_HEADERS img_nt_headers = reinterpret_cast<PIMAGE_NT_HEADERS>(reinterpret_cast<std::uint8_t*>(img_dos_headers) + img_dos_headers->e_lfanew);

	iat_resolution::imp_at retn;
	retn.size = (img_nt_headers->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IAT].Size);
	retn.address = reinterpret_cast<void*>((std::uint32_t)mod + img_nt_headers->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IAT].VirtualAddress);
	return retn;
}

std::uint32_t iat_resolution::calc_virtual_page_count(iat_resolution::imp_at iat)
{
	MEMORY_BASIC_INFORMATION mbi;
	VirtualQuery(iat.address, &mbi, sizeof(mbi));

	std::uint32_t precedent = mbi.RegionSize;

	if (iat.size < precedent)
		return 1;

	return (iat.size / precedent);
}


void iat_resolution::resolve_iat(LPCSTR first_module, LPCSTR copy_module)
{
	iat_resolution::imp_at origin_module_iat = get_iat(first_module);
	iat_resolution::imp_at copy_module_iat = get_iat(copy_module);
	std::uint32_t vpc = calc_virtual_page_count(origin_module_iat);
	std::uint32_t old_protect;

	VirtualProtect(copy_module_iat.address, vpc, static_cast<std::uint32_t>(dbg_redef::page_protection::page_xrw), reinterpret_cast<unsigned long*>(&old_protect));
	memcpy(copy_module_iat.address, origin_module_iat.address, origin_module_iat.size);
	VirtualProtect(copy_module_iat.address, vpc, old_protect, reinterpret_cast<unsigned long*>(&old_protect));
}
