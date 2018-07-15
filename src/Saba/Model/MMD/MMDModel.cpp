//
// Copyright(c) 2016-2017 benikabocha.
// Distributed under the MIT License (http://opensource.org/licenses/MIT)
//

#include "MMDModel.h"
#include "MMDPhysics.h"
#include "VPDFile.h"
#include "VMDAnimation.h"

#include <glm/gtc/matrix_transform.hpp>

#include <Saba/Base/Log.h>

namespace saba
{
	MMDPhysicsManager::MMDPhysicsManager()
	{
	}

	MMDPhysicsManager::~MMDPhysicsManager()
	{
		for (auto& joint : m_joints)
		{
			m_mmdPhysics->RemoveJoint(joint.get());
		}
		m_joints.clear();

		for (auto& rb : m_rigidBodys)
		{
			m_mmdPhysics->RemoveRigidBody(rb.get());
		}
		m_rigidBodys.clear();

		m_mmdPhysics.reset();
	}

	bool MMDPhysicsManager::Create()
	{
		m_mmdPhysics = std::make_unique<MMDPhysics>();
		return m_mmdPhysics->Create();
	}

	MMDPhysics* MMDPhysicsManager::GetMMDPhysics()
	{
		return m_mmdPhysics.get();
	}

	MMDRigidBody* MMDPhysicsManager::AddRigidBody()
	{
		SABA_ASSERT(m_mmdPhysics != nullptr);
		auto rigidBody = std::make_unique<MMDRigidBody>();
		auto ret = rigidBody.get();
		m_rigidBodys.emplace_back(std::move(rigidBody));

		return ret;
	}

	MMDJoint* MMDPhysicsManager::AddJoint()
	{
		SABA_ASSERT(m_mmdPhysics != nullptr);
		auto joint = std::make_unique<MMDJoint>();
		auto ret = joint.get();
		m_joints.emplace_back(std::move(joint));

		return ret;
	}

	void MMDModel::SaveBaseAnimation()
	{
		auto nodeMan = GetNodeManager();
		for (size_t i = 0; i < nodeMan->GetNodeCount(); i++)
		{
			auto node = nodeMan->GetMMDNode(i);
			node->SaveBaseAnimation();
		}

		auto morphMan = GetMorphManager();
		for (size_t i = 0; i < morphMan->GetMorphCount(); i++)
		{
			auto morph = morphMan->GetMorph(i);
			morph->SaveBaseAnimation();
		}

		auto ikMan = GetIKManager();
		for (size_t i = 0; i < ikMan->GetIKSolverCount(); i++)
		{
			auto ikSolver = ikMan->GetMMDIKSolver(i);
			ikSolver->SaveBaseAnimation();
		}
	}

	void MMDModel::LoadBaseAnimation()
	{
		auto nodeMan = GetNodeManager();
		for (size_t i = 0; i < nodeMan->GetNodeCount(); i++)
		{
			auto node = nodeMan->GetMMDNode(i);
			node->LoadBaseAnimation();
		}

		auto morphMan = GetMorphManager();
		for (size_t i = 0; i < morphMan->GetMorphCount(); i++)
		{
			auto morph = morphMan->GetMorph(i);
			morph->LoadBaseAnimation();
		}

		auto ikMan = GetIKManager();
		for (size_t i = 0; i < ikMan->GetIKSolverCount(); i++)
		{
			auto ikSolver = ikMan->GetMMDIKSolver(i);
			ikSolver->LoadBaseAnimation();
		}
	}

	void MMDModel::ClearBaseAnimation()
	{
		auto nodeMan = GetNodeManager();
		for (size_t i = 0; i < nodeMan->GetNodeCount(); i++)
		{
			auto node = nodeMan->GetMMDNode(i);
			node->ClearBaseAnimation();
		}

		auto morphMan = GetMorphManager();
		for (size_t i = 0; i < morphMan->GetMorphCount(); i++)
		{
			auto morph = morphMan->GetMorph(i);
			morph->ClearBaseAnimation();
		}

		auto ikMan = GetIKManager();
		for (size_t i = 0; i < ikMan->GetIKSolverCount(); i++)
		{
			auto ikSolver = ikMan->GetMMDIKSolver(i);
			ikSolver->ClearBaseAnimation();
		}
	}

	namespace
	{
		glm::mat3 InvZ(const glm::mat3& m)
		{
			const glm::mat3 invZ = glm::scale(glm::mat4(1.0f), glm::vec3(1, 1, -1));
			return invZ * m * invZ;
		}
		glm::quat InvZ(const glm::quat& q)
		{
			auto rot0 = glm::mat3_cast(q);
			auto rot1 = InvZ(rot0);
			return glm::quat_cast(rot1);
		}
	}

	void MMDModel::UpdateAllAnimation(VMDAnimation * vmdAnim, float vmdFrame, float physicsElapsed)
	{
		if (vmdAnim != nullptr)
		{
			vmdAnim->Evaluate(vmdFrame);
		}

		UpdateMorphAnimation();

		UpdateNodeAnimation(false);

		UpdatePhysicsAnimation(physicsElapsed);

		UpdateNodeAnimation(true);
	}

	void MMDModel::LoadPose(const VPDFile & vpd, int frameCount)
	{
		struct Pose
		{
			MMDNode*	m_node;
			glm::vec3	m_beginTranslate;
			glm::vec3	m_endTranslate;
			glm::quat	m_beginRotate;
			glm::quat	m_endRotate;
		};
		std::vector<Pose> poses;
		for (const auto& bone : vpd.m_bones)
		{
			auto nodeIdx = GetNodeManager()->FindNodeIndex(bone.m_boneName);
			if (MMDNodeManager::NPos != nodeIdx)
			{
				Pose pose;
				pose.m_node = GetNodeManager()->GetMMDNode(bone.m_boneName);
				pose.m_beginTranslate = pose.m_node->GetAnimationTranslate();
				pose.m_endTranslate = bone.m_translate * glm::vec3(1, 1, -1);
				pose.m_beginRotate = pose.m_node->GetAnimationRotate();
				pose.m_endRotate = InvZ(bone.m_quaternion);
				poses.emplace_back(std::move(pose));
			}
		}

		struct Morph
		{
			MMDMorph*	m_morph;
			float		m_beginWeight;
			float		m_endWeight;
		};
		std::vector<Morph> morphs;
		for (const auto& vpdMorph : vpd.m_morphs)
		{
			auto morphIdx = GetMorphManager()->FindMorphIndex(vpdMorph.m_morphName);
			if (MMDMorphManager::NPos != morphIdx)
			{
				Morph morph;
				morph.m_morph = GetMorphManager()->GetMorph(vpdMorph.m_morphName);
				morph.m_beginWeight = morph.m_morph->GetWeight();
				morph.m_endWeight = vpdMorph.m_weight;
				morphs.emplace_back(std::move(morph));
			}
		}

		// Physicsを反映する
		for (int i = 0; i < frameCount; i++)
		{
			BeginAnimation();

			// evaluate
			float w = float(1 + i) / float(frameCount);
			for (auto& pose : poses)
			{
				auto t = glm::mix(pose.m_beginTranslate, pose.m_endTranslate, w);
				auto q = glm::slerp(pose.m_beginRotate, pose.m_endRotate, w);
				pose.m_node->SetAnimationTranslate(t);
				pose.m_node->SetAnimationRotate(q);
			}

			for (auto& morph : morphs)
			{
				auto weight = glm::mix(morph.m_beginWeight, morph.m_endWeight, w);
				morph.m_morph->SetWeight(weight);
			}

			UpdateMorphAnimation();
			UpdateNodeAnimation(false);
			UpdatePhysicsAnimation(1.0f / 30.0f);
			UpdateNodeAnimation(true);

			EndAnimation();
		}

		//SaveBaseAnimation();
	}

	void MMDModel::UpdateAnimation()
	{
		UpdateMorphAnimation();
		UpdateNodeAnimation(false);
	}

	void MMDModel::UpdatePhysics(float elapsed)
	{
		UpdatePhysicsAnimation(elapsed);
	}
}
