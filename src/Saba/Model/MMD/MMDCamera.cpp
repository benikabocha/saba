//
// Copyright(c) 2016-2017 benikabocha.
// Distributed under the MIT License (http://opensource.org/licenses/MIT)
//

#include "MMDCamera.h"

#include <glm/gtc/matrix_transform.hpp>

namespace saba
{
	MMDCamera::MMDCamera()
	{
		m_interest = glm::vec3(0, 10, 0);
		m_rotate = glm::vec3(0, 0, 0);
		m_distance = 50;
		m_fov = glm::radians(30.0f);
	}

	MMDLookAtCamera::MMDLookAtCamera(const MMDCamera & cam)
	{
		glm::mat4 view(1.0f);
		view = glm::translate(view, glm::vec3(0, 0, std::abs(cam.m_distance)));
		//view = glm::mat4_cast(cam.m_rotate) * view;
		//glm::mat4 rot;
		auto degree = glm::degrees(cam.m_rotate);
		glm::mat4 rot;
		rot = glm::rotate(rot, cam.m_rotate.y, glm::vec3(0, 1, 0));
		rot = glm::rotate(rot, cam.m_rotate.z, glm::vec3(0, 0, -1));
		rot = glm::rotate(rot, cam.m_rotate.x, glm::vec3(1, 0, 0));
		view = rot * view;

		m_eye = glm::vec3(view[3]) + cam.m_interest;
		m_center = glm::mat3(view) * glm::vec3(0, 0, -1) + m_eye;
		m_up = glm::mat3(view) * glm::vec3(0, 1, 0);
	}
}
