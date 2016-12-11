#include "GLMMDModelDrawer.h"

#include "GLMMDModelDrawContext.h"

#include <Saba/Base/Log.h>
#include <Saba/GL/GLShaderUtil.h>
#include <Saba/GL/GLTextureUtil.h>

#include <imgui.h>

namespace saba
{
	GLMMDModelDrawer::GLMMDModelDrawer(GLMMDModelDrawContext * ctxt, std::shared_ptr<GLMMDModel> mmdModel)
		: m_drawContext(ctxt)
		, m_mmdModel(mmdModel)
		, m_playMode(PlayMode::None)
		, m_clipElapsed(true)
	{
		SABA_ASSERT(ctxt != nullptr);
		SABA_ASSERT(mmdModel != nullptr);
	}

	GLMMDModelDrawer::~GLMMDModelDrawer()
	{
		Destroy();
	}

	bool GLMMDModelDrawer::Create()
	{
		int matIdx = 0;
		for (const auto& mat : m_mmdModel->GetMaterials())
		{
			GLSLDefine define;

			MaterialShader matShader;
			matShader.m_objMaterialIndex = matIdx;
			matShader.m_objShaderIndex = m_drawContext->GetShaderIndex(define);
			if (matShader.m_objShaderIndex == -1)
			{
				SABA_ERROR("Obj Material Shader not found.");
				return false;
			}
			if (!matShader.m_vao.Create())
			{
				SABA_ERROR("Vertex Array Object Create fail.");
				return false;
			}

			auto shader = m_drawContext->GetShader(matShader.m_objShaderIndex);

			glBindVertexArray(matShader.m_vao);

			m_mmdModel->GetPositionBinder().Bind(shader->m_inPos, m_mmdModel->GetPositionVBO());
			glEnableVertexAttribArray(shader->m_inPos);

			m_mmdModel->GetNormalBinder().Bind(shader->m_inNor, m_mmdModel->GetNormalVBO());
			glEnableVertexAttribArray(shader->m_inNor);

			if (shader->m_inUV != -1)
			{
				m_mmdModel->GetUVBinder().Bind(shader->m_inUV, m_mmdModel->GetUVVBO());
				glEnableVertexAttribArray(shader->m_inUV);
			}

			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_mmdModel->GetIBO());

			glBindVertexArray(0);

			m_materialShaders.emplace_back(std::move(matShader));

			matIdx++;
		}
		m_playMode = PlayMode::Stop;
		return true;
	}

	void GLMMDModelDrawer::Destroy()
	{
		m_materialShaders.clear();
	}

	void GLMMDModelDrawer::DrawUI(ViewerContext * ctxt)
	{
		ImGui::Begin("MMDDrawCtrl");
		if (ImGui::CollapsingHeader("Animation"))
		{
			ImGui::Checkbox("Clip Elapsed", &m_clipElapsed);
			float animFrame = (float)(m_mmdModel->GetAnimationTime() * 30.0);
			if (ImGui::InputFloat("Frame", &animFrame))
			{
				m_mmdModel->SetAnimationTime(animFrame / 30.0);
				m_playMode = PlayMode::Update;
			}

			if (m_playMode == PlayMode::Play)
			{
				if (ImGui::Button("Stop"))
				{
					m_playMode = PlayMode::Stop;
				}
			}
			else
			{
				if (ImGui::Button("Play"))
				{
					m_playMode = PlayMode::Play;
				}
			}
			if (ImGui::Button("Step FF"))
			{
				m_playMode = PlayMode::StepFF;
			}
			if (ImGui::Button("Step FR"))
			{
				m_playMode = PlayMode::StepFR;
			}
		}
		if (ImGui::CollapsingHeader("Morph"))
		{
			auto model = m_mmdModel->GetMMDModel();
			auto morphMan = model->GetMorphManager();
			size_t morphCount = morphMan->GetMorphCount();
			for (size_t morphIdx = 0; morphIdx < morphCount; morphIdx++)
			{
				auto morph = morphMan->GetMorph(morphIdx);
				float weight = morph->GetWeight();
				if (ImGui::SliderFloat(morph->GetName().c_str(), &weight, 0.0f, 1.0f))
				{
					morph->SetWeight(weight);
					m_playMode = PlayMode::Update;
				}
			}
		}
		ImGui::End();
	}

	void GLMMDModelDrawer::Update(ViewerContext * ctxt)
	{
		double elapsed = ctxt->GetElapsed();
		float animFrame = (float)(m_mmdModel->GetAnimationTime() * 30.0);
		if (m_clipElapsed)
		{
			if (elapsed > 1.0f / 30.0f)
			{
				elapsed = 1.0f / 30.0f;
			}
		}

		switch (m_playMode)
		{
		case PlayMode::None:
			break;
		case PlayMode::Play:
			m_mmdModel->Update(elapsed);
			break;
		case PlayMode::Stop:
			break;
		case PlayMode::Update:
			m_mmdModel->Update(0.0f);
			m_playMode = PlayMode::Stop;
			break;
		case PlayMode::StepFF:
			m_mmdModel->Update(1.0f / 30.0f);
			m_playMode = PlayMode::Stop;
			break;
		case PlayMode::StepFR:
			m_mmdModel->Update(-1.0f / 30.0f);
			m_playMode = PlayMode::Stop;
			break;
		default:
			break;
		}
	}


	void GLMMDModelDrawer::Draw(ViewerContext * ctxt)
	{
		const auto& view = ctxt->GetCamera()->GetViewMatrix();
		const auto& proj = ctxt->GetCamera()->GetProjectionMatrix();

		auto wv = view * m_world;
		auto wvp = proj * view * m_world;
		auto wvit = glm::mat3(view * m_world);
		wvit = glm::inverse(wvit);
		wvit = glm::transpose(wvit);
		for (const auto& subMesh : m_mmdModel->GetSubMeshes())
		{
			int matID = subMesh.m_materialID;
			const auto& matShader = m_materialShaders[matID];
			const auto& pmdMat = m_mmdModel->GetMaterials()[matID];
			auto shader = m_drawContext->GetShader(matShader.m_objShaderIndex);

			if (pmdMat.m_alpha == 0.0f)
			{
				continue;
			}

			glUseProgram(shader->m_prog);
			glBindVertexArray(matShader.m_vao);

			SetUniform(shader->m_uWV, wv);
			SetUniform(shader->m_uWVP, wvp);

			bool alphaBlend = true;

			SetUniform(shader->m_uAmbinet, pmdMat.m_ambient);
			SetUniform(shader->m_uDiffuse, pmdMat.m_diffuse);
			SetUniform(shader->m_uSpecular, pmdMat.m_specular);
			SetUniform(shader->m_uSpecularPower, pmdMat.m_specularPower);
			SetUniform(shader->m_uAlpha, pmdMat.m_alpha);

			glActiveTexture(GL_TEXTURE0 + 0);
			SetUniform(shader->m_uTex, (GLint)0);
			if (pmdMat.m_texture != 0)
			{
				if (!IsAlphaTexture(pmdMat.m_texture))
				{
					// Use Material Alpha
					SetUniform(shader->m_uTexMode, (GLint)1);
				}
				else
				{
					// Use Material Alpha * Texture Alpha
					SetUniform(shader->m_uTexMode, (GLint)2);
				}
				glBindTexture(GL_TEXTURE_2D, pmdMat.m_texture);
			}
			else
			{
				SetUniform(shader->m_uTexMode, (GLint)0);
				glBindTexture(GL_TEXTURE_2D, 0);
			}

			glActiveTexture(GL_TEXTURE0 + 1);
			SetUniform(shader->m_uSphereTex, (GLint)1);
			if (pmdMat.m_spTexture != 0)
			{
				if (pmdMat.m_spTextureMode == MMDMaterial::SphereTextureMode::Mul)
				{
					SetUniform(shader->m_uSphereTexMode, (GLint)1);
				}
				else if (pmdMat.m_spTextureMode == MMDMaterial::SphereTextureMode::Add)
				{
					SetUniform(shader->m_uSphereTexMode, (GLint)2);
				}
				glBindTexture(GL_TEXTURE_2D, pmdMat.m_spTexture);
			}
			else
			{
				SetUniform(shader->m_uSphereTexMode, (GLint)0);
				glBindTexture(GL_TEXTURE_2D, 0);
			}

			glActiveTexture(GL_TEXTURE0 + 2);
			SetUniform(shader->m_uToonTex, 2);
			if (pmdMat.m_toonTexture != 0)
			{
				SetUniform(shader->m_uToonTexMode, (GLint)1);
				glBindTexture(GL_TEXTURE_2D, pmdMat.m_toonTexture);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			}
			else
			{
				SetUniform(shader->m_uToonTexMode, (GLint)0);
				glBindTexture(GL_TEXTURE_2D, 0);
			}

			SetUniform(shader->m_uLightDir, glm::vec3(0, 0, 1));
			SetUniform(shader->m_uLightColor, glm::vec3(1, 1, 1));

			if (pmdMat.m_bothFace)
			{
				glDisable(GL_CULL_FACE);
			}
			else
			{
				glEnable(GL_CULL_FACE);
				glCullFace(GL_BACK);
			}

			if (alphaBlend)
			{
				glEnable(GL_BLEND);
				glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			}
			else
			{
				glDisable(GL_BLEND);
			}

			size_t offset = subMesh.m_beginIndex * m_mmdModel->GetIndexTypeSize();
			glDrawElements(
				GL_TRIANGLES,
				subMesh.m_vertexCount,
				m_mmdModel->GetIndexType(),
				(GLvoid*)offset
			);

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

		glDisable(GL_BLEND);
		glDisable(GL_CULL_FACE);
	}
}

