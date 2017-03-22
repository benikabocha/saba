#include "VMDCameraOverrider.h"

namespace saba
{
	VMDCameraOverrider::VMDCameraOverrider()
	{
	}

	bool VMDCameraOverrider::Create(const VMDFile& vmdFile)
	{
		m_camAnim.Destroy();
		return m_camAnim.Create(vmdFile);
	}

	void VMDCameraOverrider::OnOverride(ViewerContext* ctxt, Camera* camera)
	{
		m_camAnim.Evaluate((float)(ctxt->GetAnimationTime() * 30.0));
		const auto mmdCam = m_camAnim.GetCamera();
		MMDLookAtCamera lookAtCam(mmdCam);
		camera->LookAt(lookAtCam.m_center, lookAtCam.m_eye, lookAtCam.m_up);
		camera->SetFovY(mmdCam.m_fov);
	}
}

