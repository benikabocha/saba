#ifndef SABA_MODEL_MMD_MMDMORPH_H
#define SABA_MODEL_MMD_MMDMORPH_H

#include <string>

namespace saba
{
	class MMDMorph
	{
	public:
		MMDMorph() {}

		void SetName(const std::string& name) { m_name = name; }
		const std::string& GetName() const { return m_name; }

		void SetWeight(float weight) { m_weight = weight; }
		float GetWeight() const { return m_weight; }

	private:
		std::string	m_name;
		float		m_weight;
	};
}

#endif // !SABA_MODEL_MMD_MMDMORPH_H
