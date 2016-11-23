
#if ENABLE_GLTEST

#include <GL/gl3w.h>
#include <GLFW/glfw3.h>

#endif // ENABLE_GLTEST

#include <iostream>
#include <gtest/gtest.h>

int main(int argc, char** argv)
{
	::testing::InitGoogleTest(&argc, argv);

#if ENABLE_GLTEST
	if (!glfwInit())
	{
		std::cout << "error: glfwInit fail.\n";
		return 1;
	}

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	GLFWwindow* window = glfwCreateWindow(640, 480, "Hello World", NULL, NULL);
	if (window == nullptr)
	{
		std::cout << "error: glfwCreateWindow fail.\n";
		glfwTerminate();
		return 1;
	}
	glfwMakeContextCurrent(window);
	if (gl3wInit())
	{
		std::cout << "error: glfwCreateWindow fail.\n";
		glfwDestroyWindow(window);
		glfwTerminate();
		return 1;
	}
#endif // ENABLE_GLTEST

	const int result = RUN_ALL_TESTS();

#if ENABLE_GLTEST
	glfwDestroyWindow(window);
	glfwTerminate();
#endif // ENABLE_GLTEST

	return result;
}
