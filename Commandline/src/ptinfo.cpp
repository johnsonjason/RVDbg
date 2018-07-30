#include "stdafx.h"
#include "ptinfo.h"

std::uint32_t getpid_n(const std::wstring& process_name) {

	PROCESSENTRY32W process_info;
	process_info.dwSize = sizeof(process_info);

	HANDLE processes_snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);

	if (processes_snapshot == INVALID_HANDLE_VALUE)
	{
		return 0;
	}

	Process32FirstW(processes_snapshot, &process_info);
	if (!process_name.compare(process_info.szExeFile)) 
	{
		CloseHandle(processes_snapshot);
		return process_info.th32ProcessID;
	}

	while (Process32NextW(processes_snapshot, &process_info)) 
	{
		if (!process_name.compare(process_info.szExeFile)) 
		{
			CloseHandle(processes_snapshot);
			return process_info.th32ProcessID;
		}
	}
	CloseHandle(processes_snapshot);
	return 0;
}
