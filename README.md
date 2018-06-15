# RVDbg - Red Vice Debugger

## Overview

RVDbg is a debugger and arbitrary exception handler. Labeled arbitrary because it purposefully throws exceptions at an inputted location in a portable executable's memory. It first makes a hook to the NT routine KiUserExceptionDispatcher and then dumps the contents of the CPU context registers into a structure defined as virtual_registers. Then it makes a call to a routine named call_chain and suspends all the threads in the process with the exception of the current thread and resumes any threads assigned to the debugger's internal thread pool.

This is different from a traditional debugger in the sense that it doesn't use any platform-specific debugging APIs targeted for developer use, hardware breakpoints / debug registers (DR0, DR1, ...), use of the trap flag, debug interrupts, vectored exception handling, and changes wouldn't appear in the Process Environment Block (PEB), so calling a routine like IsDebuggerPresent would yield nil results. The Red Vice Debugger bypasses anti-debugging checks.

It accomplishes this by redirecting the system exception dispatcher callback to another subroutine put in place by the Red Vice Debugger which handles critical information. Then it resolves the contents of the stack pointer as well as the instruction pointer and makes a copy of basic context information such as the registers before the thread required an exception to be handled and then it stores this information. Then the subroutine calls the call_chain subroutine which uses mutexes and critical sections to suspend the current thread which the exception occurred in and then signal the external threads that are part of the debugger for mechanisms such as IO, reading/writing to the thread context, setting the instruction pointer, etc...

Immediate Exceptions - Expect code to be executed as soon as a breakpoint is set. This kind of exception can be triggered through multiple means such as, illegal instructions, privileged instructions, access violations, etc... When the exception is triggered, it sets the memory back to it's original state immediately from inside the call_chain routine, the purpose for that is so that it will rewrite the original state of the memory before any kind of memory integrity check could detect changes.

Page Exceptions - Don't change contents of the virtual memory, they change the protection rights of a virtual memory page and redirect the memory to another location. This is done by calling the routine VirtualProtect and setting the memory region meant to be executed to the protection constant PAGE_READONLY. Data Execution Prevention (DEP) would immediately throw a memory access violation and then be redirected to our KiUserExceptionDispatcher hook, which will decide how to deal with the exception. It will then redirect EIP (extended instruction pointer) to a copy page which has executable properties.

Access Exceptions - This type of exception is a new idea in implementation. It will work the same way as page exceptions, except they're for memory that's being read from or written to. It sets the page meant to be read or written to as the protection constant PAGE_NOACCESS. The call_chain routine will then watch all access/read/write exceptions that occur in the routine and solve them until the targeted address has an exception and it can debug the accessed data that way.

## API Usage

```Cpp
    rvdbg::attach_debugger();
    rvdbg::assign_thread(GetCurrentThread());

    rvdbg::set_pause_mode(rvdbg::suspension_mode::suspend_all);
    rvdbg::set_exception_mode(dispatcher::exception_type::immediate_exception);
    
    dispatcher::add_exception(rvdbg::get_sector(), rvdbg::get_sector_size(), rvdbg::get_exception_mode(), reinterpret_cast<unsigned long>(process) + CHECK_OFFSET);
    
    wait_on_debug();
    
    rvdbg::set_register(static_cast<std::uint32_t>(rvdbg::gp_reg_32::ebp), check_copy);
    rvdbg::continue_debugger() 
```
    

## Screenshots

**RVDbg - GUI:**

![alt text](https://i.imgur.com/vUek6Bf.png)

Enumerates through the process list, thread list of the selected process, details the exception type, memory address the exception occurred at, the return address, a breakpoint list, and the dumped contents of the 32-bit x86 registers.

**RVDbg - Commandline:**

![alt_text](https://i.imgur.com/3PYyVLR.png)


## Commandline - commands:
```
Symbol @ Address - set the program to that symbol
```
```
Breakpoint - set a breakpoint at the symbol
```
```
Get - meta-information about the exception
```
```
DbgGet - information about the exception (return address, exception type, etc)
```
```
DbgDisplayRegisters - Display the general purpose registers in hexadecimal formatting
```
```
xmm-f - Display the SSE XMM registers in single precision floating point format
```
```
xmm-d - Display the SSE XMM registers in double precision floating point format
```
```
setreg x value - set a general purpose register (x is a numeric mapping) to a value
where x is 0
0 : EAX
setreg 0 55555555
EAX would now be set to 55555555
```
```
fsetreg x value - set xmm register with single floating point precision
```
```
dsetreg x value - set xmm register with double floating point precision
```
```
Exit - cleans up the program and exits it
```
