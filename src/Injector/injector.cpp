#include "stdafx.h"
#include "injector.h"

bool dll_inject(std::uint32_t id, const char* dll)

{
	HANDLE process = nullptr;
	HANDLE remote_thread = nullptr;
	void* memory;
	void* load_library;

	if (!id)
		return false;

	process = OpenProcess(PROCESS_ALL_ACCESS, static_cast<std::uint8_t>(false), id);

	load_library = reinterpret_cast<void*>(GetProcAddress(GetModuleHandleA("kernel32.dll"), "LoadLibraryA"));

	memory = const_cast<void*>(VirtualAllocEx(process, nullptr, strlen(dll) + 1, 
		MEM_RESERVE | MEM_COMMIT, static_cast<std::uint32_t>(dbg_redef::page_protection::page_xrw)));

	if (memory == nullptr)
		return GetLastError();

	if (WriteProcessMemory(process, const_cast<void*>(memory), dll, strlen(dll) + 1, nullptr) == 0)
		return GetLastError();

	remote_thread = CreateRemoteThread(process, nullptr, dbg_redef::nullval, (LPTHREAD_START_ROUTINE)load_library, 
		const_cast<void*>(memory), dbg_redef::nullval, nullptr);

	if (remote_thread == nullptr)
		return GetLastError();

	std::uint32_t objects_result = WaitForSingleObject(remote_thread, dbg_redef::infinite);

	if (objects_result == WAIT_OBJECT_0)
	{
		CloseHandle(process);
		CloseHandle(remote_thread);
		VirtualFreeEx(process, const_cast<void*>(memory), 0, MEM_RELEASE);
		return true;
	}

	return false;
}
