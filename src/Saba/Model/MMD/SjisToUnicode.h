//
// Copyright(c) 2016-2017 benikabocha.
// Distributed under the MIT License (http://opensource.org/licenses/MIT)
//

#ifndef SABA_MODEL_MMD_SJISTOUNICODE_H_
#define SABA_MODEL_MMD_SJISTOUNICODE_H_

#include <string>

bool IsSjis1ByteChar(int ch);
int ConvertSjisToUnicode(int ch);
std::wstring ConvertSjisToWString(const char* sjisCode);

#endif // !SABA_MODEL_MMD_SJISTOUNICODE_H_
