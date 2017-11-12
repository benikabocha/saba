//
// Copyright(c) 2016-2017 benikabocha.
// Distributed under the MIT License (http://opensource.org/licenses/MIT)
//

#ifndef SABA_MODEL_MMD_VPDFILE_H_
#define SABA_MODEL_MMD_VPDFILE_H_

#include <vector>
#include <string>

#include <glm/vec3.hpp>
#include <glm/gtc/quaternion.hpp>

namespace saba
{
	struct VPDBone
	{
		std::string	m_boneName;
		glm::vec3	m_translate;
		glm::quat	m_quaternion;
	};

	struct VPDMorph
	{
		std::string	m_morphName;
		float		m_weight;
	};

	struct VPDFile
	{
		std::vector<VPDBone>	m_bones;
		std::vector<VPDMorph>	m_morphs;
	};

	bool ReadVPDFile(VPDFile* vpd, const char* filename);
}

#endif // !SABA_MODEL_MMD_VPDFILE_H_
