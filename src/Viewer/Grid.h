#ifndef VIEWER_GRID_H_
#define VIEWER_GRID_H_

#include "ViewerContext.h"

#include <Saba/GL/GLObject.h>
#include <Saba/GL/GLVertexUtil.h>

namespace saba
{
	class Grid
	{
	public:
		Grid();

		bool Initialize(
			const ViewerContext& ctxt,
			float gridSize,
			int gridCount,
			int bold
		);

		void SetWVPMatrix(const glm::mat4 wvp) { m_WVP = wvp; }

		void Draw();

	private:
		GLProgramObject	m_prog;

		GLVertexArrayObject	m_vao;

		GLint	m_inPos;
		GLint	m_inColor;
		GLBufferObject		m_posVBO;
		GLBufferObject		m_colorVBO;
		GLsizei						m_vertexCount;

		VertexBinder		m_posBinder;
		VertexBinder		m_colorBinder;

		GLint		m_uWVP;
		glm::mat4	m_WVP;

	};
}

#endif // !VIEWER_GRID_H_
