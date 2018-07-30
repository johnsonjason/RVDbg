#ifndef _RVDBG
#define _RVDBG
#include <windows.h>
#include "exceptiondispatcher.h"
#include "execthread.h"
#include "..\CHooks\chooks.h"
#include "..\Injector\injector.h"
#include "..\IATResolution\iatresolve.h"
#include <array>

#define PAUSE_ALL 0
#define PAUSE_SINGLE 1
#define PAUSE_CONTINUE 2
#define PAUSE_SINGLE_STEP 3

#define IMMEDIATE_EXCEPTION 0
#define PAGE_EXCEPTION 1
#define ACCESS_EXCEPTION 2
#define MEMORY_EXCEPTION_CONTINUE 3

#define MOD_OPT 30
static bool use_module = false;

namespace rvdbg
{

	struct virtual_registers
	{
		// general purpose registers
		std::uint32_t eax;
		std::uint32_t ebx;
		std::uint32_t ecx;
		std::uint32_t edx;
		std::uint32_t esi;
		std::uint32_t edi;
		std::uint32_t ebp;
		std::uint32_t esp;
		void* eip;
		// sse_set specifies whether xmm registers were modified
		bool sse_set;
		// bxmm* - b is bool, bxmm* specifies the type of precision used for the xmm register
		std::uint8_t bxmm0; 
		std::uint8_t bxmm1;
		std::uint8_t bxmm2;
		std::uint8_t bxmm3;
		std::uint8_t bxmm4;
		std::uint8_t bxmm5;
		std::uint8_t bxmm6;
		std::uint8_t bxmm7;
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
		std::uint32_t return_address;
	};

	enum class gp_reg_32 : std::uint32_t
	{
		eax,
		ebx,
		ecx,
		edx,
		edi,
		esi,
		ebp,
		esp,
		eip
	};

	enum class sse_register : std::uint32_t
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

	enum suspension_mode : std::uint32_t
	{
		suspend_all,
		suspend_single,
		suspend_continue,
		suspend_single_step,
		mod_suspend_all,
		mod_suspend_single,
		mod_suspend_continue,
		mod_suspend_single_step
	};

	void set_module(bool use, std::string origin_mod_name, std::string mod_copy_name);
	std::string get_copy_module_name();

	extern bool debugger;
	extern CRITICAL_SECTION repr;
	extern CONDITION_VARIABLE reprcondition;
	void set_pause_mode(suspension_mode pause_status);
	suspension_mode get_pause_mode();

	void attach_debugger();
	void detach_debugger();
	void continue_debugger();

	void set_register(std::uint8_t dwregister, std::uint32_t value);
	void set_register_fp(std::uint8_t dwregister, bool precision, double value);
	virtual_registers get_registers();

	void set_exception_mode(const dispatcher::exception_type& exception_status_mode);
	dispatcher::exception_type get_exception_mode();

	std::uint32_t get_dbg_exception_address();

	bool is_aeh_present();

	int assign_thread(HANDLE hthread);
	void remove_thread(HANDLE hthread);

	std::size_t get_sector_size();
	std::array<dispatcher::pool_sect, 128>& get_sector();
	dispatcher::pool_sect get_current_section();
}
#endif
