#include <Saba/GL/GLSLUtil.h>

#include <gtest/gtest.h>

#define __u8(x) u8 ## x
#define _u8(x)	__u8(x)

TEST(GLTest, GLSLUtilTest)
{
	std::string vsCode =
		"#version 140\n"
		"in vec3 in_Pos;\n"
		"void main()\n"
		"{\n"
		"	gl_Position = vec4(in_Pos, 1.0);\n"
		"}\n"
		;
	std::string fsCode =
		"#version 140\n"
		"out vec4 fs_Color;\n"
		"void main()\n"
		"{\n"
		"	fs_Color = vec4(1.0, 1.0, 0.0, 1.0);\n"
		"}\n";

	// preprocess
	{
		std::string outCode;
		EXPECT_TRUE(saba::PreprocessGLSL(
			&outCode,
			saba::GLSLShaderLang::Vertex,
			vsCode
		));

		EXPECT_TRUE(saba::PreprocessGLSL(
			&outCode,
			saba::GLSLShaderLang::Fragment,
			fsCode
		));
	}

	// GLSLUtil
	{
		saba::GLSLShaderUtil shaderUtil;

		std::string dataPath = _u8(TEST_DATA_PATH);
		shaderUtil.SetShaderDir(dataPath + "/Shader");

		// missing file
		{
			auto prog = shaderUtil.CreateProgram("test0");
			EXPECT_EQ(0, prog.Get());
		}

		// success pattern
		{
			auto prog = shaderUtil.CreateProgram("test1");
			EXPECT_NE(0, prog.Get());
		}

		// include success pattern
		{
			auto prog = shaderUtil.CreateProgram("test2");
			EXPECT_NE(0, prog.Get());
		}

		// recursive include pattern
		{
			auto prog = shaderUtil.CreateProgram("test3");
			EXPECT_NE(0, prog.Get());
		}

		// include fail pattern
		{
			auto prog = shaderUtil.CreateProgram("test4");
			EXPECT_EQ(0, prog.Get());
		}

		saba::GLSLInclude include;
		include.AddInclude(dataPath + "/Shader/common");
		shaderUtil.SetGLSLInclude(include);
		// include success pattern
		{
			auto prog = shaderUtil.CreateProgram("test4");
			EXPECT_NE(0, prog.Get());
		}

		// define test
		{
			{
				auto prog = shaderUtil.CreateProgram("test5");
				EXPECT_EQ(0, prog.Get());
			}

			saba::GLSLDefine define;
			define.Define("ENABLE_CODE", "1");
			shaderUtil.SetGLSLDefine(define);
			{
				auto prog = shaderUtil.CreateProgram("test5");
				EXPECT_NE(0, prog.Get());
			}

			define.Define("ENABLE_CODE", "0");
			shaderUtil.SetGLSLDefine(define);
			{
				auto prog = shaderUtil.CreateProgram("test5");
				EXPECT_EQ(0, prog.Get());
			}
		}
	}
}
