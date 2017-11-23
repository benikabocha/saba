#include <gtest/gtest.h>

#include <Saba/Base/UnicodeUtil.h>

TEST(BaseTest, UnicodeUtil)
{
	std::string utf8Str = u8"日本語";
	std::wstring wStr = L"日本語";

	std::string utf8StrRet;
	std::wstring wStrRet;

	EXPECT_TRUE(saba::TryToUtf8String(wStr, utf8StrRet));
	EXPECT_TRUE(saba::TryToWString(utf8Str, wStrRet));

	EXPECT_EQ(utf8Str, utf8StrRet);
	EXPECT_EQ(wStr, wStrRet);
}
