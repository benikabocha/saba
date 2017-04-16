//
// Copyright(c) 2016-2017 benikabocha.
// Distributed under the MIT License (http://opensource.org/licenses/MIT)
//

#include "MMDModel.h"
#include "MMDPhysics.h"

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
}
