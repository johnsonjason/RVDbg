#ifndef RVDBG
#define RVDBG
#include <windows.h>
#include "exceptiondispatcher.h"
#include "execthread.h"
#include "..\CHooks\chooks.h"
#include "..\Injector\injector.h"


static VirtualRegisters r_registers;

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


struct IMP_AT
{
	DWORD Size;
	PVOID Address;
};

static BOOLEAN UseModule;

static DWORD ExceptionComparator; // If the exception is the address compared
static DWORD ExceptionCode; // The exception status code
static DWORD AccessException;
static DWORD SelectedAddress;
static BYTE SelectedARegister;

static BOOLEAN Debugger;
extern BOOLEAN tDSend;

static BOOLEAN ExceptionMode;
static BOOLEAN ChunkExecutable;

static PVOID Decision; // The code to jump to, what we would call the catch block
static PVOID KiUserRealDispatcher; // the code to the real exception dispatcher
static PVOID KiUser;

static CONDITION_VARIABLE Runnable;
static CRITICAL_SECTION RunLock;

extern CRITICAL_SECTION repr;
extern CONDITION_VARIABLE reprcondition;

static std::vector<PVOID> Swaps;
static HANDLE Threads[16];
static PoolSect Sector[128];
static PoolSect CurrentPool;


static PVOID CallChain();
static void SetKiUser();
static IMP_AT GetIAT(LPCSTR ModuleName);
static void SetImportAddressTable(const char* ModuleName);

int WaitOptModule(const char* OptModuleName);
void SetModule(BOOLEAN use);

void AttachRVDbg();
void DetachRVDbg();
void ContinueDebugger();

void SetRegister(DWORD Register, DWORD value);
void SetRegisterFP(DWORD Register, BOOLEAN Precision, double Value);
VirtualRegisters GetRegisters();

void SetExceptionMode(BOOLEAN lExceptionMode);
BOOLEAN GetExceptionMode();
DWORD GetExceptionAddress();
PoolSect GetPool();

BOOLEAN IsAEHPresent();

int AssignThread(HANDLE Thread);
void RemoveThread(HANDLE Thread);

PoolSect* GetSector();
int GetSectorSize();

#endif
