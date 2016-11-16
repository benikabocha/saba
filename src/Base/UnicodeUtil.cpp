#include "UnicodeUtil.h"
#include "Singleton.h"

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
}
