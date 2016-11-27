#ifndef SABA_GL_MODEL_MMD_GLMMDMODELDRAWER_H_
#define SABA_GL_MODEL_MMD_GLMMDMODELDRAWER_H_

#include "GLMMDModel.h"
#include <Saba/Viewer/ModelDrawer.h>

#include <vector>

#include <glm/mat4x4.hpp>

namespace saba
{
	class GLMMDModelDrawContext;

	class GLMMDModelDrawer : public ModelDrawer
	{
	public:
		GLMMDModelDrawer(GLMMDModelDrawContext* ctxt, std::shared_ptr<GLMMDModel> mmdModel);
		~GLMMDModelDrawer() override;

		GLMMDModelDrawer(const GLMMDModelDrawer&) = delete;
		GLMMDModelDrawer& operator =(const GLMMDModelDrawer&) = delete;

		bool Create();
		void Destroy();

		virtual void Draw(ViewerContext* ctxt) override;

	private:
		struct MaterialShader
		{
			int					m_objMaterialIndex;
			int					m_objShaderIndex;
			GLVertexArrayObject	m_vao;
		};

	private:
		GLMMDModelDrawContext*		m_drawContext;
		std::shared_ptr<GLMMDModel>	m_mmdModel;

		std::vector<MaterialShader>	m_materialShaders;

		glm::mat4	m_world;
	};
}

#endif // !SABA_GL_MODEL_MMD_GLMMDMODELDRAWER_H_
