//
// Copyright(c) 2016-2017 benikabocha.
// Distributed under the MIT License (http://opensource.org/licenses/MIT)
//

#ifndef SABA_BASE_UNICODEUTIL_H_
#define SABA_BASE_UNICODEUTIL_H_

#include <string>

namespace saba
{
	std::wstring ToWString(const std::string& utf8Str);
	std::string ToUtf8String(const std::wstring& wStr);
}

#endif // !SABA_BASE_UNICODEUTIL_H_
