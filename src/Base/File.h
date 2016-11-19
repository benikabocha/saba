#ifndef SABA_BASE_FILE_H_
#define SABA_BASE_FILE_H_

#include <cstdio>
#include <vector>
#include <cstdint>
#include <string>

namespace saba
{
	class File
	{
	public:
		using Offset = int64_t;

		File();

		File(const File&) = delete;
		File& operator = (const File&) = delete;

		bool Open(const char* filepath);
		bool OpenText(const char* filepath);
		bool Create(const char* filepath);
		bool CreateText(const char* filepath);

		bool Open(const std::string& filepath) { return Open(filepath.c_str()); }
		bool OpenText(const std::string& filepath) { return OpenText(filepath.c_str()); }
		bool Create(const std::string& filepath) { return Create(filepath.c_str()); }
		bool CreateText(const std::string& filepath) { return CreateText(filepath.c_str()); }

		void Close();
		bool IsOpen();

		FILE* GetFilePointer() const;

		bool ReadAll(std::vector<char>* buffer);

		enum class SeekDir
		{
			Begin,
			Current,
			End,
		};
		bool Seek(Offset offset, SeekDir origin);
		Offset Tell();

		template <typename T>
		bool Read(T* buffer, size_t count = 1)
		{
			if (buffer == nullptr)
			{
				return false;
			}

			if (!IsOpen())
			{
				return false;
			}
#if _WIN32
			return fread_s(buffer, sizeof(T) * count, sizeof(T), count, m_fp) == count;
#else // !_WIN32
			return fread(buffer, sizeof(T), count, m_fp) == count;
#endif //!_WIN32
		}

		template <typename T>
		bool Write(T* buffer, size_t count = 1)
		{
			if (buffer == nullptr)
			{
				return false;
			}

			if (!IsOpen())
			{
				return false;
			}

			return fwrite(buffer, sizeof(T), count, m_fp) == count;
		}

	private:
		bool OpenFile(const char* filepath, const char* mode);

	private:
		FILE*	m_fp;
	};
}

#endif // !BASE_FILE_H_