#pragma once

#include <exception>
#include <string>

// Runs one step of the OpenGL device's shutdown so that a failure is logged and never
// propagated. The device is destroyed while the stack unwinds from a failed frame, and a
// second exception during unwinding ends the process before the first one can be reported.
// Returns true when the step completed.
template<typename Step, typename Log>
bool GLShutdownStep(const char* name, Step&& step, Log&& log)
{
	try
	{
		step();
		return true;
	}
	catch (const std::exception& e)
	{
		log(std::string(name) + ": " + e.what());
	}
	catch (...)
	{
		log(std::string(name) + ": unknown exception");
	}
	return false;
}
