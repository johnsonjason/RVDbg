#include "injector.h"

BOOL DLLInject(DWORD ID, const char* dll)

{
	HANDLE Process = NULL;
	HANDLE RemoteThread = NULL;

	HANDLE Objects[2] = { RemoteThread, Process };

	LPVOID Memory;

	LPVOID LoadLibrary;

	if (!ID)
		return false;

	Process = OpenProcess(PROCESS_ALL_ACCESS, FALSE, ID);

	LoadLibrary = (LPVOID)GetProcAddress(GetModuleHandleA("kernel32.dll"), "LoadLibraryA");

	Memory = (LPVOID)VirtualAllocEx(Process, NULL, strlen(dll) + 1, MEM_RESERVE | MEM_COMMIT, PAGE_EXECUTE_READWRITE);
	if (Memory == NULL)
		return 6;

	if (WriteProcessMemory(Process, (LPVOID)Memory, dll, strlen(dll) + 1, NULL) == 0)
		return 7;

	RemoteThread = CreateRemoteThread(Process, NULL, NULL, (LPTHREAD_START_ROUTINE)LoadLibrary, (LPVOID)Memory, NULL, NULL);
	if (RemoteThread == NULL)
		return 8;

	DWORD ObjectsResult = WaitForMultipleObjects(2, Objects, TRUE, INFINITE);

	if (ObjectsResult == WAIT_OBJECT_0)
	{
		for (auto Object : Objects)
			CloseHandle(Objects);

		VirtualFreeEx(Process, (LPVOID)Memory, 0, MEM_RELEASE);

		return TRUE;
	}
	return FALSE;
}
