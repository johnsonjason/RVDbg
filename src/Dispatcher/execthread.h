#ifndef EXECTHREAD
#define EXECTHREAD
#include <windows.h>
#include <tlhelp32.h>
#include <cstdint>

void suspend_threads(std::uint32_t process_id, std::uint32_t except_thread_id);
void resume_threads(std::uint32_t process_id, std::uint32_t except_thread_id);

#endif EXECTHREAD;
