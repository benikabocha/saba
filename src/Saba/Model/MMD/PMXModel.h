//
// Copyright(c) 2016-2017 benikabocha.
// Distributed under the MIT License (http://opensource.org/licenses/MIT)
//

#ifndef SABA_MODEL_MMD_PMXMODEL_H_
#define SABA_MODEL_MMD_PMXMODEL_H_

#include "MMDMaterial.h"
#include "MMDModel.h"
#include "MMDIkSolver.h"
#include "PMXFile.h"

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/gtc/quaternion.hpp>
#include <vector>
#include <string>
#include <algorithm>
#include <future>

namespace saba
{
	class PMXNode : public MMDNode
	{
	public:
		PMXNode();

		void SetDeformDepth(int32_t depth) { m_deformDepth = depth; }
		int32_t GetDeformdepth() const { return m_deformDepth; }

		void EnableDeformAfterPhysics(bool enable) { m_isDeformAfterPhysics = enable; }
		bool IsDeformAfterPhysics() { return m_isDeformAfterPhysics; }

		void SetAppendNode(PMXNode* node) { m_appendNode = node; }
		PMXNode* GetAppendNode() const { return m_appendNode; }

		void EnableAppendRotate(bool enable) { m_isAppendRotate = enable; }
		void EnableAppendTranslate(bool enable) { m_isAppendTranslate = enable; }
		void EnableAppendLocal(bool enable) { m_isAppendLocal = enable; }
		void SetAppendWeight(float weight) { m_appendWeight = weight; }
		float GetAppendWeight() const { return m_appendWeight; }

		const glm::vec3& GetAppendTranslate() const { return m_appendTranslate; }
		const glm::quat& GetAppendRotate() const { return m_appendRotate; }

		void SetIKSolver(MMDIkSolver* ik) { m_ikSolver = ik; }
		MMDIkSolver* GetIKSolver() const { return m_ikSolver; }

		void UpdateAppendTransform();


	protected:
		void OnBeginUpdateTransform() override;
		void OnEndUpdateTransfrom() override;
		void OnUpdateLocalTransform() override;

	private:
		int32_t		m_deformDepth;
		bool		m_isDeformAfterPhysics;

		PMXNode*	m_appendNode;
		bool		m_isAppendRotate;
		bool		m_isAppendTranslate;
		bool		m_isAppendLocal;
		float		m_appendWeight;

		glm::vec3	m_appendTranslate;
		glm::quat	m_appendRotate;

		MMDIkSolver*	m_ikSolver;

	};

	class PMXModel : public MMDModel
	{
	public:
		PMXModel();
		~PMXModel();

		MMDNodeManager* GetNodeManager() override { return &m_nodeMan; }
		MMDIKManager* GetIKManager() override { return &m_ikSolverMan; }
		MMDMorphManager* GetMorphManager() override { return &m_morphMan; };
		MMDPhysicsManager* GetPhysicsManager() override { return &m_physicsMan; }

		size_t GetVertexCount() const override { return m_positions.size(); }
		const glm::vec3* GetPositions() const override { return m_positions.data(); }
		const glm::vec3* GetNormals() const override { return m_normals.data(); }
		const glm::vec2* GetUVs() const override { return m_uvs.data(); }
		const glm::vec3* GetUpdatePositions() const override { return m_updatePositions.data(); }
		const glm::vec3* GetUpdateNormals() const override { return m_updateNormals.data(); }
		const glm::vec2* GetUpdateUVs() const override { return m_updateUVs.data(); }

		size_t GetIndexElementSize() const override { return m_indexElementSize; }
		size_t GetIndexCount() const override { return m_indexCount; }
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
		void SetParallelUpdateHint(uint32_t parallelCount) override;

		bool Load(const std::string& filepath, const std::string& mmdDataDir);
		void Destroy();

		const glm::vec3& GetBBoxMin() const { return m_bboxMin; }
		const glm::vec3& GetBBoxMax() const { return m_bboxMax; }

	public:
		enum class SkinningType
		{
			Weight1,
			Weight2,
			Weight4,
			DualQuaternion,
		};
		struct VertexBoneInfo
		{
			SkinningType	m_skinningType;
			glm::ivec4		m_boneIndex;
			glm::vec4		m_boneWeight;
		};

	private:
		struct PositionMorph
		{
			uint32_t	m_index;
			glm::vec3	m_position;
		};

		struct PositionMorphData
		{
			std::vector<PositionMorph>	m_morphVertices;
		};

		struct UVMorph
		{
			uint32_t	m_index;
			glm::vec4	m_uv;
		};

		struct UVMorphData
		{
			std::vector<UVMorph>	m_morphUVs;
		};

		struct MaterialFactor
		{
			MaterialFactor() = default;
			MaterialFactor(const saba::PMXMorph::MaterialMorph& pmxMat);

			void Mul(const MaterialFactor& val, float weight);
			void Add(const MaterialFactor& val, float weight);

			glm::vec3	m_diffuse;
			float		m_alpha;
			glm::vec3	m_specular;
			float		m_specularPower;
			glm::vec3	m_ambient;
			glm::vec4	m_edgeColor;
			float		m_edgeSize;
			glm::vec4	m_textureFactor;
			glm::vec4	m_spTextureFactor;
			glm::vec4	m_toonTextureFactor;
		};

		struct MaterialMorphData
		{
			std::vector<saba::PMXMorph::MaterialMorph>	m_materialMorphs;
		};

		struct BoneMorphElement
		{
			MMDNode*	m_node;
			glm::vec3	m_position;
			glm::quat	m_rotate;
		};

		struct BoneMorphData
		{
			std::vector<BoneMorphElement>	m_boneMorphs;
		};

		struct GroupMorphData
		{
			std::vector<saba::PMXMorph::GroupMorph>		m_groupMorphs;
		};

		enum class MorphType
		{
			None,
			Position,
			UV,
			Material,
			Bone,
			Group,
		};

		class PMXMorph : public MMDMorph
		{
		public:
			MorphType	m_morphType;
			size_t		m_dataIndex;
		};

		struct UpdateRange
		{
			size_t	m_vertexOffset;
			size_t	m_vertexCount;
		};

	private:
		void SetupParallelUpdate();
		void Update(const UpdateRange& range);

		void Morph(PMXMorph* morph, float weight);

		void MorphPosition(const PositionMorphData& morphData, float weight);

		void MorphUV(const UVMorphData& morphData, float weight);

		void BeginMorphMaterial();
		void EndMorphMaterial();
		void MorphMaterial(const MaterialMorphData& morphData, float weight);

		void MorphBone(const BoneMorphData& morphData, float weight);

	private:
		std::vector<glm::vec3>	m_positions;
		std::vector<glm::vec3>	m_normals;
		std::vector<glm::vec2>	m_uvs;
		std::vector<VertexBoneInfo>	m_vertexBoneInfos;
		std::vector<glm::vec3>	m_updatePositions;
		std::vector<glm::vec3>	m_updateNormals;
		std::vector<glm::vec2>	m_updateUVs;
		std::vector<glm::mat4>	m_transforms;

		std::vector<char>	m_indices;
		size_t				m_indexCount;
		size_t				m_indexElementSize;

		std::vector<PositionMorphData>	m_positionMorphDatas;
		std::vector<UVMorphData>		m_uvMorphDatas;
		std::vector<MaterialMorphData>	m_materialMorphDatas;
		std::vector<BoneMorphData>		m_boneMorphDatas;
		std::vector<GroupMorphData>		m_groupMorphDatas;

		// PositionMorph用
		std::vector<glm::vec3>	m_morphPositions;
		std::vector<glm::vec4>	m_morphUVs;

		// マテリアルMorph用
		std::vector<MMDMaterial>	m_initMaterials;
		std::vector<MaterialFactor>	m_mulMaterialFactors;
		std::vector<MaterialFactor>	m_addMaterialFactors;

		glm::vec3		m_bboxMin;
		glm::vec3		m_bboxMax;

		std::vector<MMDMaterial>	m_materials;
		std::vector<MMDSubMesh>		m_subMeshes;
		std::vector<PMXNode*>		m_sortedNodes;

		MMDNodeManagerT<PMXNode>	m_nodeMan;
		MMDIKManagerT<MMDIkSolver>	m_ikSolverMan;
		MMDMorphManagerT<PMXMorph>	m_morphMan;
		MMDPhysicsManager			m_physicsMan;

		uint32_t							m_parallelUpdateCount;
		std::vector<UpdateRange>			m_updateRanges;
		std::vector<std::future<void>>		m_parallelUpdateFutures;
	};
}

#endif // !SABA_MODEL_MMD_PMXMODEL_H_
