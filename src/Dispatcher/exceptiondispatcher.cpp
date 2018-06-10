// Programmed by jasonfish4
#include "stdafx.h"
#include "exceptiondispatcher.h"
#include "rvdbg.h"

void dispatcher::raise_instr_av(std::uint8_t* ptr, std::uint8_t save, bool on)
{
	std::uint32_t old_protect;
	switch (on)
	{
	case false:
		VirtualProtect(ptr, 1, static_cast<unsigned long>(dbg_redef::page_protection::page_xrw), reinterpret_cast<unsigned long*>(&old_protect));
		*const_cast<std::uint8_t*>(ptr) = save;
		VirtualProtect(ptr, 1, old_protect, reinterpret_cast<unsigned long*>(&old_protect));
		return;
	case true:
		VirtualProtect(ptr, 1, static_cast<unsigned long>(dbg_redef::page_protection::page_xrw), reinterpret_cast<unsigned long*>(&old_protect));
		*const_cast<std::uint8_t*>(ptr) = 0xFF;
		VirtualProtect(ptr, 1, old_protect, reinterpret_cast<unsigned long*>(&old_protect));
		return;
	}
}

void dispatcher::raise_page_av(std::uint8_t* ptr, std::uint32_t save, bool on)
{
	std::uint32_t old_protect;
	switch (on)
	{
	case false:
		VirtualProtect(ptr, 1, save, reinterpret_cast<unsigned long*>(&old_protect));
		return;
	case true:
		VirtualProtect(ptr, 1, static_cast<unsigned long>(dbg_redef::page_protection::page_ro), reinterpret_cast<unsigned long*>(&old_protect));
		return;
	}
}

void dispatcher::raise_breakpoint_excpt(std::uint8_t* ptr, std::uint8_t save, bool on)
{
	std::uint32_t old_protect;
	switch (on)
	{
	case false:
		VirtualProtect(ptr, 1, static_cast<unsigned long>(dbg_redef::page_protection::page_xrw), reinterpret_cast<unsigned long*>(&old_protect));
		*const_cast<std::uint8_t*>(ptr) = save;
		VirtualProtect(ptr, 1, old_protect, reinterpret_cast<unsigned long*>(&old_protect));
		return;
	case true:
		VirtualProtect(ptr, 1, static_cast<unsigned long>(dbg_redef::page_protection::page_xrw), reinterpret_cast<unsigned long*>(&old_protect));
		*const_cast<std::uint8_t*>(ptr) = 0xCC;
		VirtualProtect(ptr, 1, old_protect, reinterpret_cast<unsigned long*>(&old_protect));
		return;
	}
}

void dispatcher::raise_priv_code_excpt(std::uint8_t* ptr, std::uint8_t save, bool on)
{
	std::uint32_t old_protect;
	switch (on)
	{
	case false:
		VirtualProtect(ptr, 1, static_cast<unsigned long>(dbg_redef::page_protection::page_xrw), reinterpret_cast<unsigned long*>(&old_protect));
		*const_cast<std::uint8_t*>(ptr) = save;
		VirtualProtect(ptr, 1, old_protect, reinterpret_cast<unsigned long*>(&old_protect));
		return;
	case true:
		VirtualProtect(ptr, 1, static_cast<unsigned long>(dbg_redef::page_protection::page_xrw), reinterpret_cast<unsigned long*>(&old_protect));
		*const_cast<std::uint8_t*>(ptr) = 0xF4;
		VirtualProtect(ptr, 1, old_protect, reinterpret_cast<unsigned long*>(&old_protect));
		return;
	}
}

void* dispatcher::handle_exception(dispatcher::pool_sect segment, std::string module_name, bool b_module)
{
	segment.use_module = b_module;
	switch (segment.dbg_exception_code)
	{
	case static_cast<std::uint32_t>(dbg_redef::exception_status_code::access_violation):
		dispatcher::raise_instr_av(reinterpret_cast<std::uint8_t*>(segment.dbg_exception_address), segment.save_code, false);
		if (segment.use_module)
		{
			return reinterpret_cast<void*>(reinterpret_cast<std::uint32_t>(GetModuleHandleA(module_name.c_str())) + segment.dbg_exception_offset);
		}
		return reinterpret_cast<void*>(segment.dbg_exception_address);
	case static_cast<std::uint32_t>(dbg_redef::exception_status_code::breakpoint_exception):
		dispatcher::raise_breakpoint_excpt(reinterpret_cast<std::uint8_t*>(segment.dbg_exception_address), segment.save_code, false);
		if (segment.use_module)
		{
			return reinterpret_cast<void*>(reinterpret_cast<std::uint32_t>(GetModuleHandleA(module_name.c_str())) + segment.dbg_exception_offset);
		}
		return reinterpret_cast<void*>(segment.dbg_exception_address);
	case static_cast<std::uint32_t>(dbg_redef::exception_status_code::privileged_instruction):
		dispatcher::raise_priv_code_excpt(reinterpret_cast<std::uint8_t*>(segment.dbg_exception_address), segment.save_code, false);
		if (segment.use_module)
		{
			return reinterpret_cast<void*>(reinterpret_cast<std::uint32_t>(GetModuleHandleA(module_name.c_str())) + segment.dbg_exception_offset);
		}
		return reinterpret_cast<void*>(segment.dbg_exception_address);
	}
	return nullptr;
}

std::size_t dispatcher::check_sector(std::array<dispatcher::pool_sect, 128>& sector)
{
	for (std::size_t iterator = 0; iterator < sector.size(); iterator++)
	{
		if (sector[iterator].used == false)
			return iterator;
	}
	return (sector.size() + 1);
}

std::size_t dispatcher::search_sector(std::array<dispatcher::pool_sect, 128>& sector, std::uint32_t address)
{
	for (std::size_t iterator = 0; iterator < sector.size(); iterator++)
	{
		if (sector[iterator].used == true && sector[iterator].is_aeh_present == false 
			&& sector[iterator].dbg_exception_address == address)
			return iterator;
	}
	return (sector.size() + 1);
}

void dispatcher::unlock_sector(std::array<dispatcher::pool_sect, 128>& sector, std::size_t index)
{
	dispatcher::pool_sect filled_segment = { 0 };
	std::fill_n(stdext::checked_array_iterator<dispatcher::pool_sect*>(&sector[index], 
		sizeof(dispatcher::pool_sect)), sizeof(dispatcher::pool_sect), filled_segment);
}

void dispatcher::lock_sector(std::array<dispatcher::pool_sect, 128>& sector, std::size_t index)
{
	sector[index].used = true;
	sector[index].is_aeh_present = false;
}


void dispatcher::add_exception(std::array<dispatcher::pool_sect, 128>& sector, std::size_t index, dispatcher::exception_type type, std::uint32_t dbg_exception_address)
{
	sector[index].dbg_exception_address = dbg_exception_address;
	sector[index].save_code = *(std::uint32_t*)dbg_exception_address;
	sector[index].dbg_exception_type = type;
	sector[index].index = index;
	dispatcher::lock_sector(sector, index);

	switch (type)
	{
	case dispatcher::exception_type::immediate_exception:
		dispatcher::raise_priv_code_excpt((std::uint8_t*)sector[index].dbg_exception_address, 0, true);
		return;
	case dispatcher::exception_type::page_exception:
		dispatcher::raise_page_av((std::uint8_t*)sector[index].dbg_exception_address, 0, true);
		return;
	}

}
