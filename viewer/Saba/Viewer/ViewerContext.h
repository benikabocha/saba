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

		Camera* GetCamera() { return &m_camera; }

		bool IsUIEnabled() const { return m_uiEnable; }
		double GetElapsed() const { return m_elapsed; }
		bool IsMSAAEnabled() const { return m_msaaEnable; }
		int GetFrameBufferWidth() const { return m_frameBufferWidth; }
		int GetFrameBufferHeight() const { return m_frameBufferHeight; }
		int GetWindowWidth() const { return m_windowWidth; }
		int GetWindowHeight() const { return m_windowHeight; }

	private:
		void EnableUI(bool enable) { m_uiEnable = enable; }
		void SetElapsedTime(double elapsed) { m_elapsed = elapsed; }
		void EnableMSAA(bool enable) { m_msaaEnable = enable; }
		void SetFrameBufferSize(int w, int h) { m_frameBufferWidth = w; m_frameBufferHeight = h; }
		void SetWindowSize(int w, int h) { m_windowWidth = w; m_windowHeight = h; }

	private:
		std::string	m_workDir;
		std::string	m_resourceDir;
		std::string	m_shaderDir;

		Camera	m_camera;

		bool	m_uiEnable;

		double	m_elapsed;

		bool	m_msaaEnable;

		int		m_frameBufferWidth;
		int		m_frameBufferHeight;
		int		m_windowWidth;
		int		m_windowHeight;
	};
}

#endif // !SABA_VIEWER_VIEWERCONTEXT_H_
