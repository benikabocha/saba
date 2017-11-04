//
// Copyright(c) 2016-2017 benikabocha.
// Distributed under the MIT License (http://opensource.org/licenses/MIT)
//


#ifndef SABA_MODEL_XFILE_OBJMODEL_H_
#define SABA_MODEL_XFILE_OBJMODEL_H_

#include <vector>
#include <memory>
#include <string>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>

namespace saba
{
	class XFileModel
	{
	public:
		struct Material
		{
			glm::vec4	m_diffuse;
			glm::vec3	m_specular;
			float		m_speculatPower;
			glm::vec3	m_emissive;

			std::string	m_texture;

			enum class SpTextureMode
			{
				None,
				Mul,
				Add,
			};
			SpTextureMode	m_spTextureMode;
			std::string	m_spTexture;
		};

		struct Face
		{
			int	m_position[3];
			int	m_normal[3];
			int	m_uv[3];
			int	m_material;
		};

		struct Mesh
		{
			std::string				m_name;
			std::vector<glm::vec3>	m_positions;
			std::vector<glm::vec3>	m_normals;
			std::vector<glm::vec2>	m_uvs;
			std::vector<Material>	m_materials;
			std::vector<Face>		m_faces;
		};

		struct Frame
		{
			std::string	m_name;
			glm::mat4	m_local;
			glm::mat4	m_global;
			Mesh*		m_mesh;

			Frame*		m_parent;
			Frame*		m_child;
			Frame*		m_next;
		};

	public:
		bool Load(const char* filepath);
		void Destroy();

		size_t GetFrameCount() const { return m_frames.size(); }
		const Frame* GetFrame(size_t i) const { return m_frames[i].get(); }

		const glm::vec3& GetBBoxMin() const { return m_bboxMin; }
		const glm::vec3& GetBBoxMax() const { return m_bboxMax; }

	private:
		void UpdateGlobalTransform(Frame* frame);

	private:
		using MeshUPtr = std::unique_ptr<Mesh>;
		using FrameUPtr = std::unique_ptr<Frame>;

		std::vector<MeshUPtr>	m_meshes;
		std::vector<FrameUPtr>	m_frames;

		glm::vec3		m_bboxMin;
		glm::vec3		m_bboxMax;

	};
}

#endif // !SABA_MODEL_XFILE_OBJMODEL_H_
