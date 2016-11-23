#include "Log.h"
#include "UnicodeUtil.h"

#include <iostream>

#if _WIN32

#include <Windows.h>

#endif // _WIN32

namespace saba
{
	void DefaultSink::log(const spdlog::details::log_msg & msg)
	{
#if _WIN32
		auto utf8Message = msg.formatted.str();
		auto wMessage = ToWString(utf8Message);
		int chCount = WideCharToMultiByte(
			CP_OEMCP,
			0,
			wMessage.c_str(),
			(int)wMessage.size(),
			nullptr,
			0,
			0,
			FALSE
		);
		std::string oemMessage(chCount, '\0');
		WideCharToMultiByte(
			CP_OEMCP,
			0,
			wMessage.c_str(),
			(int)wMessage.size(),
			&oemMessage[0],
			chCount,
			0,
			FALSE
		);
		std::cout << oemMessage;
#else // _WIN32
		std::cout << msg.formatted.str();
#endif // _WIN32
	}

	void DefaultSink::flush()
	{
	}
}
