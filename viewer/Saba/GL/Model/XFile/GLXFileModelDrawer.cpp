//
// Copyright(c) 2016-2017 benikabocha.
// Distributed under the MIT License (http://opensource.org/licenses/MIT)
//

#include "GLXFileModelDrawer.h"

#include "GLXFileModel.h"
#include "GLXFileModelDrawContext.h"
#include "../../GLSLUtil.h"
#include "../../GLShaderUtil.h"
#include <Saba/Base/Log.h>

namespace saba
{
	GLXFileModelDrawer::GLXFileModelDrawer(GLXFileModelDrawContext* ctxt, std::shared_ptr<GLXFileModel> model)
		: m_drawContext(ctxt)
		, m_xfileModel(model)
	{
	}

	GLXFileModelDrawer::~GLXFileModelDrawer()
	{
		Destroy();
	}

	bool GLXFileModelDrawer::Create()
	{
		Destroy();

		size_t numMeshes = m_xfileModel->GetMeshCount();
		for (size_t i = 0; i < numMeshes; i++)
		{
			MaterialShaders meshMaterialShaders;
			const auto& mesh = m_xfileModel->GetMesh(i);
			for (const auto& mat : mesh->m_materials)
			{
				GLSLDefine define;
				bool useTex = false;

				MaterialShader matShader;
				matShader.m_shaderIndex = m_drawContext->GetShaderIndex(define);
				if (matShader.m_shaderIndex == -1)
				{
					SABA_ERROR("Faied to find XFile shader.");
					return false;
				}
				if (!matShader.m_vao.Create())
				{
					SABA_WARN("Failed to create Vertex Array Object.");
					return false;
				}

				auto shader = m_drawContext->GetShader(matShader.m_shaderIndex);

				glBindVertexArray(matShader.m_vao);

				mesh->m_posBinder.Bind(shader->m_inPos, mesh->m_posVBO);
				glEnableVertexAttribArray(shader->m_inPos);

				mesh->m_norBinder.Bind(shader->m_inNor, mesh->m_norVBO);
				glEnableVertexAttribArray(shader->m_inNor);

				if (shader->m_inUV != -1)
				{
					mesh->m_uvBinder.Bind(shader->m_inUV, mesh->m_uvVBO);
					glEnableVertexAttribArray(shader->m_inUV);
				}

				glEnable(GL_CULL_FACE);
				glCullFace(GL_BACK);

				glBindVertexArray(0);

				meshMaterialShaders.emplace_back(std::move(matShader));
			}
			m_materialShaders.emplace_back(std::move(meshMaterialShaders));
		}

		return true;
	}

	void GLXFileModelDrawer::Destroy()
	{
		m_materialShaders.clear();
	}

	void GLXFileModelDrawer::ResetAnimation(ViewerContext * ctxt)
	{
	}

	void GLXFileModelDrawer::Update(ViewerContext * ctxt)
	{
	}

	void GLXFileModelDrawer::DrawUI(ViewerContext * ctxt)
	{
	}

	void GLXFileModelDrawer::DrawShadowMap(ViewerContext * ctxt, size_t csmIdx)
	{
	}

	void GLXFileModelDrawer::Draw(ViewerContext * ctxt)
	{
		const auto& view = ctxt->GetCamera()->GetViewMatrix();
		const auto& proj = ctxt->GetCamera()->GetProjectionMatrix();

		size_t numMeshes = m_xfileModel->GetMeshCount();
		for (size_t meshIdx = 0; meshIdx < numMeshes; meshIdx++)
		{
			const auto& mesh = m_xfileModel->GetMesh(meshIdx);

			auto world = GetTransform() * mesh->m_transform;
			auto wv = view * world;
			auto wvp = proj * view * world;
			auto wvit = glm::mat3(view * world);
			wvit = glm::inverse(wvit);
			wvit = glm::transpose(wvit);
			for (const auto& subMesh : mesh->m_subMeshes)
			{
				int matID = subMesh.m_materialID;
				const auto& matShader = m_materialShaders[meshIdx][matID];
				const auto& mat = mesh->m_materials[matID];
				auto shader = m_drawContext->GetShader(matShader.m_shaderIndex);

				glUseProgram(shader->m_prog);
				glBindVertexArray(matShader.m_vao);

				SetUniform(shader->m_uWV, wv);
				SetUniform(shader->m_uWVP, wvp);
				SetUniform(shader->m_uWVIT, wvit);
				SetUniform(shader->m_uDiffuse, mat.m_diffuse);
				SetUniform(shader->m_uSpecular, mat.m_specular);
				SetUniform(shader->m_uSpecularPower, mat.m_specularPower);
				SetUniform(shader->m_uEmissive, mat.m_emissive);

				glm::vec3 lightColor = ctxt->GetLight()->GetLightColor();
				glm::vec3 lightDir = ctxt->GetLight()->GetLightDirection();
				glm::mat3 viewMat = glm::mat3(ctxt->GetCamera()->GetViewMatrix());
				lightDir = viewMat * lightDir;
				SetUniform(shader->m_uLightDir, lightDir);
				SetUniform(shader->m_uLightColor, lightColor);

				glActiveTexture(GL_TEXTURE0 + 0);
				SetUniform(shader->m_uTex, 0);
				if (mat.m_texture != 0)
				{
					glBindTexture(GL_TEXTURE_2D, mat.m_texture);
					SetUniform(shader->m_uTexMode, (GLint)1);
				}
				else
				{
					glBindTexture(GL_TEXTURE_2D, ctxt->GetDummyColorTexture());
					SetUniform(shader->m_uTexMode, (GLint)0);
				}

				glActiveTexture(GL_TEXTURE0 + 1);
				SetUniform(shader->m_uSphereTex, 1);
				if (mat.m_spTexture != 0)
				{
					glBindTexture(GL_TEXTURE_2D, mat.m_spTexture);
					SetUniform(shader->m_uSphereTexMode, (GLint)mat.m_spTextureMode);
				}
				else
				{
					glBindTexture(GL_TEXTURE_2D, ctxt->GetDummyColorTexture());
					SetUniform(shader->m_uSphereTexMode, (GLint)0);
				}

				glEnable(GL_BLEND);
				glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
				glEnable(GL_CULL_FACE);
				glCullFace(GL_BACK);

				glDrawArrays(GL_TRIANGLES, subMesh.m_beginIndex, subMesh.m_vertexCount);

				glActiveTexture(GL_TEXTURE0 + 1);
				glBindTexture(GL_TEXTURE_2D, 0);
				glActiveTexture(GL_TEXTURE0 + 0);
				glBindTexture(GL_TEXTURE_2D, 0);

				glBindVertexArray(0);
				glUseProgram(0);
			}

			glBindVertexArray(0);
			glUseProgram(0);

			glDisable(GL_BLEND);
			glDisable(GL_CULL_FACE);
		}
	}
}

