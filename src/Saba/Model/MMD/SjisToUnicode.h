//
// Copyright(c) 2016-2017 benikabocha.
// Distributed under the MIT License (http://opensource.org/licenses/MIT)
//

#ifndef SABA_MODEL_MMD_SJISTOUNICODE_H_
#define SABA_MODEL_MMD_SJISTOUNICODE_H_

#include <string>

namespace saba
{
	char16_t ConvertSjisToU16Char(int ch);
	std::u16string ConvertSjisToU16String(const char* sjisCode);
	std::u32string ConvertSjisToU32String(const char* sjisCode);
}

#endif // !SABA_MODEL_MMD_SJISTOUNICODE_H_
