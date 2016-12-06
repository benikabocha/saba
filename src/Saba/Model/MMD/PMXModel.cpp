#include "PMXModel.h"

#include "PMXFile.h"
#include "MMDPhysics.h"

#include <Saba/Base/Path.h>
#include <Saba/Base/File.h>
#include <Saba/Base/Log.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <map>
#include <limits>
#include <algorithm>
#include <sstream>
#include <iomanip>

namespace saba
{
	void PMXModel::InitializeAnimation()
	{
		for (const auto& node : (*m_nodeMan.GetNodes()))
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

		for (auto& solver : (*m_ikSolverMan.GetIKSolvers()))
		{
			solver->Solve();
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
		for (const auto& node : (*m_nodeMan.GetNodes()))
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

		for (auto& solver : (*m_ikSolverMan.GetIKSolvers()))
		{
			solver->Solve();
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

		// BlendShapeの処理
		if (m_baseShape.m_vertices.empty())
		{
			for (const auto& keyShape : (*m_blendShapeMan.GetBlendKeyShapes()))
			{
				float weight = keyShape->m_weight;
				if (weight == 0.0f)
				{
					continue;
				}
				for (const auto& bsVtx : keyShape->m_vertices)
				{
					updatePosition[bsVtx.m_index] += bsVtx.m_position * weight;
				}
			}
		}
		else
		{
			for (const auto& bsVtx : m_baseShape.m_vertices)
			{
				updatePosition[bsVtx.m_index] = bsVtx.m_position;
			}
			for (const auto& keyShape : (*m_blendShapeMan.GetBlendKeyShapes()))
			{
				float weight = keyShape->m_weight;
				if (weight == 0.0f)
				{
					continue;
				}
				for (const auto& bsVtx : keyShape->m_vertices)
				{
					const auto& baseBsVtx = m_baseShape.m_vertices[bsVtx.m_index];
					updatePosition[baseBsVtx.m_index] += bsVtx.m_position * weight;
				}
			}
		}

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

		// Morph
		for (const auto& morph : pmx.m_morphs)
		{
			if (morph.m_morphType == PMXMorphType::Position)
			{
				MMDBlendShape* shape = nullptr;
				shape = m_blendShapeMan.AddBlendKeyShape();
				shape->m_name = morph.m_name;
				size_t numVtx = morph.m_positionMorph.size();
				shape->m_weight = 0;
				shape->m_vertices.reserve(numVtx);
				for (const auto vtx : morph.m_positionMorph)
				{
					MMDBlendShapeVertex bsVtx;
					bsVtx.m_index = vtx.m_vertexIndex;
					bsVtx.m_position = vtx.m_position * glm::vec3(1, 1, -1);
					shape->m_vertices.push_back(bsVtx);
				}
			}
			else
			{
				SABA_WARN("Not Supported Morp Type({}): [{}]",
					(uint8_t)morph.m_morphType,
					morph.m_name
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

			node->m_deformDepth = bone.m_deformDepth;
			node->m_giftRotate = ((uint16_t)bone.m_boneFlag & (uint16_t)PMXBoneFlags::GiftRotate) != 0;
			node->m_giftTranslate = ((uint16_t)bone.m_boneFlag & (uint16_t)PMXBoneFlags::GiftTranslate) != 0;
			if (node->m_giftRotate || node->m_giftTranslate)
			{
				node->m_giftLocal = ((uint16_t)bone.m_boneFlag & (uint16_t)PMXBoneFlags::GiftLocal) != 0;
				node->m_giftNode = m_nodeMan.GetNode(bone.m_giftBoneIndex);
				node->m_giftWeight = bone.m_giftWeight;
			}
		}

		// IK
		for (size_t i = 0; i < pmx.m_bones.size(); i++)
		{
			const auto& bone = pmx.m_bones[i];
			if ((uint16_t)bone.m_boneFlag & (uint16_t)PMXBoneFlags::IK)
			{
				auto solver = m_ikSolverMan.AddIKSolver();
				auto* ikNode = m_nodeMan.GetNode(i);
				solver->SetIKNode(ikNode);

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
			if (pmxRB.m_boneIndex != 0xFFFF)
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
}
