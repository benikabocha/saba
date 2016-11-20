#include "File.h"
#include "UnicodeUtil.h"

namespace saba
{
	saba::File::File()
		: m_fp(nullptr)
	{
	}

	bool File::OpenFile(const char * filepath, const char * mode)
	{
		if (m_fp != nullptr)
		{
			Close();
		}
#if _WIN32
		std::wstring wFilepath = ToWString(filepath);
		std::wstring wMode = ToWString(mode);
		auto err = _wfopen_s(&m_fp, wFilepath.c_str(), wMode.c_str());
		return err == 0;
#else
		m_fp = fopen(filepath, mode);
		return m_fp != nullptr;
#endif
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
		}
	}

	bool saba::File::IsOpen()
	{
		return m_fp != nullptr;
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

		Seek(0, SeekDir::End);
		auto fileSize = Tell();
		if (fileSize == -1)
		{
			return false;
		}
		buffer->resize(fileSize);
		Seek(0, SeekDir::Begin);
		if (!Read(&(*buffer)[0], fileSize))
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
		return _fseeki64(m_fp, offset, cOrigin) == 0;
#else // _WIN32
		return fseek(m_fp, offset, cOrigin) == 0;
#endif // _WIN32
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
}

