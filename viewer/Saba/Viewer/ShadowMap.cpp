//
// Copyright(c) 2016-2017 benikabocha.
// Distributed under the MIT License (http://opensource.org/licenses/MIT)
//

#include "ShadowMap.h"
#include "Camera.h"
#include "Light.h"
#include "ViewerContext.h"
#include "Saba/Base/Log.h"

#include <limits>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>

#if _WIN32
#undef min
#undef max
#endif

namespace saba
{
	ShadowMap::ShadowMap()
		: m_splitCount(4)
		, m_width(1024)
		, m_height(1024)
		, m_nearClip(0.01f)
		, m_farClip(1000.0f)
		, m_bias(0.01f)
	{
	}

	bool ShadowMap::InitializeShader(ViewerContext * ctxt)
	{
		GLSLShaderUtil glslShaderUtil;
		glslShaderUtil.SetShaderDir(ctxt->GetShaderDir());
		m_shader.m_prog = glslShaderUtil.CreateProgram("shadow_shader");
		if (m_shader.m_prog == 0)
		{
			return false;
		}
		m_shader.Initialize();
		return true;
	}

	bool ShadowMap::Setup(int width, int height, size_t splitCount)
	{
		m_splitPositions.resize(splitCount + 1);
		m_clipSpaces.clear();
		m_clipSpaces.resize(splitCount);

		for (auto& clipSpace : m_clipSpaces)
		{
			clipSpace.m_shadomap.Create();
			glBindTexture(GL_TEXTURE_2D, clipSpace.m_shadomap);
			glTexImage2D(
				GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT16,
				width, height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr
			);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LESS);
			glm::vec4 once(1.0f);
			glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, &once[0]);
			glBindTexture(GL_TEXTURE_2D, 0);

			clipSpace.m_shadowmapFBO.Create();
			glBindFramebuffer(GL_FRAMEBUFFER, clipSpace.m_shadowmapFBO);
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, clipSpace.m_shadomap, 0);
			auto status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
			if (GL_FRAMEBUFFER_COMPLETE != status)
			{
				SABA_WARN("Shadowmap Framebuffer status : {}", status);
				glBindFramebuffer(GL_FRAMEBUFFER, 0);
				return false;
			}
			glBindFramebuffer(GL_FRAMEBUFFER, 0);
		}

		m_width = width;
		m_height = height;
		m_splitCount = splitCount;

		return true;
	}

	void ShadowMap::CalcShadowMap(const Camera* camera, const Light* light)
	{
		glm::mat4 view = camera->GetViewMatrix();
		glm::mat4 invView = glm::inverse(view);

		{
			glm::vec3 n(1, 0, 0);
			glm::vec3 m(0, 1, 0);
			glm::vec3 w = light->GetLightDirection();
			glm::vec3 u = glm::cross(w, n);
			if (glm::length(u) < 1.0e-5)
			{
				u = glm::cross(w, m);
			}
			u = glm::normalize(u);
			glm::vec3 v = glm::cross(u, w);
			glm::mat4 lightView = glm::lookAt(glm::vec3(0.0f), w, v);
			m_view = lightView;
		}

		glm::mat4 toLightView = m_view * invView;

		float aspect = camera->GetWidth() / camera->GetHeight();
		float tanHalfFovY = glm::tan(camera->GetFovY() * 0.5f);
		float tanHalfFovX = tanHalfFovY * aspect;

		float shadowMapAspect = float(m_width) / float(m_height);

		CalcSplitPositions(m_nearClip, m_farClip, 0.5f);
		for (size_t i = 0; i < m_splitCount; i++)
		{
			float nearClip = m_splitPositions[i];
			float farClip = m_splitPositions[i + 1];

			float xn = nearClip * tanHalfFovX;
			float xf = farClip * tanHalfFovX;
			float yn = nearClip * tanHalfFovY;
			float yf = farClip * tanHalfFovY;

			glm::vec3 frustumCorners[] = {
				glm::vec3(xn, yn, -nearClip),
				glm::vec3(-xn, yn, -nearClip),
				glm::vec3(xn, -yn, -nearClip),
				glm::vec3(-xn, -yn, -nearClip),

				glm::vec3(xf, yf, -farClip),
				glm::vec3(-xf, yf, -farClip),
				glm::vec3(xf, -yf, -farClip),
				glm::vec3(-xf, -yf, -farClip),
			};
			glm::vec3 bboxMin = glm::vec3(std::numeric_limits<float>::max());
			glm::vec3 bboxMax = glm::vec3(-std::numeric_limits<float>::max());

			for (const auto& corner : frustumCorners)
			{
				auto p = glm::vec3(toLightView * glm::vec4(corner, 1.0f));
				bboxMin = glm::min(bboxMin, p);
				bboxMax = glm::max(bboxMax, p);
			}
			glm::mat4 ortho;
			ortho[0][0] = 2.0f / (bboxMax.x - bboxMin.x);
			ortho[1][1] = 2.0f / (bboxMax.y - bboxMin.y);
			ortho[2][2] = 2.0f / (bboxMax.z - bboxMin.z);
			ortho[3][0] = -(bboxMax.x + bboxMin.x) / (bboxMax.x - bboxMin.x);
			ortho[3][1] = -(bboxMax.y + bboxMin.y) / (bboxMax.y - bboxMin.y);
			ortho[3][2] = -(bboxMax.z + bboxMin.z) / (bboxMax.z - bboxMin.z);
			m_clipSpaces[i].m_projection = ortho;
			m_clipSpaces[i].m_nearClip = nearClip;
			m_clipSpaces[i].m_farClip = farClip;
		}
	}

	size_t ShadowMap::GetClipSpaceCount() const
	{
		return m_clipSpaces.size();
	}

	const ShadowMap::ClipSpace& ShadowMap::GetClipSpace(size_t i) const
	{
		return m_clipSpaces[i];
	}

	const ShadowMapShader * ShadowMap::GetShader() const
	{
		return &m_shader;
	}

	int ShadowMap::GetWidth() const
	{
		return m_width;
	}

	int ShadowMap::GetHeight() const
	{
		return m_height;
	}

	size_t ShadowMap::GetShadowMapCount() const
	{
		return m_splitCount;
	}

	void ShadowMap::SetClip(float nearClip, float farClip)
	{
		m_nearClip = nearClip;
		m_farClip = farClip;
	}

	float ShadowMap::GetNearClip() const
	{
		return m_nearClip;
	}

	float ShadowMap::GetFarClip() const
	{
		return m_farClip;
	}

	size_t ShadowMap::GetSplitPositionCount() const
	{
		return m_splitCount + 1;
	}

	const float * ShadowMap::GetSplitPositions() const
	{
		return m_splitPositions.data();
	}

	void ShadowMap::SetBias(float bias)
	{
		m_bias = bias;
	}

	float ShadowMap::GetBias() const
	{
		return m_bias;
	}

	void ShadowMap::CalcSplitPositions(float nearClip, float farClip, float lamda)
	{
		if (m_splitCount == 1)
		{
			m_splitPositions[0] = nearClip;
			m_splitPositions[1] = farClip;
			return;
		}

		for (size_t i = 1; i < m_splitCount + 1; i++)
		{
			float ciLog = nearClip * glm::pow(farClip / nearClip, 1.0f / float(m_splitCount) * i);
			float ciUni = nearClip + (farClip - nearClip) * i * 1.0f / float(m_splitCount);
			m_splitPositions[i] = lamda * ciLog + ciUni * (1.0f - lamda);
		}
		m_splitPositions[0] = nearClip;
		m_splitPositions[m_splitCount] = farClip;
	}

	void ShadowMapShader::Initialize()
	{
		// attribute
		m_inPos = glGetAttribLocation(m_prog, "in_Pos");

		// uniform
		m_uWVP = glGetUniformLocation(m_prog, "u_WVP");
	}
}
