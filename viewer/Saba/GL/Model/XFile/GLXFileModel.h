//
// Copyright(c) 2016-2017 benikabocha.
// Distributed under the MIT License (http://opensource.org/licenses/MIT)
//

#ifndef SABA_GL_MODEL_XFILE_XFILEMODEL_H_
#define SABA_GL_MODEL_XFILE_XFILEMODEL_H_

#include "../../GLObject.h"
#include "../../GLVertexUtil.h"
#include <Saba/Model/XFile/XFileModel.h>

namespace saba
{
	class ViewerContext;

	class GLXFileModel
	{
	public:
		struct Material
		{
			glm::vec4	m_diffuse;
			glm::vec3	m_specular;
			float		m_specularPower;
			glm::vec3	m_emissive;

			GLTextureRef	m_texture;

			XFileModel::Material::SpTextureMode	m_spTextureMode;
			GLTextureRef	m_spTexture;
		};

		struct SubMesh
		{
			int	m_beginIndex;
			int	m_vertexCount;
			int	m_materialID;
		};

		struct Mesh
		{
			GLBufferObject	m_posVBO;
			GLBufferObject	m_norVBO;
			GLBufferObject	m_uvVBO;

			VertexBinder	m_posBinder;
			VertexBinder	m_norBinder;
			VertexBinder	m_uvBinder;

			std::vector<Material>	m_materials;
			std::vector<SubMesh>	m_subMeshes;

			glm::mat4	m_transform;
		};

		size_t GetMeshCount() const { return m_meshes.size(); }
		const Mesh* GetMesh(size_t i) const { return m_meshes[i].get(); }

	public:
		GLXFileModel();
		~GLXFileModel();

		bool Create(ViewerContext* ctxt, const XFileModel& xfileModel);
		void Destroy();

		const glm::vec3& GetBBoxMin() const { return m_bboxMin; }
		const glm::vec3& GetBBoxMax() const { return m_bboxMax; }

	private:
		using MeshUPtr = std::unique_ptr<Mesh>;
		std::vector<MeshUPtr>	m_meshes;

		glm::vec3		m_bboxMin;
		glm::vec3		m_bboxMax;

	};
}

#endif // !SABA_GL_MODEL_XFILE_XFILEMODEL_H_
