#include "stdafx.h"
#include "dbgprofiler.h"

// dynamic container for profiles that can be acquired and reused
static std::vector<profiling::exception_profiler> profilers;

// construct the profiler with a numeric and name-based identifier, initialize the state, and push it into our profiler container
profiling::exception_profiler::exception_profiler(std::size_t tag, std::string& name)
{
	this->state = profiling::profiler_state::ready;
	this->tag = tag;
	this->name = name;
	// make sure the profiler does not have any features of an error before pushing it into the container
	if (tag != PROFILER_ERROR && name != "error") 
	{
		profilers.push_back(*this);
	}
}

// set the profiler state to running and start the performance counter
void profiling::exception_profiler::begin_profiling()
{
	this->state = profiling::profiler_state::running;
	this->instruction_pt1 = std::chrono::high_resolution_clock::now();
}

// complete the performance counter and set the profiler state to complete, get the elapsed time
void profiling::exception_profiler::end_profiling()
{
	this->instruction_pt2 = std::chrono::high_resolution_clock::now();
	this->state = profiler_state::complete;
	this->performance = std::chrono::duration_cast<std::chrono::nanoseconds>(instruction_pt2 - instruction_pt1).count();
}

// get the profiler from the container by name
std::size_t profiling::get_profiler_name(std::string& tag)
{
	for (std::size_t _tag = 0; _tag < profilers.size(); _tag++)
	{
		if (profilers[_tag].name == tag)
		{
			return _tag;
		}
	}
	return PROFILER_ERROR;
}

// get the profiler from the container by numeric id
std::size_t profiling::get_profiler_tag(std::size_t tag)
{
	for (std::size_t _tag = 0; _tag < profilers.size(); _tag++)
	{
		if (profilers[_tag].tag == tag)
		{
			return _tag;
		}
	}
	return PROFILER_ERROR;
}
