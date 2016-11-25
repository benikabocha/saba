#include <Saba/GL/GLTextureUtil.h>
#include <Saba/Base/Path.h>

#include <gtest/gtest.h>

#define __u8(x) u8 ## x
#define _u8(x)	__u8(x)

TEST(GLTest, TextureTest)
{
	std::string dataPath = _u8(TEST_DATA_PATH);
	dataPath = saba::PathUtil::Combine(dataPath, "Image");

	{
		auto imagePath = saba::PathUtil::Combine(dataPath, "test.png");
		auto tex = saba::CreateTextureFromFile(imagePath.c_str());
		EXPECT_NE(0, tex.Get());
	}

	{
		auto imagePath = saba::PathUtil::Combine(dataPath, "test.bmp");
		auto tex = saba::CreateTextureFromFile(imagePath.c_str());
		EXPECT_NE(0, tex.Get());
	}

	{
		auto imagePath = saba::PathUtil::Combine(dataPath, "test.jpg");
		auto tex = saba::CreateTextureFromFile(imagePath.c_str());
		EXPECT_NE(0, tex.Get());
	}

	{
		auto imagePath = saba::PathUtil::Combine(dataPath, "test.tga");
		auto tex = saba::CreateTextureFromFile(imagePath.c_str());
		EXPECT_NE(0, tex.Get());
	}

	{
		auto imagePath = saba::PathUtil::Combine(dataPath, "test.gif");
		auto tex = saba::CreateTextureFromFile(imagePath.c_str());
		EXPECT_NE(0, tex.Get());
	}

	{
		auto imagePath = saba::PathUtil::Combine(dataPath, u8"日本語.png");
		auto tex = saba::CreateTextureFromFile(imagePath.c_str());
		EXPECT_NE(0, tex.Get());
	}

	{
		auto imagePath = saba::PathUtil::Combine(dataPath, "test.dds");
		auto tex = saba::CreateTextureFromFile(imagePath.c_str());
		EXPECT_NE(0, tex.Get());
	}

	{
		auto imagePath = saba::PathUtil::Combine(dataPath, u8"日本語.dds");
		auto tex = saba::CreateTextureFromFile(imagePath.c_str());
		EXPECT_NE(0, tex.Get());
	}

	{
		auto imagePath = saba::PathUtil::Combine(dataPath, "error.png");
		auto tex = saba::CreateTextureFromFile(imagePath.c_str());
		EXPECT_EQ(0, tex.Get());
	}

	// DEBUGバージョンでは、GLI 内部のAssertで止まってしまう
#if defined(NDEBUG)
	{
		auto imagePath = saba::PathUtil::Combine(dataPath, "error.dds");
		auto tex = saba::CreateTextureFromFile(imagePath.c_str());
		EXPECT_EQ(0, tex.Get());
	}
#endif // defined(NDEBUG)
}
