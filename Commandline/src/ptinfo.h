#ifndef PTINFO
#define PTINFO
#include <windows.h>
#include <TlHelp32.h>
#include <string>

std::uint32_t getpid_n(const std::wstring& processName);

#endif
