//
// Copyright(c) 2016-2017 benikabocha.
// Distributed under the MIT License (http://opensource.org/licenses/MIT)
//

#include "Path.h"

#include <algorithm>

#if _WIN32
#include <Windows.h>
#elif __APPLE__
#include <mach-o/dyld.h>
#include <unistd.h>
#include <cstdlib>
#elif __linux
#include <unistd.h>
#endif

#include "UnicodeUtil.h"

namespace saba
{
	namespace
	{
#if _WIN32
		const char PathDelimiter = '\\';
		const char* PathDelimiters = "\\/";
#else
		const char PathDelimiter = '/';
		const char* PathDelimiters = "/";
#endif
	}

	std::string PathUtil::GetCWD()
	{
		std::string workDir;
#if _WIN32
		DWORD sz = GetCurrentDirectoryW(0, nullptr);
		std::vector<wchar_t> buffer(sz);
		GetCurrentDirectory(sz, &buffer[0]);
		workDir = ToUtf8String(&buffer[0]);
#else // _WIN32
		char* buffer = getcwd(nullptr, 0);
		workDir = buffer;
		free(buffer);
#endif // _WIN32
		return workDir;
	}

	std::string PathUtil::GetExecutablePath()
	{
#if _WIN32
		std::vector<wchar_t> modulePath(MAX_PATH);
		if (GetModuleFileNameW(NULL, modulePath.data(), (DWORD)modulePath.size()) == 0)
		{
			return "";
		}
		return ToUtf8String(modulePath.data());
#elif __APPLE__
		char pathbuf[PATH_MAX + 1];
		uint32_t bufsize = sizeof(pathbuf);
		if (_NSGetExecutablePath(pathbuf, &bufsize) != 0)
		{
			return "";
		}
		return pathbuf;
#elif __linux
		char pathbuf[1024 + 1] = {};
		ssize_t sz = readlink("/proc/self/exe", pathbuf, sizeof(pathbuf) - 1);
		if (sz == -1)
		{
			return "";
		}
		else
		{
			pathbuf[sz] = '\0';
		}
		return pathbuf;
#else
		return "";
#endif
	}

	std::string PathUtil::Combine(const std::vector<std::string>& parts)
	{
		std::string result;
		for (const auto part : parts)
		{
			if (!part.empty())
			{
				auto pos = part.find_last_not_of(PathDelimiters);
				if (pos != std::string::npos)
				{
					if (!result.empty())
					{
						result.append(&PathDelimiter, 1);
					}
					result.append(part.c_str(), pos + 1);
				}
			}
		}
		return result;
	}

	std::string PathUtil::Combine(const std::string & a, const std::string & b)
	{
		return Combine({ a, b });
	}

	std::string PathUtil::GetDirectoryName(const std::string & path)
	{
		auto pos = path.find_last_of(PathDelimiters);
		if (pos == std::string::npos)
		{
			return "";
		}

		return path.substr(0, pos);
	}

	std::string PathUtil::GetFilename(const std::string & path)
	{
		auto pos = path.find_last_of(PathDelimiters);
		if (pos == std::string::npos)
		{
			return path;
		}

		return path.substr(pos + 1, path.size() - pos);
	}

	std::string PathUtil::GetFilenameWithoutExt(const std::string & path)
	{
		const std::string filename = GetFilename(path);
		auto pos = filename.find_last_of('.');
		if (pos == std::string::npos)
		{
			return filename;
		}

		return filename.substr(0, pos);
	}

	std::string PathUtil::GetExt(const std::string & path)
	{
		auto pos = path.find_last_of('.');
		if (pos == std::string::npos)
		{
			return "";
		}

		std::string ext = path.substr(pos + 1, path.size() - pos);
		for (auto& ch : ext)
		{
			ch = (char)tolower(ch);
		}
		return ext;
	}

	std::string PathUtil::GetDelimiter()
	{
#if _WIN32
		return "\\";
#else // _WIN32
		return "/";
#endif
	}

	std::string PathUtil::Normalize(const std::string & path)
	{
		std::string result = path;
#if _WIN32
		std::replace(result.begin(), result.end(), '/', '\\');
#else // _WIN32
		std::replace(result.begin(), result.end(), '\\', '/');
#endif
		return result;
	}

}
