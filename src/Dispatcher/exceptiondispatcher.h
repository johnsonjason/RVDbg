#ifndef EXCEPTIONDISPATCHER
#define EXCEPTIONDISPATCHER
#include <windows.h>
#include <tlhelp32.h>
#include <stdlib.h>


// Contain pointers that registers use, save them for later use aka register storage 2

struct VirtualRegisters
{
	DWORD eax;
	DWORD ebx;
	DWORD ecx;
	DWORD edx;
	DWORD esi;
	DWORD edi;
	DWORD ebp;
	DWORD esp;
	PVOID eip;
	BOOLEAN SSESet;
	BOOLEAN bxmm0;
	BOOLEAN bxmm1;
	BOOLEAN bxmm2;
	BOOLEAN bxmm3;
	BOOLEAN bxmm4;
	BOOLEAN bxmm5;
	BOOLEAN bxmm6;
	BOOLEAN bxmm7;
	double dxmm0;
	double dxmm1;
	double dxmm2;
	double dxmm3;
	double dxmm4;
	double dxmm5;
	double dxmm6;
	double dxmm7;
	float xmm0;
	float xmm1;
	float xmm2;
	float xmm3;
	float xmm4;
	float xmm5;
	float xmm6;
	float xmm7;
	DWORD ReturnAddress;
};



struct PoolSect
{
	char ModuleName[MAX_PATH];
	HANDLE Thread;
	BOOLEAN Used;
	BOOLEAN UseModule;
	BOOLEAN IsAEHPresent;
	BOOLEAN ExceptionType;
	DWORD ExceptionCode;
	DWORD ExceptionAddress;
	DWORD ExceptionOffset;
	DWORD ReturnAddress;
	DWORD SaveCode;
	DWORD Index;
};


void RaiseILGLAccessViolation(BYTE* ptr, BYTE save, BOOLEAN on);
void RaisePageAccessViolation(BYTE* ptr, DWORD save, BOOLEAN on);
void RaiseBreakpointException(BYTE* ptr, BYTE save, BOOLEAN on);
void RaisePrivilegedCodeException(BYTE* ptr, BYTE save, BOOLEAN on);

PVOID HandleException(PoolSect segment, const char* ModuleName);
size_t CheckSector(PoolSect sector[], size_t size);
size_t SearchSector(PoolSect sector[], size_t size, DWORD address);
void UnlockSector(PoolSect sector[], size_t index);
void LockSector(PoolSect sector[], size_t index);
void AddException(PoolSect sector[], size_t index, BOOLEAN Type, DWORD ExceptionAddress);
DWORD SwapAccess(DWORD AccessException, DWORD Test);

#endif
