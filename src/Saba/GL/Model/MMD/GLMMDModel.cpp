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
		m_updateTime = 0;
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
			dest.m_edgeFlag = src.m_edgeFlag;
			if (!src.m_texture.empty())
			{
				dest.m_texture = CreateMMDTexture(texMan, src.m_texture, true, true);
			}
			if (!src.m_spTexture.empty())
			{
				dest.m_spTexture = CreateMMDTexture(texMan, src.m_spTexture);
			}
			dest.m_spTextureMode = src.m_spTextureMode;
			if (!src.m_toonTexture.empty())
			{
				dest.m_toonTexture = CreateMMDTexture(texMan, src.m_toonTexture);
			}
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

		m_vmdAnim = std::make_unique<VMDAnimation>();
		if (!m_vmdAnim->Create(vmd, m_mmdModel))
		{
			m_vmdAnim.reset();
			return false;
		}

		m_animTime = 0.0f;

		m_vmdAnim->Evaluate((float)m_animTime);
		m_mmdModel->InitializeAnimation();

		return true;
	}

	void GLMMDModel::SetAnimationTime(double time)
	{
		m_animTime = time;
	}

	double GLMMDModel::GetAnimationTime() const
	{
		return m_animTime;
	}

	void GLMMDModel::Update(double elapsed)
	{
		m_updateTime = 0;
		if (m_mmdModel == nullptr)
		{
			return;
		}
		if (m_vmdAnim == nullptr)
		{
			return;
		}

		double startTime = GetTime();
		if (m_vmdAnim != 0)
		{
			m_animTime += elapsed;
			double frame = m_animTime * 30.0;
			m_vmdAnim->Evaluate((float)frame);
		}

		m_mmdModel->UpdateAnimation((float)elapsed);
		//m_mmdModel->UpdateAnimation(0);
		m_mmdModel->Update((float)elapsed);

		size_t vtxCount = m_mmdModel->GetVertexCount();
		UpdateVBO(m_posVBO, m_mmdModel->GetUpdatePositions(), vtxCount);
		UpdateVBO(m_norVBO, m_mmdModel->GetUpdateNormals(), vtxCount);
		double endTime = GetTime();
		m_updateTime = endTime - startTime;
	}

}
