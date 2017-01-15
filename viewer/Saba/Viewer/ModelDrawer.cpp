//
// Copyright(c) 2016-2017 benikabocha.
// Distributed under the MIT License (http://opensource.org/licenses/MIT)
//

#include "ModelDrawer.h"

#include <glm/gtc/matrix_transform.hpp>

namespace saba
{
	ModelDrawer::ModelDrawer()
		: m_bboxMin(0)
		, m_bboxMax(0)
		, m_translate(0)
		, m_rotate(0)
		, m_scale(1)
	{
		UpdateTransform();
	}

	void ModelDrawer::SetTranslate(const glm::vec3& t)
	{
		m_translate = t;
		UpdateTransform();
	}

	void ModelDrawer::SetRotate(const glm::vec3& r)
	{
		m_rotate = r;
		UpdateTransform();
	}

	void ModelDrawer::SetScale(const glm::vec3& s)
	{
		m_scale = s;
		UpdateTransform();
	}

	void ModelDrawer::UpdateTransform()
	{
		m_transform = glm::mat4(1);
		glm::mat4 s = glm::scale(m_transform, m_scale);
		glm::mat4 rx = glm::rotate(m_transform, m_rotate.x, glm::vec3(1, 0, 0));
		glm::mat4 ry = glm::rotate(m_transform, m_rotate.y, glm::vec3(0, 1, 0));
		glm::mat4 rz = glm::rotate(m_transform, m_rotate.z, glm::vec3(0, 0, 1));
		glm::mat4 t = glm::translate(m_transform, m_translate);
		m_transform = t * rz * ry * rx * s;
	}

}
