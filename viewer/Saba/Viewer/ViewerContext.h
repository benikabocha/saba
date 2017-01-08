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

	private:
		void EnableUI(bool enable) { m_uiEnable = enable; }
		void SetElapsedTime(double elapsed) { m_elapsed = elapsed; }
		void EnableMSAA(bool enable) { m_msaaEnable = enable; }

	private:
		std::string	m_workDir;
		std::string	m_resourceDir;
		std::string	m_shaderDir;

		Camera	m_camera;

		bool	m_uiEnable;

		double	m_elapsed;

		bool	m_msaaEnable;
	};
}

#endif // !SABA_VIEWER_VIEWERCONTEXT_H_
