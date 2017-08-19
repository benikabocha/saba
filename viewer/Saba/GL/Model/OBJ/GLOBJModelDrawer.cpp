//
// Copyright(c) 2016-2017 benikabocha.
// Distributed under the MIT License (http://opensource.org/licenses/MIT)
//

#include "GLOBJModelDrawer.h"

#include "GLOBJModel.h"
#include "GLOBJModelDrawContext.h"
#include "../../GLSLUtil.h"
#include "../../GLShaderUtil.h"
#include <Saba/Base/Log.h>

namespace saba
{
	GLOBJModelDrawer::GLOBJModelDrawer(GLOBJModelDrawContext* ctxt, std::shared_ptr<GLOBJModel> model)
		: m_drawContext(ctxt)
		, m_objModel(model)
	{
	}

	GLOBJModelDrawer::~GLOBJModelDrawer()
	{
		Destroy();
	}

	bool GLOBJModelDrawer::Create()
	{
		Destroy();

		int matIdx = 0;
		for (const auto& mat : m_objModel->GetMaterials())
		{
			GLSLDefine define;
			bool useUV = false;
			if (mat.m_ambientTex != 0)
			{
				define.Define("USE_AMBIENT_TEX");
				useUV = true;
			}

			if (mat.m_diffuseTex != 0)
			{
				define.Define("USE_DIFFUSE_TEX");
				useUV = true;
			}

			if (mat.m_specularTex != 0)
			{
				define.Define("USE_SPECULAR_TEX");
				useUV = true;
			}

			if (mat.m_transparencyTex != 0)
			{
				define.Define("USE_TRANSPARENCY_TEX");
				useUV = true;
			}

			if (useUV)
			{
				define.Define("USE_UV");
				define.Define("USE_BLEND_TEX_COLOR");
			}

			MaterialShader matShader;
			matShader.m_objMaterialIndex = matIdx;
			matShader.m_objShaderIndex = m_drawContext->GetShaderIndex(define);
			if (matShader.m_objShaderIndex == -1)
			{
				SABA_ERROR("OBJ Material Shader not found");
				return false;
			}
			if (!matShader.m_vao.Create())
			{
				SABA_WARN("Vertex Array Object Create fail");
				return false;
			}

			auto objShader = m_drawContext->GetShader(matShader.m_objShaderIndex);

			glBindVertexArray(matShader.m_vao);

			m_objModel->GetPositionBinder().Bind(objShader->m_inPos, m_objModel->GetPositionVBO());
			glEnableVertexAttribArray(objShader->m_inPos);

			m_objModel->GetNormalBinder().Bind(objShader->m_inNor, m_objModel->GetNormalVBO());
			glEnableVertexAttribArray(objShader->m_inNor);

			if (objShader->m_inUV != -1)
			{
				m_objModel->GetUVBinder().Bind(objShader->m_inUV, m_objModel->GetUVVBO());
				glEnableVertexAttribArray(objShader->m_inUV);
			}

			glBindVertexArray(0);

			m_materialShaders.emplace_back(std::move(matShader));

			matIdx++;
		}

		return true;
	}

	void GLOBJModelDrawer::Destroy()
	{
		m_materialShaders.clear();
	}

	void GLOBJModelDrawer::ResetAnimation(ViewerContext * ctxt)
	{
	}

	void GLOBJModelDrawer::Update(ViewerContext * ctxt)
	{
	}

	void GLOBJModelDrawer::DrawUI(ViewerContext * ctxt)
	{
	}

	void GLOBJModelDrawer::DrawShadowMap(ViewerContext * ctxt, size_t csmIdx)
	{
	}

	void GLOBJModelDrawer::Draw(ViewerContext * ctxt)
	{
		const auto& view = ctxt->GetCamera()->GetViewMatrix();
		const auto& proj = ctxt->GetCamera()->GetProjectionMatrix();

		m_world = GetTransform();
		auto wv = view * m_world;
		auto wvp = proj * view * m_world;
		auto wvit = glm::mat3(view * m_world);
		wvit = glm::inverse(wvit);
		wvit = glm::transpose(wvit);
		for (const auto& subMesh : m_objModel->GetSubMeshes())
		{
			int matID = subMesh.m_materialID;
			const auto& matShader = m_materialShaders[matID];
			const auto& objMat = m_objModel->GetMaterials()[matID];
			auto objShader = m_drawContext->GetShader(matShader.m_objShaderIndex);

			glUseProgram(objShader->m_prog);
			glBindVertexArray(matShader.m_vao);

			SetUniform(objShader->m_uWV, wv);
			SetUniform(objShader->m_uWVP, wvp);
			SetUniform(objShader->m_uWVIT, wvit);
			SetUniform(objShader->m_uAmbinet, objMat.m_ambient);
			SetUniform(objShader->m_uDiffuse, objMat.m_diffuse);
			SetUniform(objShader->m_uSpecular, objMat.m_specular);
			SetUniform(objShader->m_uSpecularPower, objMat.m_specularPower);
			SetUniform(objShader->m_uTransparency, objMat.m_transparency);

			glm::vec3 lightColor = ctxt->GetLight()->GetLightColor();
			glm::vec3 lightDir = ctxt->GetLight()->GetLightDirection();
			glm::mat3 viewMat = glm::mat3(ctxt->GetCamera()->GetViewMatrix());
			lightDir = viewMat * lightDir;
			SetUniform(objShader->m_uLightDir, lightDir);
			SetUniform(objShader->m_uLightColor, lightColor);

			if (objMat.m_ambientTex != 0)
			{
				glActiveTexture(GL_TEXTURE0 + 0);
				glBindTexture(GL_TEXTURE_2D, objMat.m_ambientTex);
				glUniform1i(objShader->m_uAmbinetTex, 0);
			}
			if (objMat.m_diffuseTex != 0)
			{
				glActiveTexture(GL_TEXTURE0 + 1);
				glBindTexture(GL_TEXTURE_2D, objMat.m_diffuseTex);
				glUniform1i(objShader->m_uDiffuseTex, 1);
			}
			if (objMat.m_specularTex != 0)
			{
				glActiveTexture(GL_TEXTURE0 + 2);
				glBindTexture(GL_TEXTURE_2D, objMat.m_specularTex);
				glUniform1i(objShader->m_uSpecularTex, 2);
			}
			if (objMat.m_transparencyTex != 0)
			{
				glActiveTexture(GL_TEXTURE0 + 3);
				glBindTexture(GL_TEXTURE_2D, objMat.m_transparencyTex);
				glUniform1i(objShader->m_uTransparencyTex, 3);
			}

			glDrawArrays(GL_TRIANGLES, subMesh.m_beginIndex, subMesh.m_vertexCount);

			glActiveTexture(GL_TEXTURE0 + 3);
			glBindTexture(GL_TEXTURE_2D, 0);
			glActiveTexture(GL_TEXTURE0 + 2);
			glBindTexture(GL_TEXTURE_2D, 0);
			glActiveTexture(GL_TEXTURE0 + 1);
			glBindTexture(GL_TEXTURE_2D, 0);
			glActiveTexture(GL_TEXTURE0 + 0);
			glBindTexture(GL_TEXTURE_2D, 0);

			glBindVertexArray(0);
			glUseProgram(0);
		}

		glBindVertexArray(0);
		glUseProgram(0);
	}
}

