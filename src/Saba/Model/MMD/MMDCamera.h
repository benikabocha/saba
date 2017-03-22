//
// Copyright(c) 2016-2017 benikabocha.
// Distributed under the MIT License (http://opensource.org/licenses/MIT)
//

#ifndef SABA_MODEL_MMD_MMDCAMERA_H_
#define SABA_MODEL_MMD_MMDCAMERA_H_

#include <glm/vec3.hpp>
#include <glm/gtc/quaternion.hpp>

namespace saba
{
	struct MMDCamera
	{
		MMDCamera();

		glm::vec3	m_interest;
		glm::vec3	m_rotate;
		float		m_distance;
		float		m_fov;
	};

	struct MMDLookAtCamera
	{
		explicit MMDLookAtCamera(const MMDCamera& cam);

		glm::vec3	m_center;
		glm::vec3	m_eye;
		glm::vec3	m_up;
	};
}

#endif // !SABA_MODEL_MMD_MMDCAMERA_H_
