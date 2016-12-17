#include <gtest/gtest.h>

#include <Saba/Base/Path.h>

TEST(BaseTest, PathTest)
{
	// Combine
#if _WIN32
	EXPECT_EQ(std::string("abc\\def\\efg"), saba::PathUtil::Combine({ "abc", "def\\", "", "efg" }));
	EXPECT_EQ(std::string("abc\\def\\efg"), saba::PathUtil::Combine("abc", "def\\efg"));
#else
	EXPECT_EQ(std::string("abc/def/efg"), saba::PathUtil::Combine({ "abc", "def/", "", "efg" }));
	EXPECT_EQ(std::string("abc/def/efg"), saba::PathUtil::Combine("abc", "def/efg"));
#endif

	// DirectoryName
#if _WIN32
	EXPECT_EQ(std::string("abc\\def"), saba::PathUtil::GetDirectoryName("abc\\def/efg.txt"));
#else // _WIN32
	EXPECT_EQ(std::string("abc\\def"), saba::PathUtil::GetDirectoryName("abc\\def/efg.txt"));
#endif
	EXPECT_EQ(std::string("abc/def"), saba::PathUtil::GetDirectoryName("abc/def/"));
	EXPECT_EQ(std::string(""), saba::PathUtil::GetDirectoryName("abc"));
	EXPECT_EQ(std::string(""), saba::PathUtil::GetDirectoryName(""));

	// Filename
	EXPECT_EQ(std::string("test.txt"), saba::PathUtil::GetFilename("abc/test.txt"));
	EXPECT_EQ(std::string("test.txt"), saba::PathUtil::GetFilename("test.txt"));
	EXPECT_EQ(std::string(""), saba::PathUtil::GetFilename(""));

	// FilenameWithoutText
	EXPECT_EQ(std::string("test"), saba::PathUtil::GetFilenameWithoutExt("abc/test.txt"));
	EXPECT_EQ(std::string("test"), saba::PathUtil::GetFilenameWithoutExt("test.txt"));
	EXPECT_EQ(std::string("test"), saba::PathUtil::GetFilenameWithoutExt("test"));
	EXPECT_EQ(std::string("abc.test"), saba::PathUtil::GetFilenameWithoutExt("abc.test.txt"));
	EXPECT_EQ(std::string(""), saba::PathUtil::GetFilenameWithoutExt(""));

	// GetExt
	EXPECT_EQ(std::string("txt"), saba::PathUtil::GetExt("abc/test.txt"));
	EXPECT_EQ(std::string("txt"), saba::PathUtil::GetExt("test.txt"));
	EXPECT_EQ(std::string("txt"), saba::PathUtil::GetExt("abc.test.txt"));
	EXPECT_EQ(std::string(""), saba::PathUtil::GetExt("test"));
	EXPECT_EQ(std::string(""), saba::PathUtil::GetExt("."));
	EXPECT_EQ(std::string(""), saba::PathUtil::GetExt(""));

	// Normalize
#if _WIN32
	EXPECT_EQ(std::string("a\\b\\c\\"), saba::PathUtil::Normalize("a/b\\c/"));
#else // _WIN32
	EXPECT_EQ(std::string("a/b/c/"), saba::PathUtil::Normalize("a\\b/c\\"));
#endif
}
