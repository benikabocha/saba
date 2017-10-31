//
// Copyright(c) 2016-2017 benikabocha.
// Distributed under the MIT License (http://opensource.org/licenses/MIT)
//

#include "Camera.h"

#include <glm/gtc/matrix_transform.hpp>

namespace saba
{
	Camera::Camera()
		: m_target(0)
		, m_eye(0, 0, 1)
		, m_up(0, 1, 0)
		, m_radius(1.0f)
		, m_fovYRad(glm::radians(30.0f))
		, m_nearClip(0.01f)
		, m_farClip(100.0f)
		, m_width(1)
		, m_height(1)
	{
	}

	void Camera::Initialize(const glm::vec3 & center, float radius)
	{
		m_target = center;
		m_eye = center + glm::vec3(0.5f, 0.5f, 1) * radius * 5.0f;
		m_up = glm::vec3(0, 1, 0);
		m_radius = radius;

		m_nearClip = radius * 0.01f;
		m_farClip = radius * 100.0f;

		UpdateMatrix();
	}

	void Camera::Initialize(const glm::vec3 & center, glm::vec3 & eye, float nearClip, float farClip, float radius)
	{
		m_target = center;
		m_eye = eye;
		m_up = glm::vec3(0, 1, 0);
		m_radius = radius;
		m_nearClip = nearClip;
		m_farClip = farClip;
	}

	namespace
	{
		glm::vec2 VectorToLatLong(const glm::vec3& vec)
		{
			const float phi = std::atan2(vec.x, vec.z);
			const float theta = std::acos(vec.y);

			return glm::vec2(
				(glm::pi<float>() + phi) / glm::pi<float>() * 0.5f,
				theta / glm::pi<float>()
			);
		}

		glm::vec3 LatLongToVector(const glm::vec2& latLong)
		{
			const float phi = latLong.x * 2.0f * glm::pi<float>();
			const float theta = latLong.y * glm::pi<float>();

			const float st = std::sin(theta);
			const float sp = std::sin(phi);
			const float ct = std::cos(theta);
			const float cp = std::cos(phi);

			return glm::vec3(-st*sp, ct, -st*cp);
		}
	}

	void Camera::Orbit(float x, float y)
	{
		auto toEye = m_eye - m_target;
		auto toEyeLen = glm::length(toEye);
		auto toEyeNormal = toEye / toEyeLen;

		auto latLong = VectorToLatLong(toEyeNormal);
		latLong.x -= x;
		latLong.y -= y;
		latLong.y = glm::clamp(latLong.y, 0.02f, 0.98f);

		auto newToEyeNormal = LatLongToVector(latLong);
		auto newToEye = newToEyeNormal * toEyeLen;
		m_eye = m_target + newToEye;
	}

	void Camera::Dolly(float z)
	{
		const float minLen = 0.01f * m_radius;
		const float maxLen = 10.0f * m_radius;

		auto toTarget = m_target - m_eye;
		auto toTargetLen = glm::length(toTarget);
		auto toTargetNormal = toTarget / toTargetLen;

		auto delta = toTargetLen * z;
		auto newLen = toTargetLen + delta;

		if ((minLen < newLen || z < 0.0f) &&
			(newLen < maxLen || z > 0.0f)
			)
		{
			m_eye += toTargetNormal * delta;
		}
	}

	void Camera::Pan(float x, float y)
	{
		float len = glm::length(m_target - m_eye);
		float ay = std::tan(m_fovYRad) * len;
		float ax = ay * (m_width / m_height);
		float dy = ay * y;
		float dx = ax * -x;

		glm::vec3 zAxis = glm::normalize(m_eye - m_target);
		glm::vec3 xAxis = glm::normalize(glm::cross(m_up, zAxis));
		glm::vec3 yAxis = glm::normalize(glm::cross(zAxis, xAxis));
		m_target += dx * xAxis + dy * yAxis;
		m_eye += dx * xAxis + dy * yAxis;
	}

	void Camera::LookAt(const glm::vec3 & center, const glm::vec3 & eye, const glm::vec3 & up)
	{
		m_target = center;
		m_eye = eye;
		m_up = up;
	}

	void Camera::UpdateMatrix()
	{
		m_viewMatrix = glm::lookAtRH(m_eye, m_target, m_up);

		if (m_width <= 0 || m_height <= 0)
		{
			return;
		}
		m_projectionMatrix = glm::perspectiveFovRH(
			m_fovYRad,
			m_width,
			m_height,
			m_nearClip,
			m_farClip
		);
	}

	const glm::mat4& Camera::GetViewMatrix() const
	{
		return m_viewMatrix;
	}

	void Camera::SetFovY(float fovY)
	{
		m_fovYRad = fovY;
	}

	void Camera::SetSize(float w, float h)
	{
		m_width = w;
		m_height = h;
	}

	void Camera::SetClip(float nearClip, float farClip)
	{
		m_nearClip = nearClip;
		m_farClip = farClip;
	}

	const glm::mat4& Camera::GetProjectionMatrix() const
	{
		return m_projectionMatrix;
	}

	glm::vec3 Camera::GetEyePostion() const
	{
		return m_eye;
	}

	glm::vec3 Camera::GetUp() const
	{
		return glm::normalize(m_up);
	}

	glm::vec3 Camera::GetForward() const
	{
		return glm::normalize(m_target - m_eye);
	}

	float Camera::GetFovY() const
	{
		return m_fovYRad;
	}

	float Camera::GetNearClip() const
	{
		return m_nearClip;
	}

	float Camera::GetFarClip() const
	{
		return m_farClip;
	}

	float Camera::GetWidth() const
	{
		return m_width;
	}

	float Camera::GetHeight() const
	{
		return m_height;
	}
}

