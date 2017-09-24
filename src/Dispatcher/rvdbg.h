#ifndef RVDBG
#define RVDBG
#include <windows.h>
#include "exceptiondispatcher.h"
#include "execthread.h"
#include "..\CHooks\chooks.h"
#include "..\Injector\injector.h"

#define D_ADDRESSING 1
#define R_ADDRESSING 2
#define IMMEDIATE 3

#define mEAX (0xA1)
#define mEBX (0x8B + 0x1D)
#define mECX (0x8B + 0x0D)
#define mEDX (0x8B + 0x15)
#define mESI (0x8B + 0x35)
#define mEDI (0x8B + 0x3D)
#define mEBP (0x8B + 0x2D)
#define mESP (0x8B + 0x25)

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

static VirtualRegisters r_registers;

static CONDITION_VARIABLE Runnable;
static CRITICAL_SECTION RunLock;

static HANDLE Threads[16];
static std::vector<void*> Swaps;
static PoolSect Sector[128];
static PoolSect CurrentPool;

static PVOID CallChain();
static void SetKiUser();

int WaitOptModule(const char* OptModuleName);
void SetModule(BOOLEAN use);

void AttachRVDbg();
void DetachRVDbg();
void ContinueDebugger();

void SetRegister(DWORD Register);

void SetExceptionMode(BOOLEAN lExceptionMode);
BOOLEAN GetExceptionMode();
DWORD GetExceptionAddress();
PoolSect GetPool();
VirtualRegisters GetRegisters();

BOOLEAN IsAEHPresent();

int AssignThread(HANDLE Thread);
void RemoveThread(HANDLE Thread);

PoolSect* GetSector();

#endif
