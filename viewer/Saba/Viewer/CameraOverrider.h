//
// Copyright(c) 2016-2017 benikabocha.
// Distributed under the MIT License (http://opensource.org/licenses/MIT)
//

#ifndef SABA_VIEWER_CAMERAOVERRIDER_H_
#define SABA_VIEWER_CAMERAOVERRIDER_H_

#include "ViewerContext.h"

namespace saba
{
	class CameraOverrider
	{
	public:
		virtual ~CameraOverrider();

		void Override(ViewerContext* ctxt, Camera* camera);

	protected:
		virtual void OnOverride(ViewerContext* ctxt, Camera* camera) = 0;

	};
}

#endif // !SABA_VIEWER_CAMERAOVERRIDER_H_
