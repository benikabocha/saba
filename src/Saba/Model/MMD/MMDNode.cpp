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
		, m_scale(1)
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
		SABA_ASSERT(child->m_child == nullptr);
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

	void MMDNode::UpdateLocalMatrix()
	{
		auto s = glm::scale(glm::mat4(1), m_scale);
		auto r = glm::mat4_cast(m_rotate);
		auto t = glm::translate(glm::mat4(1), m_translate);
		if (m_enableIK)
		{
			r = glm::mat4_cast(m_ikRotate) * r;
		}
		m_local = t * r * s;
	}

	void MMDNode::UpdateGlobalMatrix()
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
			child->UpdateGlobalMatrix();
			child = child->m_next;
		}
	}

}
