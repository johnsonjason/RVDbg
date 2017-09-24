#ifndef EXECTHREAD
#define EXECTHREAD
#include <windows.h>
#include <tlhelp32.h>

void SuspendThreads(DWORD ProcessId, DWORD ExceptThreadId);
void ResumeThreads(DWORD ProcessId, DWORD ExceptThreadId);

#endif EXECTHREAD;
