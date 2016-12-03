#ifndef SABA_GL_MODEL_MMD_GLMMDMODEL_H_
#define SABA_GL_MODEL_MMD_GLMMDMODEL_H_

#include <Saba/GL/GLObject.h>
#include <Saba/GL/GLVertexUtil.h>
#include <Saba/Model/MMD/MMDModel.h>
#include <Saba/Model/MMD/MMDMaterial.h>

#include <Saba/Model/MMD/VMDAnimation.h>

#include <memory>

namespace saba
{
	struct GLMMDMaterial
	{
		using SphereTextureMode = MMDMaterial::SphereTextureMode;
		glm::vec3		m_diffuse;
		float			m_alpha;
		float			m_specularPower;
		glm::vec3		m_specular;
		glm::vec3		m_ambient;
		uint8_t			m_edgeFlag;
		GLTextureRef	m_texture;
		GLTextureRef	m_spTexture;
		SphereTextureMode	m_spTextureMode;
		GLTextureRef	m_toonTexture;
		bool			m_bothFace;
	};

	class GLMMDModel
	{
	public:
		GLMMDModel();
		~GLMMDModel();

		bool Create(std::shared_ptr<MMDModel> mmdModel);
		void Destroy();

		bool LoadAnimation(const VMDFile& vmd);

		void SetAnimationTime(double time);
		double GetAnimationTime() const;
		void Update(double elapsed);

		const GLBufferObject& GetPositionVBO() const { return m_posVBO; }
		const GLBufferObject& GetNormalVBO() const { return m_norVBO; }
		const GLBufferObject& GetUVVBO() const { return m_uvVBO; }

		const VertexBinder& GetPositionBinder() const { return m_posBinder; }
		const VertexBinder& GetNormalBinder() const { return m_norBinder; }
		const VertexBinder& GetUVBinder() const { return m_uvBinder; }

		size_t GetIndexTypeSize() const { return m_indexTypeSize; }
		GLenum GetIndexType() const { return m_indexType; }
		const GLBufferObject& GetIBO() const { return m_ibo; }

		MMDModel* GetMMDModel() const { return m_mmdModel.get(); }
		const std::vector<GLMMDMaterial>& GetMaterials() const { return m_materials; }
		const std::vector<MMDSubMesh>& GetSubMeshes() const { return m_subMeshes; }

		double GetUpdateTime() const { return m_updateTime; }
	private:
		std::shared_ptr<MMDModel>		m_mmdModel;

		std::unique_ptr<VMDAnimation>	m_vmdAnim;
		double							m_animTime;

		GLBufferObject	m_posVBO;
		GLBufferObject	m_norVBO;
		GLBufferObject	m_uvVBO;

		VertexBinder	m_posBinder;
		VertexBinder	m_norBinder;
		VertexBinder	m_uvBinder;

		GLenum			m_indexType;
		size_t			m_indexTypeSize;
		GLBufferObject	m_ibo;

		std::vector<GLMMDMaterial>	m_materials;
		std::vector<MMDSubMesh>		m_subMeshes;

		double						m_updateTime;
	};
}

#endif // !SABA_GL_MODEL_MMD_GLMMDMODEL_H_
