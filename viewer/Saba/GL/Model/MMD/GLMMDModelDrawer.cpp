//
// Copyright(c) 2016-2017 benikabocha.
// Distributed under the MIT License (http://opensource.org/licenses/MIT)
//

#include "GLMMDModelDrawer.h"

#include "GLMMDModelDrawContext.h"

#include <Saba/Base/Log.h>
#include <Saba/GL/GLShaderUtil.h>
#include <Saba/GL/GLTextureUtil.h>
#include <Saba/Model/MMD/MMDPhysics.h>

#include <imgui.h>

namespace saba
{
	GLMMDModelDrawer::GLMMDModelDrawer(GLMMDModelDrawContext * ctxt, std::shared_ptr<GLMMDModel> mmdModel)
		: m_drawContext(ctxt)
		, m_mmdModel(mmdModel)
		, m_clipElapsed(true)
		, m_viewLocal(true)
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

			// Shadow
			if (!matShader.m_shadowVao.Create())
			{
				SABA_ERROR("Vertex Array Object Create fail.");
				return false;
			}

			glBindVertexArray(matShader.m_shadowVao);
			auto shadowShader = m_drawContext->GetViewerContext()->GetShadowMap()->GetShader();

			m_mmdModel->GetPositionBinder().Bind(shadowShader->m_inPos, m_mmdModel->GetPositionVBO());
			glEnableVertexAttribArray(shadowShader->m_inPos);

			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_mmdModel->GetIBO());

			glBindVertexArray(0);

			// Ground Shadow
			matShader.m_mmdGroundShadowShaderIndex = m_drawContext->GetGroundShadowShaderIndex(define);
			if (matShader.m_mmdGroundShadowShaderIndex == -1)
			{
				SABA_ERROR("MMD Ground Shadow Material Shader not found.");
				return false;
			}
			if (!matShader.m_mmdGroundShadowVao.Create())
			{
				SABA_ERROR("Vertex Array Object Create fail.");
				return false;
			}

			auto groundShadowShader = m_drawContext->GetGroundShadowShader(
				matShader.m_mmdGroundShadowShaderIndex
			);

			glBindVertexArray(matShader.m_mmdGroundShadowVao);

			m_mmdModel->GetPositionBinder().Bind(edgeShader->m_inPos, m_mmdModel->GetPositionVBO());
			glEnableVertexAttribArray(groundShadowShader->m_inPos);

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
			if (m_selectedNode != nullptr)
			{
				name = m_selectedNode->GetName();
			}
			ImGui::Text("name:%s", name.c_str());

			if (ImGui::RadioButton("Local", m_viewLocal))
			{
				m_viewLocal = true;
			}
			if (ImGui::RadioButton("Global", !m_viewLocal))
			{
				m_viewLocal = false;
			}
			glm::vec3 t(0);
			glm::quat q;
			if (m_selectedNode != nullptr)
			{
				if (m_viewLocal)
				{
					auto local = m_selectedNode->GetLocalTransform();
					t = glm::vec3(local[3]);
					q = glm::quat_cast(local);
				}
				else
				{
					auto global = m_selectedNode->GetGlobalTransform();
					t = glm::vec3(global[3]);
					q = glm::quat_cast(global);
				}
			}
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
			auto physics = m_mmdModel->GetMMDModel()->GetMMDPhysics();
			float fps = physics->GetFPS();
			if (ImGui::InputFloat("FPS", &fps, 0, 0, 1))
			{
				if (1 <= fps)
				{
					physics->SetFPS(fps);
				}
			}
			int subStepCount = physics->GetMaxSubStepCount();
			if (ImGui::SliderInt("Max Sub Step", &subStepCount, 1, 100))
			{
				physics->SetMaxSubStepCount(subStepCount);
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
					m_mmdModel->UpdateMorph();
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
		if (ImGui::TreeNode("GraoundShadow"))
		{
			bool enableGroundShadow = m_mmdModel->IsEnableGroundShadow();
			if (ImGui::Checkbox("Enable", &enableGroundShadow))
			{
				m_mmdModel->EnableGroundShadow(enableGroundShadow);
			}
			ImGui::TreePop();
		}
	}

	void GLMMDModelDrawer::DrawShadowMap(ViewerContext * ctxt, size_t csmIdx)
	{
		const auto shadowMap = ctxt->GetShadowMap();
		const auto shader = shadowMap->GetShader();
		const auto& clipSpace = shadowMap->GetClipSpace(csmIdx);

		const auto& world = GetTransform();
		const auto& view = shadowMap->GetShadowViewMatrix();
		const auto& proj = clipSpace.m_projection;
		auto wvp = proj * view * world;

		glUseProgram(shader->m_prog);
		SetUniform(shader->m_uWVP, wvp);

		for (const auto& subMesh : m_mmdModel->GetSubMeshes())
		{
			int matID = subMesh.m_materialID;
			const auto& matShader = m_materialShaders[matID];
			const auto& mmdMat = m_mmdModel->GetMaterials()[matID];
			if (!mmdMat.m_shadowCaster)
			{
				continue;
			}

			glBindVertexArray(matShader.m_shadowVao);

			if (mmdMat.m_bothFace)
			{
				glDisable(GL_CULL_FACE);
			}
			else
			{
				glEnable(GL_CULL_FACE);
				glCullFace(GL_BACK);
			}

			size_t offset = subMesh.m_beginIndex * m_mmdModel->GetIndexTypeSize();
			glDrawElements(
				GL_TRIANGLES,
				subMesh.m_vertexCount,
				m_mmdModel->GetIndexType(),
				(GLvoid*)offset
			);

			glBindVertexArray(0);
		}

		glUseProgram(0);
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
		m_mmdModel->ClearPerfInfo();

		double animTime = ctxt->GetAnimationTime();
		double elapsed = ctxt->GetElapsed();
		if (ctxt->GetPlayMode() != ViewerContext::PlayMode::Stop)
		{
			m_mmdModel->UpdateAnimation(animTime, elapsed);
		}
		else
		{
			m_mmdModel->UpdateAnimationIgnoreVMD(elapsed);
		}

		m_mmdModel->Update();
	}


	void GLMMDModelDrawer::Draw(ViewerContext * ctxt)
	{
		const auto& view = ctxt->GetCamera()->GetViewMatrix();
		const auto& proj = ctxt->GetCamera()->GetProjectionMatrix();

		const auto& world = GetTransform();
		auto wv = view * world;
		auto wvp = proj * view * world;
		auto wvit = glm::mat3(view * world);
		wvit = glm::inverse(wvit);
		wvit = glm::transpose(wvit);

		const static size_t MaxShadowMap = 4;
		GLint shadowMapTexs[MaxShadowMap] = { 0 };
		glm::mat4 shadowMapVPs[MaxShadowMap];
		auto shadowMap = ctxt->GetShadowMap();
		size_t numShadowMap = glm::min(MaxShadowMap, shadowMap->GetShadowMapCount());
		const float* shadowMapSplitPositions = shadowMap->GetSplitPositions();
		size_t numShadowMapSplitPosition = glm::max(MaxShadowMap + 1, shadowMap->GetSplitPositionCount());
		if (ctxt->IsShadowEnabled())
		{
			for (size_t i = 0; i < numShadowMap; i++)
			{
				const auto& clipSpace = shadowMap->GetClipSpace(i);
				GLint texIdx = GLint(i + 3);
				glActiveTexture(GL_TEXTURE0 + texIdx);
				glBindTexture(GL_TEXTURE_2D, clipSpace.m_shadomap);

				glm::mat4 offset;
				offset[0] = glm::vec4(0.5f, 0.0f, 0.0f, 0.0f);
				offset[1] = glm::vec4(0.0f, 0.5f, 0.0f, 0.0f);
				offset[2] = glm::vec4(0.0f, 0.0f, 0.5f, 0.0f);
				offset[3] = glm::vec4(0.5f, 0.5f, 0.5f, 1.0f);
				glm::mat4 bias;
				bias[3][2] = -shadowMap->GetBias();
				shadowMapTexs[i] = texIdx;
				shadowMapVPs[i] = bias * offset * clipSpace.m_projection * shadowMap->GetShadowViewMatrix() * world;
			}
		}
		else
		{
			for (size_t i = 0; i < numShadowMap; i++)
			{
				const auto& clipSpace = shadowMap->GetClipSpace(i);
				GLint texIdx = GLint(i + 3);
				glActiveTexture(GL_TEXTURE0 + texIdx);
				glBindTexture(GL_TEXTURE_2D, ctxt->GetDummyShadowDepthTexture());
				shadowMapTexs[i] = texIdx;
			}
		}

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
				glBindTexture(GL_TEXTURE_2D, ctxt->GetDummyColorTexture());
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
				glBindTexture(GL_TEXTURE_2D, ctxt->GetDummyColorTexture());
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
				glBindTexture(GL_TEXTURE_2D, ctxt->GetDummyColorTexture());
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

			if (ctxt->IsShadowEnabled() && mmdMat.m_shadowReceiver)
			{
				SetUniform(shader->m_uShadowMapEnabled, 1);
				SetUniform(shader->m_uShadowMap0, shadowMapTexs[0]);
				SetUniform(shader->m_uShadowMap1, shadowMapTexs[1]);
				SetUniform(shader->m_uShadowMap2, shadowMapTexs[2]);
				SetUniform(shader->m_uShadowMap3, shadowMapTexs[3]);
				SetUniform(shader->m_uShadowMapSplitPositions, shadowMapSplitPositions, static_cast<GLsizei>(numShadowMapSplitPosition));
				SetUniform(shader->m_uLightVP, shadowMapVPs, static_cast<GLsizei>(numShadowMap));
			}
			else
			{
				SetUniform(shader->m_uShadowMapEnabled, 0);
				SetUniform(shader->m_uShadowMap0, shadowMapTexs[0]);
				SetUniform(shader->m_uShadowMap1, shadowMapTexs[1]);
				SetUniform(shader->m_uShadowMap2, shadowMapTexs[2]);
				SetUniform(shader->m_uShadowMap3, shadowMapTexs[3]);
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

		for (size_t i = 0; i < MaxShadowMap; i++)
		{
			GLint texIdx = GLint(i + 3);
			glActiveTexture(GL_TEXTURE0 + texIdx);
			glBindTexture(GL_TEXTURE_2D, 0);
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

		if (m_mmdModel->IsEnableGroundShadow())
		{
			glEnable(GL_POLYGON_OFFSET_FILL);
			glPolygonOffset(-1, -1);
			auto plane = glm::vec4(0, 1, 0, 0);
			auto light = -ctxt->GetLight()->GetLightDirection();
			auto shadow = glm::mat4(1);

			shadow[0][0] = plane.y * light.y + plane.z * light.z;
			shadow[0][1] = -plane.x * light.y;
			shadow[0][2] = -plane.x * light.z;
			shadow[0][3] = 0;

			shadow[1][0] = -plane.y * light.x;
			shadow[1][1] = plane.x * light.x + plane.z * light.z;
			shadow[1][2] = -plane.y * light.z;
			shadow[1][3] = 0;

			shadow[2][0] = -plane.z * light.x;
			shadow[2][1] = -plane.z * light.y;
			shadow[2][2] = plane.x * light.x + plane.y * light.y;
			shadow[2][3] = 0;

			shadow[3][0] = -plane.w * light.x;
			shadow[3][1] = -plane.w * light.y;
			shadow[3][2] = -plane.w * light.z;
			shadow[3][3] = plane.x * light.x + plane.y * light.y + plane.z * light.z;

			auto wsvp = proj * view * shadow * world;

			auto shadowColor = ctxt->GetMMDGroundShadowColor();
			if (shadowColor.a < 1.0f)
			{
				glEnable(GL_BLEND);
				glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

				glStencilFuncSeparate(GL_FRONT_AND_BACK, GL_NOTEQUAL, 1, 1);
				glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
				glEnable(GL_STENCIL_TEST);
			}
			else
			{
				glDisable(GL_BLEND);
			}
			glDisable(GL_CULL_FACE);

			for (const auto& subMesh : m_mmdModel->GetSubMeshes())
			{
				int matID = subMesh.m_materialID;
				const auto& matShader = m_materialShaders[matID];
				const auto& mmdMat = m_mmdModel->GetMaterials()[matID];
				if (!mmdMat.m_groundShadow)
				{
					continue;
				}
				if (mmdMat.m_alpha == 0.0f)
				{
					continue;
				}

				auto shader = m_drawContext->GetGroundShadowShader(matShader.m_mmdGroundShadowShaderIndex);

				glUseProgram(shader->m_prog);
				glBindVertexArray(matShader.m_mmdGroundShadowVao);

				SetUniform(shader->m_uWVP, wsvp);
				SetUniform(shader->m_uShadowColor, shadowColor);

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

			glDisable(GL_POLYGON_OFFSET_FILL);
			glDisable(GL_STENCIL_TEST);
			glDisable(GL_BLEND);
		}

		glBindVertexArray(0);
		glUseProgram(0);

		glDisable(GL_BLEND);
		glDisable(GL_CULL_FACE);
	}
}

