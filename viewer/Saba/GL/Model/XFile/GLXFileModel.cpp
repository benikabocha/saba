//
// Copyright(c) 2016-2017 benikabocha.
// Distributed under the MIT License (http://opensource.org/licenses/MIT)
//

#include "GLXFileModel.h"

#include <Saba/Base/Path.h>

#include "../../../Viewer/ViewerContext.h"
#include "../../GLTextureUtil.h"

namespace saba
{
	GLXFileModel::GLXFileModel()
	{
	}

	GLXFileModel::~GLXFileModel()
	{
		Destroy();
	}

	bool GLXFileModel::Create(ViewerContext* ctxt, const XFileModel& xfileModel)
	{
		size_t numFrames = xfileModel.GetFrameCount();
		for (size_t i = 0; i < numFrames; i++)
		{
			const auto& frame = xfileModel.GetFrame(i);
			if (frame->m_mesh == nullptr)
			{
				continue;
			}

			MeshUPtr newMesh = std::make_unique<Mesh>();
			Mesh* mesh = newMesh.get();
			m_meshes.emplace_back(std::move(newMesh));

			const auto& xmesh = frame->m_mesh;

			mesh->m_transform = frame->m_global;

			// copy materials
			mesh->m_materials.reserve(xmesh->m_materials.size());
			for (const auto& xmat : xmesh->m_materials)
			{
				Material mat;
				mat.m_diffuse = xmat.m_diffuse;
				mat.m_specular = xmat.m_specular;
				mat.m_specularPower = xmat.m_speculatPower;
				mat.m_emissive = xmat.m_emissive;
				if (!xmat.m_texture.empty())
				{
					auto texname = PathUtil::GetFilenameWithoutExt(xmat.m_texture);
					if (texname == "screen")
					{
						mat.m_texture = ctxt->GetCaptureTexture();
					}
					else
					{
						auto tex = CreateTextureFromFile(xmat.m_texture);
						mat.m_texture = std::move(tex);
					}
				}
				mat.m_spTextureMode = xmat.m_spTextureMode;
				if (!xmat.m_spTexture.empty())
				{
					auto tex = CreateTextureFromFile(xmat.m_spTexture);
					mat.m_spTexture = std::move(tex);
				}
				mesh->m_materials.emplace_back(std::move(mat));
			}

			// build mesh
			using MB = MeshBuilder;
			MB mb;
			std::vector<glm::vec3>* positions;
			std::vector<glm::vec3>* normals;
			std::vector<glm::vec2>* uvs;
			auto posID = mb.AddComponent(&positions);
			auto norID = mb.AddComponent(&normals);
			auto uvID = mb.AddComponent(&uvs);
			*positions = xmesh->m_positions;
			*normals = xmesh->m_normals;
			*uvs = xmesh->m_uvs;

			for (const auto& xface : xmesh->m_faces)
			{
				auto polyID = mb.AddPolygon();
				mb.SetPolygon(polyID, posID, xface.m_position[0], xface.m_position[1], xface.m_position[2]);
				mb.SetPolygon(polyID, norID, xface.m_normal[0], xface.m_normal[1], xface.m_normal[2]);
				mb.SetPolygon(polyID, uvID, xface.m_uv[0], xface.m_uv[1], xface.m_uv[2]);
				mb.SetFaceMaterial(polyID, xface.m_material);
			}
			mb.SortFaces();

			std::vector<MB::SubMesh> mbSubMeshes;
			mb.MakeSubMeshList(&mbSubMeshes);
			for (const auto& mbSubMesh : mbSubMeshes)
			{
				SubMesh subMesh;
				subMesh.m_beginIndex = (int)mbSubMesh.m_startIndex;
				subMesh.m_vertexCount = (int)mbSubMesh.m_numVertices;
				subMesh.m_materialID = mbSubMesh.m_material;
				mesh->m_subMeshes.emplace_back(std::move(subMesh));
			}

			mesh->m_posVBO = mb.CreateVBO(posID);
			mesh->m_norVBO = mb.CreateVBO(norID);
			mesh->m_uvVBO = mb.CreateVBO(uvID);

			mesh->m_posBinder = mb.MakeVertexBinder(posID);
			mesh->m_norBinder = mb.MakeVertexBinder(norID);
			mesh->m_uvBinder = mb.MakeVertexBinder(uvID);
		}

		m_bboxMax = xfileModel.GetBBoxMax();
		m_bboxMin = xfileModel.GetBBoxMin();

		return true;
	}

	void GLXFileModel::Destroy()
	{
		m_meshes.clear();
	}

}
