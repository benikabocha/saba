#ifndef GTESTS_GL_TEST_H_
#define GTESTS_GL_TEST_H_

#include <gtest/gtest.h>

#include <GLFW/glfw3.h>

class GLTest : public testing::Test
{
public:
	GLTest();

protected:

	void SetUp() override;
	void TearDown() override;

	GLFWwindow*	m_window;
};

#endif // !GTESTS_GL_TEST_H_
