#include "stdafx.h"
#include "rvdbg.h"

bool rvdbg::debugger;
CRITICAL_SECTION rvdbg::repr;
CONDITION_VARIABLE rvdbg::reprcondition;
static rvdbg::virtual_registers r_registers;
static HANDLE thread_pool[16];
static void* dbg_decision; // The code to jump to, what we would call the catch block
static void* kiuser_realdispatcher; // the code to the real exception dispatcher
static void* kiuser;
static std::uint32_t exception_comparator; // If the exception is the address compared
static std::uint32_t dbg_exception_code; // The exception status code
static std::uint32_t access_exception;
static std::array<dispatcher::pool_sect, 128> sector;
static dispatcher::pool_sect current_section;
static rvdbg::suspension_mode pause; // {0 = complete pause ; 1 = only the thread being debugged is pause ; 2 = full continue}
static std::string copy_module;
static std::string main_module;

static dispatcher::exception_type exception_mode;

static void handle_sse();

/* Resume exception-type thread_pool */
static void self_resume_threads()
{
	for (std::size_t iterator = 0; iterator < sizeof(thread_pool); iterator++)
	{
		if (thread_pool[iterator] != nullptr)
			ResumeThread(thread_pool[iterator]);
	}
}

/* WIP, will be used for page exceptions */
static void* call_chain_vpa()
{
	std::size_t exception_element = dispatcher::search_sector(sector, exception_comparator);
	if (exception_element > sector.size())
		return nullptr;
	return nullptr;
}

static void* call_chain()
{
	// Search a sector for a listed exception
	std::size_t exception_element = dispatcher::search_sector(sector, exception_comparator); 

	if (exception_element > sector.size())
	{
		// If true, used to bypass memory integrity checks
		if (exception_mode == dispatcher::exception_type::memory_exception_continue)
		{
			return (GetModuleHandleA(rvdbg::get_copy_module_name().c_str()) + 
				(exception_comparator - reinterpret_cast<std::uint32_t>(GetModuleHandleA(main_module.c_str()))));
		}
		return nullptr; 
	}

	if (rvdbg::get_pause_mode() == rvdbg::suspension_mode::suspend_all || 
		static_cast<std::uint32_t>(rvdbg::get_pause_mode()) == static_cast<uint32_t>(rvdbg::suspension_mode::suspend_all) ^ MOD_OPT)
	{
		suspend_threads(GetCurrentProcessId(), GetCurrentThreadId()); 
		rvdbg::debugger = true;
		WakeConditionVariable(&rvdbg::reprcondition); 
		self_resume_threads(); 
	}

	sector[exception_element].dbg_exception_code = dbg_exception_code; 

	if (static_cast<std::uint32_t>(rvdbg::get_pause_mode()) == static_cast<std::uint32_t>(rvdbg::suspension_mode::suspend_all) ^ MOD_OPT || 
		static_cast<std::uint32_t>(rvdbg::get_pause_mode()) == static_cast<std::uint32_t>(rvdbg::suspension_mode::suspend_single) ^ MOD_OPT || 
		static_cast<std::uint32_t>(rvdbg::get_pause_mode()) == static_cast<std::uint32_t>(rvdbg::suspension_mode::suspend_continue) ^ MOD_OPT)
	{
		r_registers.eip = dispatcher::handle_exception(sector[exception_element], sector[exception_element].module_name, true); 
	}
	else
	{
		r_registers.eip = dispatcher::handle_exception(sector[exception_element], sector[exception_element].module_name, false); 
	}
	
	sector[exception_element].thread_id = GetCurrentThreadId();
	sector[exception_element].thread = GetCurrentThread();
	sector[exception_element].is_aeh_present = true; 
	sector[exception_element].return_address = r_registers.return_address; 

	current_section = sector[exception_element];

	if (rvdbg::get_pause_mode() == rvdbg::suspension_mode::suspend_all || 
		rvdbg::get_pause_mode() == rvdbg::suspension_mode::suspend_single || 
		static_cast<std::uint32_t>(rvdbg::get_pause_mode()) ==  static_cast<std::uint32_t>(rvdbg::suspension_mode::suspend_all) ^ MOD_OPT || 
		static_cast<std::uint32_t>(rvdbg::get_pause_mode()) == static_cast<std::uint32_t>(rvdbg::suspension_mode::suspend_single) ^ MOD_OPT)
	{
		EnterCriticalSection(&run_lock);
		SleepConditionVariableCS(&run_condition, &run_lock, dbg_redef::infinite); 
		LeaveCriticalSection(&run_lock);
		if (rvdbg::get_pause_mode() != rvdbg::suspension_mode::suspend_single && 
			rvdbg::get_pause_mode() != rvdbg::suspension_mode::suspend_continue)
		{
			resume_threads(GetCurrentProcessId(), GetCurrentThreadId());
		}
	}

	dispatcher::unlock_sector(sector, exception_element);

	if (rvdbg::get_pause_mode() == rvdbg::suspension_mode::suspend_continue)
	{
		std::size_t step_element = dispatcher::check_sector(rvdbg::get_sector());
		dispatcher::add_exception(rvdbg::get_sector(), step_element, exception_mode, sector[exception_element].dbg_exception_address);
	}

	rvdbg::debugger = false;
	return r_registers.eip;
}

static __declspec(naked) void __stdcall KiUserExceptionDispatcher(PEXCEPTION_RECORD ExceptionRecord, PCONTEXT Context)
{
	__asm
	{
		movss r_registers.xmm0, xmm0;
		movss r_registers.xmm1, xmm1;
		movss r_registers.xmm2, xmm2;
		movss r_registers.xmm3, xmm3;
		movss r_registers.xmm4, xmm4;
		movss r_registers.xmm5, xmm5;
		movss r_registers.xmm6, xmm6;
		movss r_registers.xmm7, xmm7;
		movsd r_registers.dxmm0, xmm0;
		movsd r_registers.dxmm1, xmm1;
		movsd r_registers.dxmm2, xmm2;
		movsd r_registers.dxmm3, xmm3;
		movsd r_registers.dxmm4, xmm4;
		movsd r_registers.dxmm5, xmm5;
		movsd r_registers.dxmm6, xmm6;
		movsd r_registers.dxmm7, xmm7;
		mov r_registers.eax, eax;
		mov r_registers.ebx, ebx;
		mov r_registers.ecx, ecx;
		mov r_registers.edx, edx;
		mov r_registers.esi, esi;
		mov r_registers.edi, edi;
		mov r_registers.ebp, ebp;

		// Reserved former esp
		mov eax, [esp + 0x11c];
		mov r_registers.esp, eax;

		mov eax, [eax];
		mov r_registers.return_address, eax;

		// [esp + 0x14] contains the exception address
		mov eax, [esp + 0x14]; 
		// move into the exception_comparator, aka the address to be compared with the exception address
		mov[exception_comparator], eax; 
		// [esp + 0x0C] contains the exception code
		mov eax, [esp + 0x08]; 
		// move into the dbg_exception_code global.
		mov[dbg_exception_code], eax; 
		mov eax, [esp + 20];
		// when access exceptions are enabled, move the accessed memory address here
		mov[access_exception], eax;
	}
	
	// Jump to an assigned exception handler for the exception mode
	switch (rvdbg::get_exception_mode())
	{
	case dispatcher::exception_type::immediate_exception:
		// Initiate the call_chain function used for immediate/execution exceptions
		dbg_decision = static_cast<void*>(call_chain()); 
		break;
	case dispatcher::exception_type::access_exception:
		// Initiate the call_chain_vpa function used for page exceptions
		dbg_decision = static_cast<void*>(call_chain_vpa());
		break;
	}

	// if the dbg_decision is zero, then jump back to the real dispatcher
	if (!dbg_decision)
	{
		__asm
		{
			mov eax, r_registers.eax;
			mov ecx, [esp + 04];
			mov ebx, [esp];
			jmp kiuser_realdispatcher;
		}
	}

	if (r_registers.sse_set == true)
		handle_sse();
	r_registers.sse_set = false;

	__asm
	{
		mov eax, r_registers.eax;
		mov ebx, r_registers.ebx;
		mov ecx, r_registers.ecx;
		mov edx, r_registers.edx;
		mov esi, r_registers.esi;
		mov edi, r_registers.edi;
		// [esp + 0x11c] contains stack initation information such as the return address, arguments, etc...
		mov esp, r_registers.esp;
		// jump to the catch block
		jmp dbg_decision;
	}

}

/* Get and set essential hook information */
static void set_kiuser()
{
	// Hook, tampered exception dispatcher later
	kiuser = static_cast<void*>(GetProcAddress(GetModuleHandleA("ntdll.dll"), "KiUserExceptionDispatcher")); 
	// If something fails, will jump back to the real dispatcher
	kiuser_realdispatcher = static_cast<void*>(GetProcAddress(GetModuleHandleA("ntdll.dll"), "KiUserExceptionDispatcher"));

	std::uint32_t kiuser_realdispatcher2 = static_cast<std::uint32_t>(reinterpret_cast<std::uint32_t>(kiuser_realdispatcher) + 8);
	std::uint32_t kiuser2 = static_cast<std::uint32_t>(reinterpret_cast<std::uint32_t>(kiuser) + 1);

	kiuser = reinterpret_cast<void*>(kiuser2);
	kiuser_realdispatcher = reinterpret_cast<void*>(kiuser_realdispatcher2);
}

/* Set the pause-specification for the debug session */
void rvdbg::set_pause_mode(rvdbg::suspension_mode pause_status)
{
	pause = pause_status;
}

/* Get the pause-specification for the debug session */
rvdbg::suspension_mode rvdbg::get_pause_mode()
{
	return pause;
}

/* Wait for the module to load and then resolve the import address table */
static int wait_opt_module(std::string origin_mod_name, std::string opt_mod_name)
{
	if (!use_module)
		return -1;
	volatile void* mod_ptr = nullptr;
	// Wait for a copy-module to load if there is none
	while (!mod_ptr)
		mod_ptr = static_cast<void*>(GetModuleHandleA(opt_mod_name.c_str()));
	// Resolve the import address table for this copy-module
	iat_resolution::resolve_iat(origin_mod_name.c_str(), opt_mod_name.c_str());
	return 0;
}

/* Set the copy-module to be used in control flow redirection */
void rvdbg::set_module(bool use, std::string origin_mod_name, std::string mod_copy_name)
{
	use_module = use;
	if (!use_module)
	{
		return;
	}
	else
	{
		copy_module.assign(mod_copy_name, mod_copy_name.size());
		main_module.assign(origin_mod_name, origin_mod_name.size());
		
		wait_opt_module(origin_mod_name, mod_copy_name);
	}
}

/* Acquire the name of the CopyModule */
std::string rvdbg::get_copy_module_name()
{
	return copy_module;
}

/* Assign a thread to the thread-excepted pool */
int rvdbg::assign_thread(HANDLE hthread)
{
	for (std::size_t iterator = 0; iterator < sizeof(thread_pool); iterator++)
	{
		if (thread_pool[iterator] == nullptr)
		{
			thread_pool[iterator] = hthread;
			return iterator;
		}
	}
	return -1;
}

/* Remove a thread from the thread-excepted pool*/
void rvdbg::remove_thread(HANDLE hthread)
{
	for (std::size_t iterator = 0; iterator < sizeof(thread_pool); iterator++)
	{
		if (thread_pool[iterator] == hthread)
		{
			CloseHandle(thread_pool[iterator]);
			thread_pool[iterator] = nullptr;
			return;
		}
	}
}

/* Attach the debugger by hooking the user-system supplied exception dispatcher */
void rvdbg::attach_debugger()
{
	InitializeConditionVariable(&run_condition);
	InitializeCriticalSection(&run_lock);
	set_kiuser();
	hook_function(kiuser, static_cast<void*>(KiUserExceptionDispatcher), "ntdll.dll:KiUserExceptionDispatcher");
}

/* Detach the debugger by unhooking the user-system supplied exception dispatcher */
void rvdbg::detach_debugger()
{
	Unhook_function(kiuser, static_cast<void*>(KiUserExceptionDispatcher), "ntdll.dll:KiUserExceptionDispatcher");
}

/* Continue the suspendeded debugger state */
void rvdbg::continue_debugger()
{
	WakeConditionVariable(&run_condition);
}

/* Check if the arbitrary exception handler is present within the sector */
bool rvdbg::is_aeh_present()
{
	return current_section.is_aeh_present;
}

/* Set the value of a general purpose register */
void rvdbg::set_register(std::uint32_t dwregister, std::uint32_t value)
{
	switch (dwregister) 
	{
	case static_cast<std::uint32_t>(rvdbg::gp_reg_32::eax):
		r_registers.eax = value;
		return;
	case static_cast<std::uint32_t>(rvdbg::gp_reg_32::ebx):
		r_registers.ebx = value;
		return;
	case static_cast<std::uint32_t>(rvdbg::gp_reg_32::ecx):
		r_registers.ecx = value;
		return;
	case static_cast<std::uint32_t>(rvdbg::gp_reg_32::edx):
		r_registers.edx = value;
		return;
	case static_cast<std::uint32_t>(rvdbg::gp_reg_32::esi):
		r_registers.esi = value;
		return;
	case static_cast<std::uint32_t>(rvdbg::gp_reg_32::edi):
		r_registers.edi = value;
		return;
	case static_cast<std::uint32_t>(rvdbg::gp_reg_32::ebp):
		r_registers.ebp = value;
		return;
	case static_cast<std::uint32_t>(rvdbg::gp_reg_32::esp):
		r_registers.esp = value;
		return;
	case static_cast<std::uint32_t>(rvdbg::gp_reg_32::eip):
		r_registers.eip = (void*)value;
	}
}

/* Set the value of an SSE register as well as the value's precision type
* bxmm* = (1 : double-precision) | (2 : single-precision)
* dxmm* = double-precision storage
* xmm* = single-precision storage
*/
void rvdbg::set_register_fp(std::uint32_t dwregister, bool precision, double value)
{
	r_registers.sse_set = true;
	// Map a value - representation of an SSE register to the actual register model
	switch (dwregister)
	{
	case static_cast<std::uint32_t>(rvdbg::sse_register::xmm0):
		if (precision)
		{
			r_registers.bxmm0 = 1;
			r_registers.dxmm0 = value;
			return;
		}
		r_registers.bxmm0 = 2;
		r_registers.xmm0 = static_cast<float>(value);
		return;
	case static_cast<std::uint32_t>(rvdbg::sse_register::xmm1):
		if (precision)
		{
			r_registers.bxmm1 = 1;
			r_registers.dxmm1 = value;
			return;
		}
		r_registers.bxmm1 = 2;
		r_registers.xmm1 = static_cast<float>(value);
		return;
	case static_cast<std::uint32_t>(rvdbg::sse_register::xmm2):
		if (precision)
		{
			r_registers.bxmm2 = 1;
			r_registers.dxmm2 = value;
			return;
		}
		r_registers.bxmm2 = 2;
		r_registers.xmm2 = static_cast<float>(value);
		return;
	case static_cast<std::uint32_t>(rvdbg::sse_register::xmm3):
		if (precision)
		{
			r_registers.bxmm3 = 1;
			r_registers.dxmm3 = value;
			return;
		}
		r_registers.bxmm3 = 2;
		r_registers.xmm3 = static_cast<float>(value);
		return;
	case static_cast<std::uint32_t>(rvdbg::sse_register::xmm4):
		if (precision)
		{
			r_registers.bxmm4 = 1;
			r_registers.dxmm4 = value;
			return;
		}
		r_registers.bxmm4 = 2;
		r_registers.xmm4 = static_cast<float>(value);
		return;
	case static_cast<std::uint32_t>(rvdbg::sse_register::xmm5):
		if (precision)
		{
			r_registers.bxmm5 = 1;
			r_registers.dxmm5 = value;
			return;
		}
		r_registers.bxmm5 = 2;
		r_registers.xmm5 = static_cast<float>(value);
		return;
	case static_cast<std::uint32_t>(rvdbg::sse_register::xmm6):
		if (precision)
		{
			r_registers.bxmm6 = 1;
			r_registers.dxmm6 = value;
			return;
		}
		r_registers.bxmm6 = 2;
		r_registers.xmm6 = static_cast<float>(value);
		return;
	case static_cast<std::uint32_t>(rvdbg::sse_register::xmm7):
		if (precision)
		{
			r_registers.bxmm7 = 1;
			r_registers.dxmm7 = value;
			return;
		}
		r_registers.bxmm7 = 2;
		r_registers.xmm7 = static_cast<float>(value);
		return;
	}
}

/* Set the exception mode for the debug session */
void rvdbg::set_exception_mode(dispatcher::exception_type exception_status_mode)
{
	exception_mode = exception_status_mode;
}

/* Get the exception mode for the debug session */
dispatcher::exception_type rvdbg::get_exception_mode()
{
	return exception_mode;
}

/* Get the exception virtual address of the current section */
std::uint32_t rvdbg::get_dbg_exception_address()
{
	return current_section.dbg_exception_address;
}

/* Return a model-representation of the registers */
rvdbg::virtual_registers rvdbg::get_registers()
{
	return r_registers;
}

/* Return the current section being used in the debug session */
dispatcher::pool_sect rvdbg::get_current_section()
{
	return current_section;
}

/* Return the current sector being used to store sections for a debug session */
std::array<dispatcher::pool_sect, 128> rvdbg::get_sector()
{
	return sector;
}

/* Get the size of the current sector being used to store sections for a debug session */
std::size_t rvdbg::get_sector_size()
{
	return sizeof(sector) / sizeof(dispatcher::pool_sect);
}

/*
* Decides whether to use double-precision or single-precision on floating point values
* Uses bxmm* as a flag for the dbg_decision
*/
static void handle_sse()
{
	if (r_registers.bxmm0 == 1)
		__asm movsd xmm0, r_registers.dxmm0;
	else if (r_registers.bxmm0 == 2)
		__asm movss xmm0, r_registers.xmm0;

	if (r_registers.bxmm1 == 1)
		__asm movsd xmm1, r_registers.dxmm1;
	else if (r_registers.bxmm1 == 2)
		__asm movss xmm1, r_registers.xmm1;

	if (r_registers.bxmm2 == 1)
		__asm movsd xmm2, r_registers.dxmm2;
	else if (r_registers.bxmm2 == 2)
		__asm movss xmm2, r_registers.xmm2;

	if (r_registers.bxmm3 == 1)
		__asm movsd xmm3, r_registers.dxmm3;
	else if (r_registers.bxmm3 == 2)
		__asm movss xmm3, r_registers.xmm3;

	if (r_registers.bxmm4 == 1)
		__asm movsd xmm4, r_registers.dxmm4;
	else if (r_registers.bxmm4 == 2)
		__asm movss xmm4, r_registers.xmm4;

	if (r_registers.bxmm5 == 1)
		__asm movsd xmm5, r_registers.dxmm5;
	else if (r_registers.bxmm5 == 2)
		__asm movss xmm5, r_registers.xmm5;

	if (r_registers.bxmm6 == 1)
		__asm movsd xmm6, r_registers.dxmm6;
	else if (r_registers.bxmm6 == 2)
		__asm movss xmm6, r_registers.xmm6;

	if (r_registers.bxmm7 == 1)
		__asm movsd xmm7, r_registers.dxmm7;
	else if (r_registers.bxmm7 == 2)
		__asm movss xmm7, r_registers.xmm7;

	r_registers.bxmm0 = 0;
	r_registers.bxmm1 = 0;
	r_registers.bxmm2 = 0;
	r_registers.bxmm3 = 0;
	r_registers.bxmm4 = 0;
	r_registers.bxmm5 = 0;
	r_registers.bxmm6 = 0;
	r_registers.bxmm7 = 0;
}
