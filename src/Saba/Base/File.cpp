//
// Copyright(c) 2016-2017 benikabocha.
// Distributed under the MIT License (http://opensource.org/licenses/MIT)
//

#include "File.h"
#include "UnicodeUtil.h"

#include <streambuf>
#include <iterator>

namespace saba
{
	saba::File::File()
		: m_fp(nullptr)
		, m_fileSize(0)
		, m_badFlag(false)
	{
	}

	File::~File()
	{
		Close();
	}

	bool File::OpenFile(const char * filepath, const char * mode)
	{
		if (m_fp != nullptr)
		{
			Close();
		}
#if _WIN32
		std::wstring wFilepath;
		if (!TryToWString(filepath, wFilepath))
		{
			return false;
		}
		std::wstring wMode;
		if (!TryToWString(mode, wMode))
		{
			return false;
		}
		auto err = _wfopen_s(&m_fp, wFilepath.c_str(), wMode.c_str());
		if (err != 0)
		{
			return false;
		}
#else
		m_fp = fopen(filepath, mode);
		if (m_fp == nullptr)
		{
			return false;
		}
#endif

		ClearBadFlag();

		Seek(0, SeekDir::End);
		m_fileSize = Tell();
		Seek(0, SeekDir::Begin);
		if (IsBad())
		{
			Close();
			return false;
		}
		return true;
	}

	bool File::Open(const char * filepath)
	{
		return OpenFile(filepath, "rb");
	}

	bool File::OpenText(const char * filepath)
	{
		return OpenFile(filepath, "r");
	}

	bool File::Create(const char * filepath)
	{
		return OpenFile(filepath, "wb");
	}

	bool File::CreateText(const char * filepath)
	{
		return OpenFile(filepath, "w");
	}

	void saba::File::Close()
	{
		if (m_fp != nullptr)
		{
			fclose(m_fp);
			m_fp = nullptr;
			m_fileSize = 0;
			m_badFlag = false;
		}
	}

	bool saba::File::IsOpen()
	{
		return m_fp != nullptr;
	}

	File::Offset File::GetSize() const
	{
		return m_fileSize;
	}

	bool File::IsBad() const
	{
		return m_badFlag;
	}

	void File::ClearBadFlag()
	{
		m_badFlag = false;
	}

	FILE * saba::File::GetFilePointer() const
	{
		return m_fp;
	}

	bool saba::File::ReadAll(std::vector<char>* buffer)
	{
		if (buffer == nullptr)
		{
			return false;
		}

		buffer->resize(m_fileSize);
		Seek(0, SeekDir::Begin);
		if (!Read(&(*buffer)[0], m_fileSize))
		{
			return false;
		}

		return true;
	}

	bool saba::File::Seek(Offset offset, SeekDir origin)
	{
		if (m_fp == nullptr)
		{
			return false;
		}
		int cOrigin = 0;
		switch (origin)
		{
		case SeekDir::Begin:
			cOrigin = SEEK_SET;
			break;
		case SeekDir::Current:
			cOrigin = SEEK_CUR;
			break;
		case SeekDir::End:
			cOrigin = SEEK_END;
			break;
		default:
			return false;
		}
#if _WIN32
		if (_fseeki64(m_fp, offset, cOrigin) != 0)
		{
			m_badFlag = true;
			return false;
		}
#else // _WIN32
		if (fseek(m_fp, offset, cOrigin) != 0)
		{
			m_badFlag = true;
			return false;
		}
#endif // _WIN32
		return true;
	}

	File::Offset File::Tell()
	{
		if (m_fp == nullptr)
		{
			return -1;
		}
#if _WIN32
		return (Offset)_ftelli64(m_fp);
#else // _WIN32
		return (Offset)ftell(m_fp);
#endif // _WIN32
	}

	TextFileReader::TextFileReader(const char * filepath)
	{
		Open(filepath);
	}

	TextFileReader::TextFileReader(const std::string & filepath)
	{
		Open(filepath);
	}

	bool TextFileReader::Open(const char * filepath)
	{
#if _WIN32
		std::wstring wFilepath;
		if (!TryToWString(filepath, wFilepath))
		{
			return false;
		}
		m_ifs.open(wFilepath);
#else
		m_ifs.open(filepath, std::ios::binary);
#endif
		return m_ifs.is_open();
	}

	bool TextFileReader::Open(const std::string & filepath)
	{
		return Open(filepath.c_str());
	}

	void TextFileReader::Close()
	{
		m_ifs.close();
	}

	bool TextFileReader::IsOpen()
	{
		return m_ifs.is_open();
	}

	std::string TextFileReader::ReadLine()
	{
		if (!IsOpen() || IsEof())
		{
			return "";
		}
		std::istreambuf_iterator<char> it(m_ifs);
		std::istreambuf_iterator<char> end;
		std::string line;
		auto outputIt = std::back_inserter(line);
		while (it != end && (*it) != '\r' && (*it) != '\n')
		{
			(*outputIt) = (*it);
			++outputIt;
			++it;
		}
		if (it != end)
		{
			auto ch = *it;
			++it;
			if (it != end && ch == '\r' && (*it) == '\n')
			{
				++it;
			}
		}

		return line;
	}

	void TextFileReader::ReadAllLines(std::vector<std::string>& lines)
	{
		lines.clear();
		if (!IsOpen() || IsEof())
		{
			return;
		}
		while (!IsEof())
		{
			lines.emplace_back(ReadLine());
		}
	}

	std::string TextFileReader::ReadAll()
	{
		std::istreambuf_iterator<char> begin(m_ifs);
		std::istreambuf_iterator<char> end;
		return std::string(begin, end);
	}

	bool TextFileReader::IsEof()
	{
		if (!m_ifs.is_open())
		{
			return m_ifs.eof();
		}

		if (m_ifs.eof())
		{
			return true;
		}
		int ch = m_ifs.peek();
		if (ch == std::ifstream::traits_type::eof())
		{
			return true;
		}

		return false;
	}

	bool OpenFromUtf8Path(const std::string & filepath, std::ifstream & ifs, std::ios_base::openmode mode)
	{
#if _WIN32
		std::wstring wFilepath;
		if (!TryToWString(filepath, wFilepath))
		{
			return false;
		}
		ifs.open(wFilepath, mode);
#else
		ifs.open(filepath, mode);
#endif
		return ifs.is_open();
	}
}

