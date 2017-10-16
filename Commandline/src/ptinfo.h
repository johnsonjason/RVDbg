#ifndef PTINFO
#define PTINFO
#include <windows.h>
#include <TlHelp32.h>
#include <string>

DWORD FindProcessIdFromProcessName(const std::wstring processName);

#endif
