//
// Copyright(c) 2016-2017 benikabocha.
// Distributed under the MIT License (http://opensource.org/licenses/MIT)
//

#ifndef SABA_MODEL_OBJ_OBJMODEL_H_
#define SABA_MODEL_OBJ_OBJMODEL_H_

#include <vector>
#include <string>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

namespace saba
{
	class OBJModel
	{
	public:
		struct Material
		{
			std::string	m_name;

			glm::vec3	m_ambient;
			glm::vec3	m_diffuse;
			glm::vec3	m_specular;
			float		m_specularPower;
			float		m_transparency;

			std::string	m_ambientTex;
			std::string	m_diffuseTex;
			std::string	m_specularTex;
			std::string	m_transparencyTex;
		};

		struct Face
		{
			int	m_position[3];
			int	m_normal[3];
			int	m_uv[3];
			int m_material;
		};

	public:
		bool Load(const char* filepath);
		void Destroy();

		const std::vector<glm::vec3>& GetPositions() const { return m_positions; }
		const std::vector<glm::vec3>& GetNormals() const { return m_normals; }
		const std::vector<glm::vec2>& GetUVs() const { return m_uvs; }
		const std::vector<Material>& GetMaterials() const { return m_materials; }
		const std::vector<Face>& GetFaces() const { return m_faces; }

		const glm::vec3& GetBBoxMin() const { return m_bboxMin; }
		const glm::vec3& GetBBoxMax() const { return m_bboxMax; }

	private:
		std::vector<glm::vec3>	m_positions;
		std::vector<glm::vec3>	m_normals;
		std::vector<glm::vec2>	m_uvs;

		glm::vec3		m_bboxMin;
		glm::vec3		m_bboxMax;

		std::vector<Material>	m_materials;
		std::vector<Face>		m_faces;
	};
}

#endif // !SABA_MODEL_OBJ_OBJMODEL_H_

