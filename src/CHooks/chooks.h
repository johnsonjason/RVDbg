#ifndef CHOOKS
#define CHOOKS
#include <vector>
#include <windows.h>
#include <cstdint>
#include "../dbgredefs.h"

struct hook_record
{
	void* function_hook;
	void* hook_function;
	std::string function_hook_data;
	unsigned char origin_bytes[5];
};

int hook_function(void* function_origin, void* function_end, std::string& function_hook_data);
int rehook_function(std::string& function_hook_data);
int temp_unhook_function(std::string& function_hook_data);
int unhook_function(std::string& function_hook_data);

#endif
