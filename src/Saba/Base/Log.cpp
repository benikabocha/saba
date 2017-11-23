//
// Copyright(c) 2016-2017 benikabocha.
// Distributed under the MIT License (http://opensource.org/licenses/MIT)
//

#include "Log.h"
#include "UnicodeUtil.h"

#include <iostream>

#if _WIN32

#include <Windows.h>

#endif // _WIN32

namespace saba
{
	DefaultSink::DefaultSink()
	{
		m_defaultLogger = spdlog::stdout_color_mt("default");
	}

	void DefaultSink::log(const spdlog::details::log_msg & msg)
	{
#if _WIN32
		auto utf8Message = msg.raw.str();
		std::wstring wMessage;
		if (!TryToWString(utf8Message, wMessage))
		{
			m_defaultLogger->log(msg.level, "Failed to convert message.");
			return;
		}
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
		m_defaultLogger->log(msg.level, oemMessage);
#else // _WIN32
		m_defaultLogger->log(msg.level, msg.raw.c_str());
#endif // _WIN32
	}

	void DefaultSink::flush()
	{
		m_defaultLogger->flush();
	}
}
