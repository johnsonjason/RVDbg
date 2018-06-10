#ifndef INJECTOR
#define INJECTOR
#include <windows.h>
#include <cstdint>
#include "../dbgredefs.h"

bool dll_inject(std::uint32_t id, const char* dll);

#endif
