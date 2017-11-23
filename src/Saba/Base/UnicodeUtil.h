//
// Copyright(c) 2016-2017 benikabocha.
// Distributed under the MIT License (http://opensource.org/licenses/MIT)
//

#ifndef SABA_BASE_UNICODEUTIL_H_
#define SABA_BASE_UNICODEUTIL_H_

#include <string>
#include <array>

namespace saba
{
	std::wstring ToWString(const std::string& utf8Str);
	std::string ToUtf8String(const std::wstring& wStr);

	bool TryToWString(const std::string& utf8Str, std::wstring& wStr);
	bool TryToUtf8String(const std::wstring& wStr, std::string& utf8Str);

	bool ConvChU8ToU16(const std::array<char, 4>& u8Ch, std::array<char16_t, 2>& u16Ch);
	bool ConvChU8ToU32(const std::array<char, 4>& u8Ch, char32_t& u32Ch);

	bool ConvChU16ToU8(const std::array<char16_t, 2>& u16Ch, std::array<char, 4>& u8Ch);
	bool ConvChU16ToU32(const std::array<char16_t, 2>& u16Ch, char32_t& u32Ch);

	bool ConvChU32ToU8(const char32_t u32Ch, std::array<char, 4>& u8Ch);
	bool ConvChU32ToU16(const char32_t u32Ch, std::array<char16_t, 2>& u16Ch);

	bool ConvU8ToU16(const std::string& u8Str, std::u16string& u16Str);
	bool ConvU8ToU32(const std::string& u8Str, std::u32string& u32Str);

	bool ConvU16ToU8(const std::u16string& u16Str, std::string& u8Str);
	bool ConvU16ToU32(const std::u16string& u16Str, std::u32string& u32Str);

	bool ConvU32ToU8(const std::u32string& u32Str, std::string& u8Str);
	bool ConvU32ToU16(const std::u32string& u32Str, std::u16string& u16Str);
}

#endif // !SABA_BASE_UNICODEUTIL_H_
