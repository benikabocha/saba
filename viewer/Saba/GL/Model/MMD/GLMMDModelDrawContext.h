//
// Copyright(c) 2016-2017 benikabocha.
// Distributed under the MIT License (http://opensource.org/licenses/MIT)
//

#ifndef SABA_GL_MODEL_MMD_GLMMDMODELDRAWCONTEXT_H_
#define SABA_GL_MODEL_MMD_GLMMDMODELDRAWCONTEXT_H_

#include <Saba/GL/GLSLUtil.h>
#include <Saba/GL/GLObject.h>

#include <memory>
#include <vector>

namespace saba
{
	class ViewerContext;

	struct GLMMDShader
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

		GLint	m_uAmbinet;
		GLint	m_uDiffuse;
		GLint	m_uSpecular;
		GLint	m_uSpecularPower;
		GLint	m_uAlpha;

		GLint	m_uTexMode;
		GLint	m_uTex;
		GLint	m_uTexMulFactor;
		GLint	m_uTexAddFactor;

		GLint	m_uSphereTexMode;
		GLint	m_uSphereTex;
		GLint	m_uSphereTexMulFactor;
		GLint	m_uSphereTexAddFactor;

		GLint	m_uToonTexMode;
		GLint	m_uToonTex;
		GLint	m_uToonTexMulFactor;
		GLint	m_uToonTexAddFactor;

		GLint	m_uLightColor;
		GLint	m_uLightDir;

		void Initialize();
	};

	struct GLMMDEdgeShader
	{
		GLSLDefine			m_define;
		GLProgramObject		m_prog;

		// attribute
		GLint	m_inPos;
		GLint	m_inNor;

		// uniform
		GLint	m_uWV;
		GLint	m_uWVP;
		GLint	m_uScreenSize;
		GLint	m_uEdgeSize;

		GLint	m_uEdgeColor;

		void Initialize();
	};

	class GLMMDModelDrawContext
	{
	public:
		GLMMDModelDrawContext(ViewerContext* ctxt);

		GLMMDModelDrawContext(const GLMMDModelDrawContext&) = delete;
		GLMMDModelDrawContext& operator = (const GLMMDModelDrawContext&) = delete;

		int GetShaderIndex(const GLSLDefine& define);
		GLMMDShader* GetShader(int shaderIndex) const;

		int GetEdgeShaderIndex(const GLSLDefine& define);
		GLMMDEdgeShader* GetEdgeShader(int edgeShaderIndex) const;

	private:
		using MMDShaderPtr = std::unique_ptr<GLMMDShader>;
		using MMDEdgeShaderPtr = std::unique_ptr<GLMMDEdgeShader>;
		ViewerContext*				m_viewerContext;
		std::vector<MMDShaderPtr>	m_shaders;
		std::vector<MMDEdgeShaderPtr>	m_edgeShaders;
	};
}

#endif // !SABA_GL_MODEL_MMD_GLMMDMODELDRAWCONTEXT_H_
