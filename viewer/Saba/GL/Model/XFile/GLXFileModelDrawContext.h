//
// Copyright(c) 2016-2017 benikabocha.
// Distributed under the MIT License (http://opensource.org/licenses/MIT)
//

#ifndef SABA_GL_MODEL_GLXFILEMODELDRAWCONTEXT_H_
#define SABA_GL_MODEL_GLXFILEMODELDRAWCONTEXT_H_

#include "../../GLSLUtil.h"

#include <memory>
#include <vector>

namespace saba
{
	class ViewerContext;

	struct GLXFileShader
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
		GLint	m_uDiffuse;
		GLint	m_uSpecular;
		GLint	m_uSpecularPower;
		GLint	m_uEmissive;
		GLint	m_uLightDir;
		GLint	m_uLightColor;
		GLint	m_uTexMode;
		GLint	m_uTex;
		GLint	m_uSphereTexMode;
		GLint	m_uSphereTex;

		void Initialize();
	};

	class GLXFileModelDrawContext
	{
	public:
		GLXFileModelDrawContext(ViewerContext* ctxt);

		GLXFileModelDrawContext(const GLXFileShader&) = delete;
		GLXFileModelDrawContext& operator = (const GLXFileShader&) = delete;

		int GetShaderIndex(const GLSLDefine& define);
		GLXFileShader* GetShader(int shaderIndex) const;

	private:
		using XFileShaderPtr = std::unique_ptr<GLXFileShader>;
		ViewerContext*				m_viewerContext;
		std::vector<XFileShaderPtr>	m_shaders;

	};
}

#endif // !SABA_GL_MODEL_GLXFILEMODELDRAWCONTEXT_H_
