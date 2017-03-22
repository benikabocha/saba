//
// Copyright(c) 2016-2017 benikabocha.
// Distributed under the MIT License (http://opensource.org/licenses/MIT)
//

#ifndef SABA_VIEWER_VIEWERCONTEXT_H_
#define SABA_VIEWER_VIEWERCONTEXT_H_

#include "Camera.h"

#include <string>
#include "../GL/GLSLUtil.h"

namespace saba
{
	class ViewerContext
	{
		friend class Viewer;
	public:
		ViewerContext();

		const std::string& GetWorkDir() const { return m_workDir; }
		const std::string& GetResourceDir() const { return m_resourceDir; }
		const std::string& GetShaderDir() const { return m_shaderDir; }

		const Camera* GetCamera() const { return &m_camera; }

		bool IsUIEnabled() const { return m_uiEnable; }
		bool IsCameraOverride() const { return m_cameraOverride; }
		bool IsClipElapsed() const { return m_clipElapsed; }
		double GetElapsed() const { return m_elapsed; }
		double GetAnimationTime() const { return m_animationTime; }
		bool IsMSAAEnabled() const { return m_msaaEnable; }
		int GetFrameBufferWidth() const { return m_frameBufferWidth; }
		int GetFrameBufferHeight() const { return m_frameBufferHeight; }
		int GetWindowWidth() const { return m_windowWidth; }
		int GetWindowHeight() const { return m_windowHeight; }

	private:
		void EnableUI(bool enable) { m_uiEnable = enable; }
		void EnableCameraOverride(bool enable) { m_cameraOverride = enable; }
		void SetClipElapsed(bool enable) { m_clipElapsed = enable; }
		void SetElapsedTime(double elapsed);
		void SetAnimationTime(double animTime) { m_animationTime = animTime; }
		void UpdateAnimationTime();
		void EnableMSAA(bool enable) { m_msaaEnable = enable; }
		void SetFrameBufferSize(int w, int h) { m_frameBufferWidth = w; m_frameBufferHeight = h; }
		void SetWindowSize(int w, int h) { m_windowWidth = w; m_windowHeight = h; }

		void SetCamera(const Camera& cam) { m_camera = cam; }

	private:
		std::string	m_workDir;
		std::string	m_resourceDir;
		std::string	m_shaderDir;

		Camera	m_camera;

		bool	m_uiEnable;
		bool	m_cameraOverride;

		bool	m_clipElapsed;
		double	m_elapsed;
		double	m_animationTime;

		bool	m_msaaEnable;

		int		m_frameBufferWidth;
		int		m_frameBufferHeight;
		int		m_windowWidth;
		int		m_windowHeight;
	};
}

#endif // !SABA_VIEWER_VIEWERCONTEXT_H_
