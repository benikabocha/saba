//
// Copyright(c) 2016-2017 benikabocha.
// Distributed under the MIT License (http://opensource.org/licenses/MIT)
//

#ifndef SABA_MODEL_MMD_MMDMATERIAL_H_
#define SABA_MODEL_MMD_MMDMATERIAL_H_

#include <string>
#include <cstdint>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

namespace saba
{
	struct MMDMaterial
	{
		MMDMaterial();
		enum class SphereTextureMode
		{
			None,
			Mul,
			Add,
		};

		glm::vec3		m_diffuse;
		float			m_alpha;
		glm::vec3		m_specular;
		float			m_specularPower;
		glm::vec3		m_ambient;
		uint8_t			m_edgeFlag;
		float			m_edgeSize;
		glm::vec4		m_edgeColor;
		std::string		m_texture;
		std::string		m_spTexture;
		SphereTextureMode	m_spTextureMode;
		std::string		m_toonTexture;
		glm::vec4		m_textureMulFactor;
		glm::vec4		m_spTextureMulFactor;
		glm::vec4		m_toonTextureMulFactor;
		glm::vec4		m_textureAddFactor;
		glm::vec4		m_spTextureAddFactor;
		glm::vec4		m_toonTextureAddFactor;
		bool			m_bothFace;
		bool			m_groundShadow;
		bool			m_shadowCaster;
		bool			m_shadowReceiver;
	};
}

#endif // !SABA_MODEL_MMD_MMDMATERIAL_H_
