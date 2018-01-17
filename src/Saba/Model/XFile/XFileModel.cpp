#include "XFileModel.h"

#define TINYXLOADER_IMPLEMENTATION
#include <tinyxfileloader.h>

#include <map>
#include <sstream>
#include <glm/gtc/matrix_transform.hpp>

#include "../../Base/Path.h"
#include "../../Base/File.h"
#include "../../Base/Log.h"

namespace saba
{
	namespace
	{
		glm::mat4 InvZ(const glm::mat4& m)
		{
			const glm::mat4 invZ = glm::scale(glm::mat4(), glm::vec3(1, 1, -1));
			return invZ * m * invZ;
		}
	}

	bool XFileModel::Load(const char* filepath)
	{
		Destroy();

		TextFileReader textReader;
		if (!textReader.Open(filepath))
		{
			SABA_WARN("Failed to open file.[{}]", filepath);
			return false;
		}
		std::string text = textReader.ReadAll();
		std::stringstream ss(text);

		tinyxfile::XFile xfile;
		tinyxfile::XFileLoader loader;
		if (!loader.Load(ss, &xfile))
		{
			SABA_WARN("Failed to open file.[{}]", filepath);
			SABA_WARN("XFileLoader message : {}", loader.GetErrorMessage());
			return false;
		}


		std::string fileDir = PathUtil::GetDirectoryName(filepath);

		std::map<Frame*, tinyxfile::Frame*> toXFrameMap;
		std::map<tinyxfile::Frame*, Frame*>	fromXFrameMap;
		for (const auto& xfileFrame : xfile.m_frames)
		{
			FrameUPtr newFrame = std::make_unique<Frame>();
			Frame* frame = newFrame.get();
			m_frames.emplace_back(std::move(newFrame));

			toXFrameMap[frame] = xfileFrame.get();
			fromXFrameMap[xfileFrame.get()] = frame;

			frame->m_name = xfileFrame->m_name;
			frame->m_parent = nullptr;
			frame->m_child = nullptr;
			frame->m_next = nullptr;
			frame->m_local[0][0] = xfileFrame->m_transform.m[0];
			frame->m_local[0][1] = xfileFrame->m_transform.m[1];
			frame->m_local[0][2] = xfileFrame->m_transform.m[2];
			frame->m_local[0][3] = xfileFrame->m_transform.m[3];

			frame->m_local[1][0] = xfileFrame->m_transform.m[4];
			frame->m_local[1][1] = xfileFrame->m_transform.m[5];
			frame->m_local[1][2] = xfileFrame->m_transform.m[6];
			frame->m_local[1][3] = xfileFrame->m_transform.m[7];

			frame->m_local[2][0] = xfileFrame->m_transform.m[8];
			frame->m_local[2][1] = xfileFrame->m_transform.m[9];
			frame->m_local[2][2] = xfileFrame->m_transform.m[10];
			frame->m_local[2][3] = xfileFrame->m_transform.m[11];

			frame->m_local[3][0] = xfileFrame->m_transform.m[12];
			frame->m_local[3][1] = xfileFrame->m_transform.m[13];
			frame->m_local[3][2] = xfileFrame->m_transform.m[14];
			frame->m_local[3][3] = xfileFrame->m_transform.m[15];

			frame->m_local = InvZ(frame->m_local);

			if (xfileFrame->m_mesh.m_positions.size() != 0)
			{
				const auto& xfileMesh = xfileFrame->m_mesh;

				// check face num
				const size_t numFaces = xfileMesh.m_positionFaces.size();
				if (numFaces != xfileMesh.m_normalFaces.size() || numFaces != xfileMesh.m_faceMaterials.size())
				{
					SABA_ERROR("XFile face size error.");
					return false;
				}

				// check uv num
				if (xfileMesh.m_positions.size() != xfileMesh.m_textureCoords.size())
				{
					SABA_ERROR("XFile uv size error.");
					return false;
				}

				MeshUPtr newMesh = std::make_unique<Mesh>();
				Mesh* mesh = newMesh.get();
				m_meshes.emplace_back(std::move(newMesh));
				frame->m_mesh = mesh;

				mesh->m_name = xfileMesh.m_name;
				
				// position
				m_bboxMax = glm::vec3(-std::numeric_limits<float>::max());
				m_bboxMin = glm::vec3(std::numeric_limits<float>::max());
				mesh->m_positions.reserve(xfileMesh.m_positions.size());
				for (const auto& xfilePos : xfileMesh.m_positions)
				{
					auto pos = glm::vec3(xfilePos.x, xfilePos.y, -xfilePos.z) * 10.0f;
					mesh->m_positions.push_back(pos);
					m_bboxMax = glm::max(m_bboxMax, pos);
					m_bboxMin = glm::min(m_bboxMin, pos);
				}

				// normal
				mesh->m_normals.reserve(xfileMesh.m_normals.size());
				for (const auto& xfileNor : xfileMesh.m_normals)
				{
					mesh->m_normals.emplace_back(glm::vec3(xfileNor.x, xfileNor.y, -xfileNor.z));
				}

				// uv
				mesh->m_uvs.reserve(xfileMesh.m_textureCoords.size());
				for (const auto& xfileUV : xfileMesh.m_textureCoords)
				{
					mesh->m_uvs.emplace_back(glm::vec2(xfileUV.x, 1.0f - xfileUV.y));
				}

				// material
				mesh->m_materials.reserve(xfileMesh.m_materials.size());
				for (const auto& xfileMat : xfileMesh.m_materials)
				{
					Material mat;
					mat.m_diffuse = glm::vec4(xfileMat.m_diffuse.r, xfileMat.m_diffuse.g, xfileMat.m_diffuse.b, xfileMat.m_diffuse.a);
					mat.m_specular = glm::vec3(xfileMat.m_specular.r, xfileMat.m_specular.g, xfileMat.m_specular.b);
					mat.m_speculatPower = xfileMat.m_specularPower;
					mat.m_emissive = glm::vec3(xfileMat.m_emissive.r, xfileMat.m_emissive.g, xfileMat.m_emissive.b);
					mat.m_spTextureMode = Material::SpTextureMode::None;
					if (!xfileMat.m_texture.empty())
					{
						auto spPos = xfileMat.m_texture.find('*');
						if (spPos == std::string::npos)
						{
							std::string ext = PathUtil::GetExt(xfileMat.m_texture);
							if (ext == "sph")
							{
								mat.m_spTextureMode = Material::SpTextureMode::Mul;
								mat.m_spTexture = PathUtil::Combine(fileDir, xfileMat.m_texture);
							}
							else if (ext == "spa")
							{
								mat.m_spTextureMode = Material::SpTextureMode::Add;
								mat.m_spTexture = PathUtil::Combine(fileDir, xfileMat.m_texture);
							}
							else
							{
								mat.m_texture = PathUtil::Combine(fileDir, xfileMat.m_texture);
							}
						}
						else
						{
							std::string texture = xfileMat.m_texture.substr(0, spPos);
							std::string spTexture = xfileMat.m_texture.substr(spPos + 1);
							if (!texture.empty())
							{
								mat.m_texture = PathUtil::Combine(fileDir, texture);
							}
							mat.m_spTexture = PathUtil::Combine(fileDir, spTexture);

							std::string ext = PathUtil::GetExt(spTexture);
							if (ext == "sph")
							{
								mat.m_spTextureMode = Material::SpTextureMode::Mul;
							}
							else if (ext == "spa")
							{
								mat.m_spTextureMode = Material::SpTextureMode::Add;
							}
						}
					}
					mesh->m_materials.emplace_back(mat);
				}

				// face
				mesh->m_faces.reserve(numFaces);
				for (size_t i = 0; i < numFaces; i++)
				{
					Face face;
					face.m_position[0] = xfileMesh.m_positionFaces[i].v1;
					face.m_position[1] = xfileMesh.m_positionFaces[i].v3;
					face.m_position[2] = xfileMesh.m_positionFaces[i].v2;

					face.m_normal[0] = xfileMesh.m_normalFaces[i].v1;
					face.m_normal[1] = xfileMesh.m_normalFaces[i].v3;
					face.m_normal[2] = xfileMesh.m_normalFaces[i].v2;

					face.m_uv[0] = xfileMesh.m_positionFaces[i].v1;
					face.m_uv[1] = xfileMesh.m_positionFaces[i].v3;
					face.m_uv[2] = xfileMesh.m_positionFaces[i].v2;

					face.m_material = xfileMesh.m_faceMaterials[i];

					mesh->m_faces.emplace_back(face);
				}
			}
		}

		// Frame Link
		for (auto& frame : m_frames)
		{
			const auto& xfileFrame = toXFrameMap[frame.get()];
			if (xfileFrame->m_parent != nullptr)
			{
				frame->m_parent = fromXFrameMap[xfileFrame->m_parent];
			}
			for (const auto& xfileChildFrame : xfileFrame->m_childFrames)
			{
				if (frame->m_child == nullptr)
				{
					frame->m_child = fromXFrameMap[xfileChildFrame];
				}
				else
				{
					Frame* child = frame->m_child;
					while (child->m_next != nullptr)
					{
						child = child->m_next;
					}
					child->m_next = fromXFrameMap[xfileChildFrame];
				}
			}
		}

		// Update Global Transfrom
		for (auto& frame : m_frames)
		{
			if (frame->m_parent == nullptr)
			{
				UpdateGlobalTransform(frame.get());
			}
		}

		return true;
	}

	void XFileModel::Destroy()
	{
		m_meshes.clear();
		m_frames.clear();
	}

	void XFileModel::UpdateGlobalTransform(Frame * frame)
	{
		if (frame->m_parent != nullptr)
		{
			frame->m_global = frame->m_parent->m_global * frame->m_local;
		}
		else
		{
			frame->m_global = frame->m_local;
		}

		auto child = frame->m_child;
		while (child != nullptr)
		{
			UpdateGlobalTransform(child);
			child = child->m_next;
		}
	}
}
