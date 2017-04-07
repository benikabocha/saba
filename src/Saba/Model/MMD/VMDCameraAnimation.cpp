#include "VMDCameraAnimation.h"

#include <glm/gtc/matrix_transform.hpp>

namespace saba
{
	namespace
	{
		void SetVMDBezier(VMDBezier& bezier, int x0, int x1, int y0, int y1)
		{
			bezier.m_cp[0] = glm::vec2(0, 0);
			bezier.m_cp[1] = glm::vec2((float)x0 / 127.0f, (float)y0 / 127.0f);
			bezier.m_cp[2] = glm::vec2((float)x1 / 127.0f, (float)y1 / 127.0f);
			bezier.m_cp[3] = glm::vec2(1, 1);
		}
	} // namespace


	VMDCameraController::VMDCameraController()
	{
	}

	void VMDCameraController::Evaluate(float t)
	{
		if (m_keys.empty())
		{
			return;
		}

		auto boundIt = std::upper_bound(
			std::begin(m_keys),
			std::end(m_keys),
			int32_t(t),
			[](int32_t lhs, const KeyType& rhs) { return lhs < rhs.m_time; }
		);
		if (boundIt == std::end(m_keys))
		{
			const auto& selectKey = m_keys[m_keys.size() - 1];
			m_camera.m_interest = selectKey.m_interest;
			m_camera.m_rotate = selectKey.m_rotate;
			m_camera.m_distance = selectKey.m_distance;
			m_camera.m_fov = selectKey.m_fov;
		}
		else
		{
			const auto& selectKey = (*boundIt);
			m_camera.m_interest = selectKey.m_interest;
			m_camera.m_rotate = selectKey.m_rotate;
			m_camera.m_distance = selectKey.m_distance;
			m_camera.m_fov = selectKey.m_fov;
			if (boundIt != std::begin(m_keys))
			{
				const auto& key0 = *(boundIt - 1);
				const auto& key1 = *boundIt;

				if ((key1.m_time - key0.m_time) > 1)
				{
					float timeRange = float(key1.m_time - key0.m_time);
					float time = (t - float(key0.m_time)) / timeRange;
					float ix_x = key0.m_ixBezier.FindBezierX(time);
					float iy_x = key0.m_iyBezier.FindBezierX(time);
					float iz_x = key0.m_izBezier.FindBezierX(time);
					float rotate_x = key0.m_rotateBezier.FindBezierX(time);
					float distance_x = key0.m_distanceBezier.FindBezierX(time);
					float fov_x = key0.m_fovBezier.FindBezierX(time);

					float ix_y = key0.m_ixBezier.EvalY(ix_x);
					float iy_y = key0.m_iyBezier.EvalY(iy_x);
					float iz_y = key0.m_izBezier.EvalY(iz_x);
					float rotate_y = key0.m_rotateBezier.EvalY(rotate_x);
					float distance_y = key0.m_distanceBezier.EvalY(distance_x);
					float fov_y = key0.m_fovBezier.EvalY(fov_x);

					m_camera.m_interest = glm::mix(key0.m_interest, key1.m_interest, glm::vec3(ix_y, iy_y, iz_y));
					m_camera.m_rotate = glm::mix(key0.m_rotate, key1.m_rotate, rotate_y);
					m_camera.m_distance = glm::mix(key0.m_distance, key1.m_distance, distance_y);
					m_camera.m_fov = glm::mix(key0.m_fov, key1.m_fov, fov_y);
				}
				else
				{
					/*
					カメラアニメーションでキーが1フレーム間隔で打たれている場合、
					カメラの切り替えと判定し補間を行わないようにする（key0を使用する）
					*/
					m_camera.m_interest = key0.m_interest;
					m_camera.m_rotate = key0.m_rotate;
					m_camera.m_distance = key0.m_distance;
					m_camera.m_fov = key0.m_fov;
				}
			}
		}

	}

	void VMDCameraController::SortKeys()
	{
		std::sort(
			std::begin(m_keys),
			std::end(m_keys),
			[](const KeyType& a, const KeyType& b) { return a.m_time < b.m_time; }
		);
	}

	VMDCameraAnimation::VMDCameraAnimation()
	{
		Destroy();
	}

	bool VMDCameraAnimation::Create(const VMDFile & vmd)
	{
		if (!vmd.m_cameras.empty())
		{
			m_cameraController = std::make_unique<VMDCameraController>();
			for (const auto& cam : vmd.m_cameras)
			{
				VMDCameraAnimationKey key;
				key.m_time = int32_t(cam.m_frame);
				key.m_interest = cam.m_interest * glm::vec3(1, 1, -1);
				key.m_rotate = cam.m_rotate;
				key.m_distance = cam.m_distance;
				key.m_fov = glm::radians((float)cam.m_viewAngle);

				const uint8_t* ip = cam.m_interpolation.data();
				SetVMDBezier(key.m_ixBezier, ip[0], ip[1], ip[2], ip[3]);
				SetVMDBezier(key.m_iyBezier, ip[4], ip[5], ip[6], ip[7]);
				SetVMDBezier(key.m_izBezier, ip[8], ip[9], ip[10], ip[11]);
				SetVMDBezier(key.m_rotateBezier, ip[12], ip[13], ip[14], ip[15]);
				SetVMDBezier(key.m_distanceBezier, ip[16], ip[17], ip[18], ip[19]);
				SetVMDBezier(key.m_fovBezier, ip[20], ip[21], ip[22], ip[23]);

				m_cameraController->AddKey(key);
			}
			m_cameraController->SortKeys();
		}
		else
		{
			return false;
		}
		return true;
	}

	void VMDCameraAnimation::Destroy()
	{
		m_cameraController.reset();
	}

	void VMDCameraAnimation::Evaluate(float t)
	{
		m_cameraController->Evaluate(t);
		m_camera = m_cameraController->GetCamera();
	}


}

