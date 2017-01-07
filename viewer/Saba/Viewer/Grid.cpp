//
// Copyright(c) 2016-2017 benikabocha.
// Distributed under the MIT License (http://opensource.org/licenses/MIT)
//

#include "Grid.h"

#include <GL/gl3w.h>

#include <Saba/GL/GLSLUtil.h>
#include <Saba/GL/GLShaderUtil.h>

namespace saba
{
	Grid::Grid()
	{
	}

	bool Grid::Initialize(const ViewerContext & ctxt, float gridSize, int gridCount, int bold)
	{
		std::vector<glm::vec3> posVec;
		std::vector<glm::vec3> colorVec;
		float gridLen = gridSize * gridCount;
		glm::vec3 lineColor(0.5f);
		glm::vec3 boldColor(0.8f);
		posVec.push_back(glm::vec3(0, 0, gridLen));
		posVec.push_back(glm::vec3(0, 0, -gridLen));
		posVec.push_back(glm::vec3(gridLen, 0, 0));
		posVec.push_back(glm::vec3(-gridLen, 0, 0));
		colorVec.push_back(boldColor);
		colorVec.push_back(boldColor);
		colorVec.push_back(boldColor);
		colorVec.push_back(boldColor);
		for (int grid = 1; grid < gridCount + 1; grid++)
		{
			posVec.push_back(glm::vec3(grid * gridSize, 0, gridLen));
			posVec.push_back(glm::vec3(grid * gridSize, 0, -gridLen));
			posVec.push_back(glm::vec3(-grid * gridSize, 0, gridLen));
			posVec.push_back(glm::vec3(-grid * gridSize, 0, -gridLen));
			posVec.push_back(glm::vec3(gridLen, 0, grid * gridSize));
			posVec.push_back(glm::vec3(-gridLen, 0, grid * gridSize));
			posVec.push_back(glm::vec3(gridLen, 0, -grid * gridSize));
			posVec.push_back(glm::vec3(-gridLen, 0, -grid * gridSize));

			glm::vec3 gridColor = lineColor;
			if ((grid % bold) == 0 || grid == gridCount)
			{
				gridColor = boldColor;
			}
			colorVec.push_back(gridColor);
			colorVec.push_back(gridColor);
			colorVec.push_back(gridColor);
			colorVec.push_back(gridColor);
			colorVec.push_back(gridColor);
			colorVec.push_back(gridColor);
			colorVec.push_back(gridColor);
			colorVec.push_back(gridColor);
		}
		m_vertexCount = (GLsizei)posVec.size();

		m_posVBO = CreateVBO(posVec);
		m_colorVBO = CreateVBO(colorVec);
		m_posBinder = MakeVertexBinder<glm::vec3>();
		m_colorBinder = MakeVertexBinder<glm::vec3>();


		if (m_prog == 0)
		{
			GLSLShaderUtil glslShaderUtil;
			glslShaderUtil.SetShaderDir(ctxt.GetShaderDir());
			m_prog = glslShaderUtil.CreateProgram("grid");
			if (m_prog == 0)
			{
				return false;
			}
		}
		m_inPos = glGetAttribLocation(m_prog, "in_Pos");
		m_inColor = glGetAttribLocation(m_prog, "in_Color");
		m_uWVP = glGetUniformLocation(m_prog, "u_WVP");

		if (m_vao == 0)
		{
			if (!m_vao.Create())
			{
				return false;
			}
		}

		glBindVertexArray(m_vao);

		glBindBuffer(GL_ARRAY_BUFFER, m_posVBO);
		m_posBinder.Bind(m_inPos, m_posVBO);
		glEnableVertexAttribArray(m_inPos);

		glBindBuffer(GL_ARRAY_BUFFER, m_colorVBO);
		m_colorBinder.Bind(m_inColor, m_colorVBO);
		glEnableVertexAttribArray(m_inColor);

		glBindVertexArray(0);

		return true;
	}

	void Grid::Draw()
	{
		glUseProgram(m_prog);
		glBindVertexArray(m_vao);

		SetUniform(m_uWVP, m_WVP);
		glDrawArrays(GL_LINES, 0, m_vertexCount);

		glBindVertexArray(0);
		glUseProgram(0);
	}
}
