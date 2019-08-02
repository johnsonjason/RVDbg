#ifndef DBG_HOOKS_H
#define DBG_HOOKS_H
#include <Windows.h>
#include <vector>
#include <string>

// 32-bit Unfinished, WIP

//
// Maintains a record for an API hook
// Contains the original code that was patched in the hook for restoration
//

typedef struct _HOOK_RECORD
{
	PVOID APIHook;
	PVOID NewAPIRoutine;
	std::string APIHookData;
	BYTE OriginCode[5];
} HOOK_RECORD;

DWORD HookAPIRoutine(PVOID RoutineOrigin, PVOID NewRoutine, std::string APIHookData);
DWORD RehookAPIRoutine(std::string APIHookData);
DWORD TempUnhookAPIRoutine(std::string APIHookData);
DWORD UnhookAPIRoutine(std::string APIHookData);

#endif
