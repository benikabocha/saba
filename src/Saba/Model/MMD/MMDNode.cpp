//
// Copyright(c) 2016-2017 benikabocha.
// Distributed under the MIT License (http://opensource.org/licenses/MIT)
//

#include "MMDNode.h"

#include <Saba/Base/Log.h>

#include <glm/gtc/matrix_transform.hpp>

namespace saba
{
	MMDNode::MMDNode()
		: m_enableIK(false)
		, m_local(1.0f)
		, m_global(1.0f)
		, m_parent(nullptr)
		, m_child(nullptr)
		, m_next(nullptr)
		, m_prev(nullptr)
		, m_translate(0)
		, m_rotate(1, 0, 0, 0)
		, m_scale(1)
		, m_animTranslate(0)
		, m_animRotate(1, 0, 0, 0)
		, m_initTranslate(0)
		, m_initScale(1)
	{
	}

	void MMDNode::AddChild(MMDNode * child)
	{
		SABA_ASSERT(child != nullptr);
		if (child == nullptr)
		{
			return;
		}

		SABA_ASSERT(child->m_parent == nullptr);
		SABA_ASSERT(child->m_next == nullptr);
		SABA_ASSERT(child->m_prev == nullptr);
		child->m_parent = this;
		if (m_child == nullptr)
		{
			m_child = child;
			m_child->m_next = nullptr;
			m_child->m_prev = m_child;
		}
		else
		{
			auto lastNode = m_child->m_prev;
			lastNode->m_next = child;
			child->m_prev = lastNode;

			m_child->m_prev = child;
		}
	}

	void MMDNode::BeginUpdateTransform()
	{
		LoadInitialTRS();
		SetIKRotate(glm::quat());
		OnBeginUpdateTransform();
	}

	void MMDNode::EndUpdateTransform()
	{
		OnEndUpdateTransfrom();
	}

	void MMDNode::UpdateLocalTransform()
	{
		OnUpdateLocalTransform();
	}

	void MMDNode::UpdateGlobalTransform()
	{
		if (m_parent == nullptr)
		{
			m_global = m_local;
		}
		else
		{
			m_global = m_parent->m_global * m_local;
		}
		MMDNode* child = m_child;
		while (child != nullptr)
		{
			child->UpdateGlobalTransform();
			child = child->m_next;
		}
	}

	void MMDNode::UpdateChildTransform()
	{
		MMDNode* child = m_child;
		while (child != nullptr)
		{
			child->UpdateGlobalTransform();
			child = child->m_next;
		}
	}

	void MMDNode::CalculateInverseInitTransform()
	{
		m_inverseInit = glm::inverse(m_global);
	}

	void MMDNode::OnBeginUpdateTransform()
	{
	}

	void MMDNode::OnEndUpdateTransfrom()
	{
	}

	void MMDNode::OnUpdateLocalTransform()
	{
		auto s = glm::scale(glm::mat4(1), GetScale());
		auto r = glm::mat4_cast(AnimateRotate());
		auto t = glm::translate(glm::mat4(1), AnimateTranslate());
		if (m_enableIK)
		{
			r = glm::mat4_cast(m_ikRotate) * r;
		}
		m_local = t * r * s;
	}

}
