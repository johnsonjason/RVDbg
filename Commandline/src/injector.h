#ifndef INJECTOR
#define INJECTOR
#include <windows.h>
#include <cstdint>
#include <string>
#include "dbgredefs.h"

std::uint32_t dll_inject(std::uint32_t id, const std::wstring& dll);

#endif
