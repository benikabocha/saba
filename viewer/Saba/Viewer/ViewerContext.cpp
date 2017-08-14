//
// Copyright(c) 2016-2017 benikabocha.
// Distributed under the MIT License (http://opensource.org/licenses/MIT)
//

#include "ViewerContext.h"

#include <Saba/Base/UnicodeUtil.h>
#include <Saba/Base/Path.h>

#include <vector>

#if _WIN32
#include <Windows.h>
#else // _WIN32
#include <unistd.h>
#endif // _WIN32

namespace saba
{
	ViewerContext::ViewerContext()
		: m_uiEnable(true)
		, m_cameraOverride(true)
		, m_clipElapsed(true)
		, m_elapsed(0.0)
		, m_animationTime(0.0)
		, m_msaaEnable(false)
		, m_msaaCount(0)
		, m_frameBufferWidth(0)
		, m_frameBufferHeight(0)
		, m_windowWidth(0)
		, m_windowHeight(0)
		, m_playMode(PlayMode::None)
	{
#if _WIN32
		DWORD sz = GetCurrentDirectoryW(0, nullptr);
		std::vector<wchar_t> buffer(sz);
		GetCurrentDirectory(sz, &buffer[0]);
		m_workDir = ToUtf8String(&buffer[0]);
#else // _WIN32
		char* buffer = getcwd(nullptr, 0);
		m_workDir = buffer;
		free(buffer);
#endif // _WIN32

		m_resourceDir = PathUtil::Combine(m_workDir, u8"resource");
		m_shaderDir = PathUtil::Combine(m_resourceDir, u8"shader");
	}

	void ViewerContext::SetElapsedTime(double elapsed)
	{
		if (m_clipElapsed)
		{
			if (elapsed > 1.0f / 30.0f)
			{
				m_elapsed = 1.0f / 30.0f;
			}
			else
			{
				m_elapsed = elapsed;
			}
		}
		else
		{
			m_elapsed = elapsed;
		}
	}

}
