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

#include <Saba/Model/MMD/SjisToUnicode.h>

TEST(BaseTest, MMSSjisToUnicode)
{
	{
		// Test Ascii code.
		const char* testStr = "abcABC=+";
		auto convStr = saba::ConvertSjisToU32String(testStr);
		EXPECT_EQ(convStr, U"abcABC=+");
	}
	{
		// Test Hankaku.
		const char testStr[] = {
			char(0xB1),
			char(0xB2),
			char(0xB3),
			char(0x00),
		};
		auto convStr = saba::ConvertSjisToU32String(testStr);
		EXPECT_EQ(convStr, U"ｱｲｳ");
	}
	{
		// Test 2byte sjis.
		const char testStr[] = {
			char(0x83), // ア
			char(0x41),
			char(0x87), // ①
			char(0x40),
			char(0x95), // 表
			char(0x5c),
			char(0x83), // ポ
			char(0x7C),
			char(0x00)
		};
		auto convStr = saba::ConvertSjisToU32String(testStr);
		EXPECT_EQ(convStr, U"ア①表ポ");
	}
	{
		// Test 2byte sjis.
		const char testStr[] = {
			char(0xE0), // 漾
			char(0x40),
			char(0xED), // 纊
			char(0x40),
			char(0xE6), // 觸
			char(0x5c),
			char(0xEA), // 黥
			char(0x7C),
			char(0x00)
		};
		auto convStr = saba::ConvertSjisToU32String(testStr);
		EXPECT_EQ(convStr, U"漾纊觸黥");
	}
	{
		// Test bad sjis code.
		const char badSjis[] = {
			char(0x81),
			char(0x00),
		};
		auto convStr = saba::ConvertSjisToU32String(badSjis);
		EXPECT_EQ(convStr, U"\u30FB");
	}
}
