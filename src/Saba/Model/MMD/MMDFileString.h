//
// Copyright(c) 2016-2017 benikabocha.
// Distributed under the MIT License (http://opensource.org/licenses/MIT)
//

#ifndef SABA_MODEL_MMD_MMDFILESTRING_H_
#define SABA_MODEL_MMD_MMDFILESTRING_H_

#include <Saba/Base/UnicodeUtil.h>
#include <Saba/Base/File.h>

#include "SjisToUnicode.h"

#include <string>


namespace saba
{
	template <size_t Size>
	struct MMDFileString
	{
		MMDFileString()
		{
			Clear();
		}

		void Clear()
		{
			for (auto& ch : m_buffer)
			{
				ch = '\0';
			}
		}

		void Set(const char* str)
		{
			size_t i = 0;
			while (i < Size && str[i] != '\0')
			{
				m_buffer[i] = str[i];
				i++;
			}

			for (; i < Size + 1; i++)
			{
				m_buffer[i] = '\0';
			}
		}

		const char* ToCString() const { return m_buffer; }
		std::string ToString() const { return std::string(m_buffer); }
		std::wstring ToWString() const { return ConvertSjisToWString(m_buffer); }
		std::string ToUtf8String() const { return saba::ToUtf8String(ToWString()); }

		char	m_buffer[Size + 1];
	};

	template <size_t Size>
	bool Read(MMDFileString<Size>* str, File& file)
	{
		return file.Read(str->m_buffer, Size);
	}
}

#endif // !SABA_MODEL_MMD_MMDFILESTRING_H_
