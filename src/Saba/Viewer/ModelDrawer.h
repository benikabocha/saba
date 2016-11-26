#ifndef SABA_VIEWER_MODELDRAWER_H_
#define SABA_VIEWER_MODELDRAWER_H_

#include "ViewerContext.h"

namespace saba
{
	class ModelDrawer
	{
	public:
		ModelDrawer() {}
		virtual ~ModelDrawer() {};

		ModelDrawer(const ModelDrawer&) = delete;
		ModelDrawer& operator =(const ModelDrawer&) = delete;


		virtual void Draw(ViewerContext* ctxt) = 0;
	};
}

#endif // !SABA_VIEWER_MODELDRAWER_H_

