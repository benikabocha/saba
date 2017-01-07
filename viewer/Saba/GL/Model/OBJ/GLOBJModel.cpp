//
// Copyright(c) 2016-2017 benikabocha.
// Distributed under the MIT License (http://opensource.org/licenses/MIT)
//

#include "GLOBJModel.h"

#include "../../GLTextureUtil.h"

namespace saba
{
	GLOBJModel::GLOBJModel()
	{
	}

	GLOBJModel::~GLOBJModel()
	{
		Destroy();
	}

	bool GLOBJModel::Create(const OBJModel& objModel)
	{
		// Materialをコピー
		auto materials = objModel.GetMaterials();

		m_materials.clear();
		m_materials.reserve(materials.size());
		for (const auto& objMat : materials)
		{
			Material mat;
			mat.m_name = objMat.m_name;

			mat.m_ambient = objMat.m_ambient;
			mat.m_diffuse = objMat.m_diffuse;
			mat.m_specular = objMat.m_specular;
			mat.m_specularPower = objMat.m_specularPower;
			mat.m_transparency = objMat.m_transparency;

			if (!objMat.m_ambientTex.empty())
			{
				auto tex = CreateTextureFromFile(objMat.m_ambientTex);
				mat.m_ambientTex = std::move(tex);
			}
			if (!objMat.m_diffuseTex.empty())
			{
				auto tex = CreateTextureFromFile(objMat.m_diffuseTex);
				mat.m_diffuseTex = std::move(tex);
			}
			if (!objMat.m_specularTex.empty())
			{
				auto tex = CreateTextureFromFile(objMat.m_specularTex);
				mat.m_specularTex = std::move(tex);
			}
			if (!objMat.m_transparencyTex.empty())
			{
				auto tex = CreateTextureFromFile(objMat.m_transparencyTex);
				mat.m_transparencyTex = std::move(tex);
			}

			m_materials.push_back(mat);
		}

		// Meshを作成
		using MB = MeshBuilder;
		MB mb;
		auto objPos = objModel.GetPositions();
		auto objNor = objModel.GetNormals();
		auto objUV = objModel.GetUVs();
		std::vector<glm::vec3>* positions;
		std::vector<glm::vec3>* normals;
		std::vector<glm::vec2>* uvs;
		auto posID = mb.AddComponent(&positions);
		auto norID = mb.AddComponent(&normals);
		auto uvID = mb.AddComponent(&uvs);
		*positions = objPos;
		*normals = objNor;
		*uvs = objUV;
		m_bboxMin = objModel.GetBBoxMin();
		m_bboxMax = objModel.GetBBoxMax();

		auto objFaces = objModel.GetFaces();
		for (const auto& face : objFaces)
		{
			auto polyID = mb.AddPolygon();
			mb.SetPolygon(polyID, posID, face.m_position[0], face.m_position[1], face.m_position[2]);
			mb.SetPolygon(polyID, norID, face.m_normal[0], face.m_normal[1], face.m_normal[2]);
			mb.SetPolygon(polyID, uvID, face.m_uv[0], face.m_uv[1], face.m_uv[2]);
			mb.SetFaceMaterial(polyID, face.m_material);
		}
		mb.SortFaces();

		std::vector<MB::SubMesh> objSubMeshes;
		mb.MakeSubMeshList(&objSubMeshes);

		m_subMeshes.clear();
		m_subMeshes.reserve(objSubMeshes.size());
		for (const auto& objSubMesh : objSubMeshes)
		{
			SubMesh subMesh;
			subMesh.m_beginIndex = (int)objSubMesh.m_startIndex;
			subMesh.m_vertexCount = (int)objSubMesh.m_numVertices;
			subMesh.m_materialID = objSubMesh.m_material;
			m_subMeshes.push_back(subMesh);
		}

		m_posVBO = mb.CreateVBO(posID);
		m_norVBO = mb.CreateVBO(norID);
		m_uvVBO = mb.CreateVBO(uvID);

		m_posBinder = mb.MakeVertexBinder(posID);
		m_norBinder = mb.MakeVertexBinder(norID);
		m_uvBinder = mb.MakeVertexBinder(uvID);

		return true;
	}

	void GLOBJModel::Destroy()
	{
		m_posVBO.Destroy();
		m_norVBO.Destroy();
		m_uvVBO.Destroy();
		m_materials.clear();
		m_subMeshes.clear();
	}

}
