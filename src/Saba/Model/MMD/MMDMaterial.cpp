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
		, m_textureMulFactor(1)
		, m_spTextureMulFactor(1)
		, m_toonTextureMulFactor(1)
		, m_textureAddFactor(0)
		, m_spTextureAddFactor(0)
		, m_toonTextureAddFactor(0)
		, m_bothFace(false)
	{
	}
}

