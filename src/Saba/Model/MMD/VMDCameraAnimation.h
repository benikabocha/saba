//
// Copyright(c) 2016-2017 benikabocha.
// Distributed under the MIT License (http://opensource.org/licenses/MIT)
//

#ifndef SABA_MODEL_MMD_VMDCAMERAANIMATION_H_
#define SABA_MODEL_MMD_VMDCAMERAANIMATION_H_

#include "MMDCamera.h"
#include "VMDAnimation.h"

#include <cstdint>
#include <memory>

namespace saba
{
	struct VMDCameraAnimationKey
	{
		int32_t		m_time;
		glm::vec3	m_interest;
		glm::vec3	m_rotate;
		float		m_distance;
		float		m_fov;

		VMDBezier	m_ixBezier;
		VMDBezier	m_iyBezier;
		VMDBezier	m_izBezier;
		VMDBezier	m_rotateBezier;
		VMDBezier	m_distanceBezier;
		VMDBezier	m_fovBezier;
	};

	class VMDCameraController
	{
	public:
		using KeyType = VMDCameraAnimationKey;

		VMDCameraController();

		void Evaluate(float t);
		const MMDCamera& GetCamera() { return m_camera; }

		void AddKey(const KeyType& key)
		{
			m_keys.push_back(key);
		}
		void SortKeys();

	private:
		std::vector<VMDCameraAnimationKey>	m_keys;
		MMDCamera							m_camera;
	};

	class VMDCameraAnimation
	{
	public:
		VMDCameraAnimation();

		bool Create(const VMDFile& vmd);
		void Destroy();

		void Evaluate(float t);

		const MMDCamera& GetCamera() const { return m_camera; }

	private:
		using CameraControllerPtr = std::unique_ptr<VMDCameraController>;

		CameraControllerPtr	m_cameraController;

		MMDCamera	m_camera;
	};

}

#endif // !SABA_MODEL_MMD_VMDCAMERAANIMATION_H_
