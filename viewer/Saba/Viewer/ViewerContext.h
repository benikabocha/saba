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

		bool IsUIEnabled() const { return m_enableUI; }
		double GetElapsed() const { return m_elapsed; }

	private:
		void EnableUI(bool enable) { m_enableUI = enable; }
		void SetElapsedTime(double elapsed) { m_elapsed = elapsed; }

	private:
		std::string	m_workDir;
		std::string	m_resourceDir;
		std::string	m_shaderDir;

		Camera	m_camera;

		bool	m_enableUI;

		double	m_elapsed;
	};
}

#endif // !SABA_VIEWER_VIEWERCONTEXT_H_
