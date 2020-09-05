# RVDbg - Red Vice Debugger

## Overview

RVDbgv2 - core

RVDbgv2 is a debugger and exception handler and unlike the former version, this is intended to support IA-32 and AMD64. This will be a much more cleaner and documented code base.

The entire premise of RVDbg is not to openly use any processor features (e.g. setting the CPU debug registers, using the trap flag in EFLAGS, etc...) or any documented API features for debugging that would result in calls and callbacks to known routines such as DbgUiRemoteBreakin to avoid detection from anti-debuggers. This means the agenda will also disclude registering vectored exception handlers and structured exception handlers, but to manually craft our own exception handler from a hook in KiUserExceptionDispatcher which can gracefully discard non-debugger related exceptions and redirect them to the regular flow of execution for exception handling.

By doing this, the following anti-debugger detections are subverted:

* BeingDebugged/IsDebuggerPresent will not be set/triggered
* The hardware debug registers are not set (dr0, dr1, etc...)
* The EFLAGS register is not touched by the debugger
* NtGlobalFlag is not set, nor are the Heap Flags
* DbgUiRemoteBreakin callback is not executed due to not using the Windows/NT Debugger APIs
* DebugActiveProcess is called, so self-debugging to detect the presence of a debugger should not raise red flags
* No use of OutputDebugString

There are obvious triggers that the debugger will be vulnerable to, such as memory integrity detection, timing checks, etc...

As of right now, the debugger supports one type of exception that will serve as a breakpoint (Though any type of exception raised by an instruction could be used.) which is throwing a privileged instruction exception through the use of a hlt instruction (this type of instruction never executes in user-mode). When our manually crafted exception handler receives the privileged exception status, it reads the general purpose registers, SSE registers, EFLAGS, and exception information which can be modified by the debugger. In the future, the debugger should support other types of exceptions such as page exceptions that will not require modifying the actual memory.

Currently the debugger supports a form of "stepping" or emulates it, by using [Zydis](https://github.com/zyantific/zydis), an open source disassembler licensed under the MIT license. The debugger classifies instructions by their purpose, such as whether they will redirect execution or not. If these are not execution changing instructions then it calculates the instruction size and increments the instruction pointer. If it encounters an execution changing instruction, it then checks if it is conditional or not. If the instruction is for example, a conditional near jump, then it allocates memory with VirtualAlloc and converts it to a short jump. It then places a breakpoint at both "virtual" jump locations and then detects if the jump was linear (A jump did not happen.) or nonlinear (A jump happened.), when this happens an exception is triggered that results in a "Step Dispatcher" being executed. The dispatcher for stepping will redirect the virtual jumped location to the real location in memory. This essentially emulates the decision on which branch will be executed. 

So far stepping only supports conditional jumps and regular instructions that do not redirect execution. It currently does not support unconditional jumps, calls, or returns.
