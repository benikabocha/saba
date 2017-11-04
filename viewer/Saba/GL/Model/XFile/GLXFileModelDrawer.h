//
// Copyright(c) 2016-2017 benikabocha.
// Distributed under the MIT License (http://opensource.org/licenses/MIT)
//

#ifndef SABA_GL_MODEL_XFile_GLXFileMODELDRAWER_H_
#define SABA_GL_MODEL_XFile_GLXFileMODELDRAWER_H_

#include "../../../Viewer/ModelDrawer.h"

#include <vector>
#include <memory>

#include <glm/mat4x4.hpp>

namespace saba
{
	class GLXFileModel;
	class GLXFileModelDrawContext;

	class GLXFileModelDrawer : public ModelDrawer
	{
	public:
		GLXFileModelDrawer(GLXFileModelDrawContext* ctxt, std::shared_ptr<GLXFileModel> model);
		~GLXFileModelDrawer() override;

		GLXFileModelDrawer(const GLXFileModelDrawer&) = delete;
		GLXFileModelDrawer& operator =(const GLXFileModelDrawer&) = delete;

		bool Create();
		void Destroy();

		ModelDrawerType GetType() const override { return ModelDrawerType::XFileModelDrawer; }

		void Play() override {};
		void Stop() override {};

		void ResetAnimation(ViewerContext* ctxt) override;
		void Update(ViewerContext* ctxt) override;
		void DrawUI(ViewerContext* ctxt) override;
		void DrawShadowMap(ViewerContext* ctxt, size_t csmIdx) override;
		void Draw(ViewerContext* ctxt) override;

	private:
		struct MaterialShader
		{
			int					m_shaderIndex;
			GLVertexArrayObject	m_vao;
		};
	private:
		GLXFileModelDrawContext*		m_drawContext;
		std::shared_ptr<GLXFileModel>	m_xfileModel;

		using MaterialShaders = std::vector<MaterialShader>;
		std::vector<MaterialShaders>	m_materialShaders;
	};
}

#endif // !SABA_GL_MODEL_XFile_GLXFileMODELDRAWER_H_

