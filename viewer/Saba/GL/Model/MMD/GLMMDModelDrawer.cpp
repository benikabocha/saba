//
// Copyright(c) 2016-2017 benikabocha.
// Distributed under the MIT License (http://opensource.org/licenses/MIT)
//

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
		, m_clipElapsed(true)
		, m_selectedNode(nullptr)
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
			matShader.m_mmdMaterialIndex = matIdx;

			// MMD Shader
			matShader.m_mmdShaderIndex = m_drawContext->GetShaderIndex(define);
			if (matShader.m_mmdShaderIndex == -1)
			{
				SABA_ERROR("MMD Material Shader not found.");
				return false;
			}
			if (!matShader.m_mmdVao.Create())
			{
				SABA_ERROR("Vertex Array Object Create fail.");
				return false;
			}

			auto shader = m_drawContext->GetShader(matShader.m_mmdShaderIndex);

			glBindVertexArray(matShader.m_mmdVao);

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

			// MMD Edge Shaer
			matShader.m_mmdEdgeShaderIndex = m_drawContext->GetEdgeShaderIndex(define);
			if (matShader.m_mmdEdgeShaderIndex == -1)
			{
				SABA_ERROR("MMD Edge Material Shader not found.");
				return false;
			}
			if (!matShader.m_mmdEdgeVao.Create())
			{
				SABA_ERROR("Vertex Array Object Create fail.");
				return false;
			}

			auto edgeShader = m_drawContext->GetEdgeShader(matShader.m_mmdEdgeShaderIndex);

			glBindVertexArray(matShader.m_mmdEdgeVao);

			m_mmdModel->GetPositionBinder().Bind(edgeShader->m_inPos, m_mmdModel->GetPositionVBO());
			glEnableVertexAttribArray(edgeShader->m_inPos);

			m_mmdModel->GetNormalBinder().Bind(edgeShader->m_inNor, m_mmdModel->GetNormalVBO());
			glEnableVertexAttribArray(edgeShader->m_inNor);

			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_mmdModel->GetIBO());

			glBindVertexArray(0);

			// Add
			m_materialShaders.emplace_back(std::move(matShader));

			matIdx++;
		}
		return true;
	}

	void GLMMDModelDrawer::Destroy()
	{
		m_materialShaders.clear();
		m_selectedNode = nullptr;
	}

	void GLMMDModelDrawer::DrawUI(ViewerContext * ctxt)
	{
		if (ImGui::TreeNode("Bone"))
		{
			std::string name = "";
			glm::vec3 t(0);
			glm::quat q;
			if (m_selectedNode != nullptr)
			{
				name = m_selectedNode->GetName();
				t = m_selectedNode->GetAnimationTranslate();
				q = m_selectedNode->GetAnimationRotate();
			}
			ImGui::Text("name:%s", name.c_str());
			ImGui::InputFloat3("T", &t[0], ImGuiInputTextFlags_ReadOnly);
			ImGui::InputFloat4("Q", &q[0], ImGuiInputTextFlags_ReadOnly);
			auto model = m_mmdModel->GetMMDModel();
			auto nodeMan = model->GetNodeManager();
			MMDNode* clickNode = nullptr;
			for (size_t nodeIdx = 0; nodeIdx < nodeMan->GetNodeCount(); nodeIdx++)
			{
				std::function<void(MMDNode*)> ViewNodes = [&ViewNodes, &clickNode, this](saba::MMDNode* node)
				{
					ImGuiTreeNodeFlags node_flags =
						ImGuiTreeNodeFlags_OpenOnArrow |
						ImGuiTreeNodeFlags_OpenOnDoubleClick |
						((this->m_selectedNode == node) ? ImGuiTreeNodeFlags_Selected : 0);
					if (node->GetChild() != nullptr)
					{
						if (ImGui::TreeNodeEx(node->GetName().c_str(), node_flags))
						{
							auto child = node->GetChild();
							while (child != nullptr)
							{
								ViewNodes(child);
								child = child->GetNext();
							}
							ImGui::TreePop();
						}
					}
					else
					{
						node_flags |=
							ImGuiTreeNodeFlags_Leaf |
							ImGuiTreeNodeFlags_NoTreePushOnOpen;
						ImGui::TreeNodeEx(node->GetName().c_str(), node_flags);
					}
					if (clickNode == nullptr && ImGui::IsItemClicked())
					{
						clickNode = node;
					}
				};
				auto node = nodeMan->GetMMDNode(nodeIdx);
				if (node->GetParent() == nullptr)
				{
					ViewNodes(node);
				}
			}

			ImGui::TreePop();
			if (clickNode != nullptr)
			{
				m_selectedNode = clickNode;
			}
		}
		if (ImGui::TreeNode("Physics"))
		{
			bool enabledPhysics = m_mmdModel->IsEnabledPhysics();
			if (ImGui::Checkbox("Enable", &enabledPhysics))
			{
				m_mmdModel->EnablePhysics(enabledPhysics);
			}
			ImGui::TreePop();
		}
		if (ImGui::TreeNode("Morph"))
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
					auto animTime = ctxt->GetAnimationTime();
					m_mmdModel->UpdateAnimation(animTime, false);
				}
			}
			ImGui::TreePop();
		}
		if (ImGui::TreeNode("Edge"))
		{
			bool enableEdge = m_mmdModel->IsEnabledEdge();
			if (ImGui::Checkbox("Enable", &enableEdge))
			{
				m_mmdModel->EnableEdge(enableEdge);
			}
			ImGui::TreePop();
		}
	}

	void GLMMDModelDrawer::Play()
	{
	}

	void GLMMDModelDrawer::Stop()
	{
	}

	void GLMMDModelDrawer::ResetAnimation(ViewerContext * ctxt)
	{
		m_mmdModel->ResetAnimation();
	}

	void GLMMDModelDrawer::Update(ViewerContext * ctxt)
	{
		if (ctxt->GetPlayMode() != ViewerContext::PlayMode::Stop)
		{
			double animTime = ctxt->GetAnimationTime();
			m_mmdModel->UpdateAnimation(animTime);
		}
		double elapsed = ctxt->GetElapsed();
		m_mmdModel->Update(elapsed);
	}


	void GLMMDModelDrawer::Draw(ViewerContext * ctxt)
	{
		const auto& view = ctxt->GetCamera()->GetViewMatrix();
		const auto& proj = ctxt->GetCamera()->GetProjectionMatrix();

		m_world = GetTransform();
		auto wv = view * m_world;
		auto wvp = proj * view * m_world;
		auto wvit = glm::mat3(view * m_world);
		wvit = glm::inverse(wvit);
		wvit = glm::transpose(wvit);
		for (const auto& subMesh : m_mmdModel->GetSubMeshes())
		{
			int matID = subMesh.m_materialID;
			const auto& matShader = m_materialShaders[matID];
			const auto& mmdMat = m_mmdModel->GetMaterials()[matID];
			auto shader = m_drawContext->GetShader(matShader.m_mmdShaderIndex);

			if (mmdMat.m_alpha == 0.0f)
			{
				continue;
			}

			glUseProgram(shader->m_prog);
			glBindVertexArray(matShader.m_mmdVao);

			SetUniform(shader->m_uWV, wv);
			SetUniform(shader->m_uWVP, wvp);

			bool alphaBlend = true;

			SetUniform(shader->m_uAmbinet, mmdMat.m_ambient);
			SetUniform(shader->m_uDiffuse, mmdMat.m_diffuse);
			SetUniform(shader->m_uSpecular, mmdMat.m_specular);
			SetUniform(shader->m_uSpecularPower, mmdMat.m_specularPower);
			SetUniform(shader->m_uAlpha, mmdMat.m_alpha);

			glActiveTexture(GL_TEXTURE0 + 0);
			SetUniform(shader->m_uTex, (GLint)0);
			if (mmdMat.m_texture != 0)
			{
				if (!mmdMat.m_textureHaveAlpha)
				{
					// Use Material Alpha
					SetUniform(shader->m_uTexMode, (GLint)1);
				}
				else
				{
					// Use Material Alpha * Texture Alpha
					SetUniform(shader->m_uTexMode, (GLint)2);
				}
				SetUniform(shader->m_uTexMulFactor, mmdMat.m_textureMulFactor);
				SetUniform(shader->m_uTexAddFactor, mmdMat.m_textureAddFactor);
				glBindTexture(GL_TEXTURE_2D, mmdMat.m_texture);
			}
			else
			{
				SetUniform(shader->m_uTexMode, (GLint)0);
				glBindTexture(GL_TEXTURE_2D, 0);
			}

			glActiveTexture(GL_TEXTURE0 + 1);
			SetUniform(shader->m_uSphereTex, (GLint)1);
			if (mmdMat.m_spTexture != 0)
			{
				if (mmdMat.m_spTextureMode == MMDMaterial::SphereTextureMode::Mul)
				{
					SetUniform(shader->m_uSphereTexMode, (GLint)1);
				}
				else if (mmdMat.m_spTextureMode == MMDMaterial::SphereTextureMode::Add)
				{
					SetUniform(shader->m_uSphereTexMode, (GLint)2);
				}
				SetUniform(shader->m_uSphereTexMulFactor, mmdMat.m_spTextureMulFactor);
				SetUniform(shader->m_uSphereTexAddFactor, mmdMat.m_spTextureAddFactor);
				glBindTexture(GL_TEXTURE_2D, mmdMat.m_spTexture);
			}
			else
			{
				SetUniform(shader->m_uSphereTexMode, (GLint)0);
				glBindTexture(GL_TEXTURE_2D, 0);
			}

			glActiveTexture(GL_TEXTURE0 + 2);
			SetUniform(shader->m_uToonTex, 2);
			if (mmdMat.m_toonTexture != 0)
			{
				SetUniform(shader->m_uToonTexMulFactor, mmdMat.m_toonTextureMulFactor);
				SetUniform(shader->m_uToonTexAddFactor, mmdMat.m_toonTextureAddFactor);
				SetUniform(shader->m_uToonTexMode, (GLint)1);
				glBindTexture(GL_TEXTURE_2D, mmdMat.m_toonTexture);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			}
			else
			{
				SetUniform(shader->m_uToonTexMode, (GLint)0);
				glBindTexture(GL_TEXTURE_2D, 0);
			}

			glm::vec3 lightColor = ctxt->GetLight()->GetLightColor();
			glm::vec3 lightDir = ctxt->GetLight()->GetLightDirection();
			glm::mat3 viewMat = glm::mat3(ctxt->GetCamera()->GetViewMatrix());
			lightDir = viewMat * lightDir;
			SetUniform(shader->m_uLightDir, lightDir);
			SetUniform(shader->m_uLightColor, lightColor);

			if (mmdMat.m_bothFace)
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

		if (m_mmdModel->IsEnabledEdge())
		{
			glm::vec2 screenSize(ctxt->GetFrameBufferWidth(), ctxt->GetFrameBufferHeight());
			for (const auto& subMesh : m_mmdModel->GetSubMeshes())
			{
				int matID = subMesh.m_materialID;
				const auto& matShader = m_materialShaders[matID];
				const auto& mmdMat = m_mmdModel->GetMaterials()[matID];
				auto shader = m_drawContext->GetEdgeShader(matShader.m_mmdEdgeShaderIndex);

				if (!mmdMat.m_edgeFlag)
				{
					continue;
				}
				if (mmdMat.m_alpha == 0.0f)
				{
					continue;
				}

				glUseProgram(shader->m_prog);
				glBindVertexArray(matShader.m_mmdEdgeVao);

				SetUniform(shader->m_uWV, wv);
				SetUniform(shader->m_uWVP, wvp);
				SetUniform(shader->m_uScreenSize, screenSize);
				SetUniform(shader->m_uEdgeSize, mmdMat.m_edgeSize);
				SetUniform(shader->m_uEdgeColor, mmdMat.m_edgeColor);

				bool alphaBlend = true;

				glEnable(GL_CULL_FACE);
				glCullFace(GL_FRONT);

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

				glBindVertexArray(0);
				glUseProgram(0);
			}
		}

		glBindVertexArray(0);
		glUseProgram(0);

		glDisable(GL_BLEND);
		glDisable(GL_CULL_FACE);
	}
}

