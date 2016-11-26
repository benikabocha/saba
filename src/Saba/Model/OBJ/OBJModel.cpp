#include "OBJModel.h"
#include "../../Base/Path.h"
#include "../../Base/Log.h"

#include <iostream>
#include <limits>
#include <glm/glm.hpp>
#include <tiny_obj_loader.h>

namespace saba
{
	bool OBJModel::Load(const char * filepath)
	{
		SABA_INFO("OBJ File Open. {}", filepath);

		std::string fileDir = PathUtil::GetDirectoryName(filepath);
		fileDir += PathUtil::GetDelimiter();
		tinyobj::attrib_t attrib;
		std::vector<tinyobj::shape_t> shapes;
		std::vector<tinyobj::material_t> materials;
		std::string err;
		bool ret = tinyobj::LoadObj(
			&attrib,
			&shapes,
			&materials,
			&err,
			filepath,
			fileDir.c_str(),
			true
		);

		if (!ret)
		{
			SABA_WARN("OBJ File Fail. {}", filepath);
			return false;
		}

		// MaterialÇÉRÉsÅ[
		m_materials.clear();
		m_materials.reserve(materials.size());
		for (const auto& objMat : materials)
		{
			Material mat;
			mat.m_name = objMat.name;

			mat.m_ambient.r = objMat.ambient[0];
			mat.m_ambient.g = objMat.ambient[1];
			mat.m_ambient.b = objMat.ambient[2];

			mat.m_diffuse.r = objMat.diffuse[0];
			mat.m_diffuse.g = objMat.diffuse[1];
			mat.m_diffuse.b = objMat.diffuse[2];

			mat.m_specular.r = objMat.specular[0];
			mat.m_specular.g = objMat.specular[1];
			mat.m_specular.b = objMat.specular[2];

			mat.m_specularPower = objMat.shininess;

			mat.m_transparency = objMat.dissolve;

			if (!objMat.ambient_texname.empty())
			{
				mat.m_ambientTex = PathUtil::Combine(fileDir, objMat.ambient_texname);
			}
			if (!objMat.diffuse_texname.empty())
			{
				mat.m_diffuseTex = PathUtil::Combine(fileDir, objMat.diffuse_texname);
			}
			if (!objMat.specular_texname.empty())
			{
				mat.m_specularTex = PathUtil::Combine(fileDir, objMat.specular_texname);
			}
			if (!objMat.alpha_texname.empty())
			{
				mat.m_transparencyTex = PathUtil::Combine(fileDir, objMat.alpha_texname);
			}

			m_materials.push_back(mat);
		}

		// MeshÇçÏê¨
		size_t posCount = attrib.vertices.size() / 3;
		size_t norCount = attrib.normals.size() / 3;
		size_t uvCount = attrib.texcoords.size() / 2;
		m_positions.resize(posCount);
		m_normals.resize(norCount);
		m_uvs.resize(uvCount);

		for (size_t posIdx = 0; posIdx < posCount; posIdx++)
		{
			m_positions[posIdx].x = attrib.vertices[posIdx * 3 + 0];
			m_positions[posIdx].y = attrib.vertices[posIdx * 3 + 1];
			m_positions[posIdx].z = attrib.vertices[posIdx * 3 + 2];
		}
		for (size_t norIdx = 0; norIdx < norCount; norIdx++)
		{
			m_normals[norIdx].x = attrib.normals[norIdx * 3 + 0];
			m_normals[norIdx].y = attrib.normals[norIdx * 3 + 1];
			m_normals[norIdx].z = attrib.normals[norIdx * 3 + 2];
		}
		for (size_t uvIdx = 0; uvIdx < uvCount; uvIdx++)
		{
			m_uvs[uvIdx].x = attrib.texcoords[uvIdx * 2 + 0];
			m_uvs[uvIdx].y = attrib.texcoords[uvIdx * 2 + 1];
		}

		if (!m_positions.empty())
		{
			m_bboxMin = glm::vec3(std::numeric_limits<float>::max());
			m_bboxMax = glm::vec3(-std::numeric_limits<float>::max());
			for (const auto& vec : m_positions)
			{
				m_bboxMin = glm::min(m_bboxMin, vec);
				m_bboxMax = glm::max(m_bboxMax, vec);
			}
		}
		else
		{
			m_bboxMin = glm::vec3(0);
			m_bboxMax = glm::vec3(0);
		}

		int emptyMatIdx = -1;
		for (const auto& shape : shapes)
		{
			int indexOffset = 0;
			for (size_t faceIdx = 0; faceIdx < shape.mesh.num_face_vertices.size(); faceIdx++)
			{
				auto numFaceVertices = shape.mesh.num_face_vertices[faceIdx];
				if (numFaceVertices != 3)
				{
					SABA_WARN("[num_face_vertices] != 3");
					SABA_WARN("OBJ File Fail. {}", filepath);
					return false;
				}

				auto vi0 = shape.mesh.indices[indexOffset + 0];
				auto vi1 = shape.mesh.indices[indexOffset + 1];
				auto vi2 = shape.mesh.indices[indexOffset + 2];
				auto material = shape.mesh.material_ids[faceIdx];

				if (material == -1)
				{
					if (emptyMatIdx == -1)
					{
						SABA_INFO("Material Not Assigned.");
						Material emptyMat;
						emptyMatIdx = (int)m_materials.size();
						emptyMat.m_ambient = glm::vec3(0.2f);
						emptyMat.m_diffuse = glm::vec3(0.5f);
						emptyMat.m_specularPower = 1.0f;
						m_materials.push_back(emptyMat);
					}
					material = emptyMatIdx;
				}

				Face face;
				face.m_position[0] = vi0.vertex_index;
				face.m_position[1] = vi1.vertex_index;
				face.m_position[2] = vi2.vertex_index;
				face.m_normal[0] = vi0.normal_index;
				face.m_normal[1] = vi1.normal_index;
				face.m_normal[2] = vi2.normal_index;
				face.m_uv[0] = vi0.texcoord_index;
				face.m_uv[1] = vi1.texcoord_index;
				face.m_uv[2] = vi2.texcoord_index;
				face.m_material = material;

				m_faces.emplace_back(std::move(face));

				indexOffset += 3;
			}
		}

		SABA_INFO("OBJ File Success. {}", filepath);
		return true;
	}

	void OBJModel::Destroy()
	{
		m_positions.clear();
		m_normals.clear();
		m_uvs.clear();
		m_materials.clear();
		m_faces.clear();
	}

}
