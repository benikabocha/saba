#ifndef SABA_VIEWER_CAMERA_H_
#define SABA_VIEWER_CAMERA_H_

#include <glm/vec3.hpp>
#include <glm/mat4x4.hpp>

namespace saba
{
	class Camera
	{
	public:
		Camera();

		void Initialize(const glm::vec3& center, float radius);

		void Orbit(float x, float y);
		void Dolly(float z);
		void Pan(float x, float y);

		void SetFovY(float fovY);
		void SetSize(float w, float h);
		void SetClip(float nearClip, float farClip);

		void UpdateMatrix();
		const glm::mat4& GetViewMatrix() const;
		const glm::mat4& GetProjectionMatrix() const;

	private:
		// View
		glm::vec3	m_target;
		glm::vec3	m_eye;
		glm::vec3	m_up;
		float		m_radius;

		// Projection
		float	m_fovYRad;
		float	m_nearClip;
		float	m_farClip;
		float	m_width;
		float	m_height;

		// Matrix
		glm::mat4	m_viewMatrix;
		glm::mat4	m_projectionMatrix;
	};
}

#endif // !SABA_VIEWER_CAMERA_H_

