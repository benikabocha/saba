//
// Copyright(c) 2016-2017 benikabocha.
// Distributed under the MIT License (http://opensource.org/licenses/MIT)
//

#ifndef SABA_VIEWER_GRID_H_
#define SABA_VIEWER_GRID_H_

#include "ViewerContext.h"

#include "../GL/GLObject.h"
#include "../GL/GLVertexUtil.h"

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

#endif // !SABA_VIEWER_GRID_H_
