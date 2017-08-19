//
// Copyright(c) 2016-2017 benikabocha.
// Distributed under the MIT License (http://opensource.org/licenses/MIT)
//

#ifndef SABA_VIEWER_SHADOWMAP_H_
#define SABA_VIEWER_SHADOWMAP_H_

#include <Saba/GL/GLObject.h>
#include <Saba/GL/GLSLUtil.h>

#include <vector>
#include <glm/mat4x4.hpp>

namespace saba
{
	class Camera;
	class Light;
	class ViewerContext;

	struct ShadowMapShader
	{
		GLProgramObject		m_prog;

		// attribute
		GLint	m_inPos;

		// uniform
		GLint	m_uWVP;

		void Initialize();
	};

	class ShadowMap
	{
	public:
		ShadowMap();

		ShadowMap(const ShadowMap&) = delete;
		ShadowMap& operator = (const ShadowMap&) = delete;

		bool InitializeShader(ViewerContext* ctxt);
		bool Setup(int width, int height, size_t splitCount);
		void CalcShadowMap(const Camera* camera, const Light* light);

		const glm::mat4 GetShadowViewMatrix() const { return m_view; }

		struct ClipSpace
		{
			float		m_nearClip;
			float		m_farClip;
			glm::mat4	m_projection;
			GLTextureObject		m_shadomap;
			GLFramebufferObject	m_shadowmapFBO;
		};

		size_t GetClipSpaceCount() const;
		const ClipSpace& GetClipSpace(size_t i) const;

		const ShadowMapShader* GetShader() const;
		int GetWidth() const;
		int GetHeight() const;
		size_t GetShadowMapCount() const;
		void SetClip(float nearClip, float farClip);
		float GetNearClip() const;
		float GetFarClip() const;

		size_t GetSplitPositionCount() const;
		const float* GetSplitPositions() const;

		void SetBias(float bias);
		float GetBias() const;
	private:
		void CalcSplitPositions(float nearClip, float farClip, float lamda);

	private:
		glm::mat4				m_view;
		std::vector<ClipSpace>	m_clipSpaces;
		std::vector<float>		m_splitPositions;
		size_t					m_splitCount;
		ShadowMapShader			m_shader;
		int						m_width;
		int						m_height;
		float					m_nearClip;
		float					m_farClip;
		float					m_bias;
	};
}

#endif // !SABA_VIEWER_SHADOWMAP_H_
