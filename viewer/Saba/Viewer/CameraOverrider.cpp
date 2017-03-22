#include "CameraOverrider.h"

namespace saba
{
	CameraOverrider::~CameraOverrider()
	{
	}

	void CameraOverrider::Override(ViewerContext* ctxt, Camera* camera)
	{
		OnOverride(ctxt, camera);
	}
}

