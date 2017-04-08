//
// Copyright(c) 2016-2017 benikabocha.
// Distributed under the MIT License (http://opensource.org/licenses/MIT)
//

#ifndef SABA_VIEWER_LIGHT_H_
#define SABA_VIEWER_LIGHT_H_

#include <glm/vec3.hpp>

namespace saba
{
	class Light
	{
	public:
		Light();

		void SetLightColor(const glm::vec3& color);
		const glm::vec3& GetLightColor() const;

		void SetLightDirection(const glm::vec3& dir);
		const glm::vec3& GetLightDirection() const;

	private:
		glm::vec3	m_lightColor;
		glm::vec3	m_lightDir;
	};
}

#endif // !SABA_VIEWER_LIGHT_H_

