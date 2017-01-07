//
// Copyright(c) 2016-2017 benikabocha.
// Distributed under the MIT License (http://opensource.org/licenses/MIT)
//

#ifndef SABA_GL_MODEL_OBJ_OBJMODEL_H_
#define SABA_GL_MODEL_OBJ_OBJMODEL_H_

#include "../../GLObject.h"
#include "../../GLVertexUtil.h"
#include <Saba/Model/OBJ/OBJModel.h>

namespace saba
{
	class GLOBJModel
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

			GLTextureRef	m_ambientTex;
			GLTextureRef	m_diffuseTex;
			GLTextureRef	m_specularTex;
			GLTextureRef	m_transparencyTex;
		};

		struct SubMesh
		{
			int	m_beginIndex;
			int	m_vertexCount;
			int	m_materialID;
		};

	public:
		GLOBJModel();
		~GLOBJModel();

		bool Create(const OBJModel& objModel);
		void Destroy();

		const std::vector<Material>& GetMaterials() const { return m_materials; }
		const std::vector<SubMesh>& GetSubMeshes() const { return m_subMeshes; }

		const GLBufferObject& GetPositionVBO() const { return m_posVBO; }
		const GLBufferObject& GetNormalVBO() const { return m_norVBO; }
		const GLBufferObject& GetUVVBO() const { return m_uvVBO; }

		const VertexBinder& GetPositionBinder() const { return m_posBinder; }
		const VertexBinder& GetNormalBinder() const { return m_norBinder; }
		const VertexBinder& GetUVBinder() const { return m_uvBinder; }

		const glm::vec3& GetBBoxMin() const { return m_bboxMin; }
		const glm::vec3& GetBBoxMax() const { return m_bboxMax; }

	private:
		GLBufferObject	m_posVBO;
		GLBufferObject	m_norVBO;
		GLBufferObject	m_uvVBO;

		VertexBinder	m_posBinder;
		VertexBinder	m_norBinder;
		VertexBinder	m_uvBinder;

		glm::vec3		m_bboxMin;
		glm::vec3		m_bboxMax;

		std::vector<Material>	m_materials;
		std::vector<SubMesh>	m_subMeshes;

	};
}

#endif // !SABA_GL_MODEL_OBJ_OBJMODEL_H_
