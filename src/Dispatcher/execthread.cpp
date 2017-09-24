#include "execthread.h"

void SuspendThreads(DWORD ProcessId, DWORD ExceptThreadId)
{
	HANDLE Snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
	if (Snapshot != INVALID_HANDLE_VALUE)
	{
		THREADENTRY32 ThreadEntry;
		ThreadEntry.dwSize = sizeof(ThreadEntry);
		if (Thread32First(Snapshot, &ThreadEntry))
		{
			do
			{
				if (ThreadEntry.dwSize >= FIELD_OFFSET(THREADENTRY32, th32OwnerProcessID) + sizeof(ThreadEntry.th32OwnerProcessID))
				{
					if (ThreadEntry.th32ThreadID != ExceptThreadId && ThreadEntry.th32OwnerProcessID == ProcessId)
					{
						HANDLE Thread = OpenThread(THREAD_ALL_ACCESS, FALSE, ThreadEntry.th32ThreadID);
						if (Thread != NULL)
						{
							SuspendThread(Thread);
							CloseHandle(Thread);
						}
					}
				}
				ThreadEntry.dwSize = sizeof(ThreadEntry);
			} while (Thread32Next(Snapshot, &ThreadEntry));
		}
		CloseHandle(Snapshot);
	}
}

void ResumeThreads(DWORD ProcessId, DWORD ExceptThreadId)
{
	HANDLE Snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
	if (Snapshot != INVALID_HANDLE_VALUE)
	{
		THREADENTRY32 ThreadEntry;
		ThreadEntry.dwSize = sizeof(ThreadEntry);
		if (Thread32First(Snapshot, &ThreadEntry))
		{
			do
			{
				if (ThreadEntry.dwSize >= FIELD_OFFSET(THREADENTRY32, th32OwnerProcessID) + sizeof(ThreadEntry.th32OwnerProcessID))
				{
					if (ThreadEntry.th32ThreadID != ExceptThreadId && ThreadEntry.th32OwnerProcessID == ProcessId)
					{
						HANDLE Thread = OpenThread(THREAD_ALL_ACCESS, FALSE, ThreadEntry.th32ThreadID);
						if (Thread != NULL)
						{
							ResumeThread(Thread);
							CloseHandle(Thread);
						}
					}
				}
				ThreadEntry.dwSize = sizeof(ThreadEntry);
			} while (Thread32Next(Snapshot, &ThreadEntry));
		}
		CloseHandle(Snapshot);
	}
}
