//
// Copyright(c) 2016-2017 benikabocha.
// Distributed under the MIT License (http://opensource.org/licenses/MIT)
//

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
		bool			m_edgeFlag;
		float			m_edgeSize;
		glm::vec4		m_edgeColor;
		GLTextureRef	m_texture;
		bool			m_textureHaveAlpha;
		GLTextureRef	m_spTexture;
		SphereTextureMode	m_spTextureMode;
		GLTextureRef	m_toonTexture;
		glm::vec4		m_textureMulFactor;
		glm::vec4		m_spTextureMulFactor;
		glm::vec4		m_toonTextureMulFactor;
		glm::vec4		m_textureAddFactor;
		glm::vec4		m_spTextureAddFactor;
		glm::vec4		m_toonTextureAddFactor;
		bool			m_bothFace;
		bool			m_groundShadow;
		bool			m_shadowCaster;
		bool			m_shadowReceiver;
	};

	class GLMMDModel
	{
	public:
		GLMMDModel();
		~GLMMDModel();

		bool Create(std::shared_ptr<MMDModel> mmdModel);
		void Destroy();

		bool LoadAnimation(const VMDFile& vmd);
		void LoadPose(const VPDFile& vpd, int frameCount = 30);

		/*
		現在のアニメーションの初期化と Physics の初期化を行う。
		時間は変更されない。
		*/
		void ResetAnimation();

		// アニメーションをクリアする
		void ClearAnimation();

		void SetAnimationTime(double time);
		double GetAnimationTime() const;
		void EvaluateAnimation(double animTime);
		void UpdateAnimation(double animTime, double elapsed);
		void UpdateAnimationIgnoreVMD(double elapsed);
		void UpdateMorph();
		void Update();

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

		struct PerfInfo
		{
			// Update animation
			double	m_setupAnimTime;
			double	m_updateMorphAnimTime;
			double	m_updateNodeAnimTime;
			double	m_updatePhysicsAnimTime;

			// Update vertices
			double	m_updateModelTime;
			double	m_updateGLBufferTime;

			void Clear();
			double GetUpdateTime() const;
		};
		const PerfInfo& GetPerfInfo() { return m_perfInfo; }
		void ClearPerfInfo() { m_perfInfo.Clear(); }
		double GetUpdateTime() const { return m_perfInfo.GetUpdateTime(); }

		VMDAnimation* GetVMDAnimation() const { return m_vmdAnim.get(); }

		void EnablePhysics(bool enable) { m_enablePhysics = enable; }
		bool IsEnabledPhysics() const { return m_enablePhysics; }

		void EnableEdge(bool enable) { m_enableEdge = enable; }
		bool IsEnabledEdge() const { return m_enableEdge; }

		void EnableGroundShadow(bool enable) { m_enableGroundShadow = enable; }
		bool IsEnableGroundShadow() const { return m_enableGroundShadow; }

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

		PerfInfo					m_perfInfo;

		bool	m_enablePhysics;
		bool	m_enableEdge;
		bool	m_enableGroundShadow;
	};
}

#endif // !SABA_GL_MODEL_MMD_GLMMDMODEL_H_
