#ifndef SABA_MODEL_MMD_MMDMATERIAL_H_
#define SABA_MODEL_MMD_MMDMATERIAL_H_

#include <string>
#include <cstdint>
#include <glm/vec3.hpp>

namespace saba
{
	struct MMDMaterial
	{
		enum class SphereTextureMode
		{
			None,
			Mul,
			Add,
		};

		glm::vec3		m_diffuse;
		float			m_alpha;
		float			m_specularPower;
		glm::vec3		m_specular;
		glm::vec3		m_ambient;
		uint8_t			m_edgeFlag;
		std::string		m_texture;
		std::string		m_spTexture;
		SphereTextureMode	m_spTextureMode;
		std::string		m_toonTexture;
		bool			m_bothFace;
	};
}

#endif // !SABA_MODEL_MMD_MMDMATERIAL_H_
