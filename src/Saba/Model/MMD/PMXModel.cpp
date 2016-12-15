#include "PMXModel.h"

#include "PMXFile.h"
#include "MMDPhysics.h"

#include <Saba/Base/Path.h>
#include <Saba/Base/File.h>
#include <Saba/Base/Log.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/matrix_decompose.hpp>
#include <map>
#include <limits>
#include <algorithm>
#include <sstream>
#include <iomanip>

namespace saba
{
	void PMXModel::InitializeAnimation()
	{
		for (auto& node : (*m_nodeMan.GetNodes()))
		{
			node->UpdateLocalTransform();
		}

		for (const auto& node : (*m_nodeMan.GetNodes()))
		{
			if (node->GetParent() == nullptr)
			{
				node->UpdateGlobalTransform();
			}
		}

		for (auto pmxNode : m_sortedNodes)
		{
			if (pmxNode->GetAppendNode() != nullptr)
			{
				pmxNode->UpdateAppendTransform();
				pmxNode->UpdateGlobalTransform();
			}
			if (pmxNode->GetIKSolver() != nullptr)
			{
				auto ikSolver = pmxNode->GetIKSolver();
				ikSolver->Solve();
				pmxNode->UpdateGlobalTransform();
			}
		}

		for (const auto& node : (*m_nodeMan.GetNodes()))
		{
			if (node->GetParent() == nullptr)
			{
				node->UpdateGlobalTransform();
			}
		}

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
		}

		for (auto& rb : (*rigidbodys))
		{
			rb->BeginUpdate();
		}

		physics->Update(1.0f / 60.0f);

		for (auto& rb : (*rigidbodys))
		{
			rb->EndUpdate();
		}

		for (auto& rb : (*rigidbodys))
		{
			rb->Reset(physics);
		}
	}

	void PMXModel::BeginAnimation()
	{
		for (auto& node : (*m_nodeMan.GetNodes()))
		{
			node->BeginUpateTransform();
		}
	}

	void PMXModel::EndAnimation()
	{
		for (auto& node : (*m_nodeMan.GetNodes()))
		{
			node->EndUpateTransform();
		}
	}

	void PMXModel::UpdateAnimation(float elapsed)
	{
		for (auto& node : (*m_nodeMan.GetNodes()))
		{
			node->UpdateLocalTransform();
		}

		for (const auto& node : (*m_nodeMan.GetNodes()))
		{
			if (node->GetParent() == nullptr)
			{
				node->UpdateGlobalTransform();
			}
		}

		for (auto pmxNode : m_sortedNodes)
		{
			if (pmxNode->GetAppendNode() != nullptr)
			{
				pmxNode->UpdateAppendTransform();
				pmxNode->UpdateGlobalTransform();
			}
			if (pmxNode->GetIKSolver() != nullptr)
			{
				auto ikSolver = pmxNode->GetIKSolver();
				ikSolver->Solve();
				pmxNode->UpdateGlobalTransform();
			}
		}

		for (const auto& node : (*m_nodeMan.GetNodes()))
		{
			if (node->GetParent() == nullptr)
			{
				node->UpdateGlobalTransform();
			}
		}

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

		for (auto& rb : (*rigidbodys))
		{
			rb->BeginUpdate();
		}

		physics->Update(elapsed);

		for (auto& rb : (*rigidbodys))
		{
			rb->EndUpdate();
		}
	}

	void PMXModel::Update(float elapsed)
	{
		const auto* position = m_positions.data();
		const auto* normal = m_normals.data();
		const auto* vtxInfo = m_vertexBoneInfos.data();
		auto* updatePosition = m_updatePositions.data();
		auto* updateNormal = m_updateNormals.data();

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
		BeginMorphMaterial();
		for (const auto& morph : (*m_morphMan.GetMorphs()))
		{
			Morph(morph.get(), morph->GetWeight());
		}
		EndMorphMaterial();

		for (size_t i = 0; i < numVertices; i++)
		{
			glm::mat4 m;
			switch (vtxInfo->m_skinningType)
			{
			case SkinningType::Weight1:
			{
				auto node0 = m_nodeMan.GetNode(vtxInfo->m_boneIndex.x);
				auto m0 = node0->GetGlobalTransform() * node0->GetInverseInitTransform();
				m = m0;
				break;
			}
			case SkinningType::Weight2:
			{
				auto node0 = m_nodeMan.GetNode(vtxInfo->m_boneIndex.x);
				auto node1 = m_nodeMan.GetNode(vtxInfo->m_boneIndex.y);
				auto w0 = vtxInfo->m_boneWeight.x;
				auto w1 = vtxInfo->m_boneWeight.y;
				auto m0 = node0->GetGlobalTransform() * node0->GetInverseInitTransform();
				auto m1 = node1->GetGlobalTransform() * node1->GetInverseInitTransform();
				m = m0 * w0 + m1 * w1;
				break;
			}
			case SkinningType::Weight4:
			{
				auto node0 = m_nodeMan.GetNode(vtxInfo->m_boneIndex.x);
				auto node1 = m_nodeMan.GetNode(vtxInfo->m_boneIndex.y);
				auto node2 = m_nodeMan.GetNode(vtxInfo->m_boneIndex.z);
				auto node3 = m_nodeMan.GetNode(vtxInfo->m_boneIndex.w);
				auto w0 = vtxInfo->m_boneWeight.x;
				auto w1 = vtxInfo->m_boneWeight.y;
				auto w2 = vtxInfo->m_boneWeight.z;
				auto w3 = vtxInfo->m_boneWeight.w;
				auto m0 = node0->GetGlobalTransform() * node0->GetInverseInitTransform();
				auto m1 = node1->GetGlobalTransform() * node1->GetInverseInitTransform();
				auto m2 = node2->GetGlobalTransform() * node2->GetInverseInitTransform();
				auto m3 = node3->GetGlobalTransform() * node3->GetInverseInitTransform();
				m = m0 * w0 + m1 * w1 + m2 * w2 + m3 * w3;
				break;
			}
			default:
				break;
			}

			*updatePosition = glm::vec3(m * glm::vec4(*updatePosition, 1));
			*updateNormal = glm::normalize(glm::mat3(m) * *updateNormal);

			vtxInfo++;
			updatePosition++;
			updateNormal++;
		}
	}

	bool PMXModel::Load(const std::string& filepath, const std::string& mmdDataDir)
	{
		Destroy();

		PMXFile pmx;
		if (!ReadPMXFile(&pmx, filepath.c_str()))
		{
			return false;
		}

		std::string dirPath = PathUtil::GetDirectoryName(filepath);

		size_t vertexCount = pmx.m_vertices.size();
		m_positions.reserve(vertexCount);
		m_normals.reserve(vertexCount);
		m_uvs.reserve(vertexCount);
		m_vertexBoneInfos.reserve(vertexCount);
		m_bboxMax = glm::vec3(-std::numeric_limits<float>::max());
		m_bboxMin = glm::vec3(std::numeric_limits<float>::max());

		bool warnSDEF = false;
		bool warnQDEF = false;
		for (const auto& v : pmx.m_vertices)
		{
			glm::vec3 pos = v.m_position * glm::vec3(1, 1, -1);
			glm::vec3 nor = v.m_normal * glm::vec3(1, 1, -1);
			glm::vec2 uv = glm::vec2(v.m_uv.x, v.m_uv.y);
			m_positions.push_back(pos);
			m_normals.push_back(nor);
			m_uvs.push_back(uv);
			VertexBoneInfo vtxBoneInfo;
			vtxBoneInfo.m_boneIndex = glm::ivec4(
				v.m_boneIndices[0],
				v.m_boneIndices[1],
				v.m_boneIndices[2],
				v.m_boneIndices[3]
			);
			vtxBoneInfo.m_boneWeight = glm::vec4(
				v.m_boneWeights[0],
				v.m_boneWeights[1],
				v.m_boneWeights[2],
				v.m_boneWeights[3]
			);
			switch (v.m_weightType)
			{
			case PMXVertexWeight::BDEF1:
				vtxBoneInfo.m_skinningType = SkinningType::Weight1;
				break;
			case PMXVertexWeight::BDEF2:
				vtxBoneInfo.m_skinningType = SkinningType::Weight2;
				vtxBoneInfo.m_boneWeight[1] = 1.0f - vtxBoneInfo.m_boneWeight[0];
				break;
			case PMXVertexWeight::BDEF4:
				vtxBoneInfo.m_skinningType = SkinningType::Weight4;
				break;
			case PMXVertexWeight::SDEF:
				vtxBoneInfo.m_skinningType = SkinningType::Weight2;
				vtxBoneInfo.m_boneWeight[1] = 1.0f - vtxBoneInfo.m_boneWeight[0];
				if (!warnSDEF)
				{
					SABA_WARN("SDEF Not Surpported: Use Weight2");
					warnSDEF = true;
				}
				break;
			case PMXVertexWeight::QDEF:
				vtxBoneInfo.m_skinningType = SkinningType::Weight4;
				if (!warnQDEF)
				{
					SABA_WARN("QDEF Not Surpported: Use Weight4");
					warnQDEF = true;
				}
				break;
			default:
				vtxBoneInfo.m_skinningType = SkinningType::Weight1;
				SABA_ERROR("Unknown PMX Vertex Weight Type: {}", (int)v.m_weightType);
				break;
			}
			m_vertexBoneInfos.push_back(vtxBoneInfo);

			m_bboxMax = glm::max(m_bboxMax, pos);
			m_bboxMin = glm::min(m_bboxMin, pos);
		}
		m_updatePositions.resize(m_positions.size());
		m_updateNormals.resize(m_normals.size());


		m_indexElementSize = pmx.m_header.m_vertexIndexSize;
		m_indices.resize(pmx.m_faces.size() * 3 * m_indexElementSize);
		m_indexCount = pmx.m_faces.size() * 3;
		switch (m_indexElementSize)
		{
		case 1:
		{
			int idx = 0;
			uint8_t* indices = (uint8_t*)m_indices.data();
			for (const auto& face : pmx.m_faces)
			{
				for (int i = 0; i < 3; i++)
				{
					auto vi = face.m_vertices[3 - i - 1];
					indices[idx] = (uint8_t)vi;
					idx++;
				}
			}
			break;
		}
		case 2:
		{
			int idx = 0;
			uint16_t* indices = (uint16_t*)m_indices.data();
			for (const auto& face : pmx.m_faces)
			{
				for (int i = 0; i < 3; i++)
				{
					auto vi = face.m_vertices[3 - i - 1];
					indices[idx] = (uint16_t)vi;
					idx++;
				}
			}
			break;
		}
		case 4:
		{
			int idx = 0;
			uint32_t* indices = (uint32_t*)m_indices.data();
			for (const auto& face : pmx.m_faces)
			{
				for (int i = 0; i < 3; i++)
				{
					auto vi = face.m_vertices[3 - i - 1];
					indices[idx] = (uint32_t)vi;
					idx++;
				}
			}
			break;
		}
		default:
			SABA_ERROR("Unsupported Index Size: [{}]", m_indexElementSize);
			return false;
		}

		std::vector<std::string> texturePaths;
		texturePaths.reserve(pmx.m_textures.size());
		for (const auto& pmxTex : pmx.m_textures)
		{
			std::string texPath = PathUtil::Combine(dirPath, pmxTex.m_textureName);
			texturePaths.emplace_back(std::move(texPath));
		}

		// Materialをコピー
		m_materials.reserve(pmx.m_materials.size());
		m_subMeshes.reserve(pmx.m_materials.size());
		uint32_t beginIndex = 0;
		for (const auto& pmxMat : pmx.m_materials)
		{
			MMDMaterial mat;
			mat.m_diffuse = pmxMat.m_diffuse;
			mat.m_alpha = pmxMat.m_diffuse.a;
			mat.m_specularPower = pmxMat.m_specularPower;
			mat.m_specular = pmxMat.m_specular;
			mat.m_ambient = pmxMat.m_ambient;
			mat.m_spTextureMode = MMDMaterial::SphereTextureMode::None;
			mat.m_bothFace = ((uint8_t)pmxMat.m_drawMode & (uint8_t)PMXDrawModeFlags::BothFace) != 0;

			// Texture
			if (pmxMat.m_textureIndex != -1)
			{
				mat.m_texture = texturePaths[pmxMat.m_textureIndex];
			}

			// ToonTexture
			if (pmxMat.m_toonMode == PMXToonMode::Common)
			{
				if (pmxMat.m_toonTextureIndex != -1)
				{
					std::stringstream ss;
					ss << "toon" << std::setfill('0') << std::setw(2) << (pmxMat.m_toonTextureIndex + 1) << ".bmp";
					mat.m_toonTexture = PathUtil::Combine(mmdDataDir, ss.str());
				}
			}
			else if (pmxMat.m_toonMode == PMXToonMode::Separate)
			{
				if (pmxMat.m_toonTextureIndex != -1)
				{
					mat.m_toonTexture = texturePaths[pmxMat.m_toonTextureIndex];
				}
			}

			// SpTexture
			if (pmxMat.m_sphereTextureIndex != -1)
			{
				mat.m_spTexture = texturePaths[pmxMat.m_sphereTextureIndex];
				mat.m_spTextureMode = MMDMaterial::SphereTextureMode::None;
				if (pmxMat.m_sphereMode == PMXSphereMode::Mul)
				{
					mat.m_spTextureMode = MMDMaterial::SphereTextureMode::Mul;
				}
				else if (pmxMat.m_sphereMode == PMXSphereMode::Add)
				{
					mat.m_spTextureMode = MMDMaterial::SphereTextureMode::Add;
				}
				else if (pmxMat.m_sphereMode == PMXSphereMode::SubTexture)
				{
					// TODO: SphareTexture が SubTexture の処理
				}
			}

			m_materials.emplace_back(std::move(mat));

			MMDSubMesh subMesh;
			subMesh.m_beginIndex = beginIndex;
			subMesh.m_vertexCount = pmxMat.m_numFaceVertices;
			subMesh.m_materialID = (int)(m_materials.size() - 1);
			m_subMeshes.push_back(subMesh);

			beginIndex = beginIndex + pmxMat.m_numFaceVertices;
		}
		m_initMaterials = m_materials;
		m_mulMaterialFactors.resize(m_materials.size());
		m_addMaterialFactors.resize(m_materials.size());

		// Morph
		for (const auto& pmxMorph : pmx.m_morphs)
		{
			auto morph = m_morphMan.AddMorph();
			morph->SetName(pmxMorph.m_name);
			morph->SetWeight(0.0f);
			morph->m_morphType = MorphType::None;
			if (pmxMorph.m_morphType == PMXMorphType::Position)
			{
				morph->m_morphType = MorphType::Position;
				morph->m_dataIndex = m_positionMorphDatas.size();
				PositionMorphData morphData;
				for (const auto vtx : pmxMorph.m_positionMorph)
				{
					MorphVertex morphVtx;
					morphVtx.m_index = vtx.m_vertexIndex;
					morphVtx.m_position = vtx.m_position * glm::vec3(1, 1, -1);
					morphData.m_morphVertices.push_back(morphVtx);
				}
				m_positionMorphDatas.emplace_back(std::move(morphData));
			}
			else if (pmxMorph.m_morphType == PMXMorphType::Material)
			{
				morph->m_morphType = MorphType::Material;
				morph->m_dataIndex = m_materialMorphDatas.size();

				MaterialMorphData materialMorphData;
				materialMorphData.m_materialMorphs = pmxMorph.m_materialMorph;
				m_materialMorphDatas.emplace_back(materialMorphData);
			}
			else if (pmxMorph.m_morphType == PMXMorphType::Group)
			{
				morph->m_morphType = MorphType::Group;
				morph->m_dataIndex = m_groupMorphDatas.size();

				GroupMorphData groupMorphData;
				groupMorphData.m_groupMorphs = pmxMorph.m_groupMorph;
				m_groupMorphDatas.emplace_back(groupMorphData);
			}
			else
			{
				SABA_WARN("Not Supported Morp Type({}): [{}]",
					(uint8_t)pmxMorph.m_morphType,
					pmxMorph.m_name
				);
			}
		}

		// Node
		m_nodeMan.GetNodes()->reserve(pmx.m_bones.size());
		for (const auto& bone : pmx.m_bones)
		{
			auto* node = m_nodeMan.AddNode();
			node->SetName(bone.m_name);
		}
		for (size_t i = 0; i < pmx.m_bones.size(); i++)
		{
			const auto& bone = pmx.m_bones[i];
			auto* node = m_nodeMan.GetNode(i);
			if (bone.m_parentBoneIndex != -1)
			{
				const auto& parentBone = pmx.m_bones[bone.m_parentBoneIndex];
				auto* parent = m_nodeMan.GetNode(bone.m_parentBoneIndex);
				parent->AddChild(node);
				auto localPos = bone.m_position - parentBone.m_position;
				localPos.z *= -1;
				node->SetTranslate(localPos);
			}
			else
			{
				auto localPos = bone.m_position;
				localPos.z *= -1;
				node->SetTranslate(localPos);
			}
			glm::mat4 init = glm::translate(
				glm::mat4(1),
				bone.m_position * glm::vec3(1, 1, -1)
			);
			node->SetGlobalTransform(init);
			node->CalculateInverseInitTransform();

			node->SetDeformDepth(bone.m_deformDepth);
			bool appendRotate = ((uint16_t)bone.m_boneFlag & (uint16_t)PMXBoneFlags::AppendRotate) != 0;
			bool appendTranslate = ((uint16_t)bone.m_boneFlag & (uint16_t)PMXBoneFlags::AppendTranslate) != 0;
			node->EnableAppendRotate(appendRotate);
			node->EnableAppendTranslate(appendTranslate);
			if (appendRotate || appendTranslate)
			{
				bool appendLocal = ((uint16_t)bone.m_boneFlag & (uint16_t)PMXBoneFlags::AppendLocal) != 0;
				auto appendNode = m_nodeMan.GetNode(bone.m_appendBoneIndex);
				float appendWeight = bone.m_appendWeight;
				node->EnableAppendLocal(appendLocal);
				node->SetAppendNode(appendNode);
				node->SetAppendWeight(appendWeight);
			}
			node->SaveInitialTRS();
		}
		m_sortedNodes.clear();
		m_sortedNodes.reserve(m_nodeMan.GetNodeCount());
		auto* pmxNodes = m_nodeMan.GetNodes();
		for (auto& pmxNode : (*pmxNodes))
		{
			m_sortedNodes.push_back(pmxNode.get());
		}
		std::stable_sort(
			m_sortedNodes.begin(),
			m_sortedNodes.end(),
			[](const PMXNode* x, const PMXNode* y) {return x->GetDeformdepth() < y->GetDeformdepth(); }
		);

		// IK
		for (size_t i = 0; i < pmx.m_bones.size(); i++)
		{
			const auto& bone = pmx.m_bones[i];
			if ((uint16_t)bone.m_boneFlag & (uint16_t)PMXBoneFlags::IK)
			{
				auto solver = m_ikSolverMan.AddIKSolver();
				auto* ikNode = m_nodeMan.GetNode(i);
				solver->SetIKNode(ikNode);
				ikNode->SetIKSolver(solver);

				auto* targetNode = m_nodeMan.GetNode(bone.m_ikTargetBoneIndex);
				solver->SetTargetNode(targetNode);

				for (const auto& ikLink : bone.m_ikLinks)
				{
					auto* linkNode = m_nodeMan.GetNode(ikLink.m_ikBoneIndex);
					if (ikLink.m_enableLimit)
					{
						glm::vec3 limitMax = ikLink.m_limitMin * glm::vec3(-1, -1, 1);
						glm::vec3 limitMin = ikLink.m_limitMax * glm::vec3(-1, -1, 1);
						limitMax.x = -ikLink.m_limitMin.x;
						limitMax.y = -ikLink.m_limitMin.y;
						limitMax.z = ikLink.m_limitMax.z;
						limitMin.y = -ikLink.m_limitMax.y;
						limitMin.x = -ikLink.m_limitMax.x;
						limitMin.y = ikLink.m_limitMin.z;
						solver->AddIKChain(linkNode, true, limitMin, limitMax);
					}
					else
					{
						solver->AddIKChain(linkNode);
					}
					linkNode->EnableIK(true);
				}

				solver->SetIterateCount(bone.m_ikIterationCount);
				solver->SetLimitAngle(bone.m_ikLimit);
			}
		}

		if (!m_physicsMan.Create())
		{
			SABA_ERROR("Create Physics Fail.");
			return false;
		}

		for (const auto& pmxRB : pmx.m_rigidbodies)
		{
			auto rb = m_physicsMan.AddRigidBody();
			MMDNode* node = nullptr;
			if (pmxRB.m_boneIndex != -1)
			{
				node = m_nodeMan.GetMMDNode(pmxRB.m_boneIndex);
			}
			if (!rb->Create(pmxRB, this, node))
			{
				SABA_ERROR("Create Rigid Body Fail.\n");
				return false;
			}
			m_physicsMan.GetMMDPhysics()->AddRigidBody(rb);
		}

		for (const auto& pmxJoint : pmx.m_joints)
		{
			auto joint = m_physicsMan.AddJoint();
			MMDNode* node = nullptr;
			auto rigidBodys = m_physicsMan.GetRigidBodys();
			bool ret = joint->CreateJoint(
				pmxJoint,
				(*rigidBodys)[pmxJoint.m_rigidbodyAIndex].get(),
				(*rigidBodys)[pmxJoint.m_rigidbodyBIndex].get()
			);
			if (!ret)
			{
				SABA_ERROR("Create Joint Fail.\n");
				return false;
			}
			m_physicsMan.GetMMDPhysics()->AddJoint(joint);
		}

		return true;
	}

	void PMXModel::Destroy()
	{
		m_materials.clear();
		m_subMeshes.clear();

		m_positions.clear();
		m_normals.clear();
		m_uvs.clear();
		m_vertexBoneInfos.clear();

		m_indices.clear();

		m_nodeMan.GetNodes()->clear();
	}

	void PMXModel::Morph(PMXMorph* morph, float weight)
	{
		switch (morph->m_morphType)
		{
		case MorphType::Position:
			MorphPosition(
				m_positionMorphDatas[morph->m_dataIndex],
				weight
			);
			break;
		case MorphType::Material:
			MorphMaterial(
				m_materialMorphDatas[morph->m_dataIndex],
				weight
			);
			break;
		case MorphType::Group:
		{
			auto& groupMorphData = m_groupMorphDatas[morph->m_dataIndex];
			for (const auto& groupMorph : groupMorphData.m_groupMorphs)
			{
				auto& elemMorph = (*m_morphMan.GetMorphs())[groupMorph.m_morphIndex];
				Morph(elemMorph.get(), groupMorph.m_weight * weight);
				break;
			}
		}
		default:
			break;
		}
	}

	void PMXModel::MorphPosition(const PositionMorphData & morphData, float weight)
	{
		if (weight == 0)
		{
			return;
		}

		auto* updatePosition = m_updatePositions.data();
		for (const auto& morphVtx : morphData.m_morphVertices)
		{
			updatePosition[morphVtx.m_index] += morphVtx.m_position * weight;
		}
	}

	void PMXModel::BeginMorphMaterial()
	{
		MaterialFactor initMul;
		initMul.m_diffuse = glm::vec3(1);
		initMul.m_alpha = 1;
		initMul.m_specular = glm::vec3(1);
		initMul.m_specularPower = 1;
		initMul.m_ambient = glm::vec3(1);
		initMul.m_edgeColor = glm::vec4(1);
		initMul.m_edgeSize = 1;
		initMul.m_textureFactor = glm::vec4(1);
		initMul.m_spTextureFactor = glm::vec4(1);
		initMul.m_toonTextureFactor = glm::vec4(1);

		MaterialFactor initAdd;
		initAdd.m_diffuse = glm::vec3(0);
		initAdd.m_alpha = 0;
		initAdd.m_specular = glm::vec3(0);
		initAdd.m_specularPower = 0;
		initAdd.m_ambient = glm::vec3(0);
		initAdd.m_edgeColor = glm::vec4(0);
		initAdd.m_edgeSize = 0;
		initAdd.m_textureFactor = glm::vec4(0);
		initAdd.m_spTextureFactor = glm::vec4(0);
		initAdd.m_toonTextureFactor = glm::vec4(0);

		size_t matCount = m_materials.size();
		for (size_t matIdx = 0; matIdx < matCount; matIdx++)
		{
			m_mulMaterialFactors[matIdx] = initMul;
			m_mulMaterialFactors[matIdx].m_diffuse = m_initMaterials[matIdx].m_diffuse;
			m_mulMaterialFactors[matIdx].m_alpha = m_initMaterials[matIdx].m_alpha;
			m_mulMaterialFactors[matIdx].m_specular = m_initMaterials[matIdx].m_specular;
			m_mulMaterialFactors[matIdx].m_specularPower = m_initMaterials[matIdx].m_specularPower;
			m_mulMaterialFactors[matIdx].m_ambient = m_initMaterials[matIdx].m_ambient;

			m_addMaterialFactors[matIdx] = initAdd;
		}
	}

	void PMXModel::EndMorphMaterial()
	{
		size_t matCount = m_materials.size();
		for (size_t matIdx = 0; matIdx < matCount; matIdx++)
		{
			MaterialFactor matFactor = m_mulMaterialFactors[matIdx];
			matFactor.Add(m_addMaterialFactors[matIdx], 1.0f);

			m_materials[matIdx].m_diffuse = matFactor.m_diffuse;
			m_materials[matIdx].m_alpha = matFactor.m_alpha;
			m_materials[matIdx].m_specular = matFactor.m_specular;
			m_materials[matIdx].m_specularPower = matFactor.m_specularPower;
			m_materials[matIdx].m_ambient = matFactor.m_ambient;
			m_materials[matIdx].m_textureMulFactor = m_mulMaterialFactors[matIdx].m_textureFactor;
			m_materials[matIdx].m_textureAddFactor = m_addMaterialFactors[matIdx].m_textureFactor;
			m_materials[matIdx].m_spTextureMulFactor = m_mulMaterialFactors[matIdx].m_spTextureFactor;
			m_materials[matIdx].m_spTextureAddFactor = m_addMaterialFactors[matIdx].m_spTextureFactor;
			m_materials[matIdx].m_toonTextureMulFactor = m_mulMaterialFactors[matIdx].m_toonTextureFactor;
			m_materials[matIdx].m_toonTextureAddFactor = m_addMaterialFactors[matIdx].m_toonTextureFactor;
		}
	}

	void PMXModel::MorphMaterial(const MaterialMorphData & morphData, float weight)
	{
		if (weight == 0)
		{
			return;
		}

		for (const auto& matMorph : morphData.m_materialMorphs)
		{
			const auto& initMat = m_initMaterials[matMorph.m_materialIndex];
			auto mi = matMorph.m_materialIndex;
			auto& mat = m_materials[mi];
			switch (matMorph.m_opType)
			{
			case saba::PMXMorph::MaterialMorph::OpType::Mul:
				m_mulMaterialFactors[mi].Mul(
					MaterialFactor(matMorph),
					weight
				);
				break;
			case saba::PMXMorph::MaterialMorph::OpType::Add:
				m_addMaterialFactors[mi].Add(
					MaterialFactor(matMorph),
					weight
				);
				break;
			default:
				break;
			}
		}
	}

	PMXNode::PMXNode()
		: m_deformDepth(-1)
		, m_appendNode(nullptr)
		, m_isAppendRotate(false)
		, m_isAppendTranslate(false)
		, m_isAppendLocal(false)
		, m_appendWeight(0)
		, m_ikSolver(nullptr)
	{
	}

	void PMXNode::UpdateAppendTransform()
	{
		if (m_appendNode == nullptr)
		{
			return;
		}

		if (m_isAppendRotate)
		{
			glm::quat appendRotate;
			if (m_isAppendLocal)
			{
				appendRotate = m_appendNode->GetRotate();
			}
			else
			{
				if (m_appendNode->GetAppendNode() != nullptr)
				{
					appendRotate = m_appendNode->GetAppendRotate();
				}
				else
				{
					appendRotate = m_appendNode->GetRotate();
				}
			}

			if (m_appendNode->m_enableIK)
			{
				appendRotate = m_appendNode->GetIKRotate() * appendRotate;
			}

			glm::quat appendQ = glm::slerp(
				glm::quat(),
				appendRotate,
				GetAppendWeight()
			);
			m_appendRotate = appendQ;
		}

		if (m_isAppendTranslate)
		{
			glm::vec3 appendTranslate(0.0f);
			if (m_isAppendLocal)
			{
				appendTranslate = m_appendNode->GetTranslate() - m_appendNode->GetInitialTranslate();
			}
			else
			{
				if (m_appendNode->GetAppendNode() != nullptr)
				{
					appendTranslate = m_appendNode->GetAppendTranslate();
				}
				else
				{
					appendTranslate = m_appendNode->GetTranslate() - m_appendNode->GetInitialTranslate();
				}
			}

			m_appendTranslate = appendTranslate * GetAppendWeight();
		}

		UpdateLocalTransform();
	}

	void PMXNode::OnBeginUpdateTransform()
	{
		m_appendTranslate = glm::vec3(0);
		m_appendRotate = glm::quat();
	}

	void PMXNode::OnEndUpdateTransfrom()
	{
	}

	void PMXNode::OnUpdateLocalTransform()
	{
		glm::vec3 t = GetTranslate();
		if (m_isAppendTranslate)
		{
			t += m_appendTranslate;
		}

		glm::quat r = GetRotate();
		if (m_enableIK)
		{
			r = GetIKRotate() * r;
		}
		if (m_isAppendRotate)
		{
			r = r * m_appendRotate;
		}

		glm::vec3 s = GetScale();

		m_local = glm::translate(glm::mat4(), t)
			* glm::mat4_cast(r)
			* glm::scale(glm::mat4(), s);
	}

	PMXModel::MaterialFactor::MaterialFactor(const saba::PMXMorph::MaterialMorph & pmxMat)
	{
		m_diffuse.r = pmxMat.m_diffuse.r;
		m_diffuse.g = pmxMat.m_diffuse.g;
		m_diffuse.b = pmxMat.m_diffuse.b;
		m_alpha = pmxMat.m_diffuse.a;
		m_specular = pmxMat.m_specular;
		m_specularPower = pmxMat.m_specularPower;
		m_ambient = pmxMat.m_ambient;
		m_edgeColor = pmxMat.m_edgeColor;
		m_edgeSize = pmxMat.m_edgeSize;
		m_textureFactor = pmxMat.m_textureFactor;
		m_spTextureFactor = pmxMat.m_sphereTextureFactor;
		m_toonTextureFactor = pmxMat.m_toonTextureFactor;
	}

	void PMXModel::MaterialFactor::Mul(const MaterialFactor & val, float weight)
	{
		m_diffuse = glm::mix(m_diffuse, m_diffuse * val.m_diffuse, weight);
		m_alpha = glm::mix(m_alpha, m_alpha * val.m_alpha, weight);
		m_specular = glm::mix(m_specular, m_specular * val.m_specular, weight);
		m_specularPower = glm::mix(m_specularPower, m_specularPower * val.m_specularPower, weight);
		m_ambient = glm::mix(m_ambient, m_ambient * val.m_ambient, weight);
		m_edgeColor = glm::mix(m_edgeColor, m_edgeColor * val.m_edgeColor, weight);
		m_edgeSize = glm::mix(m_edgeSize, m_edgeSize * val.m_edgeSize, weight);
		m_textureFactor = glm::mix(m_textureFactor, m_textureFactor * val.m_textureFactor, weight);
		m_spTextureFactor = glm::mix(m_spTextureFactor, m_spTextureFactor * val.m_spTextureFactor, weight);
		m_toonTextureFactor = glm::mix(m_toonTextureFactor, m_toonTextureFactor * val.m_toonTextureFactor, weight);
	}

	void PMXModel::MaterialFactor::Add(const MaterialFactor & val, float weight)
	{
		m_diffuse += val.m_diffuse * weight;
		m_alpha += val.m_alpha * weight;
		m_specular += val.m_specular * weight;
		m_specularPower += val.m_specularPower * weight;
		m_ambient += val.m_ambient * weight;
		m_edgeColor += val.m_edgeColor * weight;
		m_edgeSize += val.m_edgeSize * weight;
		m_textureFactor += val.m_textureFactor * weight;
		m_spTextureFactor += val.m_spTextureFactor * weight;
		m_toonTextureFactor += val.m_toonTextureFactor * weight;
	}
}
