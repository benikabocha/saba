//
// Copyright(c) 2016-2017 benikabocha.
// Distributed under the MIT License (http://opensource.org/licenses/MIT)
//

#ifndef SABA_VIEWER_VIEWERCONTEXT_H_
#define SABA_VIEWER_VIEWERCONTEXT_H_

#include "Camera.h"
#include "Light.h"
#include "ShadowMap.h"

#include <string>
#include "../GL/GLSLUtil.h"
#include "../GL/GLObject.h"

#include <glm/vec3.hpp>

namespace saba
{
	class ViewerContext
	{
		friend class Viewer;
	public:
		enum class PlayMode
		{
			None,
			PlayStart,
			Play,
			Stop,
			Update,
			NextFrame,
			PrevFrame,
		};

		ViewerContext();

		bool Initialize();
		void Uninitialize();

		const std::string& GetWorkDir() const { return m_workDir; }
		const std::string& GetResourceDir() const { return m_resourceDir; }
		const std::string& GetShaderDir() const { return m_shaderDir; }

		const Camera* GetCamera() const { return &m_camera; }
		const Light* GetLight() const { return &m_light; }
		const ShadowMap* GetShadowMap() const { return &m_shadowmap; }

		bool IsUIEnabled() const { return m_uiEnable; }
		bool IsCameraOverride() const { return m_cameraOverride; }
		bool IsClipElapsed() const { return m_clipElapsed; }
		bool IsShadowEnabled() const { return m_shadowEnabled; }
		double GetElapsed() const { return m_elapsed; }
		double GetAnimationTime() const { return m_animationTime; }
		bool IsMSAAEnabled() const { return m_msaaEnable; }
		int GetMSAACount() const { return m_msaaCount; }
		int GetFrameBufferWidth() const { return m_frameBufferWidth; }
		int GetFrameBufferHeight() const { return m_frameBufferHeight; }
		int GetWindowWidth() const { return m_windowWidth; }
		int GetWindowHeight() const { return m_windowHeight; }
		PlayMode GetPlayMode() const { return m_playMode; }

		GLTextureRef GetDummyColorTexture() const { return m_dummyColorTexture; }
		GLTextureRef GetDummyShadowDepthTexture() const { return m_dummyShadowDepthTexture; }
		GLTextureRef GetCaptureTexture() const { return m_captureTex; }

		const glm::vec4& GetMMDGroundShadowColor() const { return m_mmdGroundShadowColor; };

	private:
		void EnableUI(bool enable) { m_uiEnable = enable; }
		void EnableCameraOverride(bool enable) { m_cameraOverride = enable; }
		void EnableShadow(bool enable) { m_shadowEnabled = enable; }
		void SetClipElapsed(bool enable) { m_clipElapsed = enable; }
		void SetElapsedTime(double elapsed);
		void SetAnimationTime(double animTime) { m_animationTime = animTime; }
		void EnableMSAA(bool enable) { m_msaaEnable = enable; }
		void SetMSAACount(int count) { m_msaaCount = count; }
		void SetFrameBufferSize(int w, int h) { m_frameBufferWidth = w; m_frameBufferHeight = h; }
		void SetWindowSize(int w, int h) { m_windowWidth = w; m_windowHeight = h; }
		void SetPlayMode(PlayMode playMode) { m_playMode = playMode; }

		void SetCamera(const Camera& cam) { m_camera = cam; }
		void SetLight(const Light& light) { m_light = light; }

		void SetMMDGroundShadowColor(const glm::vec4& shadowColor) { m_mmdGroundShadowColor = shadowColor; }

		bool ResizeCaptureTexture();

	private:
		std::string	m_workDir;
		std::string	m_resourceDir;
		std::string	m_shaderDir;

		GLTextureRef	m_dummyColorTexture;
		GLTextureRef	m_dummyShadowDepthTexture;
		GLTextureRef	m_captureTex;

		Camera	m_camera;
		Light	m_light;
		ShadowMap	m_shadowmap;

		bool	m_uiEnable;
		bool	m_cameraOverride;

		bool	m_clipElapsed;
		double	m_elapsed;
		double	m_animationTime;

		bool	m_msaaEnable;
		int		m_msaaCount;

		int		m_frameBufferWidth;
		int		m_frameBufferHeight;
		int		m_windowWidth;
		int		m_windowHeight;

		PlayMode	m_playMode;

		bool	m_shadowEnabled;

		glm::vec4	m_mmdGroundShadowColor;
	};
}

#endif // !SABA_VIEWER_VIEWERCONTEXT_H_
