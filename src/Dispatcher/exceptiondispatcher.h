// Programmed by jasonfish4
#ifndef EXCEPTIONDISPATCHER
#define EXCEPTIONDISPATCHER
#include <windows.h>
#include <tlhelp32.h>
#include <cstdlib>
#include <cstdint>
#include <array>
#include "..\dbgredefs.h"

namespace dispatcher
{
	/* specifies information about the exception in this section */

	enum class exception_type : std::uint32_t
	{
		immediate_exception,
		access_exception,
		page_exception,
		memory_exception_continue
	};

	struct pool_sect
	{
		// copy-module name
		std::string module_name;
		// current thread of exception
		HANDLE thread;
		// current thread id of exception
		std::uint32_t thread_id;
		// if the section is being used
		bool used;
		// if a copy-module is being used
		bool use_module;
		// is the arbitrary exception handler present for this section
		bool is_aeh_present;
		// the type of exception
		exception_type dbg_exception_type;
		// the exception code information
		std::uint32_t dbg_exception_code;
		// the address the exception occurred at
		std::uint32_t dbg_exception_address;
		// the offset of the exception address
		std::uint32_t dbg_exception_offset;
		// Additional information
		std::uint32_t return_address;
		std::uint32_t save_code;
		std::uint32_t index;
	};

	void raise_instr_av(std::uint8_t* ptr, std::uint8_t save, bool on);
	void raise_page_av(std::uint8_t* ptr, std::uint32_t save, bool on);
	void raise_breakpoint_excpt(std::uint8_t* ptr, std::uint8_t save, bool on);
	void raise_priv_code_excpt(std::uint8_t* ptr, std::uint8_t save, bool on);
	void* handle_exception(dispatcher::pool_sect& segment, std::string& module_name, bool constant);
	std::size_t check_sector(std::array<dispatcher::pool_sect, 128>& sector, std::uint32_t address);
	std::size_t search_sector(std::array<dispatcher::pool_sect, 128>& sector, std::uint32_t address);
	void unlock_sector(std::array<dispatcher::pool_sect, 128>& sector, std::size_t index);
	void lock_sector(std::array<dispatcher::pool_sect, 128>& sector, std::size_t index);
	void add_exception(std::array<dispatcher::pool_sect, 128>& sector, std::size_t index, exception_type type, std::uint32_t dbg_exception_address);
}
#endif
