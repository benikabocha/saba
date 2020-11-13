//
// Copyright(c) 2016-2017 benikabocha.
// Distributed under the MIT License (http://opensource.org/licenses/MIT)
//

#include "GLShaderUtil.h"
#include "GLObject.h"

#include <iostream>
#include <fstream>
#include <vector>

#include <glm/glm.hpp>

namespace saba
{
	namespace
	{
		const char* GetShaderString(GLenum shaderType)
		{
			switch (shaderType)
			{
			case GL_COMPUTE_SHADER:
				return "Compute Shader";
			case GL_VERTEX_SHADER:
				return "Vertex Shader";
			case GL_TESS_CONTROL_SHADER:
				return "Tess Control Shader";
			case GL_TESS_EVALUATION_SHADER:
				return "Tess Evaluation Shader";
			case GL_GEOMETRY_SHADER:
				return "Geometry Shader";
			case GL_FRAGMENT_SHADER:
				return "Fragment Shader";
			default:
				return "Unknown Shader Type";
			}
		}

		template <GLenum ShaderType>
		GLShaderObject<ShaderType> CreateShader(const char* code)
		{
			std::cout << "Create: " << GetShaderString(ShaderType) << "\n";
			GLShaderObject<ShaderType> shader;
			if (shader.Create() == 0)
			{
				return GLShaderObject<ShaderType>();
			}

			const char* codeArray[] = { code };
			glShaderSource(shader, 1, codeArray, nullptr);

			glCompileShader(shader);

			GLint infoLength;
			glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLength);
			if (infoLength != 0)
			{
				std::vector<char> info;
				info.reserve(size_t(infoLength) + 1);
				info.resize(infoLength);

				GLsizei len;
				glGetShaderInfoLog(shader, infoLength, &len, &info[0]);
				if (info[size_t(infoLength) - 1] != '\0')
				{
					info.push_back('\0');
				}

				std::cout << &info[0] << "\n";
			}

			GLint compileStatus;
			glGetShaderiv(shader, GL_COMPILE_STATUS, &compileStatus);
			if (compileStatus != GL_TRUE)
			{
				glDeleteShader(shader);
				return GLShaderObject<ShaderType>();
			}

			std::cout << "Success: " << GetShaderString(ShaderType) << "\n";

			return shader;
		}
	}

	GLProgramObject CreateShaderProgram(const char * vsCode, const char * fsCode)
	{
		GLVertexShaderObject vs(CreateShader<GL_VERTEX_SHADER>(vsCode));
		if (vs == 0)
		{
			return GLProgramObject();
		}

		GLFragmentShaderObject fs(CreateShader<GL_FRAGMENT_SHADER>(fsCode));
		if (fs == 0)
		{
			return GLProgramObject();
		}

		GLProgramObject prog;
		prog.Create();

		std::cout << "Start: Shader Program Link\n";

		glAttachShader(prog, vs);
		glAttachShader(prog, fs);
		glLinkProgram(prog);

		GLint infoLength;
		glGetProgramiv(prog, GL_INFO_LOG_LENGTH, &infoLength);
		if (infoLength != 0)
		{
			std::vector<char> info;
			info.reserve(size_t(infoLength) + 1);
			info.resize(infoLength);

			GLsizei len;
			glGetProgramInfoLog(prog, infoLength, &len, &info[0]);
			if (info[size_t(infoLength) - 1] != '\0')
			{
				info.push_back('\0');
			}

			std::cout << &info[0] << "\n";
		}

		GLint linkStatus;
		glGetProgramiv(prog, GL_LINK_STATUS, &linkStatus);

		if (linkStatus != GL_TRUE)
		{
			return GLProgramObject();
		}

		std::cout << "Success: Shader Program Link\n";

		return prog;
	}
	void SetUniform(GLint uniform, GLint value)
	{
		glUniform1i(uniform, value);
	}

	void SetUniform(GLint uniform, float value)
	{
		glUniform1f(uniform, value);
	}

	void SetUniform(GLint uniform, const glm::vec2& value)
	{
		glUniform2fv(uniform, 1, &value[0]);
	}

	void SetUniform(GLint uniform, const glm::vec3& value)
	{
		glUniform3fv(uniform, 1, &value[0]);
	}

	void SetUniform(GLint uniform, const glm::vec4& value)
	{
		glUniform4fv(uniform, 1, &value[0]);
	}

	void SetUniform(GLint uniform, const glm::mat3& value)
	{
		glUniformMatrix3fv(uniform, 1, GL_FALSE, &value[0][0]);
	}

	void SetUniform(GLint uniform, const glm::mat4& value)
	{
		glUniformMatrix4fv(uniform, 1, GL_FALSE, &value[0][0]);
	}

	void SetUniform(GLint uniform, const GLint * values, GLsizei count)
	{
		glUniform1iv(uniform, count, values);
	}

	void SetUniform(GLint uniform, const float * values, GLsizei count)
	{
		glUniform1fv(uniform, count, values);
	}

	void SetUniform(GLint uniform, const glm::mat4 * values, GLsizei count)
	{
		glUniformMatrix4fv(uniform, count, GL_FALSE, &values[0][0][0]);
	}
}
