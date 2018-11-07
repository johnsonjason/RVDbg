#include "stdafx.h"
#include "injector.h"

std::uint32_t dll_inject(std::uint32_t id, const std::wstring& dll)
{
	if (!id)
	{
		return 1;
	}

	HANDLE process = OpenProcess(PROCESS_ALL_ACCESS, static_cast<std::uint8_t>(false), id);
	void* load_library = reinterpret_cast<void*>(GetProcAddress(GetModuleHandleA("kernel32.dll"), "LoadLibraryW"));
	void* memory = const_cast<void*>(VirtualAllocEx(process, nullptr, dll.size()+1,
		MEM_RESERVE | MEM_COMMIT, static_cast<std::uint32_t>(dbg_redef::page_protection::page_rwx)));

	if (memory == nullptr)
	{
		return GetLastError();
	}

	if (WriteProcessMemory(process, const_cast<void*>(memory), dll.c_str(), ((dll.size()*2) + 1), nullptr) == 0)
	{
		return GetLastError();
	}

	HANDLE remote_thread = CreateRemoteThread(process, nullptr, dbg_redef::nullval, (LPTHREAD_START_ROUTINE)load_library,
		const_cast<void*>(memory), dbg_redef::nullval, nullptr);

	if (remote_thread == nullptr)
	{
		return GetLastError();
	}

	return 0;
}
