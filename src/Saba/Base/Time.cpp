//
// Copyright(c) 2016-2017 benikabocha.
// Distributed under the MIT License (http://opensource.org/licenses/MIT)
//

#include "Time.h"

#include <chrono>

namespace saba
{
	double GetTime()
	{
		auto now = std::chrono::high_resolution_clock::now();
		return std::chrono::duration<double>(now.time_since_epoch()).count();
	}

	double GetTimeMSec()
	{
		auto now = std::chrono::high_resolution_clock::now();
		return std::chrono::duration<double, std::milli>(now.time_since_epoch()).count();
	}

	double GetTimeUSec()
	{
		auto now = std::chrono::high_resolution_clock::now();
		return std::chrono::duration<double, std::micro>(now.time_since_epoch()).count();
	}
}
