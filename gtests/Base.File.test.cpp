#include <gtest/gtest.h>

#include <Saba/Base/File.h>

#ifndef TEST_DATA_PATH

#define TEST_DATA_PATH "./Data"

#endif // !TEST_DATA_PATH

#define __u8(x) u8 ## x

#define _u8(x)	__u8(x)

TEST(BaseTest, FileTest)
{
	std::string dataPath = _u8(TEST_DATA_PATH);

	saba::File file;

	// 初期状態のテスト
	EXPECT_EQ(false, file.IsOpen());
	EXPECT_EQ(nullptr, file.GetFilePointer());
	EXPECT_EQ(-1, file.Tell());
	EXPECT_EQ(false, file.IsBad());
	EXPECT_EQ(0, file.GetSize());

	EXPECT_EQ(true, file.Open(dataPath + u8"/日本語.txt"));
	EXPECT_EQ(true, file.IsOpen());
	EXPECT_NE(nullptr, file.GetFilePointer());
	EXPECT_EQ(false, file.IsBad());
	EXPECT_EQ(4, file.GetSize());

	// Read のテスト
	char ch = 0;
	EXPECT_EQ(true, file.Read(&ch));
	EXPECT_EQ('1', ch);
	EXPECT_EQ(false, file.IsBad());
	EXPECT_EQ(1, file.Tell());
	EXPECT_EQ(false, file.IsBad());

	EXPECT_EQ(true, file.Seek(1, saba::File::SeekDir::Current));
	EXPECT_EQ(2, file.Tell());
	EXPECT_EQ(true, file.Read(&ch));
	EXPECT_EQ('3', ch);

	EXPECT_EQ(true, file.Seek(0, saba::File::SeekDir::End));
	EXPECT_EQ(4, file.Tell());
	EXPECT_EQ(true, file.Seek(10, saba::File::SeekDir::End));
	EXPECT_EQ(14, file.Tell());
	EXPECT_EQ(false, file.Read(&ch));
	EXPECT_EQ(true, file.IsBad());
	file.ClearBadFlag();
	EXPECT_EQ(false, file.IsBad());

	EXPECT_EQ(true, file.Seek(0, saba::File::SeekDir::Begin));
	EXPECT_EQ(0, file.Tell());
	EXPECT_EQ(true, file.Seek(2, saba::File::SeekDir::Begin));
	EXPECT_EQ(false, file.Seek(-10, saba::File::SeekDir::Begin));
	EXPECT_EQ(true, file.IsBad());
	file.ClearBadFlag();
	EXPECT_EQ(false, file.IsBad());
	EXPECT_EQ(2, file.Tell());

	std::vector<char> buffer;
	EXPECT_EQ(true, file.ReadAll(&buffer));
	EXPECT_EQ(4, buffer.size());
	if (4 == buffer.size())
	{
		EXPECT_EQ('1', buffer[0]);
		EXPECT_EQ('2', buffer[1]);
		EXPECT_EQ('3', buffer[2]);
		EXPECT_EQ('4', buffer[3]);
	}
	EXPECT_EQ(false, file.IsBad());

	// Closeのテスト
	file.Close();
	EXPECT_EQ(false, file.IsOpen());
	EXPECT_EQ(nullptr, file.GetFilePointer());
	EXPECT_EQ(false, file.IsBad());
	EXPECT_EQ(0, file.GetSize());

}

TEST(BaseTest, TextFileReader)
{
	std::string dataPath = _u8(TEST_DATA_PATH);

	{
		saba::TextFileReader textFile;
		EXPECT_EQ(false, textFile.IsOpen());
		EXPECT_EQ(false, textFile.IsEof());
	}

	// Open
	{
		saba::TextFileReader textFile(dataPath + u8"/text_test1.txt");
		EXPECT_EQ(true, textFile.IsOpen());
	}

	{
		saba::TextFileReader textFile((dataPath + u8"/text_test1.txt").c_str());
		EXPECT_EQ(true, textFile.IsOpen());
	}

	{
		saba::TextFileReader textFile;
		EXPECT_EQ(true, textFile.Open(dataPath + u8"/text_test1.txt"));
		EXPECT_EQ(true, textFile.IsOpen());
	}

	{
		saba::TextFileReader textFile;
		EXPECT_EQ(true, textFile.Open((dataPath + u8"/text_test1.txt").c_str()));
		EXPECT_EQ(true, textFile.IsOpen());
	}

	{
		saba::TextFileReader textFile;
		EXPECT_EQ(true, textFile.Open((dataPath + u8"/日本語.txt").c_str()));
		EXPECT_EQ(true, textFile.IsOpen());
	}

	// Open -> Close
	{
		saba::TextFileReader textFile(dataPath + u8"/text_test1.txt");
		EXPECT_EQ(true, textFile.IsOpen());
		EXPECT_EQ(false, textFile.IsEof());
		textFile.Close();
		EXPECT_EQ(false, textFile.IsOpen());
		EXPECT_EQ(false, textFile.IsEof());
	}

	// Reopen
	{
		saba::TextFileReader textFile(dataPath + u8"/text_test1.txt");
		EXPECT_EQ(true, textFile.IsOpen());
		textFile.Open(dataPath + u8"/text_test2.txt");
		EXPECT_EQ(true, textFile.IsOpen());
	}

	// ReadLine
	{
		// return eof type
		saba::TextFileReader textFile(dataPath + u8"/text_test1.txt");
		EXPECT_EQ(true, textFile.IsOpen());
		EXPECT_EQ(std::string("abc"), textFile.ReadLine());
		EXPECT_EQ(false, textFile.IsEof());
		EXPECT_EQ(std::string("def"), textFile.ReadLine());
		EXPECT_EQ(false, textFile.IsEof());
		EXPECT_EQ(std::string(""), textFile.ReadLine());
		EXPECT_EQ(false, textFile.IsEof());
		EXPECT_EQ(std::string("efg"), textFile.ReadLine());
		EXPECT_EQ(true, textFile.IsEof());
		EXPECT_EQ(std::string(""), textFile.ReadLine());
		EXPECT_EQ(true, textFile.IsEof());
	}

	// ReadLine
	{
		// no return eof type
		saba::TextFileReader textFile(dataPath + u8"/text_test2.txt");
		EXPECT_EQ(true, textFile.IsOpen());
		EXPECT_EQ(std::string("abc"), textFile.ReadLine());
		EXPECT_EQ(false, textFile.IsEof());
		EXPECT_EQ(std::string("def"), textFile.ReadLine());
		EXPECT_EQ(false, textFile.IsEof());
		EXPECT_EQ(std::string(""), textFile.ReadLine());
		EXPECT_EQ(false, textFile.IsEof());
		EXPECT_EQ(std::string("efg"), textFile.ReadLine());
		EXPECT_EQ(true, textFile.IsEof());
		EXPECT_EQ(std::string(""), textFile.ReadLine());
		EXPECT_EQ(true, textFile.IsEof());
	}

	// ReadAll
	{
		saba::TextFileReader textFile;
		EXPECT_EQ(std::string(""), textFile.ReadAll());
	}

	// ReadAll
	{
		saba::TextFileReader textFile(dataPath + u8"/text_test1.txt");
		EXPECT_EQ(true, textFile.IsOpen());
		std::string text =
			"abc\n"
			"def\n"
			"\n"
			"efg\n";
		EXPECT_EQ(text, textFile.ReadAll());
		EXPECT_EQ(true, textFile.IsEof());
	}

	// ReadAll
	{
		saba::TextFileReader textFile(dataPath + u8"/text_test2.txt");
		EXPECT_EQ(true, textFile.IsOpen());
		std::string text =
			"abc\n"
			"def\n"
			"\n"
			"efg";
		EXPECT_EQ(text, textFile.ReadAll());
		EXPECT_EQ(true, textFile.IsEof());
	}

	// ReadAllLines
	{
		saba::TextFileReader textFile;
		std::vector<std::string> lines;
		textFile.ReadAllLines(lines);
		EXPECT_TRUE(lines.empty());
	}

	// ReadAllLines
	{
		saba::TextFileReader textFile(dataPath + u8"/text_test1.txt");
		EXPECT_EQ(true, textFile.IsOpen());
		std::vector<std::string> lines;
		textFile.ReadAllLines(lines);
		EXPECT_EQ(4, lines.size());
		EXPECT_EQ(std::string("abc"), lines[0]);
		EXPECT_EQ(std::string("def"), lines[1]);
		EXPECT_EQ(std::string(""), lines[2]);
		EXPECT_EQ(std::string("efg"), lines[3]);
		EXPECT_EQ(true, textFile.IsEof());
	}

	// ReadAllLines
	{
		saba::TextFileReader textFile(dataPath + u8"/text_test2.txt");
		EXPECT_EQ(true, textFile.IsOpen());
		std::vector<std::string> lines;
		textFile.ReadAllLines(lines);
		EXPECT_EQ(4, lines.size());
		EXPECT_EQ(std::string("abc"), lines[0]);
		EXPECT_EQ(std::string("def"), lines[1]);
		EXPECT_EQ(std::string(""), lines[2]);
		EXPECT_EQ(std::string("efg"), lines[3]);
		EXPECT_EQ(true, textFile.IsEof());
	}
}
