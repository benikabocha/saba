//
// Copyright(c) 2016-2017 benikabocha.
// Distributed under the MIT License (http://opensource.org/licenses/MIT)
//

#include "MMDMaterial.h"

namespace saba
{
	MMDMaterial::MMDMaterial()
		: m_diffuse(1)
		, m_alpha(1)
		, m_specular(0)
		, m_specularPower(1)
		, m_ambient(0.2f)
		, m_edgeFlag(0)
		, m_edgeSize(0)
		, m_edgeColor(0.0f, 0.0f, 0.0f, 1.0f)
		, m_textureMulFactor(1)
		, m_spTextureMulFactor(1)
		, m_toonTextureMulFactor(1)
		, m_textureAddFactor(0)
		, m_spTextureAddFactor(0)
		, m_toonTextureAddFactor(0)
		, m_groundShadow(true)
		, m_shadowCaster(true)
		, m_shadowReceiver(true)
	{
	}
}

