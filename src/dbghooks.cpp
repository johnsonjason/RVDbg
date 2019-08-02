#include "stdafx.h"
#include "dbghooks.h"

static std::vector<HOOK_RECORD> HookRecords;

DWORD HookAPIRoutine(PVOID RoutineOrigin, PVOID NewRoutine, std::string APIHookData)
{
	HOOK_RECORD Routine;
	Routine.APIHook = RoutineOrigin;
	Routine.NewAPIRoutine = NewRoutine;
	Routine.APIHookData = APIHookData;
	std::memcpy(Routine.OriginCode, RoutineOrigin, 5);

	HookRecords.push_back(Routine);

	DWORD PageProtectionBuffer;

	if (VirtualProtect(Routine.APIHook, 1, PAGE_EXECUTE_READWRITE, reinterpret_cast<PDWORD>(&PageProtectionBuffer)) == 0)
	{
		return GetLastError();
	}

	DWORD Origin = reinterpret_cast<DWORD>(Routine.APIHook);
	DWORD End = reinterpret_cast<DWORD>(Routine.NewAPIRoutine);

	*reinterpret_cast<PBYTE>(Routine.APIHook) = 0xE9;
	*reinterpret_cast<PDWORD>(Origin + 1) = (End - Origin) - 5;

	if (VirtualProtect(Routine.APIHook, 1, PageProtectionBuffer, reinterpret_cast<PDWORD>(&PageProtectionBuffer)) == 0)
	{
		return GetLastError();
	}

	size_t RoutineCmpResult = memcmp(Routine.OriginCode, Routine.APIHook, 5);

	if (RoutineCmpResult != 0)
	{
		return RoutineCmpResult;
	}

	return 0;
}

DWORD RehookAPIRoutine(std::string APIHookData)
{
	HOOK_RECORD Routine;

	for (size_t iRecord = 0; iRecord < HookRecords.size(); iRecord++)
	{
		if (APIHookData.compare(HookRecords[iRecord].APIHookData) == 0)
		{
			Routine = HookRecords[iRecord];
			break;
		}
	}

	if (Routine.NewAPIRoutine == NULL)
	{
		return -1;
	}

	DWORD PageProtectionBuffer;

	if (VirtualProtect(Routine.APIHook, 1, PAGE_EXECUTE_READWRITE, reinterpret_cast<PDWORD>(&PageProtectionBuffer)) == 0)
	{
		return GetLastError();
	}

	DWORD Origin = reinterpret_cast<DWORD>(Routine.APIHook);
	DWORD End = reinterpret_cast<DWORD>(Routine.NewAPIRoutine);

	*reinterpret_cast<PBYTE>(Routine.APIHook) = 0xE9;
	*reinterpret_cast<PDWORD>(Origin + 1) = (End - Origin) - 5;

	if (VirtualProtect(Routine.APIHook, 1, PageProtectionBuffer, reinterpret_cast<PDWORD>(&PageProtectionBuffer)) == 0)
	{
		return GetLastError();
	}

	size_t RoutineCmpResult = memcmp(Routine.OriginCode, Routine.APIHook, 5);

	if (RoutineCmpResult != 0)
	{
		return RoutineCmpResult;
	}

	return 0;
}

DWORD TempUnhookAPIRoutine(std::string APIHookData)
{
	HOOK_RECORD Routine;

	for (size_t iRecord = 0; iRecord < HookRecords.size(); iRecord++)
	{
		if (APIHookData.compare(HookRecords[iRecord].APIHookData) == 0)
		{
			Routine = HookRecords[iRecord];
			break;
		}
	}

	if (Routine.NewAPIRoutine == NULL)
	{
		return -1;
	}

	DWORD PageProtectionBuffer;

	if (VirtualProtect(Routine.APIHook, 1, PAGE_EXECUTE_READWRITE, reinterpret_cast<PDWORD>(&PageProtectionBuffer)) == 0)
	{
		return GetLastError();
	}

	memcpy(Routine.APIHook, Routine.OriginCode, 5);

	if (VirtualProtect(Routine.APIHook, 1, PageProtectionBuffer, reinterpret_cast<PDWORD>(&PageProtectionBuffer)) == 0)
	{
		return GetLastError();
	}
	
	size_t RoutineCmpResult = memcmp(Routine.APIHook, Routine.OriginCode, 5);

	if (RoutineCmpResult != 0)
	{
		return RoutineCmpResult;
	}

	return 0;
}

DWORD UnhookAPIRoutine(std::string APIHookData)
{
	HOOK_RECORD Routine;
	size_t HookIndex = 0;

	for (size_t iRecord = 0; iRecord < HookRecords.size(); iRecord++)
	{
		if (APIHookData.compare(HookRecords[iRecord].APIHookData) == 0)
		{
			HookIndex = iRecord;
			Routine = HookRecords[iRecord];
			break;
		}
	}

	if (Routine.NewAPIRoutine == NULL)
	{
		return -1;
	}

	DWORD PageProtectionBuffer;

	if (VirtualProtect(Routine.APIHook, 1, PAGE_EXECUTE_READWRITE, reinterpret_cast<PDWORD>(&PageProtectionBuffer)) == 0)
	{
		return GetLastError();
	}

	memcpy(Routine.APIHook, Routine.OriginCode, 5);

	if (VirtualProtect(Routine.APIHook, 1, PageProtectionBuffer, reinterpret_cast<PDWORD>(&PageProtectionBuffer)) == 0)
	{
		return GetLastError();
	}

	size_t RoutineCmpResult = memcmp(Routine.APIHook, Routine.OriginCode, 5);

	if (RoutineCmpResult != 0)
	{
		return RoutineCmpResult;
	}

	HookRecords.erase(HookRecords.begin() + HookIndex);

	return 0;
}
