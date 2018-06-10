#ifndef CHOOKS
#define CHOOKS
#include <vector>
#include <windows.h>
#include <cstdint>
#include "../dbgredefs.h"

struct address_record
{
	void* address;
	const char* address_data;
};

struct hook_record
{
	void* function_hook;
	void* hook_function;
	const char* function_hook_data;
	unsigned char origin_bytes[5];
};

int hook_function(void* function_origin, void* function_end, const char* function_hook_data);
int rehook_function(void* function_origin, void* function_end, const char* function_hook_data);
int temp_unhook_function(void* function_origin, const char* function_hook_data);
int Unhook_function(void* function_origin, void* function_end, const char* function_hook_data);

#endif
