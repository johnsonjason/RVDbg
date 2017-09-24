#ifndef CHOOKS
#define CHOOKS
#include <vector>
#include <windows.h>

struct AddressRecord
{
	void* Address;
	const char* AddressData;
};

struct HookRecord
{
	void* FunctionHook;
	void* HookFunction;
	const char* FunctionHookData;
	unsigned char OriginBytes[5];
};

static std::vector<HookRecord> HookRecords;

int HookFunction(void* FunctionOrigin, void* FunctionEnd, const char* FunctionHookData);
int RehookFunction(void* FunctionOrigin, void* FunctionEnd, const char* FunctionHookData);
int TempUnhookFunction(void* FunctionOrigin, const char* FunctionHookData);
int UnhookFunction(void* FunctionOrigin, void* FunctionEnd, const char* FunctionHookData);

#endif
