#include <gtest/gtest.h>

#include <Saba/Base/UnicodeUtil.h>

TEST(BaseTest, UnicodeUtil)
{
	std::string utf8Str = u8"日本語";
	std::wstring wStr = L"日本語";

	EXPECT_EQ(utf8Str, saba::ToUtf8String(wStr));
	EXPECT_EQ(wStr, saba::ToWString(utf8Str));
}
