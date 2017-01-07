//
// Copyright(c) 2016-2017 benikabocha.
// Distributed under the MIT License (http://opensource.org/licenses/MIT)
//

#ifndef SABA_GL_GLSLUTIL_H_
#define SABA_GL_GLSLUTIL_H_

#include "GLObject.h"

#include <map>
#include <vector>
#include <string>

namespace saba
{
	class GLSLDefine
	{
	public:
		bool operator ==(const GLSLDefine& rhs) const { return m_defines == rhs.m_defines; }
		bool operator !=(const GLSLDefine& rhs) const { return m_defines != rhs.m_defines; }

		using Map = std::map<std::string, std::string>;

		void Define(const std::string& def, const std::string& defValue = "");
		void Undefine(const std::string& def);
		void Clear();

		const Map& GetMap() const { return m_defines; }

	private:
		Map	m_defines;

	};

	class GLSLInclude
	{
	public:
		GLSLInclude(const std::string& workDir = "");

		using PathList = std::vector<std::string>;

		void AddInclude(const std::string& include);
		void Clear();

		void SetWorkDir(const std::string& workDir) { m_workDir = workDir; }
		const std::string GetWorkDir() const { return m_workDir; }
		const PathList& GetPathList() const { return m_pathList; }

	private:
		std::string	m_workDir;
		PathList	m_pathList;
	};
	
	bool InitializeGLSLUtil();
	void UninitializeGLSLUtil();

	enum class GLSLShaderLang
	{
		Vertex,
		TessControl,
		TessEvaluation,
		Geometry,
		Fragment,
		Compute,
	};

	bool PreprocessGLSL(
		std::string*		outCode,
		GLSLShaderLang		lang,
		const std::string&	inCode,
		const GLSLDefine&	define = GLSLDefine(),
		const GLSLInclude&	include = GLSLInclude(),
		std::string*		outMessage = nullptr
	);

	class GLSLShaderUtil
	{
	public:
		GLSLShaderUtil() = default;

		void SetShaderDir(const std::string& shaderDir);
		void SetGLSLDefine(const GLSLDefine& define);
		void SetGLSLInclude(const GLSLInclude& include);

		GLProgramObject CreateProgram(const char* shaderName);
		GLProgramObject CreateProgram(const char* vsCode, const char* fsCode);

	private:
		std::string	m_shaderDir;
		GLSLDefine	m_define;
		GLSLInclude	m_include;
	};
}

#endif // !SABA_GL_GLSLUTIL_H_

