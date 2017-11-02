//
// Copyright(c) 2016-2017 benikabocha.
// Distributed under the MIT License (http://opensource.org/licenses/MIT)
//

#include "UnicodeUtil.h"
#include "Singleton.h"

#include <locale>
#include <codecvt>

namespace saba
{
	namespace
	{
		class UtfConverter
		{
		public:
			std::string ToUtf8String(const std::wstring & wStr)
			{
				return m_converter.to_bytes(wStr);
			}

			std::wstring ToWString(const std::string & utf8Str)
			{
				return m_converter.from_bytes(utf8Str);
			}

		private:
			std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> m_converter;
		};
	}

	std::wstring ToWString(const std::string & utf8Str)
	{
		return Singleton<UtfConverter>::Get()->ToWString(utf8Str);
	}

	std::string ToUtf8String(const std::wstring & wStr)
	{
		return Singleton<UtfConverter>::Get()->ToUtf8String(wStr);
	}

	bool TryToWString(const std::string & utf8Str, std::wstring & wStr)
	{
		try
		{
			wStr = Singleton<UtfConverter>::Get()->ToWString(utf8Str);
		}
		catch (std::exception&)
		{
			return false;
		}

		return true;
	}
	bool TryToUtf8String(const std::wstring & wStr, std::string & utf8Str)
	{
		try
		{
			utf8Str = Singleton<UtfConverter>::Get()->ToUtf8String(wStr);
		}
		catch (std::exception&)
		{
			return false;
		}

		return true;
	}
}
