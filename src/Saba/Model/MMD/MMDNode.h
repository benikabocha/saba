#ifndef SABA_MODEL_MMD_MMDNODE_H_
#define SABA_MODEL_MMD_MMDNODE_H_

#include <string>
#include <glm/vec3.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/mat4x4.hpp>

namespace saba
{
	struct MMDNode
	{
		MMDNode();

		void AddChild(MMDNode* child);
		void UpdateLocalMatrix();
		void UpdateGlobalMatrix();

		std::string		m_name;

		bool			m_enableIK;

		glm::mat4		m_local;
		glm::mat4		m_global;
		glm::mat4		m_inverseInit;

		MMDNode*		m_parent;
		MMDNode*		m_child;
		MMDNode*		m_next;
		MMDNode*		m_prev;

		glm::vec3	m_translate;
		glm::quat	m_rotate;
		glm::vec3	m_scale;

		glm::quat	m_ikRotate;

		glm::vec3	m_initTranslate;
		glm::quat	m_initRotate;
		glm::vec3	m_initScale;
	};
}

#endif // !SABA_MODEL_MMD_MMDNODE_H_

