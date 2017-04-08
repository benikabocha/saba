//
// Copyright(c) 2016-2017 benikabocha.
// Distributed under the MIT License (http://opensource.org/licenses/MIT)
//

#include "GLOBJModelDrawContext.h"

#include <Saba/Base/Log.h>
#include "../../../Viewer/ViewerContext.h"

#include <algorithm>

namespace saba
{
	void GLOBJShader::Initialize()
	{
		// attribute
		m_inPos = glGetAttribLocation(m_prog, "in_Pos");
		m_inNor = glGetAttribLocation(m_prog, "in_Nor");
		m_inUV = glGetAttribLocation(m_prog, "in_UV");

		// uniform
		m_uWV = glGetUniformLocation(m_prog, "u_WV");
		m_uWVP = glGetUniformLocation(m_prog, "u_WVP");
		m_uWVIT = glGetUniformLocation(m_prog, "u_WVIT");
		m_uAmbinet = glGetUniformLocation(m_prog, "u_Ambient");
		m_uDiffuse = glGetUniformLocation(m_prog, "u_Diffuse");
		m_uSpecular = glGetUniformLocation(m_prog, "u_Specular");
		m_uSpecularPower = glGetUniformLocation(m_prog, "u_SpecularPower");
		m_uLightDir = glGetUniformLocation(m_prog, "u_LightDir");
		m_uLightColor = glGetUniformLocation(m_prog, "u_LightColor");
		m_uTransparency = glGetUniformLocation(m_prog, "u_Transparency");
		m_uAmbinetTex = glGetUniformLocation(m_prog, "u_AmbientTex");
		m_uDiffuseTex = glGetUniformLocation(m_prog, "u_DiffuseTex");
		m_uSpecularTex = glGetUniformLocation(m_prog, "u_SpecularTex");
		m_uTransparencyTex = glGetUniformLocation(m_prog, "u_TransparencyTex");
	}

	GLOBJModelDrawContext::GLOBJModelDrawContext(ViewerContext* ctxt)
		: m_viewerContext(ctxt)
	{
	}

	int GLOBJModelDrawContext::GetShaderIndex(const GLSLDefine & define)
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
			GLSLShaderUtil glslShaderUtil;
			glslShaderUtil.SetShaderDir(m_viewerContext->GetShaderDir());

			ObjShaderPtr shader = std::make_unique<GLOBJShader>();
			shader->m_define = std::move(define);
			glslShaderUtil.SetGLSLDefine(shader->m_define);
			shader->m_prog = glslShaderUtil.CreateProgram("obj_shader");
			if (shader->m_prog == 0)
			{
				SABA_WARN("Shader Create fail.");
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

	GLOBJShader* GLOBJModelDrawContext::GetShader(int shaderIndex) const
	{
		if (shaderIndex < 0)
		{
			SABA_ERROR("shaderIndex < 0");
			return nullptr;
		}

		return m_shaders[shaderIndex].get();
	}
}
