//
// Copyright(c) 2016-2017 benikabocha.
// Distributed under the MIT License (http://opensource.org/licenses/MIT)
//

#ifndef SABA_MODEL_MMD_MMDMODEL_H_
#define SABA_MODEL_MMD_MMDMODEL_H_

#include "MMDNode.h"
#include "MMDIkSolver.h"
#include "MMDMorph.h"

#include <vector>
#include <string>
#include <algorithm>
#include <cstdint>
#include <memory>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

namespace saba
{
	struct MMDMaterial;
	class MMDPhysics;
	class MMDRigidBody;
	class MMDJoint;
	struct VPDFile;

	class MMDNodeManager
	{
	public:
		static const size_t NPos = -1;

		virtual size_t GetNodeCount() = 0;
		virtual size_t FindNodeIndex(const std::string& name) = 0;
		virtual MMDNode* GetMMDNode(size_t idx) = 0;

		MMDNode* GetMMDNode(const std::string& nodeName)
		{
			auto findIdx = FindNodeIndex(nodeName);
			if (findIdx == NPos)
			{
				return nullptr;
			}
			return GetMMDNode(findIdx);
		}
	};

	class MMDIKManager
	{
	public:
		static const size_t NPos = -1;

		virtual size_t GetIKSolverCount() = 0;
		virtual size_t FindIKSolverIndex(const std::string& name) = 0;
		virtual MMDIkSolver* GetMMDIKSolver(size_t idx) = 0;

		MMDIkSolver* GetMMDIKSolver(const std::string& ikName)
		{
			auto findIdx = FindIKSolverIndex(ikName);
			if (findIdx == NPos)
			{
				return nullptr;
			}
			return GetMMDIKSolver(findIdx);
		}
	};

	class MMDMorphManager
	{
	public:
		static const size_t NPos = -1;

		virtual size_t GetMorphCount() = 0;
		virtual size_t FindMorphIndex(const std::string& name) = 0;
		virtual MMDMorph* GetMorph(size_t idx) = 0;

		MMDMorph* GetMorph(const std::string& name)
		{
			auto findIdx = FindMorphIndex(name);
			if (findIdx == NPos)
			{
				return nullptr;
			}
			return GetMorph(findIdx);
		}
	};

	class MMDPhysicsManager
	{
	public:
		using RigidBodyPtr = std::unique_ptr<MMDRigidBody>;
		using JointPtr = std::unique_ptr<MMDJoint>;

		MMDPhysicsManager();
		~MMDPhysicsManager();

		bool Create();

		MMDPhysics* GetMMDPhysics();

		MMDRigidBody* AddRigidBody();
		std::vector<RigidBodyPtr>* GetRigidBodys() { return &m_rigidBodys; }

		MMDJoint* AddJoint();
		std::vector<JointPtr>* GetJoints() { return &m_joints; }


	private:
		std::unique_ptr<MMDPhysics>	m_mmdPhysics;

		std::vector<RigidBodyPtr>	m_rigidBodys;
		std::vector<JointPtr>		m_joints;
	};

	struct MMDSubMesh
	{
		int	m_beginIndex;
		int	m_vertexCount;
		int	m_materialID;
	};

	class VMDAnimation;

	class MMDModel
	{
	public:
		virtual MMDNodeManager* GetNodeManager() = 0;
		virtual MMDIKManager* GetIKManager() = 0;
		virtual MMDMorphManager* GetMorphManager() = 0;
		virtual MMDPhysicsManager* GetPhysicsManager() = 0;

		virtual size_t GetVertexCount() const = 0;
		virtual const glm::vec3* GetPositions() const = 0;
		virtual const glm::vec3* GetNormals() const = 0;
		virtual const glm::vec2* GetUVs() const = 0;
		virtual const glm::vec3* GetUpdatePositions() const = 0;
		virtual const glm::vec3* GetUpdateNormals() const = 0;
		virtual const glm::vec2* GetUpdateUVs() const = 0;

		virtual size_t GetIndexElementSize() const = 0;
		virtual size_t GetIndexCount() const = 0;
		virtual const void* GetIndices() const = 0;

		virtual size_t GetMaterialCount() const = 0;
		virtual const MMDMaterial* GetMaterials() const = 0;

		virtual size_t GetSubMeshCount() const = 0;
		virtual const MMDSubMesh* GetSubMeshes() const = 0;

		virtual MMDPhysics* GetMMDPhysics() = 0;

		// ノードを初期化する
		virtual void InitializeAnimation() = 0;

		// ベースアニメーション(アニメーション読み込み時、Physics反映用)
		void SaveBaseAnimation();
		void LoadBaseAnimation();
		void ClearBaseAnimation();

		// アニメーションの前後で呼ぶ (VMDアニメーションの前後)
		virtual void BeginAnimation() = 0;
		virtual void EndAnimation() = 0;
		// Morph
		virtual void UpdateMorphAnimation() = 0;
		// ノードを更新する
		[[deprecated("Please use UpdateAllAnimation() function")]]
		void UpdateAnimation();
		virtual void UpdateNodeAnimation(bool afterPhysicsAnim) = 0;
		// Physicsを更新する
		virtual void ResetPhysics() = 0;
		[[deprecated("Please use UpdateAllAnimation() function")]]
		void UpdatePhysics(float elapsed);
		virtual void UpdatePhysicsAnimation(float elapsed) = 0;
		// 頂点を更新する
		virtual void Update() = 0;
		virtual void SetParallelUpdateHint(uint32_t parallelCount) = 0;

		void UpdateAllAnimation(VMDAnimation* vmdAnim, float vmdFrame, float physicsElapsed);
		void LoadPose(const VPDFile& vpd, int frameCount = 30);

	protected:
		template <typename NodeType>
		class MMDNodeManagerT : public MMDNodeManager
		{
		public:
			using NodePtr = std::unique_ptr<NodeType>;

			size_t GetNodeCount() override { return m_nodes.size(); }

			size_t FindNodeIndex(const std::string& name) override
			{
				auto findIt = std::find_if(
					m_nodes.begin(),
					m_nodes.end(),
					[&name](const NodePtr& node) { return node->GetName() == name; }
				);
				if (findIt == m_nodes.end())
				{
					return NPos;
				}
				else
				{
					return findIt - m_nodes.begin();
				}
			}

			MMDNode* GetMMDNode(size_t idx) override
			{
				return m_nodes[idx].get();
			}

			NodeType* AddNode()
			{
				auto node = std::make_unique<NodeType>();
				node->SetIndex((uint32_t)m_nodes.size());
				m_nodes.emplace_back(std::move(node));
				return m_nodes[m_nodes.size() - 1].get();
			}

			NodeType* GetNode(size_t i)
			{
				return m_nodes[i].get();
			}

			std::vector<NodePtr>* GetNodes()
			{
				return &m_nodes;
			}

		private:
			std::vector<NodePtr>	m_nodes;
		};

		template <typename IKSolverType>
		class MMDIKManagerT : public MMDIKManager
		{
		public:
			using IKSolverPtr = std::unique_ptr<IKSolverType>;

			size_t GetIKSolverCount() override { return m_ikSolvers.size(); }

			size_t FindIKSolverIndex(const std::string& name) override
			{
				auto findIt = std::find_if(
					m_ikSolvers.begin(),
					m_ikSolvers.end(),
					[&name](const IKSolverPtr& ikSolver) { return ikSolver->GetName() == name; }
				);
				if (findIt == m_ikSolvers.end())
				{
					return NPos;
				}
				else
				{
					return findIt - m_ikSolvers.begin();
				}
			}

			MMDIkSolver* GetMMDIKSolver(size_t idx) override
			{
				return m_ikSolvers[idx].get();
			}

			IKSolverType* AddIKSolver()
			{
				m_ikSolvers.emplace_back(std::make_unique<IKSolverType>());
				return m_ikSolvers[m_ikSolvers.size() - 1].get();
			}

			IKSolverType* GetIKSolver(size_t i)
			{
				return m_ikSolvers[i].get();
			}

			std::vector<IKSolverPtr>* GetIKSolvers()
			{
				return &m_ikSolvers;
			}

		private:
			std::vector<IKSolverPtr>	m_ikSolvers;
		};

		template <typename MorphType>
		class MMDMorphManagerT : public MMDMorphManager
		{
		public:
			using MorphPtr = std::unique_ptr<MorphType>;

			size_t GetMorphCount() override { return m_morphs.size(); }

			size_t FindMorphIndex(const std::string& name) override
			{
				auto findIt = std::find_if(
					m_morphs.begin(),
					m_morphs.end(),
					[&name](const MorphPtr& morph) { return morph->GetName() == name; }
				);
				if (findIt == m_morphs.end())
				{
					return NPos;
				}
				else
				{
					return findIt - m_morphs.begin();
				}
			}

			MMDMorph* GetMorph(size_t idx) override
			{
				return m_morphs[idx].get();
			}

			MorphType* AddMorph()
			{
				m_morphs.emplace_back(std::make_unique<MorphType>());
				return m_morphs[m_morphs.size() - 1].get();
			}

			std::vector<MorphPtr>* GetMorphs()
			{
				return &m_morphs;
			}

		private:
			std::vector<MorphPtr>	m_morphs;
		};
	};
}

#endif // !SABA_MODEL_MMD_MMDMODEL_H_
