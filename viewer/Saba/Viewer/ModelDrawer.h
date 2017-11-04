//
// Copyright(c) 2016-2017 benikabocha.
// Distributed under the MIT License (http://opensource.org/licenses/MIT)
//

#ifndef SABA_VIEWER_MODELDRAWER_H_
#define SABA_VIEWER_MODELDRAWER_H_

#include "ViewerContext.h"

namespace saba
{
	enum class ModelDrawerType
	{
		OBJModelDrawer,
		MMDModelDrawer,
		XFileModelDrawer,
	};

	class ModelDrawer
	{
	public:
		ModelDrawer();
		virtual ~ModelDrawer() {};

		ModelDrawer(const ModelDrawer&) = delete;
		ModelDrawer& operator =(const ModelDrawer&) = delete;

		virtual ModelDrawerType GetType() const = 0;

		virtual void Play() = 0;
		virtual void Stop() = 0;

		virtual void ResetAnimation(ViewerContext* ctxt) = 0;
		virtual void DrawUI(ViewerContext* ctxt) = 0;
		virtual void Update(ViewerContext* ctxt) = 0;
		virtual void DrawShadowMap(ViewerContext* ctxt, size_t csmIdx) = 0;
		virtual void Draw(ViewerContext* ctxt) = 0;

		void SetName(const std::string& name) { m_name = name; }
		const std::string& GetName() const { return m_name; }

		void SetBBox(const glm::vec3& bboxMin, const glm::vec3& bboxMax)
		{
			m_bboxMin = bboxMin;
			m_bboxMax = bboxMax;
		}

		const glm::vec3& GetBBoxMin() const { return m_bboxMin; }
		const glm::vec3& GetBBoxMax() const { return m_bboxMax; }

		const glm::vec3& GetTranslate() const { return m_translate; }
		const glm::vec3& GetRotate() const { return m_rotate; }
		const glm::vec3& GetScale() const { return m_scale; }
		void SetTranslate(const glm::vec3& t);
		void SetRotate(const glm::vec3& r);
		void SetScale(const glm::vec3& s);

		const glm::mat4& GetTransform() const { return m_transform; }

	protected:
		void UpdateTransform();

	private:
		std::string	m_name;
		glm::vec3	m_bboxMin;
		glm::vec3	m_bboxMax;

		glm::vec3	m_translate;
		glm::vec3	m_rotate;
		glm::vec3	m_scale;

		glm::mat4	m_transform;
	};
}

#endif // !SABA_VIEWER_MODELDRAWER_H_

