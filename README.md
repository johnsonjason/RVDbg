# RVDbg

## IRC
Server: Freenode

Channel: ##redvice

## Discord
https://discord.gg/BFnvfv8

## Overview
RVDbg (Red Vice Debugger) is a debugger or exception handler for Windows 32-bit PEs. It's moreso an arbitrary exception handler. Labeled arbitrary because it purposefully throws exceptions at an inputted location in the PE's memory. It first makes a hook to the routine *KiUserExceptionDispatcher* and then dumps the contents of the registers into a struct defined as **VirtualRegisters**. Then it makes a call to a routine named *CallChain* and suspends all the threads in the process with the exception of the current thread and resumes any threads assigned to the debugger's internal thread pool.

This is different from a traditional debugger in the sense that it doesn't use any platform-specific debugger APIs targeted for developer use, hardware breakpoints / dr registers, use of the trap flag, debug interrupts, vectored exception handling, and changes wouldn't appear in the Process Environment Block (PEB), so calling a routine like *IsDebuggerPresent* would yield nil results.

There are two types of exceptions: **IMM/PAGE** (Immediate Exceptions & Page Exceptions)

Immediate Exceptions - Expect code to be executed as soon as a breakpoint is set. This kind of exception can be triggered through multiple means, illegal instructions, privileged instructions, access violations, etc... When the exception is triggered, it sets the memory back to it's original state immediately inside the *CallChain* routine, the purpose for that is so that it will rewrite the original state of the memory before any kind of memory integrity check could detect changes.

Page Exceptions - Don't edit memory contents, they change the protection rights of a page and redirect the memory to another location. This is done by calling the routine *VirtualProtect* and setting the memory region meant to be executed to the constant **PAGE_READONLY**. Data Execution Prevent (DEP) would immediately throw a memory access violation and then be redirected to our *KiUserExceptionDispatcher* hook, which will decide how to deal with the exception. It will then redirect EIP (extended instruction pointer) to a copy page which has executable properties.

Access Exceptions - This type of exception is a beta idea, it will work the same way as page exceptions, except they're for memory that's being read or written to. It sets the page meant to be read or written to as the constant **PAGE_NOACCESS**. The *CallChain* routine will then disassemble the location that's executing the read or write operation and check if it's using direct addressing, register addressing, or indirect register addressing. There will be a few sets of routines setup to handle the addressing modes. They will be labeled names like *Execute_Chunk1*. Those routines use a selected address or operation, these operations/addresses will come from the *CallChain* as they're allocated as "swaps". Swaps are copies of the data that caused an exception, the *CallChain* then executes a new chunk of memory that uses the swap memory instead of the original memory contents. If it is register addressing then it will just modify the contents of the register to the allocated memory.

Screenshot of RVDbg:

![alt text](https://puu.sh/xHwj6/216062a2dc.png)

Enumerates through the process list, thread list of the selected process, details the exception type, memory address the exception occurred at, the return address, a breakpoint list, and the dumped contents of the 32-bit x86 registers.
