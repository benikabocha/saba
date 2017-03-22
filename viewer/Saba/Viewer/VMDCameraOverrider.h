//
// Copyright(c) 2016-2017 benikabocha.
// Distributed under the MIT License (http://opensource.org/licenses/MIT)
//

#ifndef SABA_VIEWER_VMDCAMERAOVERRIDER_H_
#define SABA_VIEWER_VMDCAMERAOVERRIDER_H_

#include "CameraOverrider.h"

#include <Saba/Model/MMD/VMDFile.h>
#include <Saba/Model/MMD/VMDCameraAnimation.h>

namespace saba
{
	class VMDCameraOverrider : public CameraOverrider
	{
	public:
		VMDCameraOverrider();

		bool Create(const VMDFile& vmdFile);

	protected:
		void OnOverride(ViewerContext* viewer, Camera* camera) override;

	private:
		VMDCameraAnimation	m_camAnim;
	};
}

#endif // !SABA_VIEWER_VMDCAMERAOVERRIDER_H_
