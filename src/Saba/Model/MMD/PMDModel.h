//
// Copyright(c) 2016-2017 benikabocha.
// Distributed under the MIT License (http://opensource.org/licenses/MIT)
//

#ifndef SABA_MODEL_MMD_PMDMODEL_H_
#define SABA_MODEL_MMD_PMDMODEL_H_

#include "MMDMaterial.h"
#include "MMDModel.h"

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/mat4x4.hpp>
#include <vector>
#include <string>
#include <algorithm>

namespace saba
{
	class PMDModel : public MMDModel
	{
	public:
		MMDNodeManager* GetNodeManager() override { return &m_nodeMan; }
		MMDIKManager* GetIKManager() override { return &m_ikSolverMan; }
		MMDMorphManager* GetMorphManager() override { return &m_morphMan; };
		MMDPhysicsManager* GetPhysicsManager() override { return &m_physicsMan; }

		size_t GetVertexCount() const override { return m_positions.size(); }
		const glm::vec3* GetPositions() const override { return &m_positions[0]; }
		const glm::vec3* GetNormals() const override { return &m_normals[0]; }
		const glm::vec2* GetUVs() const override { return &m_uvs[0]; }
		const glm::vec3* GetUpdatePositions() const override { return &m_updatePositions[0]; }
		const glm::vec3* GetUpdateNormals() const override { return &m_updateNormals[0]; }
		const glm::vec2* GetUpdateUVs() const override { return &m_uvs[0]; }

		size_t GetIndexElementSize() const override { return sizeof(uint16_t); }
		size_t GetIndexCount() const override { return m_indices.size(); }
		const void* GetIndices() const override { return &m_indices[0]; }

		size_t GetMaterialCount() const override { return m_materials.size(); }
		const MMDMaterial* GetMaterials() const override { return &m_materials[0]; }

		size_t GetSubMeshCount() const override { return m_subMeshes.size(); }
		const MMDSubMesh* GetSubMeshes() const override { return &m_subMeshes[0]; }

		MMDPhysics* GetMMDPhysics() override { return m_physicsMan.GetMMDPhysics(); }

		void InitializeAnimation() override;
		// アニメーションの前後で呼ぶ (VMDアニメーションの前後)
		void BeginAnimation() override;
		void EndAnimation() override;
		// Morph
		void UpdateMorphAnimation() override;
		// ノードを更新する
		void UpdateNodeAnimation(bool afterPhysicsAnim) override;
		// Physicsを更新する
		void ResetPhysics() override;
		void UpdatePhysicsAnimation(float elapsed) override;
		// 頂点データーを更新する
		void Update() override;
		void SetParallelUpdateHint(uint32_t) override {}

		bool Load(const std::string& filepath, const std::string& mmdDataDir);
		void Destroy();

		const glm::vec3& GetBBoxMin() const { return m_bboxMin; }
		const glm::vec3& GetBBoxMax() const { return m_bboxMax; }

	protected:

	private:
		struct MorphVertex
		{
			uint32_t	m_index;
			glm::vec3	m_position;
		};

		class PMDMorph : public MMDMorph
		{
		public:
			std::vector<MorphVertex>	m_vertices;
		};

	private:
		std::vector<glm::vec3>	m_positions;
		std::vector<glm::vec3>	m_normals;
		std::vector<glm::vec2>	m_uvs;
		std::vector<glm::ivec2>	m_bones;
		std::vector<glm::vec2>	m_boneWeights;
		std::vector<glm::vec3>	m_updatePositions;
		std::vector<glm::vec3>	m_updateNormals;
		std::vector<glm::mat4>	m_transforms;

		std::vector<uint16_t> m_indices;

		PMDMorph					m_baseMorph;

		glm::vec3		m_bboxMin;
		glm::vec3		m_bboxMax;

		std::vector<MMDMaterial>	m_materials;
		std::vector<MMDSubMesh>		m_subMeshes;

		MMDNodeManagerT<MMDNode>	m_nodeMan;
		MMDIKManagerT<MMDIkSolver>	m_ikSolverMan;
		MMDMorphManagerT<PMDMorph>	m_morphMan;
		MMDPhysicsManager			m_physicsMan;
	};
}

#endif // !SABA_MODEL_MMD_PMDMODEL_H_
