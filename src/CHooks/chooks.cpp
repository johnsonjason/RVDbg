#include "stdafx.h"
#include "chooks.h"

static std::vector<hook_record> hook_records;

int hook_function(void* function_origin, void* function_end, const char* function_hook_data)
{
	hook_record function_record;
	function_record.function_hook = function_origin;
	function_record.hook_function = function_end;
	function_record.function_hook_data = function_hook_data;
	memcpy(function_record.origin_bytes, function_origin, 5);

	hook_records.push_back(function_record);

	std::uint32_t old_protection;

	if (VirtualProtect(function_origin, 1, static_cast<std::uint32_t>(dbg_redef::page_protection::page_xrw), 
		reinterpret_cast<unsigned long*>(&old_protection)) == 0)
		return GetLastError();

	std::uint32_t origin = reinterpret_cast<std::uint32_t>(function_origin);
	std::uint32_t end = reinterpret_cast<std::uint32_t>(function_end);

	*reinterpret_cast<std::uint8_t*>(function_origin) = 0xE9;
	*reinterpret_cast<std::uint32_t*>(origin + 1) = (end - origin) - 5;

	if (VirtualProtect(function_origin, 1, old_protection, reinterpret_cast<unsigned long*>(&old_protection)) == 0)
		return GetLastError();

	if (memcmp(function_record.origin_bytes, function_origin, 5) != 0)
		return -2;

	return 0;
}

int rehook_function(void* function_origin, void* function_end, const char* function_hook_data)
{
	hook_record function_record;

	for (std::size_t record_iterator = 0; record_iterator < hook_records.size(); record_iterator++)
	{
		if (strncmp(hook_records[record_iterator].function_hook_data, function_hook_data, strlen(function_hook_data)) == 0)
		{
			function_record = hook_records[record_iterator];
			break;
		}
	}

	if (function_record.hook_function == nullptr)
		return -1;

	std::uint32_t old_protection;

	if (VirtualProtect(function_origin, 1, static_cast<std::uint32_t>(dbg_redef::page_protection::page_xrw), 
		reinterpret_cast<unsigned long*>(&old_protection)) == 0)
		return GetLastError();

	std::uint32_t origin = reinterpret_cast<std::uint32_t>(function_origin);
	std::uint32_t end = reinterpret_cast<std::uint32_t>(function_end);

	*reinterpret_cast<std::uint8_t*>(function_origin) = 0xE9;
	*reinterpret_cast<std::uint32_t*>(origin + 1) = (end - origin) - 5;

	if (VirtualProtect(function_origin, 1, old_protection, reinterpret_cast<unsigned long*>(&old_protection)) == 0)
		return GetLastError();

	if (memcmp(function_record.origin_bytes, function_origin, 5) != 0)
		return -2;

	return 0;
}

int temp_unhook_function(void* function_origin, const char* function_hook_data)
{
	hook_record function_record;

	for (std::size_t record_iterator = 0; record_iterator < hook_records.size(); record_iterator++)
	{
		if (strncmp(hook_records[record_iterator].function_hook_data, function_hook_data, strlen(function_hook_data)) == 0)
		{
			function_record = hook_records[record_iterator];
			break;
		}
	}

	if (function_record.hook_function == nullptr)
		return -1;

	std::uint32_t old_protection;

	if (VirtualProtect(function_origin, 1, static_cast<std::uint32_t>(dbg_redef::page_protection::page_xrw), 
		reinterpret_cast<unsigned long*>(&old_protection)) == 0)
		return GetLastError();

	if (memcpy(function_origin, function_record.origin_bytes, 5) != function_origin)
		return -3;

	if (VirtualProtect(function_origin, 1, old_protection, reinterpret_cast<unsigned long*>(&old_protection)) == 0)
		return GetLastError();

	if (memcmp(function_origin, function_record.origin_bytes, 5) != 0)
		return -2;

	return 0;
}

int Unhook_function(void* function_origin, void* function_end, const char* function_hook_data)
{
	hook_record function_record;
	std::size_t hook_index = 0;

	for (std::size_t record_iterator = 0; record_iterator < hook_records.size(); record_iterator++)
	{
		if (strncmp(hook_records[record_iterator].function_hook_data, function_hook_data, strlen(function_hook_data)) == 0)
		{
			hook_index = record_iterator;
			function_record = hook_records[record_iterator];
			break;
		}
	}

	if (function_record.hook_function == nullptr)
		return -1;

	std::uint32_t old_protection;

	if (VirtualProtect(function_origin, 1, static_cast<std::uint32_t>(dbg_redef::page_protection::page_xrw), 
		reinterpret_cast<unsigned long*>(&old_protection)) == 0)
		return GetLastError();

	if (memcpy(function_origin, function_record.origin_bytes, 5) != function_origin)
		return -3;

	if (VirtualProtect(function_origin, 1, old_protection, reinterpret_cast<unsigned long*>(&old_protection)) == 0)
		return GetLastError();

	if (memcmp(function_origin, function_record.origin_bytes, 5) != 0)
		return -2;

	hook_records.erase(hook_records.begin() + hook_index);

	return 0;
}
