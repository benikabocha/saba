#include <Saba/GL/GLShaderUtil.h>

#include <gtest/gtest.h>

TEST(GLTest, ShaderTest)
{
	{
		const char* vsCode =
			"#version 140\n"
			"in vec3 in_Pos;\n"
			"void main()\n"
			"{\n"
			"	gl_Position = vec4(in_Pos, 1.0);\n"
			"}\n"
			;
		const char* fsCode =
			"#version 140\n"
			"out vec4 fs_Color;\n"
			"void main()\n"
			"{\n"
			"	fs_Color = vec4(1.0, 1.0, 0.0, 1.0);\n"
			"}\n";
		const char* errorCode = "test";

		// fail pattern
		{
			auto prog = saba::CreateShaderProgram(errorCode, fsCode);
			EXPECT_EQ(0, prog.Get());
		}

		// fail pattern
		{
			auto prog = saba::CreateShaderProgram(vsCode, errorCode);
			EXPECT_EQ(0, prog.Get());
		}

		// success pattern
		{
			auto prog = saba::CreateShaderProgram(vsCode, fsCode);
			EXPECT_NE(0, prog.Get());
		}

	}
}
