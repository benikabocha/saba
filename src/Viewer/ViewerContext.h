#ifndef VIEWER_VIEWERCONTEXT_H_
#define VIEWER_VIEWERCONTEXT_H_

#include "Camera.h"

#include <string>
#include <Saba/GL/GLSLUtil.h>

namespace saba
{
	class ViewerContext
	{
	public:
		ViewerContext();

		const std::string& GetWorkDir() const { return m_workDir; }
		const std::string& GetResourceDir() const { return m_resourceDir; }
		const std::string& GetShaderDir() const { return m_shaderDir; }

		Camera* GetCamera() { return &m_camera; }

	private:
		std::string	m_workDir;
		std::string	m_resourceDir;
		std::string	m_shaderDir;

		Camera	m_camera;
	};
}

#endif // !VIEWER_VIEWERCONTEXT_H_
