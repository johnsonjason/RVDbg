#include "stdafx.h"
#include "execthread.h"

void suspend_threads(std::uint32_t process_id, std::uint32_t except_thread_id)
{
	HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
	if (snapshot != INVALID_HANDLE_VALUE)
	{
		THREADENTRY32 thread_entry;
		thread_entry.dwSize = sizeof(thread_entry);
		if (Thread32First(snapshot, &thread_entry))
		{
			do
			{
				if (thread_entry.dwSize >= FIELD_OFFSET(THREADENTRY32, th32OwnerProcessID) + sizeof(thread_entry.th32OwnerProcessID))
				{
					if (thread_entry.th32ThreadID != except_thread_id && thread_entry.th32OwnerProcessID == process_id)
					{
						HANDLE thread = OpenThread(THREAD_ALL_ACCESS, static_cast<std::uint8_t>(false), thread_entry.th32ThreadID);
						if (thread != nullptr)
						{
							SuspendThread(thread);
							CloseHandle(thread);
						}
					}
				}
				thread_entry.dwSize = sizeof(thread_entry);
			} while (Thread32Next(snapshot, &thread_entry));
		}
		CloseHandle(snapshot);
	}
}

void resume_threads(std::uint32_t process_id, std::uint32_t except_thread_id)
{
	HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
	if (snapshot != INVALID_HANDLE_VALUE)
	{
		THREADENTRY32 thread_entry;
		thread_entry.dwSize = sizeof(thread_entry);
		if (Thread32First(snapshot, &thread_entry))
		{
			do
			{
				if (thread_entry.dwSize >= FIELD_OFFSET(THREADENTRY32, th32OwnerProcessID) + sizeof(thread_entry.th32OwnerProcessID))
				{
					if (thread_entry.th32ThreadID != except_thread_id && thread_entry.th32OwnerProcessID == process_id)
					{
						HANDLE thread = OpenThread(THREAD_ALL_ACCESS, static_cast<std::uint8_t>(false), thread_entry.th32ThreadID);
						if (thread != nullptr)
						{
							ResumeThread(thread);
							CloseHandle(thread);
						}
					}
				}
				thread_entry.dwSize = sizeof(thread_entry);
			} while (Thread32Next(snapshot, &thread_entry));
		}
		CloseHandle(snapshot);
	}
}

