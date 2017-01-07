//
// Copyright(c) 2016-2017 benikabocha.
// Distributed under the MIT License (http://opensource.org/licenses/MIT)
//

#include "Time.h"

#include "Singleton.h"

#if _WIN32
#include <Windows.h>
#else // _WIN32
#include <time.h>
#endif // _WIN32

namespace saba
{
	namespace
	{
#if _WIN32
		class Time
		{
		public:
			Time()
			{
				QueryPerformanceFrequency(&m_freq);
			}

			double GetTime() const
			{
				LARGE_INTEGER count;
				QueryPerformanceCounter(&count);

				return (double)count.QuadPart / (double)m_freq.QuadPart;
			}

		private:
			LARGE_INTEGER	m_freq;
		};
#else // _WIN32
		class Time
		{
		public:
			Time()
			{
			}

			double GetTime() const
			{
				timespec ts;
				clock_gettime(CLOCK_MONOTONIC, &ts);
				uint64_t time = (uint64_t)ts.tv_sec * 1000000000 + (uint64_t)ts.tv_nsec;
				return (double)time / 1000000000.0;
			}
		};
#endif // _WIN32
	}

	double GetTime()
	{
		return Singleton<Time>::Get()->GetTime();
	}

	double GetTimeMSec()
	{
		return GetTime() * 1000.0;
	}

	double GetTimeUSec()
	{
		return GetTimeMSec() * 1000.0;
	}
}
