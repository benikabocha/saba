//
// Copyright(c) 2016-2017 benikabocha.
// Distributed under the MIT License (http://opensource.org/licenses/MIT)
//

#ifndef SABA_GL_MODEL_GLOBJMODELDRAWCONTEXT_H_
#define SABA_GL_MODEL_GLOBJMODELDRAWCONTEXT_H_

#include "../../GLSLUtil.h"

#include <memory>
#include <vector>

namespace saba
{
	class ViewerContext;

	struct GLOBJShader
	{
		GLSLDefine			m_define;
		GLProgramObject		m_prog;

		// attribute
		GLint	m_inPos;
		GLint	m_inNor;
		GLint	m_inUV;

		// uniform
		GLint	m_uWV;
		GLint	m_uWVP;
		GLint	m_uWVIT;
		GLint	m_uAmbinet;
		GLint	m_uDiffuse;
		GLint	m_uSpecular;
		GLint	m_uSpecularPower;
		GLint	m_uTransparency;
		GLint	m_uLightDir;
		GLint	m_uLightColor;
		GLint	m_uAmbinetTex;
		GLint	m_uDiffuseTex;
		GLint	m_uSpecularTex;
		GLint	m_uTransparencyTex;

		void Initialize();
	};

	class GLOBJModelDrawContext
	{
	public:
		GLOBJModelDrawContext(ViewerContext* ctxt);

		GLOBJModelDrawContext(const GLOBJShader&) = delete;
		GLOBJModelDrawContext& operator = (const GLOBJShader&) = delete;

		int GetShaderIndex(const GLSLDefine& define);
		GLOBJShader* GetShader(int shaderIndex) const;

	private:
		using ObjShaderPtr = std::unique_ptr<GLOBJShader>;
		ViewerContext*				m_viewerContext;
		std::vector<ObjShaderPtr>	m_shaders;

	};
}

#endif // !SABA_GL_MODEL_GLOBJMODELDRAWCONTEXT_H_
