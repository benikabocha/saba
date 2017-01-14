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
	};

	class ModelDrawer
	{
	public:
		ModelDrawer() {}
		virtual ~ModelDrawer() {};

		ModelDrawer(const ModelDrawer&) = delete;
		ModelDrawer& operator =(const ModelDrawer&) = delete;

		virtual ModelDrawerType GetType() const = 0;

		virtual void DrawUI(ViewerContext* ctxt) = 0;
		virtual void Update(ViewerContext* ctxt) = 0;
		virtual void Draw(ViewerContext* ctxt) = 0;
	};
}

#endif // !SABA_VIEWER_MODELDRAWER_H_

