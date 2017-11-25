//
// Copyright(c) 2016-2017 benikabocha.
// Distributed under the MIT License (http://opensource.org/licenses/MIT)
//

#include "GLMMDModel.h"
#include <Saba/GL/GLVertexUtil.h>
#include <Saba/GL/GLTextureUtil.h>
#include <Saba/Base/Log.h>
#include <Saba/Base/Time.h>

#include <string>
#include <map>
#include <memory>

namespace saba
{
	GLMMDModel::GLMMDModel()
	{
		m_animTime = 0;
		m_updateAnimTime = 0;
		m_updatePhysicsTime = 0;
		m_updateTime = 0;
		m_perfInfo.m_updateAnimTime = 0;
		m_perfInfo.m_updatePhysicsTime = 0;
		m_perfInfo.m_updateModelTime = 0;
		m_perfInfo.m_updateGLBufferTime = 0;
		m_enablePhysics = true;
		m_enableEdge = true;
		m_enableGroundShadow = true;
	}

	GLMMDModel::~GLMMDModel()
	{
	}

	namespace
	{
		using TextureManager = std::map<std::string, GLTextureRef>;
		GLTextureRef CreateMMDTexture(
			TextureManager& texMan,
			const std::string& filename,
			bool genMipmap = true,
			bool rgba = false
		)
		{
			std::string key = filename + ":" +
				std::to_string(genMipmap) + ":" +
				std::to_string(rgba);
			auto findIt = texMan.find(key);
			if (findIt != texMan.end())
			{
				return (*findIt).second;
			}
			else
			{
				auto tex = CreateTextureFromFile(filename.c_str());
				GLTextureRef texRef = std::move(tex);
				texMan.emplace(std::make_pair(key, texRef));
				return texRef;
			}
		}
	}

	bool GLMMDModel::Create(std::shared_ptr<MMDModel> mmdModel)
	{
		Destroy();

		size_t vtxCount = mmdModel->GetVertexCount();
		auto positions = mmdModel->GetPositions();
		auto normals = mmdModel->GetNormals();
		auto uvs = mmdModel->GetUVs();
		m_posVBO = CreateVBO(positions, vtxCount, GL_DYNAMIC_DRAW);
		m_norVBO = CreateVBO(normals, vtxCount, GL_DYNAMIC_DRAW);
		m_uvVBO = CreateVBO(uvs, vtxCount, GL_DYNAMIC_DRAW);

		m_posBinder = MakeVertexBinder<glm::vec3>();
		m_norBinder = MakeVertexBinder<glm::vec3>();
		m_uvBinder = MakeVertexBinder<glm::vec2>();

		const void* iboBuf = mmdModel->GetIndices();
		size_t indexCount = mmdModel->GetIndexCount();
		size_t indexElemSize = mmdModel->GetIndexElementSize();
		switch (indexElemSize)
		{
		case 1:
			m_ibo = CreateIBO((uint8_t*)iboBuf, indexCount, GL_STATIC_DRAW);
			m_indexType = GL_UNSIGNED_BYTE;
			m_indexTypeSize = 1;
			break;
		case 2:
			m_ibo = CreateIBO((uint16_t*)iboBuf, indexCount, GL_STATIC_DRAW);
			m_indexType = GL_UNSIGNED_SHORT;
			m_indexTypeSize = 2;
			break;
		case 4:
			m_ibo = CreateIBO((uint32_t*)iboBuf, indexCount, GL_STATIC_DRAW);
			m_indexType = GL_UNSIGNED_INT;
			m_indexTypeSize = 4;
			break;
		default:
			SABA_ERROR("Unknown Index Size. [{}]", indexElemSize);
			return false;
		}

		// Material
		size_t matCount = mmdModel->GetMaterialCount();
		auto materials = mmdModel->GetMaterials();
		m_materials.resize(matCount);
		TextureManager texMan;
		for (size_t matIdx = 0; matIdx < matCount; matIdx++)
		{
			auto& dest = m_materials[matIdx];
			const auto& src = materials[matIdx];

			dest.m_diffuse = src.m_diffuse;
			dest.m_alpha = src.m_alpha;
			dest.m_specularPower = src.m_specularPower;
			dest.m_specular = src.m_specular;
			dest.m_ambient = src.m_ambient;
			dest.m_edgeFlag = src.m_edgeFlag != 0;
			dest.m_edgeSize = src.m_edgeSize;
			dest.m_edgeColor = src.m_edgeColor;
			if (!src.m_texture.empty())
			{
				dest.m_texture = CreateMMDTexture(texMan, src.m_texture, true, true);
				dest.m_textureHaveAlpha = IsAlphaTexture(dest.m_texture);
			}
			dest.m_textureMulFactor = src.m_textureMulFactor;
			dest.m_textureAddFactor = src.m_textureAddFactor;

			if (!src.m_spTexture.empty())
			{
				dest.m_spTexture = CreateMMDTexture(texMan, src.m_spTexture);
			}
			dest.m_spTextureMode = src.m_spTextureMode;
			dest.m_spTextureMulFactor = src.m_spTextureMulFactor;
			dest.m_spTextureAddFactor = src.m_spTextureAddFactor;

			if (!src.m_toonTexture.empty())
			{
				dest.m_toonTexture = CreateMMDTexture(texMan, src.m_toonTexture);
			}
			dest.m_toonTextureMulFactor = src.m_toonTextureMulFactor;
			dest.m_toonTextureAddFactor = src.m_toonTextureAddFactor;

			dest.m_bothFace = src.m_bothFace;
			dest.m_groundShadow = src.m_groundShadow;
			dest.m_shadowCaster = src.m_shadowCaster;
			dest.m_shadowReceiver = src.m_shadowReceiver;
		}

		// SubMesh
		size_t subMeshCount = mmdModel->GetSubMeshCount();
		auto subMeshes = mmdModel->GetSubMeshes();
		m_subMeshes.resize(subMeshCount);
		for (size_t subMeshIdx = 0; subMeshIdx < subMeshCount; subMeshIdx++)
		{
			auto& dest = m_subMeshes[subMeshIdx];
			const auto& src = subMeshes[subMeshIdx];

			dest.m_beginIndex = src.m_beginIndex;
			dest.m_vertexCount = src.m_vertexCount;
			dest.m_materialID = src.m_materialID;
		}

		m_mmdModel = mmdModel;

		return true;
	}

	void GLMMDModel::Destroy()
	{
		m_mmdModel.reset();

		m_posVBO.Destroy();
		m_norVBO.Destroy();
		m_uvVBO.Destroy();
		m_ibo.Destroy();
	}

	bool GLMMDModel::LoadAnimation(const VMDFile& vmd)
	{
		if (m_mmdModel == nullptr)
		{
			SABA_WARN("Loda Animation Fail. model is null");
			return false;
		}

		if (m_vmdAnim == nullptr)
		{
			m_vmdAnim = std::make_unique<VMDAnimation>();
			if (!m_vmdAnim->Create(m_mmdModel))
			{
				m_vmdAnim.reset();
				return false;
			}
		}

		if (!m_vmdAnim->Add(vmd))
		{
			m_vmdAnim.reset();
			return false;
		}

		// Physicsを同期する
		m_vmdAnim->SyncPhysics(float(m_animTime * 30.0), 30);

		return true;
	}

	void GLMMDModel::LoadPose(const VPDFile & vpd, int frameCount)
	{
		if (m_mmdModel != nullptr)
		{
			m_mmdModel->LoadPose(vpd, frameCount);
		}
	}

	void GLMMDModel::ResetAnimation()
	{
		m_mmdModel->InitializeAnimation();
		if (m_vmdAnim != nullptr)
		{
			m_vmdAnim->SyncPhysics(float(m_animTime * 30.0));
		}
	}

	void GLMMDModel::ClearAnimation()
	{
		m_vmdAnim.reset();
		m_animTime = 0;
		m_mmdModel->InitializeAnimation();
	}

	void GLMMDModel::SetAnimationTime(double time)
	{
		m_animTime = time;
	}

	double GLMMDModel::GetAnimationTime() const
	{
		return m_animTime;
	}

	void GLMMDModel::EvaluateAnimation(double animTime)
	{
		if (m_vmdAnim != 0)
		{
			m_animTime = animTime;
			double frame = m_animTime * 30.0;
			m_vmdAnim->Evaluate((float)frame);
		}
	}

	void GLMMDModel::UpdateAnimation(double animTime, bool evaluateAnim)
	{
		double startTime = GetTime();

		if (!evaluateAnim)
		{
			m_mmdModel->SaveBaseAnimation();
		}

		m_mmdModel->BeginAnimation();

		if (evaluateAnim)
		{
			EvaluateAnimation(animTime);
		}
		else
		{
			m_mmdModel->LoadBaseAnimation();
		}

		m_mmdModel->UpdateAnimation();

		m_mmdModel->EndAnimation();

		double endTime = GetTime();
		m_updateAnimTime = endTime - startTime;
	}

	void GLMMDModel::Update(double elapsed)
	{
		m_updateTime = 0;
		if (m_mmdModel == nullptr)
		{
			return;
		}

		if (m_enablePhysics)
		{
			double startTime = GetTime();
			m_mmdModel->UpdatePhysics((float)elapsed);
			double endTime = GetTime();
			m_updatePhysicsTime = endTime - startTime;
		}

		double startTime = GetTime();
		m_mmdModel->Update();

		size_t matCount = m_mmdModel->GetMaterialCount();
		for (size_t mi = 0; mi < matCount; mi++)
		{
			const auto& mmdMat = m_mmdModel->GetMaterials()[mi];
			m_materials[mi].m_diffuse = mmdMat.m_diffuse;
			m_materials[mi].m_alpha = mmdMat.m_alpha;
			m_materials[mi].m_specular = mmdMat.m_specular;
			m_materials[mi].m_specularPower = mmdMat.m_specularPower;
			m_materials[mi].m_ambient = mmdMat.m_ambient;
			m_materials[mi].m_textureMulFactor = mmdMat.m_textureMulFactor;
			m_materials[mi].m_textureAddFactor = mmdMat.m_textureAddFactor;
			m_materials[mi].m_spTextureMulFactor = mmdMat.m_spTextureMulFactor;
			m_materials[mi].m_spTextureAddFactor = mmdMat.m_spTextureAddFactor;
			m_materials[mi].m_toonTextureMulFactor = mmdMat.m_toonTextureMulFactor;
			m_materials[mi].m_toonTextureAddFactor = mmdMat.m_toonTextureAddFactor;
		}
		double endTime = GetTime();
		double updateModelTime = endTime - startTime;

		startTime = GetTime();
		size_t vtxCount = m_mmdModel->GetVertexCount();
		UpdateVBO(m_posVBO, m_mmdModel->GetUpdatePositions(), vtxCount);
		UpdateVBO(m_norVBO, m_mmdModel->GetUpdateNormals(), vtxCount);
		UpdateVBO(m_uvVBO, m_mmdModel->GetUpdateUVs(), vtxCount);
		endTime = GetTime();
		double updateGLBufferTime = endTime - startTime;

		m_perfInfo.m_updateAnimTime = m_updateAnimTime;
		m_perfInfo.m_updatePhysicsTime = m_updatePhysicsTime;
		m_perfInfo.m_updateModelTime = updateModelTime;
		m_perfInfo.m_updateGLBufferTime = updateGLBufferTime;

		m_updateTime = updateGLBufferTime + updateModelTime + m_updateAnimTime + m_updatePhysicsTime;
		m_updateAnimTime = 0;
		m_updatePhysicsTime = 0;
	}

}
