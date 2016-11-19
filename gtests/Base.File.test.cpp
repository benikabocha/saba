#include <gtest/gtest.h>

#include <Base/File.h>

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

	EXPECT_EQ(true, file.Open(dataPath + u8"/日本語.txt"));
	EXPECT_EQ(true, file.IsOpen());
	EXPECT_NE(nullptr, file.GetFilePointer());

	// Read のテスト
	char ch = 0;
	EXPECT_EQ(true, file.Read(&ch));
	EXPECT_EQ('1', ch);
	EXPECT_EQ(1, file.Tell());

	EXPECT_EQ(true, file.Seek(1, saba::File::SeekDir::Current));
	EXPECT_EQ(2, file.Tell());
	EXPECT_EQ(true, file.Read(&ch));
	EXPECT_EQ('3', ch);

	EXPECT_EQ(true, file.Seek(0, saba::File::SeekDir::End));
	EXPECT_EQ(4, file.Tell());
	EXPECT_EQ(true, file.Seek(10, saba::File::SeekDir::End));
	EXPECT_EQ(14, file.Tell());
	EXPECT_EQ(false, file.Read(&ch));

	EXPECT_EQ(true, file.Seek(0, saba::File::SeekDir::Begin));
	EXPECT_EQ(0, file.Tell());
	EXPECT_EQ(true, file.Seek(2, saba::File::SeekDir::Begin));
	EXPECT_EQ(false, file.Seek(-10, saba::File::SeekDir::Begin));
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

	// Closeのテスト
	file.Close();
	EXPECT_EQ(false, file.IsOpen());
	EXPECT_EQ(nullptr, file.GetFilePointer());

}
