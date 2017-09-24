int HookFunction(void* FunctionOrigin, void* FunctionEnd, const char* FunctionHookData)
{
	HookRecord FunctionRecord;
	FunctionRecord.FunctionHook = FunctionOrigin;
	FunctionRecord.HookFunction = FunctionEnd;
	FunctionRecord.FunctionHookData = FunctionHookData;
	memcpy(FunctionRecord.OriginBytes, FunctionOrigin, 5);

	HookRecords.push_back(FunctionRecord);

	DWORD FormerPageRight;

	if (VirtualProtect(FunctionOrigin, 1, PAGE_EXECUTE_READWRITE, &FormerPageRight) == 0)
		return GetLastError();

	DWORD Origin = (DWORD)FunctionOrigin;
	DWORD End = (DWORD)FunctionEnd;

	*(BYTE*)FunctionOrigin = 0xE9;
	*(DWORD*)(Origin + 1) = (End - Origin) - 5;

	if (VirtualProtect(FunctionOrigin, 1, FormerPageRight, &FormerPageRight) == 0)
		return GetLastError();

	if (memcmp(FunctionRecord.OriginBytes, FunctionOrigin, 5) != 0)
		return -2;

	return 0;
}

int RehookFunction(void* FunctionOrigin, void* FunctionEnd, const char* FunctionHookData)
{
	HookRecord FunctionRecord;

	for (size_t RecordIterator = 0; RecordIterator < HookRecords.size(); RecordIterator++)
	{
		if (strncmp(HookRecords[RecordIterator].FunctionHookData, FunctionHookData, strlen(FunctionHookData)) == 0)
		{
			FunctionRecord = HookRecords[RecordIterator];
			break;
		}
	}

	if (FunctionRecord.HookFunction == NULL)
		return -1;

	DWORD FormerPageRight;

	if (VirtualProtect(FunctionOrigin, 1, PAGE_EXECUTE_READWRITE, &FormerPageRight) == 0)
		return GetLastError();

	DWORD Origin = (DWORD)FunctionOrigin;
	DWORD End = (DWORD)FunctionEnd;

	*(BYTE*)FunctionOrigin = 0xE9;
	*(DWORD*)(Origin + 1) = (End - Origin) - 5;

	if (VirtualProtect(FunctionOrigin, 1, FormerPageRight, &FormerPageRight) == 0)
		return GetLastError();

	if (memcmp(FunctionRecord.OriginBytes, FunctionOrigin, 5) != 0)
		return -2;

	return 0;
}

int TempUnhookFunction(void* FunctionOrigin, const char* FunctionHookData)
{
	HookRecord FunctionRecord;

	for (size_t RecordIterator = 0; RecordIterator < HookRecords.size(); RecordIterator++)
	{
		if (strncmp(HookRecords[RecordIterator].FunctionHookData, FunctionHookData, strlen(FunctionHookData)) == 0)
		{
			FunctionRecord = HookRecords[RecordIterator];
			break;
		}
	}

	if (FunctionRecord.HookFunction == NULL)
		return -1;

	DWORD FormerPageRight;

	if (VirtualProtect(FunctionOrigin, 1, PAGE_EXECUTE_READWRITE, &FormerPageRight) == 0)
		return GetLastError();

	if (memcpy(FunctionOrigin, FunctionRecord.OriginBytes, 5) != FunctionOrigin)
		return -3;

	if (VirtualProtect(FunctionOrigin, 1, FormerPageRight, &FormerPageRight) == 0)
		return GetLastError();

	if (memcmp(FunctionOrigin, FunctionRecord.OriginBytes, 5) != 0)
		return -2;

	return 0;
}

int UnhookFunction(void* FunctionOrigin, void* FunctionEnd, const char* FunctionHookData)
{
	HookRecord FunctionRecord;
	size_t HookIndex = 0;

	for (size_t RecordIterator = 0; RecordIterator < HookRecords.size(); RecordIterator++)
	{
		if (strncmp(HookRecords[RecordIterator].FunctionHookData, FunctionHookData, strlen(FunctionHookData)) == 0)
		{
			HookIndex = RecordIterator;
			FunctionRecord = HookRecords[RecordIterator];
			break;
		}
	}

	if (FunctionRecord.HookFunction == NULL)
		return -1;

	DWORD FormerPageRight;

	if (VirtualProtect(FunctionOrigin, 1, PAGE_EXECUTE_READWRITE, &FormerPageRight) == 0)
		return GetLastError();

	if (memcpy(FunctionOrigin, FunctionRecord.OriginBytes, 5) != FunctionOrigin)
		return -3;

	if (VirtualProtect(FunctionOrigin, 1, FormerPageRight, &FormerPageRight) == 0)
		return GetLastError();

	if (memcmp(FunctionOrigin, FunctionRecord.OriginBytes, 5) != 0)
		return -2;

	HookRecords.erase(HookRecords.begin() + HookIndex);

	return 0;
}
