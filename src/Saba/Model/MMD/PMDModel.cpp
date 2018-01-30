//
// Copyright(c) 2016-2017 benikabocha.
// Distributed under the MIT License (http://opensource.org/licenses/MIT)
//

#include "PMDModel.h"
#include "PMDFile.h"
#include "MMDPhysics.h"

#include <Saba/Base/Path.h>
#include <Saba/Base/File.h>
#include <Saba/Base/Log.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <map>
#include <limits>
#include <algorithm>

namespace saba
{
	namespace
	{
		std::string ResolveToonTexturePath(
			const std::string mmdDataDir,
			const std::string mmdLoadDir,
			const std::string texName
		)
		{
			std::string filepath = PathUtil::Combine(mmdLoadDir, texName);

			File file;
			if (file.Open(filepath))
			{
				return filepath;
			}

			filepath = PathUtil::Combine(mmdDataDir, texName);
			if (file.Open(filepath))
			{
				return filepath;
			}

			SABA_WARN("Toon Texture File Not Found. [{}]", texName);
			return "";
		}
	}

	void PMDModel::InitializeAnimation()
	{
		ClearBaseAnimation();

		for (auto& node : (*m_nodeMan.GetNodes()))
		{
			node->SetAnimationTranslate(glm::vec3(0));
			node->SetAnimationRotate(glm::quat(1, 0, 0, 0));
		}

		for (auto& node : (*m_nodeMan.GetNodes()))
		{
			node->UpdateLocalTransform();
		}

		for (auto& morph : (*m_morphMan.GetMorphs()))
		{
			morph->SetWeight(0);
		}

		for (auto& node : (*m_nodeMan.GetNodes()))
		{
			if (node->GetParent() == nullptr)
			{
				node->UpdateGlobalTransform();
			}
		}

		for (auto& solver : (*m_ikSolverMan.GetIKSolvers()))
		{
			solver->Enable(true);
			solver->Solve();
		}

		ResetPhysics();
	}

	void PMDModel::BeginAnimation()
	{
		for (auto& node : (*m_nodeMan.GetNodes()))
		{
			node->BeginUpdateTransform();
		}
	}

	void PMDModel::EndAnimation()
	{
		for (auto& node : (*m_nodeMan.GetNodes()))
		{
			node->EndUpdateTransform();
		}
	}

	void PMDModel::UpdateMorphAnimation()
	{
	}

	void PMDModel::UpdateNodeAnimation(bool afterPhysicsAnim)
	{
		if (afterPhysicsAnim)
		{
			return;
		}

		for (auto& node : (*m_nodeMan.GetNodes()))
		{
			node->UpdateLocalTransform();
		}

		for (auto& node : (*m_nodeMan.GetNodes()))
		{
			if (node->GetParent() == nullptr)
			{
				node->UpdateGlobalTransform();
			}
		}

		for (auto& solver : (*m_ikSolverMan.GetIKSolvers()))
		{
			solver->Solve();
		}
	}

	void PMDModel::ResetPhysics()
	{
		MMDPhysicsManager* physicsMan = GetPhysicsManager();
		auto physics = physicsMan->GetMMDPhysics();

		if (physics == nullptr)
		{
			return;
		}

		auto rigidbodys = physicsMan->GetRigidBodys();
		auto joints = physicsMan->GetJoints();
		for (auto& rb : (*rigidbodys))
		{
			rb->SetActivation(false);
			rb->ResetTransform();
		}

		physics->Update(1.0f / 60.0f);

		for (auto& rb : (*rigidbodys))
		{
			rb->ReflectGlobalTransform();
		}
		for (auto& rb : (*rigidbodys))
		{
			rb->CalcLocalTransform();
		}

		for (auto& node : (*m_nodeMan.GetNodes()))
		{
			if (node->GetParent() == nullptr)
			{
				node->UpdateGlobalTransform();
			}
		}

		for (auto& rb : (*rigidbodys))
		{
			rb->Reset(physics);
		}
	}

	void PMDModel::UpdatePhysicsAnimation(float elapsed)
	{
		MMDPhysicsManager* physicsMan = GetPhysicsManager();
		auto physics = physicsMan->GetMMDPhysics();

		if (physics == nullptr)
		{
			return;
		}

		auto rigidbodys = physicsMan->GetRigidBodys();
		auto joints = physicsMan->GetJoints();
		for (auto& rb : (*rigidbodys))
		{
			rb->SetActivation(true);
		}

		physics->Update(elapsed);

		for (auto& rb : (*rigidbodys))
		{
			rb->ReflectGlobalTransform();
		}
		for (auto& rb : (*rigidbodys))
		{
			rb->CalcLocalTransform();
		}

		for (auto& node : (*m_nodeMan.GetNodes()))
		{
			if (node->GetParent() == nullptr)
			{
				node->UpdateGlobalTransform();
			}
		}
	}

	void PMDModel::Update()
	{
		const auto* position = &m_positions[0];
		const auto* normal = &m_normals[0];
		const auto* bone = &m_bones[0];
		const auto* boneWeight = &m_boneWeights[0];
		auto* updatePosition = &m_updatePositions[0];
		auto* updateNormal = &m_updateNormals[0];

		// 頂点をコピー
		auto srcPos = position;
		auto srcNor = normal;
		auto destPos = updatePosition;
		auto destNor = updateNormal;
		size_t numVertices = m_positions.size();
		for (size_t i = 0; i < numVertices; i++)
		{
			*destPos = *srcPos;
			*destNor = *srcNor;
			srcPos++;
			srcNor++;
			destPos++;
			destNor++;
		}

		// Morph の処理
		if (m_baseMorph.m_vertices.empty())
		{
			for (const auto& morph : (*m_morphMan.GetMorphs()))
			{
				float weight = morph->GetWeight();
				if (weight == 0.0f)
				{
					continue;
				}
				for (const auto& morphVtx : morph->m_vertices)
				{
					updatePosition[morphVtx.m_index] += morphVtx.m_position * weight;
				}
			}
		}
		else
		{
			for (const auto& morphVtx : m_baseMorph.m_vertices)
			{
				updatePosition[morphVtx.m_index] = morphVtx.m_position;
			}
			for (const auto& morph : (*m_morphMan.GetMorphs()))
			{
				float weight = morph->GetWeight();
				if (weight == 0.0f)
				{
					continue;
				}
				for (const auto& morphVtx : morph->m_vertices)
				{
					const auto& baseMorphVtx = m_baseMorph.m_vertices[morphVtx.m_index];
					updatePosition[baseMorphVtx.m_index] += morphVtx.m_position * weight;
				}
			}
		}

		// スキンメッシュに使用する変形マトリクスを事前計算
		auto& nodes = (*m_nodeMan.GetNodes());
		for (size_t i = 0; i < nodes.size(); i++)
		{
			m_transforms[i] = nodes[i]->GetGlobalTransform() * nodes[i]->GetInverseInitTransform();
		}

		for (size_t i = 0; i < numVertices; i++)
		{
			auto node0 = m_nodeMan.GetNode(bone->x);
			auto node1 = m_nodeMan.GetNode(bone->y);
			auto w0 = boneWeight->x;
			auto w1 = boneWeight->y;
			const auto& m0 = m_transforms[bone->x];
			const auto& m1 = m_transforms[bone->y];

			auto m = m0 * w0 + m1 * w1;
			*updatePosition = glm::vec3(m * glm::vec4(*updatePosition, 1));
			*updateNormal = glm::normalize(glm::mat3(m) * *updateNormal);

			bone++;
			boneWeight++;
			updatePosition++;
			updateNormal++;
		}
	}

	bool PMDModel::Load(const std::string& filepath, const std::string& mmdDataDir)
	{
		Destroy();

		PMDFile pmd;
		if (!ReadPMDFile(&pmd, filepath.c_str()))
		{
			return false;
		}

		std::string dirPath = PathUtil::GetDirectoryName(filepath);

		size_t vertexCount = pmd.m_vertices.size();
		m_positions.reserve(vertexCount);
		m_normals.reserve(vertexCount);
		m_uvs.reserve(vertexCount);
		m_bones.reserve(vertexCount);
		m_boneWeights.reserve(vertexCount);
		m_bboxMax = glm::vec3(-std::numeric_limits<float>::max());
		m_bboxMin = glm::vec3(std::numeric_limits<float>::max());
		for (const auto& v : pmd.m_vertices)
		{
			glm::vec3 pos = v.m_position * glm::vec3(1, 1, -1);
			glm::vec3 nor = v.m_normal * glm::vec3(1, 1, -1);
			glm::vec2 uv = glm::vec2(v.m_uv.x, 1.0f - v.m_uv.y);
			m_positions.push_back(pos);
			m_normals.push_back(nor);
			m_uvs.push_back(uv);
			m_bones.push_back(glm::ivec2(v.m_bone[0], v.m_bone[1]));
			float boneWeight = (float)v.m_boneWeight / 100.0f;
			m_boneWeights.push_back(glm::vec2(boneWeight, 1.0f - boneWeight));

			m_bboxMax = glm::max(m_bboxMax, pos);
			m_bboxMin = glm::min(m_bboxMin, pos);
		}
		m_updatePositions.resize(m_positions.size());
		m_updateNormals.resize(m_normals.size());

		m_indices.reserve(pmd.m_faces.size() * 3);
		for (const auto& face : pmd.m_faces)
		{
			for (int i = 0; i < 3; i++)
			{
				auto vi = face.m_vertices[3 - i - 1];
				m_indices.push_back(vi);
			}
		}

		std::vector<std::string> toonTextures;
		toonTextures.reserve(pmd.m_toonTextureNames.size());
		for (const auto& toonTexName : pmd.m_toonTextureNames)
		{
			std::string toonTexPath = ResolveToonTexturePath(
				mmdDataDir,
				dirPath,
				toonTexName.ToUtf8String()
				);
			toonTextures.emplace_back(std::move(toonTexPath));
		}

		// Materialをコピー
		m_materials.reserve(pmd.m_materials.size());
		m_subMeshes.reserve(pmd.m_materials.size());
		uint32_t beginIndex = 0;
		for (const auto& pmdMat : pmd.m_materials)
		{
			MMDMaterial mat;
			mat.m_diffuse = pmdMat.m_diffuse;
			mat.m_alpha = pmdMat.m_alpha;
			mat.m_specularPower = pmdMat.m_specularPower;
			mat.m_specular = pmdMat.m_specular;
			mat.m_ambient = pmdMat.m_ambient;
			mat.m_edgeFlag = pmdMat.m_edgeFlag;
			mat.m_edgeSize = pmdMat.m_edgeFlag == 0 ? 0.0f : 1.0f;
			mat.m_spTextureMode = MMDMaterial::SphereTextureMode::None;
			mat.m_bothFace = false;

			std::string orgTexName = pmdMat.m_textureName.ToUtf8String();
			std::string texName;
			std::string spTexName;
			auto asterPos = orgTexName.find_first_of('*');
			if (asterPos == std::string::npos)
			{
				std::string ext = PathUtil::GetExt(orgTexName);
				if (ext == "sph")
				{
					spTexName = orgTexName;
					mat.m_spTextureMode = MMDMaterial::SphereTextureMode::Mul;
				}
				else if (ext == "spa")
				{
					spTexName = orgTexName;
					mat.m_spTextureMode = MMDMaterial::SphereTextureMode::Add;
				}
				else
				{
					texName = orgTexName;
				}
			}
			else
			{
				texName = orgTexName.substr(0, asterPos);
				spTexName = orgTexName.substr(asterPos + 1);
				std::string ext = PathUtil::GetExt(spTexName);
				if (ext == "sph")
				{
					mat.m_spTextureMode = MMDMaterial::SphereTextureMode::Mul;
				}
				else if (ext == "spa")
				{
					mat.m_spTextureMode = MMDMaterial::SphereTextureMode::Add;
				}
			}

			if (!texName.empty())
			{
				std::string texPath = PathUtil::Combine(dirPath, texName);
				mat.m_texture = PathUtil::Normalize(texPath);
			}

			if (!spTexName.empty())
			{
				std::string spTexPath = PathUtil::Combine(dirPath, spTexName);
				mat.m_spTexture = PathUtil::Normalize(spTexPath);
			}

			if (pmdMat.m_toonIndex != 255)
			{
				mat.m_toonTexture = toonTextures[pmdMat.m_toonIndex];
			}

			m_materials.emplace_back(std::move(mat));

			MMDSubMesh subMesh;
			subMesh.m_beginIndex = beginIndex;
			subMesh.m_vertexCount = pmdMat.m_faceVertexCount;
			subMesh.m_materialID = (int)(m_materials.size() - 1);
			m_subMeshes.push_back(subMesh);

			beginIndex = beginIndex + pmdMat.m_faceVertexCount;
		}

		for (const auto& pmdMorph : pmd.m_morphs)
		{
			PMDMorph* morph = nullptr;
			if (pmdMorph.m_morphType == saba::PMDMorph::Base)
			{
				morph = &m_baseMorph;
			}
			else
			{
				morph = m_morphMan.AddMorph();
				morph->SetName(pmdMorph.m_morphName.ToUtf8String());
			}
			size_t numVtx = pmdMorph.m_vertices.size();
			morph->SetWeight(0.0f);
			morph->m_vertices.reserve(pmdMorph.m_vertices.size());
			for (const auto vtx : pmdMorph.m_vertices)
			{
				MorphVertex morphVtx;
				morphVtx.m_index = vtx.m_vertexIndex;
				morphVtx.m_position = vtx.m_position * glm::vec3(1, 1, -1);
				morph->m_vertices.push_back(morphVtx);
			}
		}

		// Nodeの作成
		m_nodeMan.GetNodes()->reserve(pmd.m_bones.size());
		for (const auto& bone : pmd.m_bones)
		{
			auto* node = m_nodeMan.AddNode();
			node->SetName(bone.m_boneName.ToUtf8String());
		}
		for (size_t i = 0; i < pmd.m_bones.size(); i++)
		{
			const auto& bone = pmd.m_bones[i];
			auto* node = m_nodeMan.GetNode(i);
			if (bone.m_parent != 0xFFFF)
			{
				const auto& parentBone = pmd.m_bones[bone.m_parent];
				auto* parentNode = m_nodeMan.GetNode(bone.m_parent);
				parentNode->AddChild(node);
				glm::vec3 localPos = bone.m_position - parentBone.m_position;
				localPos.z *= -1;

				node->SetTranslate(localPos);
			}
			else
			{
				glm::vec3 localPos = bone.m_position;
				localPos.z *= -1;

				node->SetTranslate(localPos);
			}
			glm::mat4 init = glm::translate(
				glm::mat4(1),
				bone.m_position * glm::vec3(1, 1, -1)
			);
			node->SetGlobalTransform(init);
			node->CalculateInverseInitTransform();
			node->SaveInitialTRS();
		}
		m_transforms.resize(m_nodeMan.GetNodeCount());

		// IKを作成
		m_ikSolverMan.GetIKSolvers()->reserve(pmd.m_iks.size());
		for (const auto& ik : pmd.m_iks)
		{
			auto solver = m_ikSolverMan.AddIKSolver();
			auto* ikNode = m_nodeMan.GetNode(ik.m_ikNode);
			solver->SetIKNode(ikNode);

			auto* targetNode = m_nodeMan.GetNode(ik.m_ikTarget);
			solver->SetTargetNode(targetNode);

			for (const auto& chain : ik.m_chanins)
			{
				auto* chainNode = m_nodeMan.GetNode(chain);
				auto findPos = chainNode->GetName().find(u8"ひざ");
				bool isKnee = false;
				if (findPos != std::string::npos)
				{
					isKnee = true;
				}
				solver->AddIKChain(chainNode, isKnee);
				chainNode->EnableIK(true);
			}

			solver->SetIterateCount(ik.m_numIteration);
			solver->SetLimitAngle(ik.m_rotateLimit * 4.0f);
		}

		if (!m_physicsMan.Create())
		{
			SABA_ERROR("Create Physics Fail.");
			return false;
		}

		for (const auto& pmdRB : pmd.m_rigidBodies)
		{
			auto rb = m_physicsMan.AddRigidBody();
			MMDNode* node = nullptr;
			if (pmdRB.m_boneIndex != 0xFFFF)
			{
				node = m_nodeMan.GetMMDNode(pmdRB.m_boneIndex);
			}
			if (!rb->Create(pmdRB, this, node))
			{
				SABA_ERROR("Create Rigid Body Fail.\n");
				return false;
			}
			m_physicsMan.GetMMDPhysics()->AddRigidBody(rb);
		}

		for (const auto& pmdJoint : pmd.m_joints)
		{
			if (pmdJoint.m_rigidBodyA != -1 &&
				pmdJoint.m_rigidBodyB != -1 &&
				pmdJoint.m_rigidBodyA != pmdJoint.m_rigidBodyB)
			{
				auto joint = m_physicsMan.AddJoint();
				MMDNode* node = nullptr;
				auto rigidBodys = m_physicsMan.GetRigidBodys();
				bool ret = joint->CreateJoint(
					pmdJoint,
					(*rigidBodys)[pmdJoint.m_rigidBodyA].get(),
					(*rigidBodys)[pmdJoint.m_rigidBodyB].get()
				);
				if (!ret)
				{
					SABA_ERROR("Create Joint Fail.\n");
					return false;
				}
				m_physicsMan.GetMMDPhysics()->AddJoint(joint);
			}
			else
			{
				SABA_WARN("Illegal Joint [{}]", pmdJoint.m_jointName.ToUtf8String());
			}
		}

		ResetPhysics();

		return true;
	}

	void PMDModel::Destroy()
	{
		m_materials.clear();
		m_subMeshes.clear();

		m_positions.clear();
		m_normals.clear();
		m_uvs.clear();
		m_bones.clear();
		m_boneWeights.clear();

		m_indices.clear();

		m_nodeMan.GetNodes()->clear();
	}

}
