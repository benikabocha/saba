#include "GL.test.h"


GLTest::GLTest()
	: m_window(nullptr)
{
}

void GLTest::SetUp()
{
	if (!glfwInit())
	{
		return;
	}
	m_window = glfwCreateWindow(640, 480, "Hello World", NULL, NULL);
	if (m_window == nullptr)
	{
		glfwTerminate();
		return;
	}
	glfwMakeContextCurrent(m_window);
}

void GLTest::TearDown()
{
	if (m_window != nullptr)
	{
		glfwDestroyWindow(m_window);
		m_window = nullptr;
		glfwTerminate();
	}
}

TEST_F(GLTest, Initialize)
{
	EXPECT_NE(nullptr, m_window);

	GLuint tex;
	glGenTextures(1, &tex);
	EXPECT_NE(0, tex);
	glDeleteTextures(1, &tex);
}
