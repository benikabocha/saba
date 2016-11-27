#include "GLMMDModelDrawContext.h"

#include <Saba/Base/Log.h>
#include <Saba/Viewer/ViewerContext.h>

#include <algorithm>

namespace saba
{
	void GLMMDShader::Initialize()
	{
		// attribute
		m_inPos = glGetAttribLocation(m_prog, "in_Pos");
		m_inNor = glGetAttribLocation(m_prog, "in_Nor");
		m_inUV = glGetAttribLocation(m_prog, "in_UV");

		// uniform
		m_uWV = glGetUniformLocation(m_prog, "u_WV");
		m_uWVP = glGetUniformLocation(m_prog, "u_WVP");

		m_uAmbinet = glGetUniformLocation(m_prog, "u_Ambient");
		m_uDiffuse = glGetUniformLocation(m_prog, "u_Diffuse");
		m_uSpecular = glGetUniformLocation(m_prog, "u_Specular");
		m_uSpecularPower = glGetUniformLocation(m_prog, "u_SpecularPower");
		m_uAlpha = glGetUniformLocation(m_prog, "u_Alpha");

		m_uTexMode = glGetUniformLocation(m_prog, "u_TexMode");
		m_uTex = glGetUniformLocation(m_prog, "u_Tex");

		m_uSphereTexMode = glGetUniformLocation(m_prog, "u_SphereTexMode");
		m_uSphereTex = glGetUniformLocation(m_prog, "u_SphereTex");

		m_uToonTexMode = glGetUniformLocation(m_prog, "u_ToonTexMode");
		m_uToonTex = glGetUniformLocation(m_prog, "u_ToonTex");

		m_uLightColor = glGetUniformLocation(m_prog, "u_LightColor");
		m_uLightDir = glGetUniformLocation(m_prog, "u_LightDir");

	}

	GLMMDModelDrawContext::GLMMDModelDrawContext(ViewerContext * ctxt)
		: m_viewerContext(ctxt)
	{
		SABA_ASSERT(ctxt != nullptr);
	}

	int GLMMDModelDrawContext::GetShaderIndex(const GLSLDefine & define)
	{
		if (m_viewerContext == nullptr)
		{
			return -1;
		}

		auto findIt = std::find_if(
			m_shaders.begin(),
			m_shaders.end(),
			[&define](const ObjShaderPtr& shader) {return shader->m_define == define;}
		);

		if (findIt == m_shaders.end())
		{
			ObjShaderPtr shader = std::make_unique<GLMMDShader>();
			shader->m_define = std::move(define);
			GLSLShaderUtil glslShaderUtil;
			glslShaderUtil.SetShaderDir(m_viewerContext->GetShaderDir());
			glslShaderUtil.SetGLSLDefine(define);
			shader->m_prog = glslShaderUtil.CreateProgram("mmd");
			if (shader->m_prog == 0)
			{
				SABA_ERROR("Shader Create fail.");
				return -1;
			}

			shader->Initialize();
			m_shaders.emplace_back(std::move(shader));
			return (int)(m_shaders.size() - 1);
		}
		else
		{
			return (int)(findIt - m_shaders.begin());
		}
	}

	GLMMDShader* GLMMDModelDrawContext::GetShader(int shaderIndex) const
	{
		if (shaderIndex < 0)
		{
			SABA_ERROR("shaderIndex < 0");
			return nullptr;
		}

		return m_shaders[shaderIndex].get();
	}

}

