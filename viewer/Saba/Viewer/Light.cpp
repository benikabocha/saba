//
// Copyright(c) 2016-2017 benikabocha.
// Distributed under the MIT License (http://opensource.org/licenses/MIT)
//

#include "Light.h"

namespace saba
{
	Light::Light()
		: m_lightColor(0.6f)
		, m_lightDir(-0.5f, -1.0f, -0.5f)
	{
	}

	void Light::SetLightColor(const glm::vec3 & color)
	{
		m_lightColor = color;
	}

	const glm::vec3 & Light::GetLightColor() const
	{
		return m_lightColor;
	}

	void Light::SetLightDirection(const glm::vec3 & dir)
	{
		m_lightDir = dir;
	}

	const glm::vec3 & Light::GetLightDirection() const
	{
		return m_lightDir;
	}

}

