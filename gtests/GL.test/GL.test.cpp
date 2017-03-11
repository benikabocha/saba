#include <Saba/GL/GLObject.h>

#include <gtest/gtest.h>

template <typename T>
class GLObjectTypedTest : public testing::Test
{
};

typedef ::testing::Types<
	saba::GLVertexShaderObject,
	saba::GLFragmentShaderObject,
	saba::GLProgramObject,
	saba::GLBufferObject,
	saba::GLVertexArrayObject,
	saba::GLTextureObject
> TestGLObjectTypes;

TYPED_TEST_CASE(GLObjectTypedTest, TestGLObjectTypes);

TYPED_TEST(GLObjectTypedTest, Common)
{
	TypeParam glObj;

	// create
	EXPECT_EQ(0, glObj.Get());
	EXPECT_EQ(true, glObj.Create());
	EXPECT_NE(0, glObj.Get());

	// move
	{
		auto id = glObj.Get();
		TypeParam glObj2 = std::move(glObj);
		EXPECT_EQ(0, glObj.Get());
		EXPECT_EQ(id, glObj2.Get());
		glObj = std::move(glObj2);
		EXPECT_EQ(0, glObj2.Get());
		EXPECT_EQ(id, glObj.Get());
	}

	// Destroy
	glObj.Destroy();
	EXPECT_EQ(0, glObj.Get());

	// release/reset
	{
		TypeParam glObj2;
		EXPECT_EQ(true, glObj.Create());
		EXPECT_EQ(true, glObj2.Create());
		auto id = glObj.Get();
		EXPECT_TRUE(glObj.Get() != glObj2.Get());
		glObj2.Reset(glObj.Release());
		EXPECT_EQ(0, glObj.Get());
		EXPECT_EQ(id, glObj2.Get());
	}
}

template <typename T>
class GLRefTypedTest : public testing::Test
{
};

typedef ::testing::Types<
	saba::GLVertexShaderRef,
	saba::GLFragmentShaderRef,
	saba::GLProgramRef,
	saba::GLBufferRef,
	saba::GLVertexArrayRef,
	saba::GLTextureRef
> TestGLRefTypes;

TYPED_TEST_CASE(GLRefTypedTest, TestGLRefTypes);

TYPED_TEST(GLRefTypedTest, Common)
{
	TypeParam glRef;
	using GLObj = saba::GLObject<typename TypeParam::Type>;

	GLObj glObj;
	EXPECT_EQ(true, glObj.Create());
	auto id = glObj.Get();

	TypeParam glRef1 = std::move(glObj);
	EXPECT_EQ(0, glObj.Get());
	EXPECT_EQ(id, glRef1.Get());

	EXPECT_EQ(true, glObj.Create());
	TypeParam glRef2;
	EXPECT_EQ(0, glRef2.Get());
	glRef2 = std::move(glObj);
	EXPECT_TRUE(glRef1.Get() != glRef2.Get());

	glRef2 = glRef1;
	EXPECT_TRUE(glRef1.Get() == glRef2.Get());
	EXPECT_EQ(2, glRef1.GetRefCount());
	EXPECT_EQ(2, glRef2.GetRefCount());

	{
		TypeParam glRef3 = glRef1;
		EXPECT_EQ(3, glRef1.GetRefCount());
	}
	EXPECT_EQ(2, glRef1.GetRefCount());

	glRef1.Release();
	EXPECT_EQ(0, glRef1.Get());
	EXPECT_EQ(0, glRef1.GetRefCount());

	EXPECT_EQ(1, glRef2.GetRefCount());
}
