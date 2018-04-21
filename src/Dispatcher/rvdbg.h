#ifndef RVDBG
#define RVDBG
#include <windows.h>
#include "exceptiondispatcher.h"
#include "execthread.h"
#include "..\CHooks\chooks.h"
#include "..\Injector\injector.h"
#include "..\IATResolution\iatresolve.h"

#define PAUSE_ALL 0
#define PAUSE_SINGLE 1
#define PAUSE_CONTINUE 2

#define IMMEDIATE_EXCEPTION 0
#define PAGE_EXCEPTION 1
#define ACCESS_EXCEPTION 2
#define MEMORY_EXCEPTION_CONTINUE 3

#define MOD_OPT 30

static BOOLEAN UseModule;

static DWORD ExceptionComparator; // If the exception is the address compared
static DWORD ExceptionCode; // The exception status code
static DWORD AccessException;

static BOOLEAN Pause; // {0 = complete pause ; 1 = only the thread being debugged is pause ; 2 = full continue}

static BOOLEAN ExceptionMode;

static PVOID Decision; // The code to jump to, what we would call the catch block
static PVOID KiUserRealDispatcher; // the code to the real exception dispatcher
static PVOID KiUser;

static CONDITION_VARIABLE Runnable;
static CRITICAL_SECTION RunLock;

static HANDLE Threads[16];

static Dispatcher::PoolSect Sector[128];
static Dispatcher::PoolSect CurrentSection;

static char CopyModule[MAX_PATH];
static char MainModule[MAX_PATH];

static BYTE g_Field[128];

namespace Dbg
{

	struct VirtualRegisters
	{
		// general purpose registers
		DWORD eax;
		DWORD ebx;
		DWORD ecx;
		DWORD edx;
		DWORD esi;
		DWORD edi;
		DWORD ebp;
		DWORD esp;
		PVOID eip;
		// SSESet specifies whether xmm registers were modified
		BOOLEAN SSESet;
		// bxmm* - b is boolean, bxmm* specifies the type of precision used for the xmm register
		BOOLEAN bxmm0; 
		BOOLEAN bxmm1;
		BOOLEAN bxmm2;
		BOOLEAN bxmm3;
		BOOLEAN bxmm4;
		BOOLEAN bxmm5;
		BOOLEAN bxmm6;
		BOOLEAN bxmm7; 
		// double precision registers
		double dxmm0;
		double dxmm1;
		double dxmm2;
		double dxmm3;
		double dxmm4;
		double dxmm5;
		double dxmm6;
		double dxmm7;
		// single point precision registers
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

	enum GPRegisters
	{
		EAX,
		EBX,
		ECX,
		EDX,
		EDI,
		ESI,
		EBP,
		ESP,
		EIP
	};

	enum SSERegisters
	{
		xmm0,
		xmm1,
		xmm2,
		xmm3,
		xmm4,
		xmm5,
		xmm6,
		xmm7,
	};

	void SetModule(BOOLEAN use, const char* OriginalModuleName, const char* ModuleCopyName);
	char* GetCopyModuleName();

	extern BOOLEAN Debugger;
	extern CRITICAL_SECTION repr;
	extern CONDITION_VARIABLE reprcondition;

	void SetPauseMode(BOOLEAN PauseMode);
	BOOLEAN GetPauseMode();

	void AttachRVDbg();
	void DetachRVDbg();
	void ContinueDebugger();

	void SetRegister(DWORD Register, DWORD value);
	void SetRegisterFP(DWORD Register, BOOLEAN Precision, double Value);
	Dbg::VirtualRegisters GetRegisters();

	void SetExceptionMode(BOOLEAN lExceptionMode);
	BOOLEAN GetExceptionMode();
	DWORD GetExceptionAddress();
	Dispatcher::PoolSect GetCurrentSection();

	BOOLEAN IsAEHPresent();

	int AssignThread(HANDLE Thread);
	void RemoveThread(HANDLE Thread);

	Dispatcher::PoolSect* GetSector();
	int GetSectorSize();
}

#endif
