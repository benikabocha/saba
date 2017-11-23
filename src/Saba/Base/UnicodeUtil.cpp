//
// Copyright(c) 2016-2017 benikabocha.
// Distributed under the MIT License (http://opensource.org/licenses/MIT)
//

#include "UnicodeUtil.h"

#include <stdexcept>

namespace saba
{
	std::wstring ToWString(const std::string & utf8Str)
	{
		std::wstring wStr;
		if (!TryToWString(utf8Str, wStr))
		{
			throw std::invalid_argument("Faild to convert wstring.");
		}
		return wStr;
	}

	std::string ToUtf8String(const std::wstring & wStr)
	{
		std::string utf8Str;
		if (!TryToUtf8String(wStr, utf8Str))
		{
			throw std::invalid_argument("Failed to convert UTF-8 string.");
		}
		return utf8Str;
	}

	bool TryToWString(const std::string & utf8Str, std::wstring & wStr)
	{
		if (sizeof(wchar_t) == sizeof(char16_t))
		{
			std::u16string utf16Str;
			if (!ConvU8ToU16(utf8Str, utf16Str))
			{
				return false;
			}
			wStr = reinterpret_cast<const wchar_t*>(utf16Str.c_str());
		}
		else if (sizeof(wchar_t) == sizeof(char32_t))
		{
			std::u32string utf32Str;
			if (!ConvU8ToU32(utf8Str, utf32Str))
			{
				return false;
			}
			wStr = reinterpret_cast<const wchar_t*>(utf32Str.c_str());
		}
		else
		{
			return false;
		}

		return true;
	}

	bool TryToUtf8String(const std::wstring & wStr, std::string & utf8Str)
	{
		if (sizeof(wchar_t) == sizeof(char16_t))
		{
			const char16_t* utf16Str = reinterpret_cast<const char16_t*>(wStr.c_str());
			if (!ConvU16ToU8(utf16Str, utf8Str))
			{
				return false;
			}
		}
		else if (sizeof(wchar_t) == sizeof(char32_t))
		{
			const char32_t* utf32Str = reinterpret_cast<const char32_t*>(wStr.c_str());
			if (!ConvU32ToU8(utf32Str, utf8Str))
			{
				return false;
			}
		}
		else
		{
			return false;
		}

		return true;
	}

	namespace {
		int GetU8ByteCount(char ch) {
			if (0 <= uint8_t(ch) && uint8_t(ch) < 0x80) {
				return 1;
			}
			if (0xC2 <= uint8_t(ch) && uint8_t(ch) < 0xE0) {
				return 2;
			}
			if (0xE0 <= uint8_t(ch) && uint8_t(ch) < 0xF0) {
				return 3;
			}
			if (0xF0 <= uint8_t(ch) && uint8_t(ch) < 0xF8) {
				return 4;
			}
			return 0;
		}

		bool IsU8LaterByte(char ch) {
			return 0x80 <= uint8_t(ch) && uint8_t(ch) < 0xC0;
		}

		bool IsU16HighSurrogate(char16_t ch) { return 0xD800 <= ch && ch < 0xDC00; }

		bool IsU16LowSurrogate(char16_t ch) { return 0xDC00 <= ch && ch < 0xE000; }
	}  // namespace

	bool ConvChU8ToU16(const std::array<char, 4>& u8Ch,
		std::array<char16_t, 2>& u16Ch) {
		char32_t u32Ch;
		if (!ConvChU8ToU32(u8Ch, u32Ch)) {
			return false;
		}
		if (!ConvChU32ToU16(u32Ch, u16Ch)) {
			return false;
		}
		return true;
	}

	bool ConvChU8ToU32(const std::array<char, 4>& u8Ch, char32_t& u32Ch) {
		int numBytes = GetU8ByteCount(u8Ch[0]);
		if (numBytes == 0) {
			return false;
		}
		switch (numBytes) {
		case 1:
			u32Ch = char32_t(uint8_t(u8Ch[0]));
			break;
		case 2:
			if (!IsU8LaterByte(u8Ch[1])) {
				return false;
			}
			if ((uint8_t(u8Ch[0]) & 0x1E) == 0) {
				return false;
			}

			u32Ch = char32_t(u8Ch[0] & 0x1F) << 6;
			u32Ch |= char32_t(u8Ch[1] & 0x3F);
			break;
		case 3:
			if (!IsU8LaterByte(u8Ch[1]) || !IsU8LaterByte(u8Ch[2])) {
				return false;
			}
			if ((uint8_t(u8Ch[0]) & 0x0F) == 0 &&
				(uint8_t(u8Ch[1]) & 0x20) == 0) {
				return false;
			}

			u32Ch = char32_t(u8Ch[0] & 0x0F) << 12;
			u32Ch |= char32_t(u8Ch[1] & 0x3F) << 6;
			u32Ch |= char32_t(u8Ch[2] & 0x3F);
			break;
		case 4:
			if (!IsU8LaterByte(u8Ch[1]) || !IsU8LaterByte(u8Ch[2]) ||
				!IsU8LaterByte(u8Ch[3])) {
				return false;
			}
			if ((uint8_t(u8Ch[0]) & 0x07) == 0 &&
				(uint8_t(u8Ch[1]) & 0x30) == 0) {
				return false;
			}

			u32Ch = char32_t(u8Ch[0] & 0x07) << 18;
			u32Ch |= char32_t(u8Ch[1] & 0x3F) << 12;
			u32Ch |= char32_t(u8Ch[2] & 0x3F) << 6;
			u32Ch |= char32_t(u8Ch[3] & 0x3F);
			break;
		default:
			return false;
		}

		return true;
	}

	bool ConvChU16ToU8(const std::array<char16_t, 2>& u16Ch,
		std::array<char, 4>& u8Ch) {
		char32_t u32Ch;
		if (!ConvChU16ToU32(u16Ch, u32Ch)) {
			return false;
		}
		if (!ConvChU32ToU8(u32Ch, u8Ch)) {
			return false;
		}
		return true;
	}

	bool ConvChU16ToU32(const std::array<char16_t, 2>& u16Ch, char32_t& u32Ch) {
		if (IsU16HighSurrogate(u16Ch[0])) {
			if (IsU16LowSurrogate(u16Ch[1])) {
				u32Ch = 0x10000 + (char32_t(u16Ch[0]) - 0xD800) * 0x400 +
					(char32_t(u16Ch[1]) - 0xDC00);
			}
			else if (u16Ch[1] == 0) {
				u32Ch = u16Ch[0];
			}
			else {
				return false;
			}
		}
		else if (IsU16LowSurrogate(u16Ch[0])) {
			if (u16Ch[1] == 0) {
				u32Ch = u16Ch[0];
			}
			else {
				return false;
			}
		}
		else {
			u32Ch = u16Ch[0];
		}

		return true;
	}

	bool ConvChU32ToU8(const char32_t u32Ch, std::array<char, 4>& u8Ch) {
		if (u32Ch > 0x10FFFF) {
			return false;
		}

		if (u32Ch < 128) {
			u8Ch[0] = char(u32Ch);
			u8Ch[1] = 0;
			u8Ch[2] = 0;
			u8Ch[3] = 0;
		}
		else if (u32Ch < 2048) {
			u8Ch[0] = 0xC0 | char(u32Ch >> 6);
			u8Ch[1] = 0x80 | (char(u32Ch) & 0x3F);
			u8Ch[2] = 0;
			u8Ch[3] = 0;
		}
		else if (u32Ch < 65536) {
			u8Ch[0] = 0xE0 | char(u32Ch >> 12);
			u8Ch[1] = 0x80 | (char(u32Ch >> 6) & 0x3F);
			u8Ch[2] = 0x80 | (char(u32Ch) & 0x3F);
			u8Ch[3] = 0;
		}
		else {
			u8Ch[0] = 0xF0 | char(u32Ch >> 18);
			u8Ch[1] = 0x80 | (char(u32Ch >> 12) & 0x3F);
			u8Ch[2] = 0x80 | (char(u32Ch >> 6) & 0x3F);
			u8Ch[3] = 0x80 | (char(u32Ch) & 0x3F);
		}

		return true;
	}

	bool ConvChU32ToU16(const char32_t u32Ch, std::array<char16_t, 2>& u16Ch) {
		if (u32Ch > 0x10FFFF) {
			return false;
		}

		if (u32Ch < 0x10000) {
			u16Ch[0] = char16_t(u32Ch);
			u16Ch[1] = 0;
		}
		else {
			u16Ch[0] = char16_t((u32Ch - 0x10000) / 0x400 + 0xD800);
			u16Ch[1] = char16_t((u32Ch - 0x10000) % 0x400 + 0xDC00);
		}

		return true;
	}

	bool ConvU8ToU16(const std::string& u8Str, std::u16string& u16Str) {
		for (auto u8It = u8Str.begin(); u8It != u8Str.end(); ++u8It) {
			auto numBytes = GetU8ByteCount((*u8It));
			if (numBytes == 0) {
				return false;
			}

			std::array<char, 4> u8Ch;
			u8Ch[0] = (*u8It);
			for (int i = 1; i < numBytes; i++) {
				++u8It;
				if (u8It == u8Str.end()) {
					return false;
				}
				u8Ch[i] = (*u8It);
			}

			std::array<char16_t, 2> u16Ch;
			if (!ConvChU8ToU16(u8Ch, u16Ch)) {
				return false;
			}

			u16Str.push_back(u16Ch[0]);
			if (u16Ch[1] != 0) {
				u16Str.push_back(u16Ch[1]);
			}
		}
		return true;
	}

	bool ConvU8ToU32(const std::string& u8Str, std::u32string& u32Str) {
		for (auto u8It = u8Str.begin(); u8It != u8Str.end(); ++u8It) {
			auto numBytes = GetU8ByteCount((*u8It));
			if (numBytes == 0) {
				return false;
			}

			std::array<char, 4> u8Ch;
			u8Ch[0] = (*u8It);
			for (int i = 1; i < numBytes; i++) {
				++u8It;
				if (u8It == u8Str.end()) {
					return false;
				}
				u8Ch[i] = (*u8It);
			}

			char32_t u32Ch;
			if (!ConvChU8ToU32(u8Ch, u32Ch)) {
				return false;
			}

			u32Str.push_back(u32Ch);
		}
		return true;
	}

	bool ConvU16ToU8(const std::u16string& u16Str, std::string& u8Str) {
		for (auto u16It = u16Str.begin(); u16It != u16Str.end(); ++u16It) {
			std::array<char16_t, 2> u16Ch;
			if (IsU16HighSurrogate((*u16It))) {
				u16Ch[0] = (*u16It);
				++u16It;
				if (u16It == u16Str.end()) {
					return false;
				}
				u16Ch[1] = (*u16It);
			}
			else {
				u16Ch[0] = (*u16It);
				u16Ch[1] = 0;
			}

			std::array<char, 4> u8Ch;
			if (!ConvChU16ToU8(u16Ch, u8Ch)) {
				return false;
			}
			if (u8Ch[0] != 0) {
				u8Str.push_back(u8Ch[0]);
			}
			if (u8Ch[1] != 0) {
				u8Str.push_back(u8Ch[1]);
			}
			if (u8Ch[2] != 0) {
				u8Str.push_back(u8Ch[2]);
			}
			if (u8Ch[3] != 0) {
				u8Str.push_back(u8Ch[3]);
			}
		}
		return true;
	}

	bool ConvU16ToU32(const std::u16string& u16Str, std::u32string& u32Str) {
		for (auto u16It = u16Str.begin(); u16It != u16Str.end(); ++u16It) {
			std::array<char16_t, 2> u16Ch;
			if (IsU16HighSurrogate((*u16It))) {
				u16Ch[0] = (*u16It);
				++u16It;
				if (u16It == u16Str.end()) {
					return false;
				}
				u16Ch[1] = (*u16It);
			}
			else {
				u16Ch[0] = (*u16It);
				u16Ch[1] = 0;
			}

			char32_t u32Ch;
			if (!ConvChU16ToU32(u16Ch, u32Ch)) {
				return false;
			}
			u32Str.push_back(u32Ch);
		}
		return true;
	}

	bool ConvU32ToU8(const std::u32string& u32Str, std::string& u8Str) {
		for (auto u32It = u32Str.begin(); u32It != u32Str.end(); ++u32It) {
			std::array<char, 4> u8Ch;
			if (!ConvChU32ToU8((*u32It), u8Ch)) {
				return false;
			}

			if (u8Ch[0] != 0) {
				u8Str.push_back(u8Ch[0]);
			}
			if (u8Ch[1] != 0) {
				u8Str.push_back(u8Ch[1]);
			}

			if (u8Ch[2] != 0) {
				u8Str.push_back(u8Ch[2]);
			}
			if (u8Ch[3] != 0) {
				u8Str.push_back(u8Ch[3]);
			}
		}
		return true;
	}

	bool ConvU32ToU16(const std::u32string& u32Str, std::u16string& u16Str) {
		for (auto u32It = u32Str.begin(); u32It != u32Str.end(); ++u32It) {
			std::array<char16_t, 2> u16Ch;
			if (!ConvChU32ToU16((*u32It), u16Ch)) {
				return false;
			}

			if (u16Ch[0] != 0) {
				u16Str.push_back(u16Ch[0]);
			}
			if (u16Ch[1] != 0) {
				u16Str.push_back(u16Ch[1]);
			}
		}
		return true;
	}
}
