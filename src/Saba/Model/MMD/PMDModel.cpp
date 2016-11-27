#include "PMDModel.h"

#include "PMDFile.h"

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

	void PMDModel::Update(float elapsed)
	{
		for (const auto& node : (*m_nodeMan.GetNodes()))
		{
			node->UpdateLocalMatrix();
		}

		for (const auto& node : (*m_nodeMan.GetNodes()))
		{
			if (node->m_parent == nullptr)
			{
				node->UpdateGlobalMatrix();
			}
		}

		for (auto& solver : (*m_ikSolverMan.GetIKSolvers()))
		{
			solver->Solve();
		}

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
			auto node0 = m_nodeMan.GetNode(bone->x);
			auto node1 = m_nodeMan.GetNode(bone->y);
			auto w0 = boneWeight->x;
			auto w1 = boneWeight->y;
			auto m0 = node0->m_global * node0->m_inverseInit;
			auto m1 = node1->m_global * node1->m_inverseInit;

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
			glm::vec2 uv = glm::vec2(v.m_uv.x, v.m_uv.y);
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
			mat.m_spTextureMode = MMDMaterial::SphereTextureMode::None;
			mat.m_bothFace = false;

			std::string orgTexName = pmdMat.m_textureName.ToUtf8String();
			std::string texName;
			std::string spTexName;
			auto asterPos = orgTexName.find_first_of('*');
			if (asterPos == std::string::npos)
			{
				texName = orgTexName;
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
				mat.m_texture = texPath;
			}

			if (!spTexName.empty())
			{
				std::string spTexPath = PathUtil::Combine(dirPath, spTexName);
				mat.m_spTexture = spTexPath;
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

		for (const auto& blendShape : pmd.m_blendShapes)
		{
			MMDBlendShape* shape = nullptr;
			if (blendShape.m_blendShapeType == PMDBlendShape::Base)
			{
				shape = &m_baseShape;
			}
			else
			{
				shape = m_blendShapeMan.AddBlendKeyShape();
				shape->m_name = blendShape.m_shapeName.ToUtf8String();
			}
			size_t numVtx = blendShape.m_vertices.size();
			shape->m_weight = 0;
			shape->m_vertices.reserve(blendShape.m_vertices.size());
			for (const auto vtx : blendShape.m_vertices)
			{
				MMDBlendShapeVertex bsVtx;
				bsVtx.m_index = vtx.m_vertexIndex;
				bsVtx.m_position = vtx.m_position * glm::vec3(1, 1, -1);
				shape->m_vertices.push_back(bsVtx);
			}
		}

		// Nodeの作成
		m_nodeMan.GetNodes()->reserve(pmd.m_bones.size());
		for (const auto& bone : pmd.m_bones)
		{
			auto* node = m_nodeMan.AddNode();
			node->m_name = bone.m_boneName.ToUtf8String();
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

				node->m_translate = localPos;
			}
			else
			{
				glm::vec3 localPos = bone.m_position;
				localPos.z *= -1;

				node->m_translate = localPos;
			}
			glm::mat4 init = glm::translate(
				glm::mat4(1),
				bone.m_position * glm::vec3(1, 1, -1)
			);
			node->m_inverseInit = glm::inverse(init);
		}

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
				auto findPos = chainNode->m_name.find(u8"ひざ");
				bool isKnee = false;
				if (findPos != std::string::npos)
				{
					isKnee = true;
				}
				solver->AddIKChain(chainNode, isKnee);
				chainNode->m_enableIK = true;
			}

			solver->SetIterateCount(ik.m_numIteration);
			solver->SetLimitAngle(ik.m_rotateLimit * 4.0f);
		}

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
