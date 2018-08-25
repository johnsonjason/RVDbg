#ifndef DBG_PROFILER
#define DBG_PROFILER
#include <cstdint>
#include <vector>
#include <chrono>

// PROFILER_ERROR is our error-code (0)
#define PROFILER_ERROR 0

namespace profiling
{
	/* profiler_state
	* ready = quasi-state
	* running = performance measurement state
	* complete = performance measurement completion state
	*/
	enum profiler_state
	{
		ready,
		running,
		complete
	};

	/* exception_profiler
	* class object for scheduling performance tests for native code
	* uses two-point exceptions to make measurements between addresses
	* returns the elapsed time in nanoseconds that the code targets took to execute
	*/
	class exception_profiler
	{
	public:
		// construct the profiler with both a numeric tag as well as a name tag
		exception_profiler(std::size_t tag, std::string& name);
		// start the performance measurement
		void begin_profiling();
		// performance completion routine
		void end_profiling();
		// unique identifier
		std::size_t tag;
		// unique identifier
		std::string name;

	private:
		// time target point one
		std::chrono::high_resolution_clock::time_point instruction_pt1;
		// time target point two
		std::chrono::high_resolution_clock::time_point instruction_pt2;
		// elapsed time points
		std::int64_t performance;
		// the current performance measuring state
		profiler_state state;
	};

	// get the profiler by its name tag
	std::size_t get_profiler_name(std::string& tag);
	// get the profiler by its numeric tag
	std::size_t get_profiler_tag(std::size_t tag);
}

/* <EXAMPLE>
push eax
mov eax, 0x00000000 => hlt => start performance timer and re-execute instruction
add eax, 0x00000005
inc eax
xor eax, eax
pop eax             => hlt => end performance timer and re-execute instruction
(code performance time: 992.56 nanoseconds)
*/

#endif
